# Chapter 6 · Step 2 — Join the IKEA bulb: a device you cannot reprogram

**What you do:** Factory-reset a retail IKEA TRÅDFRI bulb and steer
it onto the same coordinator network that already holds the
Chapter 5 DK and your Step 1 skeleton — entirely from the
coordinator shell. The bulb ships with firmware you will never see;
everything you learn about it, you learn through the toolkit.

**What this step teaches:** Commissioning's hard case — a *closed*
device. No logs, no shell, no source: one documented reset ritual,
one association handshake, and whatever the network can be asked.
This is the commercial reality Zigbee interoperability is built
for, and the moment the course's network stops being a lab demo
and starts being a *system*.

## Before you start

- Coordinator dongle running (Chapter 5, Step 1), terminal open.
- Step 1's skeleton joined and listed — it stays in the picture;
  networks are cumulative, and by the end of this step yours has
  three members.
- The bulb screwed into a lamp you can switch from the wall or the
  lamp itself. You will power-cycle it six times; use the switch,
  not the socket.

## 1. Open the network — and watch the door

A formed coordinator does not admit strangers forever: BDB
commissioning opens a joining window, and this lab works inside
one. On the coordinator:

```
uart:~$ nbr monitor on
Done

uart:~$ bdb start
Started coordinator
Done
```

`bdb start` on an already-formed coordinator re-runs top-level
commissioning, which re-opens the permit-join window (180 s in
ZBOSS's BDB defaults). `nbr monitor on` arms the neighbor-table
watcher from Chapter 5: when any device associates with the
coordinator, the table change is logged the moment it happens.

## 2. The IKEA ritual

TRÅDFRI bulbs carry exactly one user interface: the power switch.

1. Start with the bulb **on**.
2. Toggle it **off–on six times** (off, on, off, on … six full
   cycles), at a steady, unhurried pace — roughly one toggle per
   second.
3. Leave it **on** after the sixth. Expected behaviour: the bulb
   **pulses once** — dims and brightens — announcing its factory
   reset, then begins searching for a network.

A reset bulb holds no network: no key, no address, no memory of
its previous home. It is, for commissioning purposes, a newborn —
and a newborn steers immediately, for a limited window of its
own. Bulb and coordinator are now both "looking" at the same time.
That overlap is not a detail; it *is* the join.

## 3. Watch it land

Expected behaviour on the coordinator terminal, within seconds of
the pulse: the neighbor monitor logs the table growing — a new
entry appears with the bulb's EUI-64, device type **router**
(TRÅDFRI bulbs relay like any mains-powered device) — followed by
the association completing. Confirm with the listing:

```
uart:~$ zdo mgmt_lqi 0x0000
```

You are reading the coordinator's neighbor table over the air
(well — asking the coordinator to read its own). It should now
show your growing network: the DK and/or the bulb as direct
neighbors at their short addresses (a device that joined *through*
the DK lists the DK as its parent path, and appears in the DK's
table instead — ask `zdo mgmt_lqi 0x<dk-short>` too; where a
device sits teaches you how meshes actually form).

Take the bulb's short address from whichever table holds it and
introduce yourself properly:

```
uart:~$ zdo active_ep 0x<bulb>
uart:~$ zcl attr read 0x<bulb> 1 0x0000 0x0104 0x0004
```

Expected behaviour: endpoint **1** is the bulb's main endpoint,
and the Basic cluster's ManufacturerName attribute (0x0004) answers
as a character string — `IKEA of Sweden`. You just read a closed
retail product's identity card using nothing but the standard
clusters every Zigbee device must carry. ModelIdentifier
(0x0005) names the exact product the same way.

## 4. First contact

Two proofs that the join is real, both from the shell:

```
uart:~$ zcl attr write 0x<bulb> 1 0x0003 0x0104 0x0000 0x21 0x05
```

—the bulb breathes/pulses for five seconds (Identify). Then the
command this whole course is walking toward:

```
uart:~$ zcl cmd 0x<bulb> 1 0x0006 0x01
```

(On/Off cluster, command `0x01` = On — the bulb turns on;
`0x00` turns it off. You are sending raw ZCL by hand, once, so
that Chapter 8's binding — where the *sensor* sends it — reads as
automation of something you have already done manually.)

## The ideas to take away

- **Joining is two windows overlapping.** The network must be
  permitting (coordinator's commissioning window) *while* the
  device is steering (bulb's post-reset search). Most "it won't
  join" stories are window stories: one side was ready at 10:00,
  the other at 10:05. Chapter 6's lesson draws the full exchange;
  this lab makes you schedule it.
- **Factory reset is a protocol feature, not a support trick.**
  On our skeleton it is a 5-second button hold handled by app
  utils; on the bulb it is six power cycles handled by IKEA's
  firmware; in ZDO it can even be requested remotely
  (`zdo mgmt_leave`). Three mechanisms, one semantics: *forget
  the network completely — keys, addresses, bindings — and be
  ready to join as new.* Chapter 9's rollback logic is the same
  philosophy pointed at firmware.
- **A closed device is not an opaque device.** Standard clusters
  (Basic, Identify, On/Off) and standard discovery (`active_ep`,
  `simple_desc_req`, `mgmt_lqi`) describe any compliant product
  well enough to commission and control it. That is the entire
  commercial bet of Zigbee, and you just collected on it.

## Exercises

1. Reset the bulb again and repeat the join — but start the
   ritual *before* running `bdb start` on the coordinator, so
   the bulb steers first. Does it still land? How long did each
   side wait, and what does that teach you about the two windows'
   lengths?
2. Move the bulb (or the lamp) to the far end of the house and
   re-join from reset. Does it associate with the coordinator
   directly or through the DK/skeleton? Prove your answer from
   the neighbor tables, and note the LQI difference.
3. Break it on purpose: factory-reset the *coordinator*
   (`bdb factory_reset`, then form a *new* network with
   `bdb role zc` + `bdb start`). The bulb and skeleton now belong
   to a network whose coordinator no longer exists. Describe
   exactly what each device does — LEDs, logs, behaviour over
   five minutes — before you fix anything. This is the
   fault tree the lesson's stretch exercise formalises.

## Why this matters

Interoperability is not a logo on a box; it is this afternoon: a
bulb from a furniture store, a development kit from a chip vendor,
and firmware you wrote, in one network, commissioned with one
toolkit. Installers do this at scale with worse tools and no
source access. You now know what they know — plus what the
packets are doing, which most of them were never taught.
