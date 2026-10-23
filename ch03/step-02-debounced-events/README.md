# Chapter 3 · Step 2 — Debounced events: one press, one event

**What you do:** Press Button 1 any way you like — tap it, mash it,
hold it — and watch the LED toggle exactly once per press, with the
log proving the bounce was counted and discarded.

**What this step teaches:** Timestamp debounce (lockout), why the
window length is a measurement and not a magic number, and how to
keep honest counters so you can *see* a filter working.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

Expected output from your DK:

```
[00:00:11.204,000] <inf> app: PRESS #1 accepted — LED ON  (raw edges so far: 17, swallowed as bounce: 4)
[00:00:12.638,000] <inf> app: PRESS #2 accepted — LED off (raw edges so far: 41, swallowed as bounce: 11)
[00:00:15.090,000] <inf> app: PRESS #3 accepted — LED ON  (raw edges so far: 63, swallowed as bounce: 17)
```

The LED — something a user can see, not just a log — toggles once
per press. The raw-edge counter keeps climbing by double digits:
the bounce didn't go away, it was *classified* away.

## The ideas to take away

- **The lockout rule:** a press edge is real only if at least
  `DEBOUNCE_MS` passed since the last accepted press. Everything
  inside the window is dropped *by definition*, which is why the
  window must come from a measurement (Step 1) of your switch.
- **30 ms is this switch, this product.** The trade-off is real in
  both directions: too short and bounce leaks in as phantom
  presses; too long and fast intentional input (a gamer's
  double-tap, a technician's quick reset double-press) gets eaten.
- **Count what you discard.** A filter you cannot observe is a bug
  waiting for a field report. `raw edges` vs `swallowed` vs
  `accepted` is the whole story of this input, in one line.
- **The ISR classifies; the thread reacts.** Acceptance is a
  timestamp subtraction — microseconds of work, legal in an ISR.
  The LED and the logging still happen in the work handler.

## Try it (10 minutes)

1. Set `DEBOUNCE_MS` to 5 and press slowly ten times. Some presses
   now count twice — you have found this switch's true bounce
   tail. Set it to 100 and try a deliberate quick double-press:
   it merges into one. You have now felt both failure modes.
2. Count accepted presses for a *release*: there are none, because
   only the press direction is classified. What product inputs
   would need release events too? (Hold-to-dim lights do — that
   need is exactly Step 3.)
3. Alternative strategy, worth knowing: on the first edge, start a
   20 ms timer; when it fires, *read the pin* and accept only if
   it is still down (k_work_delayable does this well). Compare the
   two honestly: timer-confirm adds latency but verifies state;
   lockout is instant but trusts timing. Products use both.

## Why this matters

This exact pattern — edge interrupt, timestamp lockout, honest
counters, event to a thread — is how production firmware reads
buttons, door contacts, and tamper switches. And the *shape* of
the problem returns in Chapter 8 wearing a Zigbee costume: an
occupancy sensor also produces a chattering signal that must be
turned into clean state changes with a timeout. Learn it here,
where you can see it.
