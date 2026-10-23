# Chapter 4 · Step 1 — UART bring-up: make the sensor confess in hex

**What you do:** Wire the LD2410 to the DK's second UART, flash,
and watch raw radar frames pour into the log — unparsed, exactly
as they arrived.

**What this step teaches:** The interrupt-driven UART receive path
(FIFO → ring buffer → thread), why the sensor gets its own UART
instead of sharing the console, and the first law of datasheet
drivers: *look at the bytes before you interpret them.*

## Wiring

| LD2410 | DK (nRF52840) | Why |
|---|---|---|
| VCC | 5V pin | The module wants 5–12 V (its own regulator makes 3.3 V logic) — HLK datasheet |
| GND | GND | Common ground, always |
| TX | **P1.01** (uart1 RX) | Sensor talks, DK listens |
| RX | **P1.02** (uart1 TX) | For later config commands; the driver only listens in this course |

The module's UART levels are 3.3 V — safe for the DK's pins. Your
logs keep coming out of the USB virtual COM port (uart0); the
sensor lives on uart1. Two wires, two jobs, no interference.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

Expected output from your DK — a 23-byte frame, about ten times a
second:

```
[00:00:03.214,000] <inf> app: uart1 listening at 256000 baud — sensor frames should follow
[00:00:03.318,000] <inf> app: rx
                         f4 f3 f2 f1 0d 00 02 aa 00 00 00 00 00 00
                         00 00 55 00 f8 f7 f6 f5
[00:00:03.419,000] <inf> app: rx
                         f4 f3 f2 f1 0d 00 02 aa 03 f4 01 32 ...
```

Wave your hand in front of the sensor and the middle bytes change.
Nobody there, and most payload bytes sit at zero. You are looking
at presence, distance, and energy — you just can't read it yet.

## The ideas to take away

- **The ISR moves bytes; it never thinks.** `uart_fifo_read` into
  a ring buffer and a semaphore give is the complete interrupt.
  Interpretation is a thread's job (Chapter 2's rule, third time
  you've used it — that's deliberate).
- **The ring buffer absorbs the speed mismatch.** The sensor
  streams at its pace; the printing thread works at log pace.
  Between them, a buffer that counts what it drops — silence is
  diagnosable, loss is visible.
- **Wrong baud = plausible garbage.** If your dump shows random
  bytes instead of repeating frames, the speed or the wiring is
  wrong — not the sensor. A repeating structure in the noise is
  the signature of "almost right".
- **Why the speed is set in C, not in the overlay.** 256,000 is
  missing from both of Zephyr's baud tables for this chip — add
  `current-speed = <256000>` to the overlay and the *build*
  refuses it. But the UARTE hardware divider computes the rate
  almost exactly from the datasheet formula, so `main.c` writes
  that one register before enabling reception. Lesson inside the
  lesson: tables and bindings are conveniences; the peripheral's
  datasheet is ground truth, and real drivers occasionally have
  to visit it directly.
- **The boot-time `LD2410?` transmission is a wiring probe.**
  Jumper P1.02 to P1.01 (TX to RX, no sensor) and it echoes back
  in the dump: pins and baud proven with zero hardware trust.

## Try it (10 minutes)

1. Count frames per second from the timestamps (expect ~10/s) and
   bytes per frame (expect 23). Step 2's parser will be built to
   exactly these numbers.
2. Unplug the sensor's TX wire. The dump stops; the 5-second
   "quiet… totals" line keeps printing. That heartbeat is how
   you'll distinguish "no presence" from "no sensor" later —
   they're opposites, and raw byte totals know the difference.
3. Read one frame by hand against the map Step 2 will formalise:
   header `F4 F3 F2 F1`, a 2-byte little-endian length `0D 00`,
   payload, footer `F8 F7 F6 F5`. Find the byte that changes
   when you walk away (it should drop to `00` — nobody).

## Why this matters

Every serial peripheral in your career — GPS, modems, radar,
other MCUs — gets brought up exactly like this: datasheet baud,
raw dump, structure found by eye, parser second. Engineers who
skip the dump debug their parser and their wiring at the same
time, and finish neither.
