/*
 * Chapter 3, Step 2 — Debounced events: one press, one event.
 *
 * Step 1 measured the enemy: a press is a burst of edges spread
 * over a few milliseconds, while a human cannot press twice in
 * under ~50 ms. That gap is the whole debounce strategy:
 *
 *   TIMESTAMP DEBOUNCE (a.k.a. lockout)
 *   Accept a press edge only if at least DEBOUNCE_MS have passed
 *   since the last ACCEPTED press. Every edge inside the window
 *   is bounce by definition — count it, drop it, move on.
 *
 * The ISR still sees both edges (so the raw-edge and swallowed
 * counters measure reality), but only accepted presses reach the
 * work handler. The LED toggles once per accepted press: physical
 * proof that the count in the log matches what a user sees.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/*
 * Chosen from Step 1's measurements, not from tradition: bounce
 * bursts on this class of switch die out within ~10 ms, and the
 * fastest deliberate human double-tap is far slower than 30 ms.
 * Make it much smaller and bounce leaks through; much larger and
 * you start eating real (fast) presses. Every product re-derives
 * this number from its own switch and its own users.
 */
#define DEBOUNCE_MS 30

static const struct gpio_dt_spec button =
	GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct gpio_callback button_cb;
static struct k_work press_work;

static atomic_t raw_edges;	/* every edge the pin produced   */
static atomic_t bounce_swallowed; /* edges the lockout dropped   */
static atomic_t press_count;	/* accepted presses              */
static int64_t last_accept_ms;	/* timestamp of last accepted press */

/* ------------------------------------------------------------------ */
/* Accepted-press handling — system workqueue thread.                  */
/* ------------------------------------------------------------------ */

static void press_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	long n = atomic_inc(&press_count) + 1;
	bool on = (n % 2) == 1;

	gpio_pin_set_dt(&led, on ? 1 : 0);

	LOG_INF("PRESS #%ld accepted — LED %s "
		"(raw edges so far: %ld, swallowed as bounce: %ld)",
		n, on ? "ON " : "off",
		atomic_get(&raw_edges), atomic_get(&bounce_swallowed));
}

/* ------------------------------------------------------------------ */
/* The interrupt handler — classify, maybe accept, leave.              */
/* ------------------------------------------------------------------ */

static void button_isr(const struct device *dev,
		       struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	atomic_inc(&raw_edges);

	/* Only the press direction can start an event. Release
	 * edges are counted above — they are how we know the raw
	 * count is honest — and otherwise ignored.
	 */
	if (gpio_pin_get_dt(&button) <= 0) {
		return;
	}

	int64_t now = k_uptime_get();

	if (now - last_accept_ms < DEBOUNCE_MS) {
		/* Inside the lockout window: this "press" is the
		 * contact still settling from the last real one
		 * (or the release bouncing back across the line).
		 */
		atomic_inc(&bounce_swallowed);
		return;
	}

	last_accept_ms = now;
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
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&button_cb, button_isr, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb);

	LOG_INF("debounced button ready — %d ms lockout, one press = one event",
		DEBOUNCE_MS);

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
