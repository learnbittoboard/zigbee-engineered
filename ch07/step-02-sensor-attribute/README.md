# Chapter 7 · Step 2 — The endpoint meets the sensor

**What you do:** Feed Step 1's endpoint from Chapter 4's LD2410
driver — included **unchanged**, contract and all — so the
Occupancy attribute now follows a real radar: walk into its view
and the network reads OCCUPIED; walk out and it reads UNOCCUPIED,
proven live from the coordinator shell.

**What this step teaches:** Integration without surgery. Two
chapters of work meet at one function-call boundary —
`ld2410_start()` — and neither side knows the other's internals.
The whole step is three small pieces of *glue*, and the glue is
the lesson: a transition filter in the callback, a work item, and
a scheduled stack write. Get the contexts right and the merge is
boring. Boring is the goal.

## Wire it first

LD2410 → DK, exactly as Chapter 4: module TX → **P1.01** (uart1
RX), module RX → **P1.02**, module powered at 5 V (its logic is
3.3 V — Chapter 4's wiring notes carry the details and the
warnings). The `boards/` overlay now enables uart1 inside the
Zigbee app — look at it: the ZBOSS timer block from Chapter 6,
plus four lines.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840 \
    course/repo/ch07/step-02-sensor-attribute -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
west flash
```

Join (or confirm the reboot-join), then stand in front of the
sensor. The DK log tells the story in two voices — the driver's
silence is deliberate; the app's line is the event:

```
<inf> app: Starting Zigbee, Engineered - occupancy endpoint (LD2410-driven)
...
<inf> app: Occupancy attribute -> OCCUPIED
```

Step out of view: `Occupancy attribute -> UNOCCUPIED`. Now the
real test — from the **coordinator**, while a volunteer walks:

```
uart:~$ zcl attr read 0x<short> 10 0x0406 0x0104 0x0000
```

Person in view → `0x01`. Person gone → `0x00`. The read crosses
the network, the stack answers from `dev_ctx` storage that a UART
parser thread ultimately wrote. That chain — radar frame → driver
→ work queue → stack scheduler → attribute → a stranger's read —
is the product's entire data path, and you can now point at every
link.

On the DK itself, Chapter 4's command is waiting, one line
richer:

```
uart:~$ presence
presence : YES (stationary)
distance : 143 cm
attribute: Occupancy = 1 (OCCUPIED)
frames   : 512 parsed, 0 resyncs, 0 dropped bytes, 0 engineering frames
```

`presence` is the two-sided diagnosis habit from Chapter 4, grown
up: *sensor truth* and *network truth* on one screen. When they
disagree, the break is in the glue, and there are only three
places it can be.

## The ideas to take away

- **The callback's filter changed because the cargo changed.**
  Chapter 4 filtered by target change *and a ±10 cm deadband*,
  because the product was a distance feed. This product is a
  boolean: the only events are transitions (NONE → someone,
  someone → NONE), so the filter is a single comparison. Same
  shape, different question — the filter always encodes "what
  does the consumer consider news?"
- **Three contexts, three permissions.** The parser thread may
  filter and submit. The workqueue may read snapshots and
  schedule. Only the ZBOSS scheduler context writes attributes
  (`publish_occupancy`, via `ZB_SCHEDULE_APP_CALLBACK`) — the
  stack owns its tables, and guests knock first. If you are
  tempted to call `ZB_ZCL_SET_ATTRIBUTE` from the driver
  callback "just this once", reread Chapter 2's ISR golden rule
  and generalise it: *callbacks into a subsystem you don't own
  happen in that subsystem's context, or not at all.*
- **A failed sensor must not take the network down.** If
  `ld2410_start()` fails, the app logs it loudly and the endpoint
  still joins, answers reads, and reports a truthful "UNOCCUPIED
  — because I have no sensor" rather than crashing into a
  steering loop. Products degrade in layers; demos collapse in
  one piece.
- **What the attribute cannot say.** Occupancy is a bitmap with
  one meaningful bit. Distance, motion energy, "the same person
  as before" — all of it was in the reading, and all of it was
  *deliberately* dropped at the glue, because the standard
  cluster is the contract the rest of the world codes against.
  Chapter 7's stretch exercise explores putting one piece of it
  back — as a standard attribute, not a hack.

## Exercises

1. Cover the sensor with a book (radar-transparent it is not).
   Then uncover it at a distance of ~3 m and walk *slowly*
   closer. Watch `presence` on the DK and attribute reads on the
   coordinator side by side: which transitions does the network
   see, and does the ±deadband's absence ever make the attribute
   flicker at the edge of range? If it flickers, name the exact
   line where a product would damp it — and why that damping is
   *not* in this step. (Chapter 8 is watching your answer.)
2. Unplug the LD2410's TX wire mid-run. Read the driver's
   counters via `presence`, then read the attribute from the
   coordinator. The network now believes the last state forever.
   Whose job *should* staleness be — driver, glue, or cluster?
   Defend your pick in three sentences; Chapter 4's stretch
   exercise (LIVE/STALE) is one legitimate implementation.
3. Read the sensor-type attributes (0x0001, 0x0002) from the
   coordinator one more time and write the one-paragraph defence
   of ULTRASONIC you would give a certification reviewer. File
   it. You will meet that reviewer again in Chapter 10's world,
   where the questions have invoices attached.

## Why this matters

Every real sensor product is this diagram: a transducer with a
protocol, a driver with a contract, glue with a filter, and a
standard interface with a vocabulary. Teams that keep those
layers honest can swap any one of them — different radar, same
endpoint; same radar, Matter instead of Zigbee — without
rewriting the rest. You just proved the swap works in one
direction: Step 1's endpoint never knew the thumb left.
