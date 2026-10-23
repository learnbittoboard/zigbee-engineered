/*
 * Chapter 1, Step 1 — Hello, logging.
 *
 * The first program of the course does almost nothing, on purpose.
 * Before we touch Zigbee we need two things working on the desk:
 *   1. code that builds and flashes, and
 *   2. a way to SEE what the board is doing (logging).
 *
 * Two ways to print are shown side by side:
 *   - printk()  : the hammer. Always prints, no levels, no filtering.
 *                 Fine for "is it alive?", wrong for a product.
 *   - LOG_*()   : the logging module. Every message has a level
 *                 (ERR < WRN < INF < DBG) and belongs to a module.
 *                 You decide at build time what each module may say.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/*
 * Register this file as a logging module named "app".
 * The second argument is the highest level this module may print:
 * LOG_LEVEL_INF means DBG messages are compiled out completely —
 * they cost no flash and no CPU on the board.
 */
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

int main(void)
{
	printk("printk: the board is alive (printk always prints)\n");

	LOG_INF("the board is alive");
	LOG_WRN("something looks odd, but we can continue");
	LOG_ERR("something failed (pretend — this is a demo)");

	/* You will NOT see this line in the output. The module level
	 * above is INF, so this DBG call was removed by the compiler.
	 * Exercise 1 in the README makes it appear.
	 */
	LOG_DBG("you cannot see this... yet");

	int32_t seconds = 0;

	while (1) {
		seconds += 5;
		LOG_INF("heartbeat, uptime = %d s", seconds);
		k_sleep(K_SECONDS(5));
	}

	return 0;
}
