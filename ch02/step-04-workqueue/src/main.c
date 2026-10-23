/*
 * Chapter 2, Step 4 — Work queues: the ISR golden rule.
 *
 * THE RULE: an interrupt handler does the absolute minimum, then
 * leaves. No sleeping, no printing sprees, no sensor reads, no
 * "quick" computations that are not quick. While an ISR runs, the
 * rest of the system is frozen.
 *
 * But a button press NEEDS real work done: read the sensor, decide,
 * log it, maybe send a Zigbee command later in this course. That
 * work happens in a THREAD — and the bridge between the two worlds
 * is the WORK QUEUE:
 *
 *   button ISR        records the press, submits a work item, exits
 *   work handler      (runs later, in the system workqueue thread)
 *                     does the slow work at leisure
 *
 * Press Button 1 on the DK and watch: the press is acknowledged
 * instantly, and the "slow work" happens afterwards, in thread
 * context, where sleeping is legal.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* Reuse Step 2's idea: hardware by devicetree name, not pin number.
 * (No overlay in this step — the board's own aliases are fine here.)
 */
static const struct gpio_dt_spec button =
	GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct gpio_callback button_cb;
static struct k_work press_work;

static atomic_t press_total;
static int64_t last_press_ms;

/* ------------------------------------------------------------------ */
/* The slow work — runs in the system workqueue THREAD.                */
/* Sleeping, logging and taking your time are all legal here.          */
/* ------------------------------------------------------------------ */

static void press_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	long n = atomic_inc(&press_total) + 1;

	LOG_INF("handling press #%ld in thread context — starting slow work", n);

	/* Pretend to do something genuinely slow: a sensor transaction
	 * that takes 300 ms. In an ISR this would freeze the whole
	 * system for 300 ms. Here it freezes nothing — other threads
	 * simply run meanwhile.
	 */
	gpio_pin_set_dt(&led, 1);
	k_msleep(300);
	gpio_pin_set_dt(&led, 0);

	LOG_INF("press #%ld done (LED was on for the 'work')", n);
}

/* ------------------------------------------------------------------ */
/* The interrupt handler — the minimum, then out.                      */
/* ------------------------------------------------------------------ */

static void button_isr(const struct device *dev,
		       struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	/*
	 * Crude guard against switch bounce flooding the queue: a real
	 * press is never 50 ms after the previous accepted one.
	 * This is ONLY a flood guard — Chapter 3 does debouncing
	 * properly and explains the physics.
	 */
	int64_t now = k_uptime_get();

	if (now - last_press_ms < 50) {
		return;
	}
	last_press_ms = now;

	/* The whole job of the ISR: hand the event to a thread. */
	k_work_submit(&press_work);
}

int main(void)
{
	if (!gpio_is_ready_dt(&button) || !gpio_is_ready_dt(&led)) {
		LOG_ERR("GPIO device not ready");
		return 0;
	}

	k_work_init(&press_work, press_work_handler);

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&button, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&button_cb, button_isr, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb);

	LOG_INF("workqueue demo ready — press Button 1 on the DK");

	/* main() is done; interrupts and the workqueue run the product. */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
