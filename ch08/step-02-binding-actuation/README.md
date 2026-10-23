# Chapter 8 · Step 2 — Binding & actuation: occupancy switches the bulb

**What you do:** Add an **On/Off client cluster** to the
endpoint, wire the coordinator-side **binding** with one shell
command, and close the loop the whole course has been aiming
at: walk into the room and the IKEA bulb turns **on**; walk out,
wait out the hold time, and it turns **off** — with no hub, no
cloud, and no code on either device that knows the other's
address.

**What this step teaches:** The second output channel of a
Zigbee device. Reporting (Step 1) publishes *truth* to
subscribers. Commands *act* on the world — and binding is what
decouples the actor from the address: our firmware sends "On"
to *the binding*, and the network figures out the rest.

## Build, flash, set the stage

```bash
west build -p always -b nrf52840dk/nrf52840 \
    course/repo/ch08/step-02-binding-actuation -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
west flash
```

You need three citizens on the Chapter 5 network: the
coordinator (dongle), this firmware on the DK (joined — reboot
join is fine), and the IKEA bulb from Chapter 6, Step 2, joined
and answering. Find, from the coordinator:

- both devices' short addresses (`zdo mgmt_lqi 0x0000`),
- the bulb's light endpoint (`zdo active_ep 0x<bulb short>`,
  then `zdo simple_desc_req` — TRÅDFRI bulbs answer on
  endpoint **1**, with On/Off as a *server* cluster),
- both EUI-64s: `zdo ieee_addr 0x<short>` for each.

## Create the binding — the wiring is a shell command

```
uart:~$ zdo bind on <sensor eui64> 10 <bulb eui64> 1 0x0006
```

Dissect it, because this one line *is* the installation:
**source** = our sensor (EUI-64, endpoint 10), **destination** =
the bulb (EUI-64, its light endpoint), **cluster** = `0x0006`
On/Off. The command is executed by the coordinator but the
resulting binding-table entry is stored **on our sensor** — the
coordinator was the electrician, not the switch. From now on,
any On/Off command our endpoint sends with binding addressing
is delivered by APS to that destination. Our firmware was never
told the bulb exists.

## Close the loop

Stand in front of the sensor. The DK log:

```
<inf> app: Occupancy attribute -> OCCUPIED
<inf> app: On/Off: sent ON via binding
```

—and the bulb is on. (The attribute line matters too: Step 1's
reporting still runs in parallel, informing subscribers; two
channels, two audiences, one event.) Now step out and watch the
part products get wrong:

```
<inf> app: Occupancy attribute -> UNOCCUPIED
<inf> app: On/Off: room empty - OFF in 30 s unless presence returns
```

The attribute tells the truth *immediately*; the bulb obeys
*30 seconds later* — `CONFIG_OCCUPANCY_OFF_DELAY_SEC`, this
app's Kconfig knob (default 30, set in `Kconfig`, overridable in
prj.conf). Step back in at second 20: the pending Off dies
quietly, the bulb never flickers, and no command is sent at all
(we already asked for On; asking again would be noise — see
"last commanded" in the code). Leave for real: at 30 s,
`On/Off: sent OFF via binding`, and the room goes dark.

**Prove the binding is the mechanism** (never skip this):
`zdo bind off` with the same arguments, then walk in. The DK
still logs `sent ON via binding` — the firmware cannot tell the
difference — and the bulb stays dark. The send succeeded; the
*address book* was empty. Re-bind and the loop closes again.
Binding is not a detail of the system. Binding **is** the
system's wiring diagram, stored in the devices.

On the DK, `presence` now closes the loop from our side too:

```
uart:~$ presence
presence : YES (stationary)
distance : 143 cm
attribute: Occupancy = 1 (OCCUPIED)
bulb     : last commanded ON
frames   : ...
```

## When the bulb vanishes

Unplug the bulb. Walk in and out. The sensor logs its sends,
serenely — and nothing happens, anywhere, with no error on any
console. Dissect the silence, because it is three separate
design facts wearing a trench coat:

1. **Commands here are fire-and-forget.** We send with the
   default response disabled and keep no delivery state; APS
   retries exhaust themselves against a dead route and the
   packet dies unreported. The sensor's `bulb: last commanded`
   line is a *belief*, labelled as one in the code.
2. **The binding outlives the bound.** The table entry is not a
   connection; nothing ages it out. A replaced bulb (new EUI-64)
   leaves the old binding pointing at a ghost — re-binding is
   part of *any* device-replacement procedure, in products and
   in this lab.
3. **Evidence exists if you go and look.** From the
   coordinator, `zcl attr read <bulb> 1 0x0006 0x0104 0x0000`
   reads the bulb's actual OnOff attribute — when it answers.
   Products that *care* close the loop with exactly such
   read-backs or with Step 1's reporting on the bulb's own
   attribute. Products that don't care ship the trench coat.

Chapter 11's sniffer will show you the vanished-bulb case at the
packet level: the command, the retries, the silence — the most
instructive traffic in the course.

## The ideas to take away

- **Two channels, two verbs.** *Report* = "here is what is
  true", pushed to subscribers under configured terms. *Command*
  = "do this", sent to bound destinations. The sensor uses both
  about the same physical event, to different audiences, under
  different timing rules (truth: immediately; actuation: after
  a hold). Collapsing them into one mechanism is how occupancy
  systems end up either chatty or cruel.
- **The timeout is a policy, and it lives where policies
  belong.** Semantically it is the ZCL delay the spec defines
  for ultrasonic sensors (occupied→unoccupied delay); ZBOSS's
  standard attribute list doesn't declare those attributes, so
  the product implements the policy in its glue and configures
  it in Kconfig — named, ranged, documented, overridable per
  build. The same number as a `#define` in main.c would work
  identically and mean less.
- **Belief is labelled.** `bulb_belief` is called a belief in
  the code, starts UNKNOWN, and re-sends on transitions into
  uncommanded states. Distributed systems run on such labelled
  beliefs; the bugs live in the ones mislabelled as facts.

## Exercises

1. Set `CONFIG_OCCUPANCY_OFF_DELAY_SEC=0` in prj.conf, rebuild,
   and live with it for ten minutes in a real room: every radar
   dropout becomes a darkness event. Then set 120 and feel the
   opposite failure (a room lit for nobody). Write down the
   number you would ship for (a) a bathroom, (b) an office, (c)
   a stairwell — and the reasoning, in the language of false-on
   vs false-off costs.
2. Bind a *second* destination: the Chapter 5 DK shell device
   also carries an On/Off server? (Check its descriptor first —
   if it doesn't, bind the bulb twice via a second endpoint
   story of your choosing, or bind our sensor's occupancy
   *reporting* and commands to two different audiences.) One
   send, two destinations: what does APS do, and what does that
   make a binding, grammatically — a wire, or a mailing list?
3. Replace-the-bulb drill: factory-reset the bulb, rejoin it,
   and list everything that must be redone before the loop
   closes again. (The bulb may keep its EUI-64 across a reset —
   so does the loop revive by itself? Verify, and explain which
   piece of stored state on *our* device made the difference.)

## Why this matters

This is the chapter where the course stops being about a sensor
and becomes about *systems that act*. A hub-mediated design
would route this through a coordinator, an automation engine,
and two network hops of latency and failure modes; the Zigbee
design binds device to device and keeps working with the
coordinator unplugged (try it — the loop survives; only your
shell view dies). Direct binding, honest timeouts, labelled
beliefs: that is the professional pattern, and your desk just
ran it end to end.
