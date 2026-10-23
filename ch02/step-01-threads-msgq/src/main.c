/*
 * Chapter 2, Step 1 — Threads and a message queue.
 *
 * A Zephyr application is never one loop doing everything. It is a
 * set of THREADS, each with its own stack and priority, that take
 * turns on the single CPU. The scheduler is preemptive: the instant
 * a higher-priority thread becomes ready, it runs.
 *
 * Here two threads cooperate the way the real product will:
 *
 *   sensor_thread    pretends to be the LD2410: every 800 ms it
 *                    "measures" and posts one reading.
 *   reporter_thread  waits for readings and logs them.
 *
 * They never call each other. They talk through a MESSAGE QUEUE —
 * a fixed-size mailbox owned by the kernel. That decoupling is the
 * whole lesson: the producer never waits for the consumer's work,
 * and the consumer sleeps (using zero CPU) until data exists.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* One reading, shaped exactly like what Chapter 4's real driver
 * will produce from the LD2410: presence + distance.
 */
struct reading {
	bool present;
	int32_t distance_mm;
};

/*
 * The mailbox: holds up to 8 readings. K_MSGQ_DEFINE creates it at
 * compile time — no heap, no runtime allocation, nothing to leak.
 * (sizeof the item, max items, alignment)
 */
K_MSGQ_DEFINE(reading_queue, sizeof(struct reading), 8, 4);

/* ------------------------------------------------------------------ */
/* Producer thread                                                     */
/* ------------------------------------------------------------------ */

static void sensor_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int32_t drops = 0;

	while (1) {
		/* Invent a reading from the uptime (real sensor: Ch. 4). */
		int64_t t = k_uptime_get() / 100;
		struct reading r = {
			.present = ((t * 137) % 4700) < 2500,
			.distance_mm = 300 + (int32_t)((t * 137) % 4700),
		};

		/*
		 * K_NO_WAIT: if the queue is full, drop the reading
		 * instead of blocking. A sensor producer must NEVER be
		 * held hostage by a slow consumer — stale readings are
		 * worthless anyway. The drop counter keeps the loss
		 * visible instead of silent.
		 */
		if (k_msgq_put(&reading_queue, &r, K_NO_WAIT) != 0) {
			drops++;
			if (drops % 10 == 1) {
				LOG_WRN("queue full — %d readings dropped",
					drops);
			}
		}

		k_sleep(K_MSEC(800));
	}
}

/* ------------------------------------------------------------------ */
/* Consumer thread                                                     */
/* ------------------------------------------------------------------ */

static void reporter_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct reading r;

	while (1) {
		/*
		 * K_FOREVER: sleep until a reading exists. A blocked
		 * thread costs no CPU at all — this is how battery
		 * devices spend most of their life.
		 */
		k_msgq_get(&reading_queue, &r, K_FOREVER);

		LOG_INF("presence=%s distance=%d mm",
			r.present ? "YES" : "no", r.distance_mm);
	}
}

/*
 * Create both threads at compile time.
 * (entry, stack size, priority — lower number = more important)
 *
 * The reporter gets the BETTER priority (5 beats 7): when a reading
 * lands, reporting it promptly matters more than producing the next
 * one. Priority choices are design decisions, not decorations.
 */
K_THREAD_DEFINE(sensor_tid, 1024, sensor_thread, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(reporter_tid, 1024, reporter_thread, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	LOG_INF("two threads, one queue — watch the reporter work");

	/* main() has nothing left to do: the threads run the product.
	 * Returning from main would end the main thread only.
	 */
	return 0;
}
