# Chapter 2 · Step 2 — The devicetree: name hardware like your product does

**What you do:** Blink the DK's LED and read its button without writing a
single pin number in C. The hardware is reached through names *you* chose.

**What this step teaches:** the devicetree (Zephyr's hardware description),
aliases, overlays, and the `gpio_dt_spec` — the pattern every driver in this
course is used through.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

```
[00:00:00.210,000] <inf> app: devicetree demo: room-led on P0.13, room-button on P0.11
[00:00:00.460,000] <inf> app: room-button is PRESSED
[00:00:01.210,000] <inf> app: room-button is released
```

LED 0 blinks four times a second. Press **Button 1** and the log answers.
(No new hardware: both are soldered to the DK.)

## The ideas to take away

- **The devicetree describes hardware; Kconfig (next step) configures
  software.** Mixing those two up is the classic beginner confusion — this
  step is the devicetree half.
- **An overlay adds or renames, it does not rewrite.** `app.overlay` gave the
  board's `led0`/`button0` the product names `room-led`/`room-button`. The board
  file itself is untouched. (Names matter: `button0` is the node's label.
  `sw0` is an *alias* for the same node — `DT_ALIAS()` looks aliases up, but
  `&label` references in an overlay need the label itself.)
- **`GPIO_DT_SPEC_GET(DT_ALIAS(room_led), gpios)`** turns a devicetree node
  into a C struct at build time: port, pin, flags. Wrong name in the alias →
  the build fails, loudly, at your desk — not silently, in the field.
- **Drivers are reached through specs, never registers.** `gpio_pin_set_dt()`
  would work unchanged on any board whose devicetree has a `room-led`.

## Try it (10 minutes)

1. Point `room-led` at `&led1` in the overlay. Rebuild (pristine), reflash:
   a different LED blinks and the C code did not change. *That* is the
   devicetree earning its keep.
2. Delete the `room-button` alias and rebuild. Read the compile error —
   Zephyr refuses to build firmware against hardware that does not exist.
   Put it back.

## Why this matters

Chapter 4 hangs the LD2410's UART and Chapter 7 the whole sensor node off
devicetree descriptions exactly like this one. And when this product moves
from the DK to a custom board, the application code survives the move —
only the devicetree changes.
