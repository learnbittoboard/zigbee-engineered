/*
 * Chapter 2, Step 3 — Kconfig: software, configured.
 *
 * The devicetree (Step 2) describes HARDWARE. Kconfig configures
 * SOFTWARE: which features exist, and the numbers that tune them.
 *
 * Two files divide the work, and confusing them is the classic
 * beginner mix-up:
 *
 *   Kconfig   DEFINES options (this app's own are in ./Kconfig,
 *             next to this file's folder) — types, defaults, help.
 *   prj.conf  SETS values for those options for this build.
 *
 * main.c then just reads CONFIG_* macros. Change the value in
 * prj.conf, rebuild — the C code never changed. That is exactly how
 * the same firmware ships in a "quiet" and a "debug" variant.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sensor_sim.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("kconfig demo: period = %d ms, heartbeat feature %s",
		CONFIG_ROOM_BLINK_PERIOD_MS,
		IS_ENABLED(CONFIG_ROOM_FEATURE_HEARTBEAT_LOG) ? "ON" : "OFF");

	int32_t beats = 0;

	while (1) {
		/* One poll of the simulated sensor (its own module —
		 * watch its log lines: it is allowed to be noisier
		 * than `app`, because prj.conf says so).
		 */
		int32_t distance = sensor_sim_read();

		/*
		 * IS_ENABLED() turns the option into a compile-time
		 * 1 or 0. With the feature off, this whole block is
		 * removed by the compiler: zero flash, zero CPU.
		 * That is the difference between a feature FLAG in
		 * Kconfig and an `if` on a variable.
		 */
		if (IS_ENABLED(CONFIG_ROOM_FEATURE_HEARTBEAT_LOG)) {
			beats++;
			LOG_INF("heartbeat %d: distance = %d mm",
				beats, distance);
		}

		k_sleep(K_MSEC(CONFIG_ROOM_BLINK_PERIOD_MS));
	}

	return 0;
}
