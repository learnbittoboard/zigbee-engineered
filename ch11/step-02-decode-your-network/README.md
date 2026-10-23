# Step 2 — The packet-decode worksheet: your network, frame by frame

**Goal:** take the network you have been building for ten chapters
and read its traffic the way the stack sees it — every layer, every
field, with *your* values as the answers. Four targets: the join,
one occupancy report, one bulb command, and (stretch) the OTA
transfer.

Work through it with a capture open. Every `☐` is a value you read
off *your own* frames — if a value differs from the "expected"
column, your capture is right and the worksheet wants to know why.

## 0. Install the course Wireshark profile first

Next to this README, in `wireshark-profile/`, are two real files:

- `colorfilters` — 10 coloring rules tuned to this course's
  traffic (join frames, occupancy reports, bulb commands, OTA).
  First match wins, so the specific rules sit above the general
  ones.
- `dfilters` — 14 saved display filters, the same set this
  worksheet tells you to apply, by name.

To use them: in Wireshark, open **Help → About Wireshark →
Folders** and find your *Personal configuration* folder. Inside
its `profiles/` subfolder create a folder named `ZigbeeCourse`,
copy both files into it, then switch to it from the profile menu
in Wireshark's status bar (or **Edit → Configuration Profiles**).
Your packet list should immediately light up with the course
colors on any capture.

(The values in these files were syntax-checked field by field
against Wireshark's Display Filter Reference at course build
time. The boundary note at the end says exactly what that does
and does not prove.)

## 1. Your network identity card

Fill this in from the coordinator shell *before* capturing. Every
later checkpoint compares a frame against this card.

| Fact | Shell command | Your value |
|---|---|---|
| PAN ID (16-bit) | `bdb panid` | ☐ `0x________` |
| Extended PAN ID | `bdb extpanid` | ☐ `________________` |
| Channel | `bdb channel` | ☐ `____` |
| Network key (the one you set) | *(Step 1)* | ☐ `________________________________` |
| Coordinator EUI64 | `zdo eui64` | ☐ `________________` |
| Coordinator short address | *(always)* | `0x0000` |
| Sensor DK short address | `nbr` (neighbour table) | ☐ `0x________` |
| Sensor DK EUI64 | `zdo ieee_addr 0x<short>` | ☐ `________________` |
| Bulb short address | `nbr` | ☐ `0x________` |
| Bulb EUI64 | `zdo ieee_addr 0x<short>` | ☐ `________________` |

Keep the card next to the capture. A packet decode that does not
tie back to a shell fact is just reading numbers aloud.

## 2. Target A — decode your own join

**Capture it fresh.** Start the sniffer (Step 1) on your channel.
On the sensor DK, leave and re-join the network the Chapter 6 way
(factory reset, then let it join). Stop the capture when the DK's
log says it is on the network. Apply the saved filter
**"Join: the whole MAC exchange"**, then also hunt the two frames
that filter cannot show (they are Zigbee-layer, not MAC): filter
**"Join: Transport Key only"** and **"Join: Device Announce
only"**.

Walk the cast in order. For each frame: click it, expand the
protocol tree top-down, and check the fields.

### Frame 1 — Beacon Request *(the question)*

`IEEE 802.15.4 → Frame Control / Command Frame`

- ☐ Frame type is **Command** (`wpan.frame_type` = 3)
- ☐ Command ID is **0x07 — Beacon Request** (`wpan.cmd`)
- ☐ Destination PAN and address are broadcast (`0xFFFF` / `0xFFFF`)
- Meaning: "any coordinators here?" — the exact scan Chapter 6
  drew in its sequence diagram, now with a frame number.

### Frame 2 — Beacon *(the answer)*

- ☐ Frame type is **Beacon** (`wpan.frame_type` = 0)
- ☐ Source is the coordinator: short `0x0000`
- ☐ Source PAN ID = **your PAN ID** from the identity card
- ☐ In the Zigbee beacon payload, Extended PAN ID =
  **your extended PAN ID** (`zbee_beacon.ext_panid`)
- If a neighbour router also answers: its beacon carries the same
  two IDs. Two adverts, one network.

### Frame 3 — Association Request *(the application form)*

- ☐ Command ID **0x01** (`wpan.cmd`), destination = coordinator
- ☐ **Source EUI64 = your sensor's EUI64** (`wpan.src64`) — the
  first time in the capture the device's true identity appears
- ☐ The capability information in the payload says what kind of
  device is asking (router, receiver-on-when-idle — Chapter 6's
  FFD story)

### Frame 4 — Data Request *(the polite poll — may or may not appear)*

- ☐ Command ID **0x04** (`wpan.cmd`), from the joiner
- Some stacks deliver the response immediately; some hold it until
  the joiner polls. Both are legal. Note which yours did: ______

### Frame 5 — Association Response *(you're in — here's your number)*

