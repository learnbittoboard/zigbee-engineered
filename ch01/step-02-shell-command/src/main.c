/*
 * Chapter 1, Step 2 — A shell command of your own.
 *
 * Logging tells you what the board decided to say. A shell lets you
 * ASK the board questions while it runs — no rebuild, no reflash.
 * Every serious Nordic product keeps a shell (or something like it)
 * for bring-up and field diagnosis.
 *
 * This step adds one command of our own:
 *
 *     uart:~$ room status
 *
 * It prints a fake occupancy reading. The value is invented by a
 * silly formula from the uptime — the REAL sensor arrives in
 * Chapter 4. What matters here is the mechanism: a command, its
 * arguments, and its output, all running on the board.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/*
 * A deterministic "pretend sensor": distance grows and wraps with time.
 * Returns a distance in millimetres, 300..5000.
 */
static int32_t fake_distance_mm(void)
{
	int64_t t = k_uptime_get() / 100;	/* tenths of a second */

	return 300 + (int32_t)((t * 137) % 4700);
}

/* The command handler. Runs in the shell's thread when you type it. */
static int cmd_room_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int32_t dist = fake_distance_mm();
	bool present = dist < 2500;

	shell_print(sh, "presence : %s", present ? "YES" : "no");
	shell_print(sh, "distance : %d mm (FAKE — real sensor in Chapter 4)",
		    dist);
	shell_print(sh, "uptime   : %lld s", k_uptime_get() / 1000);

	return 0;
}

/*
 * Build the command tree:
 *   room            <- top-level command (a group, because it has children)
 *     room status   <- the actual command
 *
 * SHELL_CMD_REGISTER wires a plain command; a static subcommand set
 * gives us the grouped form that scales to real products.
 */
SHELL_STATIC_SUBCMD_SET_CREATE(room_subcmds,
	SHELL_CMD(status, NULL, "Show the (fake) room reading.", cmd_room_status),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(room, &room_subcmds, "Room occupancy commands", NULL);

int main(void)
{
	LOG_INF("shell demo running — type 'help', then 'room status'");

	while (1) {
		k_sleep(K_SECONDS(30));
		LOG_INF("still alive — the shell thread does the talking");
	}

	return 0;
}
