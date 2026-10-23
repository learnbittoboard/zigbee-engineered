# Chapter 3 · Step 1 — Meet the bounce: one press is not one signal

**What you do:** Press Button 1 once and read the log: your single
press produced a burst of edges. Measure how long the burst lasts.

**What this step teaches:** What switch bounce physically is, how to
configure a GPIO interrupt for both edges, and how to *measure* an
interrupt problem instead of guessing at it.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

Press **Button 1** once, gently, and let go. Expected output from
your DK (your numbers will differ — that's the lesson):

```
[00:00:09.412,000] <inf> app: edge #1: button PRESSED  (+0 us since previous edge)
[00:00:09.412,000] <inf> app: edge #2: button released (+63 us since previous edge)
[00:00:09.413,000] <inf> app: edge #3: button PRESSED  (+410 us since previous edge)
[00:00:09.413,000] <inf> app: edge #4: button released (+88 us since previous edge)
[00:00:09.415,000] <inf> app: edge #5: button PRESSED  (+1,204 us since previous edge)
...
[00:00:09.704,000] <inf> app: ---- 292,410 us of quiet — burst #2 starts ----
[00:00:09.704,000] <inf> app: edge #13: button released (+0 us since previous edge)
[00:00:09.705,000] <inf> app: edge #14: button PRESSED  (+71 us since previous edge)
...
```

Count the edges before the "quiet" line: that is how many signals
one press really was. The release bounces too.

## The ideas to take away

- **Bounce is physics, not a bug.** Two metal contacts are springs.
  On impact they rebound for typically 1–10 ms (cheap switches can
  exceed 20 ms — the datasheet's "bounce time" or "chatter" line is
  a number you are expected to design against).
- **Interrupts report edges, not intentions.** `GPIO_INT_EDGE_BOTH`
  is the honest configuration for observing this: every crossing,
  both directions, no filtering anywhere between the pin and you.
- **Measurement ends arguments.** The µs deltas show bounce lives in
  the tens-of-microseconds to few-milliseconds band, while a human
  press lives above ~50 ms. The debounce window in Step 2 is chosen
  from *this* measurement, not from folklore.
- The ISR still obeys Chapter 2's rule: it timestamps, stores, and
  submits. All the printing happens in the work handler.

## Try it (10 minutes)

1. Do ten careful presses and tally edges per press. Notice the
   count is never the same twice — bounce is mechanical chaos, and
   any design that assumes "about 5 extra edges" is fragile.
2. Hold the button down for two seconds before releasing. The press
   burst and the release burst are separated by silence: two
   actuations, two bounce storms.
3. Change the interrupt to `GPIO_INT_EDGE_TO_ACTIVE` only and press
   again. Fewer edges — but not one. Release bounce can cross the
   active level too, inventing presses that never happened. Keep
   this in mind: filtering edges is not the same as debouncing.

## Why this matters

Every mechanical input in a product — buttons, reed switches on
doors, rotary encoders, the tamper switch on a meter — bounces.
Firmware that counts raw edges ships counters that lie. Step 2
fixes this properly; this step exists so you know exactly what
"properly" has to survive.