- ☐ Command ID **0x02** (`wpan.cmd`)
- ☐ Assigned short address (`wpan.asoc.addr`) = **your sensor's
  short address** from the identity card — this frame is where
  that number is *born*
- ☐ Association status = success

### Frame 6 — Transport Key *(the crown jewels — colored red)*

- ☐ APS command frame, command ID **0x05** (`zbee_aps.cmd.id`),
  from `0x0000` to the sensor's new short address
- ☐ The frame only opens because you configured the Trust Center
  link key in Step 1 — before that it was a sealed APS payload
- ☐ Inside, the transported key = **your network key**, byte for
  byte against the identity card. This is the moment the lesson
  is about: the network's master secret just crossed the room,
  protected by a key the whole world knows.
- ☐ Note the NWK-layer frame counter on this and nearby frames
  (`zbee.sec.counter`): it only ever goes up. That monotonic
  counter is the replay protection.

### Frame 7 — Device Announce *(the introduction)*

- ☐ Broadcast, ZDO cluster **0x0013** (`zbee_aps.zdp_cluster`)
- ☐ The payload carries **both** addresses of the new device —
  short and EUI64 together. This frame is the network's phone
  book entry; it is how `zdo`-style discovery (Chapter 5) can
  work at all.
- ☐ Sender: the sensor itself, from its brand-new short address.

### Aftermath frames (proof the NWK layer settled)

- ☐ NWK **Link Status** commands (`zbee_nwk.frame_type` = 1)
  appear from the new router and repeat periodically — routers
  advertising their links. Chapter 6 promised routers maintain
  the mesh; this is the maintenance, on air.

**Target A is done when every ☐ above holds a value from your
capture — and Frames 5 and 6 match the identity card exactly.**

## 3. Restore the Chapter 8 state (two commands)

Your fresh join wiped the sensor's Chapter 8 configuration — the
reporting record and the binding lived on the sensor, and the
factory reset cleared them (Chapter 9's lesson: state has an
address). Restore both from the coordinator shell, with the
*new* short addresses from your identity card:

```
uart:~$ zcl subscribe on 0x<sensor short> 10 0x0406 0x0104 0x0000 24 0 60
uart:~$ zdo bind on <sensor eui64> 10 <bulb eui64> 1 0x0006
```

While the subscribe command runs, watch the capture: the
Configure Reporting exchange (ZCL command `0x06`,
`zbee_zcl.cmd.id`) is itself decodable with everything you now
know. Then continue.

## 4. Target B — decode one occupancy report

Trigger traffic: capture while you walk into the LD2410's view,
stay a few seconds, walk out. Apply **"Occupancy reports"**
(`zbee_aps.cluster == 0x0406 && zbee_zcl.cmd.id == 0x0a`). Pick
one report and dissect it top to bottom:

| Layer | Field | Expected (your network) | Yours |
|---|---|---|---|
| TAP | Channel (`wpan-tap.ch_num`) | your channel | ☐ |
| TAP | RSSI (`wpan-tap.rss`) | a live dBm value — how loud the sniffer heard it | ☐ |
| MAC | Dst / Src (`wpan.dst16` / `wpan.src16`) | next hop: sensor → coordinator directly | ☐ |
| MAC | PAN ID | your PAN ID | ☐ |
| NWK | Source (`zbee_nwk.src`) | sensor's short address | ☐ |
| NWK | Destination (`zbee_nwk.dst`) | `0x0000` — the subscriber | ☐ |
| NWK | Security frame counter (`zbee.sec.counter`) | higher than the last frame you checked | ☐ |
| APS | Source endpoint (`zbee_aps.src`) | **10** — the occupancy endpoint | ☐ |
| APS | Destination endpoint (`zbee_aps.dst`) | **64** — the coordinator's *shell* endpoint; that is who subscribed | ☐ |
| APS | Cluster (`zbee_aps.cluster`) | `0x0406` Occupancy Sensing | ☐ |
| ZCL | Command (`zbee_zcl.cmd.id`) | `0x0a` Report Attributes (a *foundation* command) | ☐ |
| ZCL | Attribute ID (`zbee_zcl.attr.id`) | `0x0000` Occupancy | ☐ |
| ZCL | Attribute type | `0x18` bitmap8 — the `24` from your subscribe command | ☐ |
| ZCL | Value (`zbee_zcl.attr.bitmap8`) | bit 0: **1 = you in the room, 0 = room empty** | ☐ |

Three captures to find, one of each:

