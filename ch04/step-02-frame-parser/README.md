# Chapter 4 · Step 2 — Parse the frames: a state machine over bytes

**What you do:** Run the same wiring as Step 1 and read a live,
human feed: "STATIONARY target at 143 cm". Sit down, stand up,
leave the room — the log follows you.

**What this step teaches:** The frame-parser state machine (hunt →
length → payload → validate), little-endian field extraction, and
how a parser survives garbage: by resynchronising, and by counting
every time it had to.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

Expected output from your DK:

```
[00:00:05.102,000] <inf> app: frame #31: nobody there
[00:00:08.433,000] <inf> app: frame #64: MOVING target at 212 cm (energy 71)
[00:00:10.015,000] <inf> app: frame #80: STATIONARY target at 143 cm (energy 58)
[00:00:31.770,000] <inf> app: frame #297: nobody there
```

Walk toward the sensor and watch the distance fall in real time.
Hold perfectly still at your desk: MOVING becomes STATIONARY —
that distinction, free from a $10 radar, is the entire product.

## The ideas to take away

- **Parsers are state machines, not string matching.** At any
  moment the parser knows exactly what the next byte *means* (a
  header byte? a length byte? payload byte 7?) — that knowledge
  IS the state. `strstr`-style framing breaks on payload bytes
  that look like headers; a state machine doesn't care, because
  in PS_PAYLOAD a `F4` is just data.
- **Sync is earned back, never assumed.** A corrupt byte costs at
  most one frame: the machine drops to PS_HUNT and finds the next
  header. The resync counter turns "the driver is flaky" into a
  number you can watch while you wiggle the wiring.
- **This protocol has no checksum — say so, and compensate.**
  The length sanity check plus the `0xAA`/`0x55` markers plus the
  footer are the validation. The code comments and this README
  state plainly that engineering-mode frames (type `0x01`,
  longer payload, per-gate energies) are *counted, not decoded*:
  documented behaviour, not a silent gap.
- **Fields are little-endian, and the code says so.**
  `sys_get_le16()` names the byte order at the exact spot it
  matters — the datasheet's "distance, low byte first" becomes a
  function call, not a comment you hope is true.

## Try it (10 minutes)

1. Breathe at your desk without moving. STATIONARY holds — the
   radar sees your chest move. This is why mmWave beats a PIR
   sensor for occupancy, and it is a one-line demo.
2. Yank the TX wire for two seconds mid-stream, then reconnect.
   Watch resyncs climb, then frames recover on their own. No
   reset, no wedge: that is the state machine paying rent.
3. Point the sensor at a running fan. Most fans read as a
   permanent MOVING target — a real deployment problem (the
   product fix is gate/energy tuning in the module's config
   protocol, a later-chapter topic). File the observation; it
   comes back in Chapter 7's design discussion.

## Why this matters

Step 1 saw bytes; this step sees *meaning* — and knows the
difference between a valid reading, a corrupt frame, and a mode
it doesn't speak. But the parser still lives inside `main.c`,
glued to its test harness. Product code needs it behind an
interface other code can call without knowing bytes exist.
That's Step 3.
