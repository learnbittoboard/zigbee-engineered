/*
 * A pretend sensor in its OWN source file — so it can be its OWN
 * logging module.
 *
 * This is the second half of the step's lesson: log levels are
 * per-MODULE, not per-application. This file registers the module
 * "sim"; main.c registers "app". prj.conf then turns sim up to DBG
 * while app stays at INF — one noisy corner, a quiet product.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sensor_sim.h"

/* Default level INF here; prj.conf raises this module to DBG. */
LOG_MODULE_REGISTER(sim, LOG_LEVEL_INF);

int32_t sensor_sim_read(void)
{
	int64_t t = k_uptime_get() / 100;	/* tenths of a second */
	int32_t distance = 300 + (int32_t)((t * 137) % 4700);

	/* DBG: invisible until prj.conf allows this module to speak
	 * at debug level. Remove the CONFIG_SIM_LOG_LEVEL_DBG line
	 * from prj.conf and this message vanishes — same code.
	 */
	LOG_DBG("raw sample: t=%lld, distance=%d mm",
		k_uptime_get(), distance);

	return distance;
}