- ☐ A report with value **1** while you stood in view.
- ☐ A report with value **0** after you left (the sensor reports
  the *change*, min interval 0 — Chapter 8's terms).
- ☐ A **heartbeat**: the same value re-sent with nobody moving,
  within ~60 s — the max interval from the same subscribe
  command, doing exactly what it promised.

**Target B is done when the table is filled from one real frame
and all three report kinds are identified.**

## 5. Target C — decode the bulb obeying (binding, on air)

Keep capturing. Walk in: the bulb turns on. Walk out and wait:
~30 seconds after the room reads empty (Chapter 8's OFF delay),
the bulb turns off. Apply **"Bulb On/Off commands"**.

Pick the ON frame and check:

- ☐ NWK source = **sensor**, destination = **bulb** — and the
  coordinator's `0x0000` appears *nowhere* in this frame. That
  absence is Chapter 8's whole argument, proven on air: binding
  means the coordinator is not in the loop.
- ☐ APS endpoints: source **10** → destination **1** (the bulb's
  endpoint from Chapter 6)
- ☐ Cluster `0x0006` On/Off; command (`zbee_zcl.cs.cmd.id` —
  cluster-specific this time, not foundation) = **0x01 On**
- ☐ The matching OFF frame later: same shape, command **0x00**
- ☐ MAC vs NWK destinations: in this three-device network they
  match (direct neighbours). In a bigger mesh the MAC frame
  would name a *relay* while the NWK destination stayed the
  bulb — that split is what routing means. Note it now; you
  will recognise it in any Zigbee capture forever.

**Target C is done when you can point at the ON frame and say,
field by field, why the coordinator had nothing to do with it.**

## 6. Stretch target — decode the Chapter 10 OTA transfer

Rebuild the Chapter 10 desk: OTA server holding the 1.3.0
package, the DK running the 1.2.0 FOTA build, sniffer running.
Start the capture *before* the client asks for its upgrade, and
let it run to the reboot. Apply **"OTA: all upgrade traffic"**
(cluster `0x0019`).

- ☐ **The gates.** Find the **Query Next Image** request
  (`zbee_zcl.cs.cmd.id` = `0x01`): the client presents
  manufacturer `0x127F`, image type `0x0141`, current file
  version `0x01020000` (the 1.2.0 build). Find the **Response**
  (`0x02`): status SUCCESS, offered file version `0x01030000`.
  Those two frames are Chapter 10's version gates, visible.
- ☐ **Count the blocks.** Apply **"OTA: Image Block Requests
  only (count these)"** (`0x03`) and read Wireshark's *Displayed* count:
  ☐ ______ blocks. Expected: ≈ **6,800** (the package payload,
  434,262 bytes, ÷ the 64-byte block size).
- ☐ **Spot the retries.** Find two block requests carrying the
  *same file offset*: a lost response, re-asked. Retries are
  the reliability machinery working — count unique offsets for
  the true block count, not raw frames.
- ☐ **The handshake at the end.** **Upgrade End Request**
  (`0x06`) → **Upgrade End Response** (`0x07`) → the DK reboots,
  re-announces (Target A's Frame 7, an old friend), and the
  version it presents from now on is `0x01030000`. Chapter 10's
  DFU zip just crossed the room in ~64-byte pieces, and you
  watched every gate it passed.

## 7. When the capture says a join is broken

The capstone skill: read a *failed* join. Find the last frame
that succeeded; the failure lives in the silence after it.

| You see | It stops after | Likely meaning |
|---|---|---|
| Beacon Requests, no Beacon | Frame 1 | Nobody joinable is answering on this channel — wrong channel, coordinator down, or its beacon carries "no association permitted" |
| Beacon, no Association Request | Frame 2 | The joiner rejected what it heard (wrong network, or it was told to look elsewhere) |
| Association Request, no Response | Frame 3 | Steering is closed — the permit-join window (Chapter 5's `bdb start` / opening) was not open |
| Association Response, no Transport Key | Frame 5 | The Trust Center refused or the joiner was dropped — security policy, not radio |
| Transport Key, no Device Announce | Frame 6 | The joiner rejected the key material (or never received the frame) and silently walked away — devices do not send error letters |
| Device Announce, then nothing | Frame 7 | The network layer is fine; the *application* never started — wrong image, crashed app, wrong endpoint story |

And the meta-rule, worth more than the table: **a missing frame
is evidence, not proof.** The sniffer hears one channel, from one
chair, with no obligation to catch everything. Absence in a
capture means "not seen", never "not sent" — verify at the shell
before you conclude.

## Honest boundary

This worksheet's field names, command IDs, and expected values
were grounded at course build time in: the ZBOSS headers of the
pinned stack (ZCL foundation and OTA/On-Off command IDs), the
coordinator shell's own source (the `bdb nwkkey` procedure and
its error strings), Nordic's sniffer sample and user guide, and
Wireshark's Display Filter Reference (every filter field in the
profile files was checked against it; the profile files also
passed a strict syntax parse). The protocol constants from the
Zigbee and IEEE 802.15.4 specifications (APS Transport Key 0x05,
Device Announce cluster 0x0013, the MAC command IDs) are the
specifications' own tables.

What could not be done in the course's build environment: it has
no radios, so no capture was taken there, and Wireshark itself
was not installed to click through. Runtime behaviour above is
written as expected output from the documented behaviour of the
pinned firmware and stack. Your desk is the first real capture —
which is, honestly, the correct order for a capstone.
