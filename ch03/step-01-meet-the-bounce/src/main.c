/*
 * Chapter 3, Step 1 — Meet the bounce: count EVERY edge, on purpose.
 *
 * A mechanical button is not a clean digital signal. The contacts
 * are tiny springs: when they meet, they bounce — connect, break,
 * connect again — for a few milliseconds. Your code sees every one
 * of those connections as a separate interrupt.
 *
 * This step does everything Chapter 2 taught (ISR records, work
 * item defers, thread prints) but removes the flood guard, so you
 * can watch the bounce happen and MEASURE it: every edge is logged
 * with the number of microseconds since the previous edge.
 *
 * One unhurried press of Button 1 on the DK typically produces
 * 10–40 edges. Until you have seen that in a log, "debounce" is
 * just a word somebody told you to add.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

static const struct gpio_dt_spec button =
	GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/*
 * The evidence table. The ISR writes one entry per edge; the work
 * handler reads them out. 128 entries is far more than one bounce
 * burst produces — if a press ever wraps the table, the total count
 * below still tells the truth.
 */
#define EDGE_LOG_SIZE 128

struct edge_event {
	uint32_t cycles;	/* k_cycle_get_32() timestamp */
	uint8_t pressed;	/* logical state after the edge: 1 = down */
};

static struct edge_event edge_log[EDGE_LOG_SIZE];
static atomic_t edge_head;	/* next entry to write (ISR side) */
static atomic_t edge_total;	/* every edge ever seen        */

static struct gpio_callback button_cb;
static struct k_work report_work;

/* ------------------------------------------------------------------ */
/* Report — runs in the system workqueue thread.                       */
/* ------------------------------------------------------------------ */

static void report_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	static uint32_t tail;		/* next entry to print */
	static uint32_t prev_cycles;	/* timestamp of previous edge */
	static uint32_t edge_no;
	static int burst_no = 1;

	while (tail != (uint32_t)atomic_get(&edge_head)) {
		struct edge_event ev = edge_log[tail % EDGE_LOG_SIZE];

		tail++;
		edge_no++;

		uint32_t delta_us = 0;

		if (prev_cycles != 0U) {
			/* Unsigned subtraction: correct even if the
			 * 32-bit cycle counter wrapped between edges.
			 */
			delta_us = k_cyc_to_us_floor32(ev.cycles - prev_cycles);
		}

		/* A gap of silence means the previous burst of edges
		 * was one physical actuation — press or release.
		 */
		if (delta_us > 20000U) {
			burst_no++;
			LOG_INF("---- %u us of quiet — burst #%d starts ----",
				delta_us, burst_no);
		}

		LOG_INF("edge #%u: button %s (+%u us since previous edge)",
			edge_no, ev.pressed ? "PRESSED " : "released", delta_us);

		prev_cycles = ev.cycles;
	}
}

/* ------------------------------------------------------------------ */
/* The interrupt handler — record and leave (Chapter 2's rule).        */
/* ------------------------------------------------------------------ */

static void button_isr(const struct device *dev,
		       struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	uint32_t idx = (uint32_t)atomic_get(&edge_head);
	struct edge_event *slot = &edge_log[idx % EDGE_LOG_SIZE];

	slot->cycles = k_cycle_get_32();
	slot->pressed = (gpio_pin_get_dt(&button) > 0) ? 1U : 0U;

	/* Publish only after the entry is complete. The handler runs
	 * in a thread, so it cannot interleave with this ISR anyway —
	 * but ordering the writes keeps the habit correct.
	 */
	atomic_set(&edge_head, idx + 1);
	atomic_inc(&edge_total);

	k_work_submit(&report_work);
}

int main(void)
{
	if (!gpio_is_ready_dt(&button)) {
		LOG_ERR("GPIO device not ready");
		return 0;
	}

	k_work_init(&report_work, report_handler);

	gpio_pin_configure_dt(&button, GPIO_INPUT);

	/* BOTH edges: presses and releases bounce alike, and we want
	 * to see all of it this step.
	 */
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&button_cb, button_isr, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb);

	LOG_INF("bounce microscope ready — press Button 1 ONCE, slowly,");
	LOG_INF("then count how many edges your one press produced");

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
