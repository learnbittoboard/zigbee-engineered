# Chapter 2 · Step 4 — Work queues: how an interrupt asks a thread for help

**What you do:** Press a button and watch slow work happen *after* the
interrupt, in a thread — the only legal place for slow work.

**What this step teaches:** GPIO interrupts, the ISR golden rule, and the
work queue — the deferral mechanism every driver and sensor in this course
is built on.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

Press **Button 1**:

```
[00:00:12.340,000] <inf> app: handling press #1 in thread context — starting slow work
[00:00:12.640,000] <inf> app: press #1 done (LED was on for the 'work')
```

LED 0 lights for the 300 ms the "slow work" runs. Mash the button: presses
are handled one after another, and a work item submitted while one is
already queued is not duplicated — the kernel coalesces it.

## The ideas to take away

- **The ISR golden rule:** in interrupt context you may record, count, and
  submit — nothing else. No sleeping, no slow I/O, no long computations.
  Everything else in the system is frozen while you run.
- **A work item is a function pointer with a queue slot.** `k_work_submit()`
  is callable from an ISR and returns immediately; the system workqueue
  thread runs your handler later, where `k_msleep()` and logging are fine.
- **This split is not ceremony.** Chapter 4's UART interrupts will do
  exactly this: ISR moves bytes to a buffer and submits work; a thread
  parses. Get the shape into your fingers now, on a harmless button.
- The 50 ms timestamp check here is only a flood guard. Real switch bounce
  is a physics problem with a proper treatment — that is Chapter 3.

## Try it (10 minutes)

1. Break the rule on purpose: move the `k_msleep(300)` into the ISR
   (replace the `k_work_submit`). The board will feel it — log timestamps
   smear and presses get lost. Put it back. You have now *felt* why the
   rule exists, which is worth more than reading it.
2. Submit a *delayable* work item instead (`k_work_delayable`,
   `k_work_schedule(..., K_SECONDS(1))`): the handling now starts a full
   second after the press. Delayed work is how timeouts and retries are
   built — remember it for Chapter 8's occupancy timeout.

## Why this matters

Every event in the final product follows this path: LD2410 UART interrupt →
work/parse thread → occupancy report. Firmware that does slow work in ISRs
is the number-one beginner failure mode this course refuses to teach you.
