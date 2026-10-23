# Chapter 8 · Step 1 — Reporting: the sensor speaks first

**What you do:** Give the endpoint a **reporting context** (one
declaration change in the device header), then configure
reporting *from the coordinator* — `zcl subscribe` with a min and
max interval — and watch Occupancy reports arrive on the
coordinator's shell as people enter and leave the sensor's view.
No polling. No new application code: diff `src/main.c` against
Chapter 7's and find only comments.

**What this step teaches:** The difference between a device that
*can be asked* and a device that *keeps its subscribers
informed* — the difference, in Zigbee, between a demo and a
deployment. Reporting is a three-party contract: an attribute
declared reportable (Chapter 7), storage for the configuration
(this step's header), and a subscriber who sets the terms
(this step's lab).

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840 \
    course/repo/ch08/step-01-reporting -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
west flash
```

Before the lab, one control experiment. With the device joined,
recruit a volunteer and — from the coordinator — poll by hand,
roughly every five seconds, while they step in and out at random:

```
uart:~$ zcl attr read 0x<short> 10 0x0406 0x0104 0x0000
```

Sooner or later they will step in *and back out* between two of
your reads, and your log will contain no trace it ever happened.
Write down the time it happened. **Polling doesn't have a
latency problem; it has an epistemology problem** — it samples
states and is blind to events. Keep the notebook page; Step 2's
light bulb cannot be run on samples.

## Configure reporting from the coordinator's chair

```
uart:~$ zcl subscribe on 0x<short> 10 0x0406 0x0104 0x0000 24 0 60
```

Reading the arguments (Chapter 5's toolkit, now spent): the
device's short address and endpoint; cluster `0x0406`; HA
profile `0x0104`; attribute `0x0000`; data type **24** (decimal
for 0x18, ZCL's bitmap8); then **min 0 s, max 60 s**. What you
just sent is a Configure Reporting command, and the device
answered it — because, and only because, the header now declares
a slot to store the terms in. The terms mean:

- **Min interval 0:** a change may be reported immediately. (A
  nonzero min would rate-limit a chatty attribute — for an
  analog value flapping around a threshold, that is mercy; for
  a boolean, immediate is honest.)
- **Max interval 60:** even if *nothing* changes, the device
  must report at least once a minute. This is not chatter; it is
  a **heartbeat with evidentiary value** — if minute 61 passes
  in silence, the listener is entitled to conclude the device
  is gone, not that the room stayed empty. Step 2's vanishing
  bulb discussion stands on this.
- **Reportable change:** for a discrete type like bitmap8, *any*
  change is reportable. (Analog types take a delta — the
  attribute reports when it has moved by at least that much,
  which is how real sensors avoid narrating their own noise.)

Now have the volunteer walk in and out. Reports land on the
coordinator's shell as they happen; the in-and-out that polling
missed is now an event with a timestamp. When you are done,
`zcl subscribe off` with the same arguments stops the flow —
configuration is a conversation, not a tattoo. (It is also, in
ZBOSS, kept in the reporting context you declared; reboot the
DK and re-check whether the coordinator still receives reports.
What you observe is the persistence question every product team
must answer for *its* stack configuration — Chapter 9's NVRAM
story is adjacent.)

## The ideas to take away

- **The writer writes; the engine reports.** `publish_occupancy()`
  did not gain a single line this step. `ZB_ZCL_SET_ATTRIBUTE`
  stores the value and the reporting engine — configured over
  the air — decides what leaves the device, and when. Separation
  of "what is true" from "who gets told" is why a sensor's
  firmware doesn't change when a new subscriber appears.
- **Reporting configuration is data with an address.** It lives
  in a fixed table the *device* allocates (`reporting_info`, one
  slot here) but the *network* writes. Sized too small, and the
  second subscriber's Configure Reporting is refused; that
  refusal is a design decision you made at compile time,
  possibly without noticing. Count your subscribers when you
  size the table. (Chapter 8's product has exactly one reporting
  consumer — the coordinator/hub view. The bulb in Step 2 does
  *not* consume reports; it receives commands. Keep the two
  channels separate in your head.)
- **Min, max, and change are a policy, stated in numbers.**
  "Report occupancy changes immediately, prove liveness every
  minute" is a *product requirement* that happens to compile
  into three integers in a shell command. When a deployment
  misbehaves — battery drain, missed events, a listener that
  can't tell death from silence — the post-mortem is usually
  found in these three numbers, chosen by someone who thought
  they were plumbing.

## Exercises

1. Set min 10, max 30 and have the volunteer cross the sensor's
   view four times inside ten seconds. Count the reports.
   Explain, from the min-interval rule, exactly which transitions
   the coordinator can and cannot reconstruct — and whether the
   *final* state it holds can ever be wrong. (It cannot; find
   out why the engine guarantees that, in the way it treats a
   change pending during the min window.)
2. Set max 0 (with min 0): what does a max of zero *mean* in the
   Configure Reporting semantics, and what happens to your
   liveness argument? Then set max 65535. Two very different
   silences; learn to hear both.
3. Size audit: a future version of this product must serve
   reports to *three* subscribers (hub, a logger, a wall panel).
   What exactly changes in the header, what does each extra slot
   cost (find the struct in the stack headers and do the RAM
   arithmetic), and what does a fourth subscriber's request get
   answered with?

## Why this matters

Polling is the shape of a prototype because it is easy to write
and easy to reason about — right up until the first missed
event, the first battery budget, the first "the dashboard said
the room was empty all night". Reporting is how Zigbee products
actually move state: declared, configured, bounded,
heartbeat-checked. You have now built both and *felt* the
difference with a notebook and a volunteer — which means you
will recognise the smell of a polling design in the field,
including, especially, your own.
