# Chapter 4 · Step 3 — The parser becomes a driver

**What you do:** Same wiring, third build. The application now
logs one line per *change* — "presence: someone (stationary) —
143 cm away" — and a shell command you can type mid-run answers
"what does the sensor see right now, and is the driver healthy?"

**What this step teaches:** What a driver *is*: the Step 2 parser
moved behind a small contract (`ld2410.h`), with its own thread,
its own log module, and counters — so Chapter 7's Zigbee code can
consume presence without ever seeing a byte.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

Expected log output from your DK (events only, no stream spam):

```
[00:00:02.114,000] <inf> ld2410: LD2410 driver started (basic reporting mode expected)
[00:00:09.551,000] <inf> app: presence: someone (moving) — 212 cm away
[00:00:11.208,000] <inf> app: presence: someone (stationary) — 143 cm away
[00:00:27.940,000] <inf> app: presence: nobody in range
```

And at the shell prompt (the shell shares the USB serial port
with the log — press Enter for a clean prompt):

```
uart:~$ presence
presence : YES (stationary)
distance : 143 cm
detail   : move 0 cm (e0), still 143 cm (e58)
frames   : 812 parsed, 1 resyncs, 0 dropped bytes, 0 engineering frames
```

## The driver contract (ld2410.h, in full)

- `ld2410_start(uart, cb, user)` — one call; the driver owns its
  interrupt and parser thread from then on, and sets the port to
  the module's 256,000 baud itself (Step 1's divider-register
  fix, now living in its one proper home). A second start is
  `-EALREADY`: one sensor, one driver.
- The callback fires per valid frame, **in the driver's thread**.
  The app's callback here does the Chapter 2 two-step: filter
  (target changed, or distance moved > 10 cm) and `k_work_submit`.
- `ld2410_latest(&r)` — a mutex-guarded snapshot copy, callable
  from anywhere. The report handler deliberately reads the
  *freshest* snapshot rather than the triggering frame: when
  changes burst, the report describes where things stand now.
- `ld2410_stats(&s)` — frames ok, resyncs, dropped bytes,
  engineering-mode frames. The `presence` command prints them:
  diagnosis is an API, not an afterthought.

## The ideas to take away

- **A driver is a boundary, not a file move.** Step 2's parser
  became a driver when three things appeared: a header that is a
  *contract*, a thread the driver owns, and state no outsider can
  touch except through functions. The parser code barely changed;
  its *obligations* did.
- **Per-module logging pays off immediately.** The driver logs as
  `ld2410`, the app as `app`. Set
  `CONFIG_LD2410_LOG_LEVEL_DBG=y` in prj.conf — a symbol that
  exists because this app's Kconfig instantiates the log template
  for the module (Chapter 2, Step 3's mechanism, second use) —
  and the driver narrates every frame while the app stays quiet.
- **Change-only reporting is a product decision in the app, not
  the driver.** The driver reports every frame; the *app* chooses
  the deadband. Chapter 8 makes the same call for Zigbee
  reporting, with the network's bandwidth in place of your eyes.
- **The shell is for questions logs can't answer.** Logs say what
  happened; `presence` says what *is*. Wiggle the sensor wiring
  and re-run it: resyncs and dropped bytes indict the hardware
  before any user reports "the sensor feels flaky".

## Try it (10 minutes)

1. Run `presence` ten times while a friend walks around. Then
   unplug the sensor's TX line and run it again: the reading
   freezes but the frame counter stops too — "stale data, dead
   link" is visible in one command. (Timestamps on the reading
   would make staleness explicit: add the reading's age in ms to
   the command's output.)
2. Set the DBG line in prj.conf, rebuild, and watch the driver
   narrate frames at 10 Hz. Set it back. You have now aimed
   per-module logging at a subsystem *you* wrote.
3. Read `ld2410.h` top to bottom and list everything Chapter 7
   will need that is *not* there. (Candidate: a "presence held
   for N seconds" helper. Resist adding it to the driver — that
   policy belongs to the application, and Chapter 8 is where
   you'll prove it.)

## Why this matters

This is the last chapter where the sensor is the story — and the
first where the sensor disappears behind an interface, which is
exactly what growing from hacker firmware to product firmware
means. Chapter 5 starts the radio. When Chapter 7 writes the
Occupancy Sensing cluster, its entire sensor knowledge will be:
`ld2410_start`, `ld2410_latest`, and a callback. Everything you
built in this chapter is underneath, working, unseen.
