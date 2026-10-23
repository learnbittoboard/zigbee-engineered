/*
 * Chapter 4, Step 2 — Parse the frames: a state machine over bytes.
 *
 * Step 1's receive path is unchanged (UART ISR → ring buffer →
 * this thread). What changes is what the thread does with bytes:
 * feed them, one at a time, into a parser that knows the LD2410
 * frame format from the module's serial-protocol document:
 *
 *   +-------------+--------+-------------------+-------------+
 *   | header      | length | payload           | footer      |
 *   | F4 F3 F2 F1 | u16 LE | (length bytes)    | F8 F7 F6 F5 |
 *   +-------------+--------+-------------------+-------------+
 *
 *   Basic report payload (length 13, the module's default mode):
 *     [0]     0x02        frame type: target basic report
 *     [1]     0xAA        data head marker
 *     [2]     state       0 none, 1 moving, 2 stationary, 3 both
 *     [3..4]  move distance, cm, little-endian
 *     [5]     move energy, 0..100
 *     [6..7]  stationary distance, cm, little-endian
 *     [8]     stationary energy, 0..100
 *     [9..10] detection distance (max of the two), cm, LE
 *     [11]    0x55        data tail marker
 *     [12]    0x00        check byte (fixed 0 in basic reports)
 *
 * Honesty notes, because a datasheet driver must keep them:
 *  - This protocol has NO checksum. Header, length, the AA/55
 *    markers and the footer are all the integrity there is —
 *    which is why the parser counts resyncs instead of hiding them.
 *  - A second frame type exists, 0x01 "engineering mode", with a
 *    longer payload of per-gate energies. The module ships in
 *    basic mode; this parser counts 0x01 frames but does not
 *    decode them. (Assumption flagged, behaviour defined.)
 *
 * The parser is a state machine fed single bytes. It never blocks,
 * never allocates, and recovers from garbage by hunting for the
 * next header — because on a real wire, garbage happens.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_uarte.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/*
 * The module's 256,000 baud is in neither of Zephyr's baud tables
 * for this chip, so — as Step 1 explains at length — the UARTE
 * divider register is written directly, from the datasheet
 * formula (baud * 2^32 / 16 MHz), before reception is enabled.
 */
static void ld2410_apply_baud(void)
{
	nrf_uarte_baudrate_set(
		(NRF_UARTE_Type *)DT_REG_ADDR(DT_NODELABEL(uart1)),
		(nrf_uarte_baudrate_t)((256000ULL << 32) / 16000000ULL));
}

/* ------------------------------------------------------------------ */
/* Receive plumbing — identical to Step 1.                             */
/* ------------------------------------------------------------------ */

static const struct device *const sensor_uart =
	DEVICE_DT_GET(DT_NODELABEL(uart1));

RING_BUF_DECLARE(rx_ring, 1024);
static K_SEM_DEFINE(rx_sem, 0, 1);

static void sensor_uart_isr(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	uint8_t chunk[32];

	if (!uart_irq_update(dev)) {
		return;
	}

	while (uart_irq_rx_ready(dev)) {
		int n = uart_fifo_read(dev, chunk, sizeof(chunk));

		if (n <= 0) {
			break;
		}

		/* If the ring ever fills, ring_buf_put stores what
		 * fits and the parser resyncs on the next header —
		 * Step 1's counters are the parser's problem now,
		 * and the parser's resync counter reports it.
		 */
		ring_buf_put(&rx_ring, chunk, (uint32_t)n);
		k_sem_give(&rx_sem);
	}
}

/* ------------------------------------------------------------------ */
/* The reading — what a parsed frame means.                            */
/* ------------------------------------------------------------------ */

struct ld2410_reading {
	uint8_t target_state;		/* 0 none, 1 moving, 2 still, 3 both */
	uint16_t move_distance_cm;
	uint8_t move_energy;
	uint16_t still_distance_cm;
	uint8_t still_energy;
	uint16_t detect_distance_cm;
};

/* ------------------------------------------------------------------ */
/* The parser.                                                         */
/* ------------------------------------------------------------------ */

#define LD2410_TYPE_BASIC 0x02
#define LD2410_MAX_PAYLOAD 64	/* basic is 13; generous ceiling so a
				 * nonsense length is caught, not obeyed */

enum parser_state {
	PS_HUNT,	/* looking for F4 F3 F2 F1 */
	PS_LEN,		/* collecting the 2 length bytes */
	PS_PAYLOAD,	/* collecting `payload_len` bytes */
	PS_FOOTER,	/* expecting F8 F7 F6 F5 */
};

static const uint8_t header_seq[4] = { 0xF4, 0xF3, 0xF2, 0xF1 };
static const uint8_t footer_seq[4] = { 0xF8, 0xF7, 0xF6, 0xF5 };

static enum parser_state ps_state;
static uint8_t ps_match;		/* header/footer bytes matched */
static uint16_t ps_len;			/* payload length being read */
static uint8_t ps_len_bytes;
static uint8_t ps_payload[LD2410_MAX_PAYLOAD];
static uint16_t ps_payload_pos;

static uint32_t stat_frames_ok;
static uint32_t stat_resyncs;		/* sequences abandoned */
static uint32_t stat_eng_frames;	/* type 0x01: counted, not parsed */
static uint32_t stat_unknown_type;

static void parser_reset(void)
{
	ps_state = PS_HUNT;
	ps_match = 0;
	ps_len = 0;
	ps_len_bytes = 0;
	ps_payload_pos = 0;
}

/* A candidate sequence died. Count it, go back to hunting. */
static void parser_abort(const char *why)
{
	stat_resyncs++;
	LOG_WRN("parser resync #%u: %s (ok=%u)", stat_resyncs, why, stat_frames_ok);
	parser_reset();
}

