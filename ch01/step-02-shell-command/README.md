# Chapter 1 · Step 2 — A shell command of your own

**What you do:** Give the board a command line, then add a command that did
not exist until you wrote it.

**What this step teaches:** the Zephyr shell — how it is enabled, how commands
are registered, and why a live command line beats printf-debugging.

## Build, flash, connect

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

Open the serial terminal (115200 8N1) and press Enter. You should see a
prompt:

```
uart:~$
```

## What you should observe

Type `help` — the board lists its commands, including built-ins like
`kernel`, `device`, and `log`. Our addition is at the bottom:

```
uart:~$ room status
presence : YES
distance : 1737 mm (FAKE — real sensor in Chapter 4)
uptime   : 42 s
```

Try `room` alone: the shell prints the subcommand help. Type the command a
few times, seconds apart — distance changes, and eventually presence flips.
Tab completion works; so does backspace. And notice: **regular log lines
keep arriving while you type**. Logs and the shell share one serial wire —
that is normal on a dev board, and Chapter 2's logging lesson shows how
products tame it (log levels, and a second backend) when it matters.

## How it works, in one breath

`CONFIG_SHELL=y` in `prj.conf` starts a shell thread on the serial port.
`SHELL_CMD_REGISTER(room, ...)` puts a top-level command named `room` into
a table the shell builds at link time — no list to update anywhere else.
The handler runs **when you press Enter**, in the shell thread, so it may
use `shell_print()` freely. It must still be quick and must never block
forever: a stuck handler is a dead command line.

## Try it (10 minutes)

1. Add a second subcommand, `room uptime`, that prints only the uptime.
   Copy the pattern — one function, one line in the subcommand set.
2. Make `room status` reject an unexpected argument: type
   `room status banana` and have the handler print a usage error
   (hint: look at `argc`/`argv`, and return `-EINVAL`).

## Why this matters

Chapter 5 forms a whole Zigbee network from exactly this kind of shell
(Nordic's coordinator *is* a shell sample). Chapter 11's sniffer work is
driven the same way. Learning to extend the shell now means the network
chapters are about Zigbee, not about fighting a terminal.
