# Chapter 3 · Step 3 — One button, four modes: short and long presses

**What you do:** Tap Button 1 to cycle the LED through OFF → ON →
BLINK SLOW → BLINK FAST. Hold it for a second to jump straight
back to OFF. The log reports each press *and its duration*.

**What this step teaches:** Debouncing both edges, measuring press
duration, and the events-in / state-changed / actuator-reacts
structure every real front panel (and the final product) is built on.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

Expected output from your DK:

```
[00:00:14.882,000] <inf> app: short press (132 ms) -> mode ON
[00:00:16.205,000] <inf> app: short press (118 ms) -> mode BLINK SLOW
[00:00:18.731,000] <inf> app: short press (141 ms) -> mode BLINK FAST
[00:00:21.540,000] <inf> app: long press (1,204 ms) -> mode OFF
```

## The ideas to take away

- **A duration is two debounced edges and a subtraction.** Step 2
  only needed the press edge. The moment a product cares about
  *releases* (dimming, hold-to-reset, press-and-hold pairing),
  both edges need the lockout — and a release with no matching
  press must be ignored, not treated as an error.
- **Classification is policy; events are facts.** The ISR and the
   work handler agree only on "a press lasted 1,204 ms". Whether
  that means OFF, pairing mode, or factory reset is a decision
  that lives in exactly one place — the handler — where it can be
  changed without touching interrupt code.
- **The LED thread owns time; nobody else does.** Pattern timing
  (500 ms toggles) lives in one thread that reads a mode variable.
  Button code never sleeps, patterns never debounce. One variable
  is the entire interface between input and output — this is the
  same separation Chapter 7 uses between sensor readings and the
  Zigbee attribute they drive.
- **Long-press thresholds are product design.** 800 ms is a
  choice: shorter feels responsive but punishes slow tappers;
  longer is forgiving but sluggish. There is no datasheet for
  this one — only users.

## Try it (10 minutes)

1. Time yourself: what is the shortest hold you can reliably
   produce, and the longest tap? Set `LONG_PRESS_MS` between your
   two numbers and feel the UI get *yours*.
2. Add a fifth mode — double-blink (two quick flashes, pause) —
   in the LED thread only. Notice the button code did not change:
   the mode variable absorbed the whole feature.
3. Find the flaw: hold the button down, and while holding, the
   LED mode does not change until you release. When would a
   product need the long-press to fire *during* the hold (think:
   volume buttons)? Sketch the change — a delayable work item,
   armed on the press edge, cancelled on release — before
   Chapter 8 shows the same trick as the occupancy timeout.

## Why this matters

You have now built, from a bouncing piece of metal, the complete
input stack of a real product: interrupt → debounce → timed event
→ state machine → actuator thread. The LD2410 in Chapter 4
replaces the button with a radar, and Chapter 8's "room empty for
N minutes → light off" is this step's long-press logic pointed at
a room instead of a finger.
