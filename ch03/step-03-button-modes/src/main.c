/*
 * Chapter 3, Step 3 — One button, four modes: short and long presses.
 *
 * Steps 1–2 turned a bouncing pin into clean press events. Real
 * one-button products need one more distinction: HOW LONG was the
 * press? A tap and a hold are two different commands.
 *
 * This step measures press duration. Both edges are debounced
 * (a transition counts only if the pin has been stable — no
 * accepted transition — for DEBOUNCE_MS), the release edge
 * computes the duration, and a work handler classifies it:
 *
 *   short press  (< 800 ms)   cycle the LED mode: OFF → ON →
 *                             BLINK SLOW → BLINK FAST → OFF …
 *   long press   (>= 800 ms)  back to OFF from anywhere
 *
 * A dedicated LED thread owns the light patterns. The button code
 * never touches pattern timing; it only changes the MODE variable.
 * Events in, state changed, actuator thread reacts — the shape of
 * the whole final product, in miniature.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define DEBOUNCE_MS   30	/* from Step 1's measurement, as before */
#define LONG_PRESS_MS 800

enum led_mode {
	MODE_OFF = 0,
	MODE_ON,
	MODE_BLINK_SLOW,
	MODE_BLINK_FAST,
	MODE_COUNT
};

static const char *const mode_names[MODE_COUNT] = {
	"OFF", "ON", "BLINK SLOW", "BLINK FAST"
};

static const struct gpio_dt_spec button =
	GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static atomic_t led_mode;		/* enum led_mode; written by the
					 * work handler, read by the LED
					 * thread — atomic because it is
					 * the whole interface between them */

static struct gpio_callback button_cb;
static struct k_work event_work;

/* ISR-side press tracking. */
static bool press_in_progress;
static int64_t press_start_ms;
static int64_t last_transition_ms;
static atomic_t bounce_swallowed;

/* Completed-press duration, handed from ISR to work handler. */
static int64_t completed_duration_ms;

/* ------------------------------------------------------------------ */
/* The LED thread — owns the patterns, reads only the mode.            */
/* ------------------------------------------------------------------ */

static void led_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	bool on = false;

	while (1) {
		switch ((enum led_mode)atomic_get(&led_mode)) {
		case MODE_OFF:
			on = false;
			gpio_pin_set_dt(&led, 0);
			k_msleep(50);
			break;
		case MODE_ON:
			on = true;
			gpio_pin_set_dt(&led, 1);
			k_msleep(50);
			break;
		case MODE_BLINK_SLOW:
			on = !on;
			gpio_pin_set_dt(&led, on ? 1 : 0);
			k_msleep(500);
			break;
		case MODE_BLINK_FAST:
			on = !on;
			gpio_pin_set_dt(&led, on ? 1 : 0);
			k_msleep(125);
			break;
		default:
			k_msleep(50);
			break;
		}
	}
}

K_THREAD_DEFINE(led_thread_id, 1024, led_thread, NULL, NULL, NULL,
		7, 0, 0);

/* ------------------------------------------------------------------ */
/* Classification — system workqueue thread.                           */
/* ------------------------------------------------------------------ */

static void event_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	int64_t duration = completed_duration_ms;
	enum led_mode mode = (enum led_mode)atomic_get(&led_mode);

	if (duration >= LONG_PRESS_MS) {
		mode = MODE_OFF;
		LOG_INF("long press (%lld ms) -> mode %s",
			duration, mode_names[mode]);
	} else {
		mode = (enum led_mode)((mode + 1) % MODE_COUNT);
		LOG_INF("short press (%lld ms) -> mode %s",
			duration, mode_names[mode]);
	}

	atomic_set(&led_mode, mode);
}

/* ------------------------------------------------------------------ */
/* The interrupt handler — debounce both edges, time the press.        */
/* ------------------------------------------------------------------ */

static void button_isr(const struct device *dev,
		       struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int64_t now = k_uptime_get();
	bool pressed = gpio_pin_get_dt(&button) > 0;

	/* Any edge that arrives while the previous accepted
	 * transition is still settling is bounce — whichever
	 * direction it claims to be.
	 */
	if (now - last_transition_ms < DEBOUNCE_MS) {
		atomic_inc(&bounce_swallowed);
		return;
	}

	if (pressed && !press_in_progress) {
		press_in_progress = true;
		press_start_ms = now;
		last_transition_ms = now;
	} else if (!pressed && press_in_progress) {
		press_in_progress = false;
		last_transition_ms = now;
		completed_duration_ms = now - press_start_ms;
		k_work_submit(&event_work);
	}
	/* A release with no press in progress (e.g. the tail of a
	 * bounce storm that outlived the lockout) changes nothing
	 * and is deliberately not counted as an event.
	 */
}

int main(void)
{
	if (!gpio_is_ready_dt(&button) || !gpio_is_ready_dt(&led)) {
		LOG_ERR("GPIO device not ready");
		return 0;
	}

	k_work_init(&event_work, event_work_handler);

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&button, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&button_cb, button_isr, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb);

	LOG_INF("one-button UI ready:");
	LOG_INF("  short press (< %d ms): OFF -> ON -> BLINK SLOW -> BLINK FAST",
		LONG_PRESS_MS);
	LOG_INF("  long press  (>= %d ms): back to OFF", LONG_PRESS_MS);

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
