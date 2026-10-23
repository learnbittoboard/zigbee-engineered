# Chapter 2 · Step 1 — Threads and a message queue

**What you do:** Run two threads that never call each other, yet cooperate
perfectly — a producer that "measures" and a consumer that reports.

**What this step teaches:** preemptive scheduling, thread priorities, and the
message queue — Zephyr's basic building block for moving data between threads.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

```
[00:00:00.210,000] <inf> app: two threads, one queue — watch the reporter work
[00:00:00.215,000] <inf> app: presence=YES distance=312 mm
[00:00:01.015,000] <inf> app: presence=YES distance=1427 mm
[00:00:01.815,000] <inf> app: presence=no distance=3811 mm
```

One new line every 800 ms. Between lines, the CPU is asleep: the reporter is
blocked on `k_msgq_get(K_FOREVER)` and the producer is in `k_sleep`. Nobody
polls, nobody spins.

## The ideas to take away

- **A thread is an independent line of execution** with its own stack.
  `K_THREAD_DEFINE` creates one before `main()` even runs.
- **Priority is a promise**: lower number wins. The reporter (5) preempts the
  producer (7) the moment a reading exists, so reporting is never late.
- **A message queue decouples timing.** The producer's only contract is
  "post and move on" (`K_NO_WAIT`, count the drops). If the consumer is slow,
  the queue absorbs a burst of 8 — then drops, loudly, instead of corrupting
  timing elsewhere.
- **Blocking is free.** A thread waiting on a queue or sleeping consumes no
  CPU. Polling loops are how batteries die.

## Try it (10 minutes)

1. Make the reporter slow: add `k_sleep(K_SECONDS(2))` at the end of its
   loop. The queue fills at 8 readings, drops begin, and the warning appears.
   You have built a tiny, honest back-pressure demonstration.
2. Swap the two priorities. Does the visible behaviour change? Why (or why
   not)? Predict first — the answer teaches more than the experiment.

## Why this matters

This exact shape returns in Chapter 4 (UART parser thread → readings queue →
application thread) and Chapter 7 (readings → Zigbee cluster attributes).
Learn it once here, where nothing else can go wrong.
