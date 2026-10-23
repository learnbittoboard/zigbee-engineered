/*
 * Chapter 2, Step 2 — The devicetree: hardware, described once.
 *
 * Zephyr does not hard-code "the LED is on pin 13" in C. The board's
 * hardware is described in a data file — the DEVICETREE — and your
 * code asks it questions at compile time:
 *
 *     "give me the GPIO spec for the node aliased `room-led`"
 *
 * This step's app.overlay gives the nRF52840 DK's built-in LED and
 * button NEW NAMES — the names our product uses:
 *
 *     room-led      (the board's led0)
 *     room-button   (the board's button0)
 *
 * The C code below only ever says "room". Retarget this product to a
 * custom board later and only the overlay changes — not one line of C.
 * That separation is the entire point of the devicetree.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/*
 * GPIO_DT_SPEC_GET pulls pin number, port and flags out of the
 * devicetree at BUILD time and produces a plain C struct.
 * If the alias did not exist, this line would fail to compile —
 * hardware mistakes become build errors, not field failures.
 */
static const struct gpio_dt_spec room_led =
	GPIO_DT_SPEC_GET(DT_ALIAS(room_led), gpios);
static const struct gpio_dt_spec room_button =
	GPIO_DT_SPEC_GET(DT_ALIAS(room_button), gpios);

int main(void)
{
	if (!gpio_is_ready_dt(&room_led) || !gpio_is_ready_dt(&room_button)) {
		LOG_ERR("GPIO device not ready — check the devicetree");
		return 0;
	}

	gpio_pin_configure_dt(&room_led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&room_button, GPIO_INPUT);

	LOG_INF("devicetree demo: room-led on pin %u, room-button on pin %u "
		"(both GPIO port 0 on the DK)",
		room_led.pin, room_button.pin);

	bool led_on = false;
	int last_button = -1;

	while (1) {
		/* Blink the room LED. */
		led_on = !led_on;
		gpio_pin_set_dt(&room_led, led_on);

		/*
		 * Read the room button. This is POLLING — a deliberate
		 * simplification so this step is only about the
		 * devicetree. Chapter 3 replaces it with a proper
		 * interrupt and debounce.
		 */
		int button = gpio_pin_get_dt(&room_button);

		if (button != last_button) {
			last_button = button;
			LOG_INF("room-button is %s", button ? "PRESSED" : "released");
		}

		k_sleep(K_MSEC(250));
	}

	return 0;
}
