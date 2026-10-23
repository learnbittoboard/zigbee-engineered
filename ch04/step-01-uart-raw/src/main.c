/*
 * Chapter 4, Step 1 — UART bring-up: make the sensor confess in hex.
 *
 * The HLK-LD2410 is already talking. Wired and powered, it streams
 * a radar report frame about ten times a second whether anyone
 * listens or not. This step builds the receive path exactly the
 * way product firmware does it:
 *
 *   UART interrupt    drains the hardware FIFO the moment bytes
 *                     arrive, into a RING BUFFER — microseconds
 *                     of work, Chapter 2's golden rule untouched
 *   main thread       sleeps on a semaphore until the ISR says
 *                     "data", then prints the bytes as a hex dump
 *
 * No parsing yet. You cannot debug a protocol you cannot see, and
 * "see the raw bytes" is always step one of a datasheet driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_uarte.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* The sensor UART. Our overlay enables uart1; its baud rate is
 * set in code below (see ld2410_apply_baud). The console stays
 * on uart0 where it belongs.
 */
static const struct device *const sensor_uart =
	DEVICE_DT_GET(DT_NODELABEL(uart1));

/*
 * The 256,000-baud problem, solved in the open.
 *
 * The LD2410's documented speed is 256000 — but that number
 * exists in NEITHER of Zephyr's tables for this chip: the
 * devicetree binding rejects it at build time, and the nRF UARTE
 * driver's baud-rate lookup returns "invalid" for it. Setting
 * 250000 instead would run 2.3% slow — at the edge of what a
 * UART receiver tolerates, which is not a thing to ship on hope.
 *
 * The hardware is not the limitation. The UARTE baud generator
 * divides the 16 MHz peripheral clock by a 32-bit register, and
 * the nRF52840 datasheet gives the formula:
 *
 *     BAUDRATE register = desired baud * 2^32 / 16,000,000
 *
 * For 256,000 that is 0x04189374 — an actual rate of
 * 255,999.997 baud, closer to perfect than most "standard"
 * rates manage. So: one register, written once, before
 * reception is enabled, with the maths in the open. (Until
 * this runs, the port sits at the board default of 115200 —
 * a few milliseconds at boot, with nothing yet listening.)
 */
static void ld2410_apply_baud(void)
{
	nrf_uarte_baudrate_set(
		(NRF_UARTE_Type *)DT_REG_ADDR(DT_NODELABEL(uart1)),
		(nrf_uarte_baudrate_t)((256000ULL << 32) / 16000000ULL));
}

/*
 * The hand-off buffer. Byte-mode ring buffers in Zephyr need a
 * power-of-two size; 1024 bytes is ~4 seconds of LD2410 stream —
 * far more than the printing thread can ever fall behind by.
 */
RING_BUF_DECLARE(rx_ring, 1024);

static K_SEM_DEFINE(rx_sem, 0, 1);	/* ISR gives, thread takes */

static atomic_t rx_total;	/* every byte received          */
static atomic_t rx_dropped;	/* bytes lost to a full ring    */

/* ------------------------------------------------------------------ */
/* The UART interrupt — move bytes, signal, leave.                     */
/* ------------------------------------------------------------------ */

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

		uint32_t stored = ring_buf_put(&rx_ring, chunk, (uint32_t)n);

		atomic_add(&rx_total, n);
		if (stored < (uint32_t)n) {
			/* Ring full: bytes were lost. Counted, never
			 * hidden — a dropped byte in a frame protocol
			 * is a parser resync later (Step 2 shows it),
			 * and the counters tell you it happened.
			 */
			atomic_add(&rx_dropped, n - (int)stored);
		}

		k_sem_give(&rx_sem);
	}
}

int main(void)
{
	if (!device_is_ready(sensor_uart)) {
		LOG_ERR("uart1 not ready — check the overlay");
		return 0;
	}

	ld2410_apply_baud();

	uart_irq_callback_user_data_set(sensor_uart, sensor_uart_isr, NULL);
	uart_irq_rx_enable(sensor_uart);

	LOG_INF("uart1 listening at 256000 baud — sensor frames should follow");

	/*
	 * Wiring self-test: one known pattern out of TX at boot. It
	 * goes nowhere with the sensor attached (the LD2410 ignores
	 * bytes that are not its config commands), but jumper TX to
	 * RX instead of the sensor and you will see this exact
	 * string come back in the dump — proof the pins and the
	 * baud rate are right before the sensor is involved at all.
	 */
	static const uint8_t selftest[] = "LD2410?\r\n";

	for (size_t i = 0; i < sizeof(selftest) - 1; i++) {
		uart_poll_out(sensor_uart, selftest[i]);
	}

	uint8_t dump[64];

	while (1) {
		/* Sleep until data — or 5 quiet seconds pass, so the
		 * totals line doubles as a "still alive, still silent"
		 * heartbeat. Zero CPU while waiting, either way.
		 */
		if (k_sem_take(&rx_sem, K_SECONDS(5)) != 0) {
			LOG_INF("quiet… totals: %ld bytes received, %ld dropped",
				atomic_get(&rx_total), atomic_get(&rx_dropped));
			continue;
		}

		uint32_t n;

		while ((n = ring_buf_get(&rx_ring, dump, sizeof(dump))) > 0) {
			LOG_HEXDUMP_INF(dump, n, "rx");
		}
	}

	return 0;
}
