# Chapter 6 · Step 1 — Our router skeleton: firmware that joins

**What you do:** Build *our own* firmware — adapted from the add-on's
`template` sample — for the DK, and watch it steer itself onto the
dongle coordinator's network from Chapter 5. This skeleton is the
base Chapters 7 and 8 extend: same startup, same signal handler,
same factory reset — only the endpoint's clusters change.

**What this step teaches:** Where commissioning lives in real
firmware. There is no `join()` call in `main.c`, and finding out why
is the chapter: the stack's startup signals, handled by the add-on's
*default signal handler*, start BDB **network steering** for you.
Your code's responsibilities are the honest ones — register an
endpoint, keep an LED truthful, offer a way back (factory reset).

## Build, flash, observe

From your NCS workspace root (the step is a standalone app in this
repo; the add-on module flag is the same one Chapter 5 used):

```bash
west build -p always -b nrf52840dk/nrf52840 \
    course/repo/ch06/step-01-router-skeleton -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
west flash
```

Power the dongle coordinator first (Chapter 5, Step 1 — its network
must exist before anything can join it), then the DK. Expected log
on the DK (yours will show your own PAN IDs — the *lines* are the
contract):

```
[00:00:00.512,000] <inf> app: Starting Zigbee, Engineered - occupancy sensor skeleton
[00:00:00.519,000] <inf> app: Skeleton started - steering will begin if no network is stored
[00:00:00.6xx,000] <inf> zigbee_app_utils: Zigbee stack initialized
[00:00:00.6xx,000] <inf> zigbee_app_utils: Device started for the first time
[00:00:00.6xx,000] <inf> zigbee_app_utils: Start network steering
...
[00:00:03.4xx,000] <inf> zigbee_app_utils: Joined network successfully (Extended PAN ID: 0b010e…, PAN ID: 0x1a2b)
```

and **LED 3 on** — joined. Cross-check the Extended PAN ID and PAN
ID in that line against the identity card you wrote down in
Chapter 5: they must match the coordinator's `bdb panid` /
`bdb extpanid` readback exactly. Your firmware just proved, in its
own log, that it joined *your* network and not a neighbour's.

## See it from the coordinator's chair

On the dongle terminal, the Chapter 5 toolkit pays off:

```
uart:~$ zdo mgmt_lqi 0x0000
```

—the skeleton now sits in the coordinator's neighbor table as a
router. Find its short address there (or watch for it), then:

```
uart:~$ zdo active_ep 0x<short>
uart:~$ zdo simple_desc_req 0x<short> 10
uart:~$ zcl attr read 0x<short> 10 0x0000 0x0104 0x0000
```

Expected behaviour: endpoint **10** answers (the number `main.c`
chose), the simple descriptor lists Basic (`0x0000`) and Identify
(`0x0003`) as server clusters, and the read returns the Basic
cluster's ZCLVersion — the same reply shape you rehearsed in
Chapter 5, now produced by *our* code. Then make it blink from
across the network: write IdentifyTime and watch LED 4:

```
uart:~$ zcl attr write 0x<short> 10 0x0003 0x0104 0x0000 0x21 0x0F
```

(Identify cluster, attribute `0x0000`, type `0x21` = unsigned 16-bit,
value 15 seconds — the shell reference's own example, aimed at our
endpoint.)

## Reset means reset

Hold **Button 4 for ~5 seconds** on the DK. The app-utils factory
reset helper fires: the device erases its stored network state,
reboots, finds NVRAM empty — and the log shows the whole story
again: "Device started for the first time", steering, join. Two
properties to verify before you trust it:

1. **Rejoin is automatic** — a reset device does not need your
   permission to come back; steering restarts on its own.
2. **The coordinator may hand back a different short address.**
   The old one was the coordinator's bookkeeping, and reset tore
   up both copies of the contract.

Also reboot the DK *without* resetting (just power-cycle): the log
now reads "Joined network successfully on reboot signal" and LED 3
lights almost immediately — no steering, because the network was
never forgotten. Stored state is the difference between a
commissioning event and a non-event; Chapter 9's bootloader work
will lean on the same idea.

## The ideas to take away

- **Commissioning is a signal-driven state machine, not a function
  call.** `main()` ends at `zigbee_enable()`; everything after —
  steering, association, key transport, the *Device Announce* —
  arrives as signals to `zboss_signal_handler()`, which delegates
  to `zigbee_default_signal_handler()`. When a join misbehaves,
  that handler's log lines are your flight recorder.
- **Steering has a budget.** The default handler steers in tracked
  10-minute windows with retries; a device powered before its
  coordinator exists does not join "eventually, somehow" — it
  joins inside its window or goes quiet until the next start.
  Products choose these numbers deliberately; now you know where
  they live.
- **The skeleton is the product's chassis.** Chapter 7 swaps the
  range-extender placeholder for the Occupancy Sensing endpoint
  and wires in Chapter 4's driver — startup, steering, reset and
  this step's diagnostics all carry over untouched. Get the
  chassis right once; spend the rest of the course on the payload.

## Exercises

1. Join with the coordinator *off*: flash, power the DK first, and
   read the steering log lines for a full minute. What repeats,
   what doesn't, and what does the LED claim during all of it?
   Then power the coordinator on and time the join.
2. From the coordinator, `zdo mgmt_leave 0x<coord-view addr>` the
   skeleton off the network (aim the management command at the
   device; read the shell reference for its exact argument shape).
   Watch both logs: what does each side believe afterwards, and
   what single DK action makes it rejoin?
3. Predict, then verify: after a factory reset, does the *bulb*
   in Step 2 still work? (It should — reset is per-device state.
   The interesting follow-up: how many independent resets can a
   network absorb before "the system" is gone? Answer carefully —
   the coordinator is a device too.)

## Why this matters

Every commercial Zigbee device you will ever debug boots into
exactly this situation: firmware that must find a network it cannot
see, prove it arrived, and offer a dignified way back to zero. You
built that chassis from a vendor template, read its commissioning
in its own logs, and interrogated it from a shell you trust. The
sensor Chapters 7–8 add is now "just" an endpoint — which is what
good infrastructure does to hard problems.