static void handle_payload(void)
{
	if (ps_payload[0] == 0x01) {
		/* Engineering mode: documented, not decoded here. */
		stat_eng_frames++;
		if (stat_eng_frames == 1) {
			LOG_WRN("engineering-mode frames arriving — the module "
				"is not in basic reporting mode (see README)");
		}
		return;
	}

	if (ps_payload[0] != LD2410_TYPE_BASIC) {
		stat_unknown_type++;
		parser_abort("unknown frame type");
		return;
	}

	/* Marker validation — with no checksum in this protocol,
	 * these fixed bytes are the frame's only credentials.
	 */
	if (ps_len != 13 || ps_payload[1] != 0xAA || ps_payload[11] != 0x55 ||
	    ps_payload[12] != 0x00) {
		parser_abort("basic frame with bad markers or length");
		return;
	}

	/* The protocol has no checksum: a framed payload can still
	 * carry a target-state byte outside 0..3. Reject it instead
	 * of logging a reading for a target that does not exist.
	 */
	if (ps_payload[2] > 3) {
		parser_abort("basic frame with invalid target state");
		return;
	}

	struct ld2410_reading r = {
		.target_state = ps_payload[2],
		.move_distance_cm = sys_get_le16(&ps_payload[3]),
		.move_energy = ps_payload[5],
		.still_distance_cm = sys_get_le16(&ps_payload[6]),
		.still_energy = ps_payload[8],
		.detect_distance_cm = sys_get_le16(&ps_payload[9]),
	};

	stat_frames_ok++;

	switch (r.target_state) {
	case 0:
		LOG_INF("frame #%u: nobody there", stat_frames_ok);
		break;
	case 1:
		LOG_INF("frame #%u: MOVING target at %u cm (energy %u)",
			stat_frames_ok, r.move_distance_cm, r.move_energy);
		break;
	case 2:
		LOG_INF("frame #%u: STATIONARY target at %u cm (energy %u)",
			stat_frames_ok, r.still_distance_cm, r.still_energy);
		break;
	case 3:
		LOG_INF("frame #%u: moving %u cm (e%u) + stationary %u cm (e%u), "
			"detection %u cm",
			stat_frames_ok, r.move_distance_cm, r.move_energy,
			r.still_distance_cm, r.still_energy,
			r.detect_distance_cm);
		break;
	default:
		LOG_INF("frame #%u: undocumented target state %u",
			stat_frames_ok, r.target_state);
		break;
	}
}

/*
 * Feed one byte. Returns nothing; results appear as log lines and
 * counters. The machine's contract: any byte sequence, in any
 * state, ends either in a validated frame or a counted resync —
 * never in a wedge.
 */
static void parser_feed(uint8_t byte)
{
	switch (ps_state) {
	case PS_HUNT:
		if (byte == header_seq[ps_match]) {
			ps_match++;
			if (ps_match == sizeof(header_seq)) {
				ps_state = PS_LEN;
				ps_match = 0;
				ps_len = 0;
				ps_len_bytes = 0;
			}
		} else {
			/* Not a continuation. This byte might itself
			 * be the start of a header (…F4 F4 F3…) — the
			 * classic overlap case; handle it or resyncs
			 * cascade.
			 */
			ps_match = (byte == header_seq[0]) ? 1 : 0;
		}
		break;

	case PS_LEN:
		ps_len |= (uint16_t)byte << (8 * ps_len_bytes);
		ps_len_bytes++;
		if (ps_len_bytes == 2) {
			if (ps_len < 3 || ps_len > LD2410_MAX_PAYLOAD) {
				parser_abort("implausible length — false sync");
			} else {
				ps_state = PS_PAYLOAD;
				ps_payload_pos = 0;
			}
		}
		break;

	case PS_PAYLOAD:
		ps_payload[ps_payload_pos++] = byte;
		if (ps_payload_pos >= ps_len) {
			ps_state = PS_FOOTER;
			ps_match = 0;
		}
		break;

	case PS_FOOTER:
		if (byte == footer_seq[ps_match]) {
			ps_match++;
			if (ps_match == sizeof(footer_seq)) {
				handle_payload();
				parser_reset();
			}
		} else {
			parser_abort("bad footer — frame corrupt or misaligned");
			/* The offending byte may be a new header's
			 * first byte; don't waste it.
			 */
			if (byte == header_seq[0]) {
				ps_state = PS_HUNT;
				ps_match = 1;
			}
		}
		break;
	}
}

/* ------------------------------------------------------------------ */

int main(void)
{
	if (!device_is_ready(sensor_uart)) {
		LOG_ERR("uart1 not ready — check the overlay");
		return 0;
	}

	parser_reset();
	ld2410_apply_baud();

	uart_irq_callback_user_data_set(sensor_uart, sensor_uart_isr, NULL);
	uart_irq_rx_enable(sensor_uart);

	LOG_INF("parser live — sit still in front of the sensor, then walk away");

	uint8_t bytes[64];

	while (1) {
		if (k_sem_take(&rx_sem, K_SECONDS(10)) != 0) {
			LOG_INF("10 s quiet — totals: %u frames ok, %u resyncs, "
				"%u engineering frames",
				stat_frames_ok, stat_resyncs, stat_eng_frames);
			continue;
		}

		uint32_t n;

		while ((n = ring_buf_get(&rx_ring, bytes, sizeof(bytes))) > 0) {
			for (uint32_t i = 0; i < n; i++) {
				parser_feed(bytes[i]);
			}
		}
	}

	return 0;
}
