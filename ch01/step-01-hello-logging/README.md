# Chapter 1 · Step 1 — Hello, logging

**What you do:** Build and flash the smallest useful Zephyr application, and
watch it talk to you over the serial port.

**What this step teaches:** the build/flash loop you will repeat for the whole
course, and the difference between `printk()` and the logging module.

## Build and flash

From this folder, in a toolchain terminal (the nRF Connect terminal
inside VS Code; see Chapter 1, Install Step 5). Versions pinned:
NCS v3.4.1, board `nrf52840dk/nrf52840`:

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

`-p always` means "pristine": a clean build from scratch. Use it whenever you
change `prj.conf` or the devicetree — incremental builds do not always notice.

## Connect a terminal

The DK shows up as a USB serial port. Open it at **115200 baud, 8N1**
(the nRF Terminal in VS Code, PuTTY, `screen`, tio — any of them), then press the
board's RESET button.

## What you should observe

```
printk: the board is alive (printk always prints)
[00:00:00.123,456] <inf> app: the board is alive
[00:00:00.123,789] <wrn> app: something looks odd, but we can continue
[00:00:00.124,012] <err> app: something failed (pretend — this is a demo)
[00:00:05.124,100] <inf> app: heartbeat, uptime = 5 s
[00:00:10.124,100] <inf> app: heartbeat, uptime = 10 s
```

Notice three things:

1. Log lines carry a **timestamp**, a **level** (`inf`, `wrn`, `err`) and the
   **module name** (`app`). printk lines carry nothing.
2. There is **no DBG line**, even though the code calls `LOG_DBG(...)`.
   The module was registered with `LOG_LEVEL_INF`, so the compiler removed
   the DBG call entirely.
3. The first lines appear a moment *after* reset, not instantly: by default
   logging is **deferred** — your code drops messages in a buffer and a
   background thread prints them, so logging never blocks your logic for long.

## Try it (5 minutes)

1. In `src/main.c`, change `LOG_LEVEL_INF` to `LOG_LEVEL_DBG` in
   `LOG_MODULE_REGISTER(...)`, rebuild with `-p always`, reflash. The missing
   DBG line appears. This is how you make one noisy module verbose while the
   rest of the product stays quiet.
2. Change the heartbeat period from 5 s to 1 s. Before flashing, predict the
   output. Were you right?

## Why this matters

Every later chapter debugs through these logs: the LD2410 parser in Chapter 4,
the Zigbee join in Chapter 6, the OTA in Chapter 10. An engineer who can make
the board explain itself never debugs blind.
