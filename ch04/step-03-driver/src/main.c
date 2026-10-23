/*
 * Chapter 4, Step 3 — The driver at work: presence as an app event.
 *
 * Everything below the line "application code" knows nothing about
 * bytes. main() starts the driver, registers a callback, and from
 * then on the application thinks in exactly one sentence:
 *
 *     "someone is 143 cm away"   /   "nobody is there"
 *
 * Two inspection tools ride along, both Chapter 1 skills reused:
 *   - the log, printing only CHANGES (a product logs events,
 *     not streams — 10 identical lines a second is noise),
 *   - a shell command YOU can type any time:
 *
 *         uart:~$ presence
 *
 *     which prints the live reading and the driver's honesty
 *     counters. Logs are the driver talking; the shell is you
 *     asking. Field diagnosis needs both.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <stdlib.h>

#include "ld2410.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

static const struct device *const sensor_uart =
	DEVICE_DT_GET(DT_NODELABEL(uart1));

static struct k_work report_work;

/* ------------------------------------------------------------------ */
/* The report — workqueue thread. Reads the FRESHEST snapshot, so      */
/* a burst of changes collapses into one truthful line.                */
/* ------------------------------------------------------------------ */

static const char *const target_words[] = {
	"nobody", "moving", "stationary", "moving + stationary"
};

static void report_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	struct ld2410_reading r;

	if (!ld2410_latest(&r)) {
		return;
	}

	if (r.target == LD2410_TARGET_NONE) {
		LOG_INF("presence: nobody in range");
	} else {
		LOG_INF("presence: someone (%s) — %u cm away",
			target_words[(r.target < 4) ? r.target : 0],
			ld2410_headline_distance_cm(&r));
	}
}

/* ------------------------------------------------------------------ */
/* The driver callback — parser-thread context, so: filter + defer.    */
/* It decides "is this worth a report?" and nothing more.              */
/* ------------------------------------------------------------------ */

#define DISTANCE_DEADBAND_CM 10

static enum ld2410_target last_target = LD2410_TARGET_NONE;
static uint16_t last_distance_cm;
static bool have_last;

static void on_reading(const struct ld2410_reading *r, void *user_data)
{
	ARG_UNUSED(user_data);

	uint16_t cm = ld2410_headline_distance_cm(r);
	bool changed = !have_last || r->target != last_target;

	if (!changed && r->target != LD2410_TARGET_NONE) {
		int32_t delta = (int32_t)cm - (int32_t)last_distance_cm;

		changed = (delta > DISTANCE_DEADBAND_CM) ||
			  (delta < -DISTANCE_DEADBAND_CM);
	}

	if (changed) {
		have_last = true;
		last_target = r->target;
		last_distance_cm = cm;
		k_work_submit(&report_work);
	}
}

/* ------------------------------------------------------------------ */
/* The shell command — ask the driver how it is doing, live.           */
/* ------------------------------------------------------------------ */

static int cmd_presence(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct ld2410_reading r;
	struct ld2410_stats s;

	ld2410_stats(&s);

	if (!ld2410_latest(&r)) {
		shell_print(sh, "presence : no valid frame received yet");
	} else if (r.target == LD2410_TARGET_NONE) {
		shell_print(sh, "presence : no");
		shell_print(sh, "distance : —");
	} else {
		shell_print(sh, "presence : YES (%s)", target_words[(r.target < 4) ? r.target : 0]);
		shell_print(sh, "distance : %u cm",
			    ld2410_headline_distance_cm(&r));
		shell_print(sh, "detail   : move %u cm (e%u), still %u cm (e%u)",
			    r.move_distance_cm, r.move_energy,
			    r.still_distance_cm, r.still_energy);
	}

	shell_print(sh, "frames   : %u parsed, %u resyncs, %u dropped bytes, "
		    "%u engineering frames",
		    s.frames_ok, s.resyncs, s.rx_dropped, s.eng_frames);

	return 0;
}

SHELL_CMD_REGISTER(presence, NULL,
		   "Show live LD2410 presence and driver statistics",
		   cmd_presence);

/* ------------------------------------------------------------------ */

int main(void)
{
	k_work_init(&report_work, report_handler);

	int err = ld2410_start(sensor_uart, on_reading, NULL);

	if (err != 0) {
		LOG_ERR("LD2410 driver failed to start (%d) — check the overlay",
			err);
		return 0;
	}

	LOG_INF("app up — walk in front of the sensor; type 'presence' any time");

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
