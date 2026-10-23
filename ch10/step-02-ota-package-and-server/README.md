# Chapter 10 · Step 2 — The package and the server: v1.3.0, served

**What you do:** Build the image Step 1's client will
actually fetch — v1.3.0, with a behavioural canary and the
hardware-version window switched on — and then run the
**server side** of the OTA conversation: a spare board as
the radio, nrfutil as the librarian, your `.zigbee` file
as the book. The client pulls it block by block while the
occupancy loop never stops, reboots into Chapter 9's trial,
confirms on join, and proves the update landed by
*behaving* differently: the bulb's OFF delay is now 10 s.

**What this step teaches:** That an OTA system is three
artifacts, not one: the **client** (Step 1), the
**package** (this step's build), and the **server policy**
(this step's procedure). Plus the packaging corollary this
step's first build earned the hard way: a policy that
isn't in the file isn't in the system.

## The package, parsed

Build first (this is Step 1's firmware with version stamps
at 1.3.0, `CONFIG_OCCUPANCY_OFF_DELAY_SEC=10`, and the HW
window below — code-identical otherwise; verified build:
app 434,088 B of the slot, 46.7%, same as Step 1 to the
byte count):

```bash
west build -p always -b nrf52840dk/nrf52840 \
    course/repo/ch10/step-02-ota-package-and-server -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
```

The build's last step writes
`127F-0141-01030000-occupancy.zigbee` (434,328 B). Parse it
next to Step 1's package and account for every differing
byte:

| Field | Step 1 package | This package | Source |
|---|---|---|---|
| Header length | 56 | **60** | HW fields added |
| Field control | 0x0000 | **0x0004** | bit 2: HW versions present |
| File version | 0x01020000 | **0x01030000** | imgtool stamp 1.3.0 |
| Min HW version | — | **52 (0x34)** | `CONFIG_ZIGBEE_FOTA_MIN_HW_VERSION` |
| Max HW version | — | **255 (0xFF)** | `CONFIG_ZIGBEE_FOTA_MAX_HW_VERSION` |
| Total size | 434,324 | 434,328 | the 4 header bytes |
| Payload | 434,262 B multi-image | 434,262 B multi-image | unchanged machinery |

### The pair rule (fix F7, learned by parsing)

This step's first build set only the *minimum* HW version
— and produced a header with field control still 0 and no
window in it, even though the build log showed the value
being passed to the generator. The generator's source
settles why: `zb_add_ota_header.py` writes the HW fields
**only as a pair** (both values must be present; they
share one field-control bit). The client-side gate was
configured; the *package* advertised nothing, because a
range with one end is not a range. Setting
`CONFIG_ENABLE_ZIGBEE_FOTA_MAX_HW_VERSION=0xFF` ("no
ceiling yet") completed the pair. Lesson, matching the
course's refrain: **the policy you intend is not the
policy you shipped until the artifact parses that way.**

## The canary

`CONFIG_OCCUPANCY_OFF_DELAY_SEC` drops from 30 to 10 in
this build's prj.conf — one visible line, chosen so the
update's success is provable by *behaviour*: after the
update, walk out of the room and count. Ten seconds to
dark: new firmware, really running. Thirty: you are
looking at the old image, whatever any version string
claims. Every test build for an update path deserves a
canary like this; version strings are claims, behaviour
is evidence.

## The server side, as a procedure

An honesty note before the commands, because this course
labels its evidence: the **build-side** of this step is
verified to the byte (above). The **over-the-air serving**
below is desk procedure — it needs your physical boards,
and nrfutil's `dfu` command is a separate install not
present in the course's build environment, so the exact
option spellings should be confirmed with
`nrfutil dfu zigbee --help` on your machine. The flow
itself is Nordic's documented one (the add-on's FOTA
library documentation describes the same discovery, query,
and server-managed upgrade the client implements).

You need: the DK running Step 1's client (v1.2.0, joined
to the Chapter 5 network, bulb bound and working) and a
**spare board** — the second DK, or the coordinator dongle
reflashed — as the server's radio.

1. **Flash the server radio** with the add-on's NCP
   sample (it ships `prj_dongle.conf` / `prj_usb.conf`
   variants for exactly this host-driven role):

   ```bash
   west build -p always -b nrf52840dk/nrf52840 \
       <workspace>/ncs-zigbee/samples/ncp -- \
       -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
   west flash
   ```

2. **Install the host tool** (once): `nrfutil install dfu`.
   The `dfu` command's `zigbee` subcommand is Nordic's OTA
   server host: it drives the NCP radio and serves image
   files to clients that come asking.

3. **Serve the package**, pointing the tool at the NCP
   board's serial port and this step's `.zigbee` file
   (option names per `nrfutil dfu zigbee --help`; the
   shape is):

   ```bash
   nrfutil dfu zigbee --file 127F-0141-01030000-occupancy.zigbee \
       --port <server board port>
   ```

   The server joins the network story at this point: it
   holds exactly one image, addressed (manufacturer,
   type, file version, HW window) to exactly our product.

4. **Watch the client do everything.** Expected sequence
   at your desk: within a query cycle, the client
   discovers the server, queries, and the DK's log begins
   `OTA download: 0% … 25% … 50% …` (the progress events
   Step 1 wired). ~6,800 blocks at 64 B payload — this
   takes minutes; use them to walk in and out and confirm
   the bulb still obeys *during* the transfer. Then
   `OTA download complete - rebooting into the new image's
   trial`, the swap pause, and `firmware v1.3.0`.
5. **Collect the verdicts, in order:** the confirm line
   (`joined and healthy - image CONFIRMED`) — Chapter 9's
   policy judging an air-delivered image; `mcumgr image
   list` over the cable showing slot 0 = 1.3.0 active +
   confirmed; and the canary: **the bulb now obeys 10 s.**
   Three independent witnesses, one conclusion.

## Version policy, posted on the wall

| Gate | Owner | This lab's setting | Refuses |
|---|---|---|---|
| Offer | the server (nrfutil + the file you give it) | only the 1.3.0 package is served | downgrades, wrong products — by silence |
| Identity + HW window | the client library | mfr 0x127F, type 0x0141, HW ∈ [52, 255] | foreign images, wrong-hardware images |
| Authenticity | MCUboot | ECDSA-P256 signature vs the burned-in key | everything unsigned or wrongly signed |

A downgrade in this system is not forbidden by physics —
it is declined at gate 1, by a human (or a release
process) choosing which file the server holds. Exercise 1
makes you feel that distinction.

## The ideas to take away

- **The server is a librarian, not a king.** It can offer,
  withhold, and pace. It cannot compel: acceptance is the
  client's gates, booting is the bootloader's signature
  check. Designs that give the server more power than that
  have moved the trust boundary without noticing.
- **Packages are addressed, not broadcast.** Manufacturer,
  type, file version, HW window — a `.zigbee` file is a
  letter with the address on the envelope, and the query
  protocol is the client reading its own name. Most "the
  update never arrived" field reports are addressing
  errors wearing a radio costume.
- **Policy travels in artifacts or it doesn't travel.**
  The HW window mattered only once it was *in the header*
  (F7). Audit policies where they execute: parsed headers,
  `.config` files, slot trailers — never intentions.

## Exercises

1. **The downgrade drill.** After v1.3.0 is confirmed and
   running, restart the server holding *Step 1's* package
   (file version 0x01020000). Observe a full query cycle:
   what does the client log, what does the server do, and
   at which gate was the downgrade declined? Now state how
   you *would* ship an intentional downgrade (a bad 1.3.0
   recall) and which single human action authorises it.
2. **The interruption drill.** Mid-transfer (note the
   percentage), unplug the server board for two minutes,
   then restore it. With this course's configuration
   (download resume off), describe the client's recovery
   from its logs: where does the transfer restart, what
   happened to the partial bytes in slot 1, and what would
   `CONFIG_ZIGBEE_FOTA_DOWNLOAD_RESUME` have changed —
   at what cost (the lesson's stretch exercise prices it)?
3. **The wrong-hardware package.** Build a variant of this
   step with `CONFIG_ZIGBEE_FOTA_MIN_HW_VERSION=0x3C`
   (60) in the *package's* configuration and serve it to
   the HW-52 client. Predict the outcome and the exact
   point of refusal before you run it; afterwards, name
   the gate and the evidence (which log, which parse) that
   proves where it died.

## Why this matters

You now own an update system end to end: signed images,
two transports, a trial with a verdict, a server with a
policy, and — rarest of all — the habit of verifying each
claim in the artifact that carries it. Chapter 11 steps
back and reads the air itself: every packet this chapter
moved, decoded in Wireshark, against everything the course
has taught you to expect.
