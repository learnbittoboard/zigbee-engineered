# Chapter 10 · Step 1 — The OTA client: a second endpoint and four wires

**What you do:** Give the product a second way to receive
its future: the Zigbee add-on's **FOTA client library**.
A new endpoint (5) carries the OTA Upgrade client cluster;
the library discovers the update server, negotiates, and
streams the image block by block into the *same slot 1*
Chapter 9's cable used — through the same DFU machinery,
into the same trial-and-confirm contract. This step is the
client half only: flashed by cable as usual, v1.2.0 waits,
correctly, for a server that Step 2 will provide.

**What this step teaches:** Integration, not invention.
The OTA client is a library; your work is its four
touch-points in the application, one endpoint decision,
and one sysbuild line that — when missing — fails
*silently*, in the tradition this course keeps charging
tuition for.

## The shape of the change

The firmware delta from Chapter 9 Step 3 is small enough
to list completely, and every piece mirrors the add-on's
own `light_switch` FOTA configuration (`prj_fota.conf` and
its `app_task_zigbee.c`) — read those files after this
step; recognising your code in the vendor's sample is the
skill:

- **`prj.conf`:** `CONFIG_ZIGBEE_FOTA=y` (the library;
  its Kconfig `imply`s the bootloader/DFU chain),
  `CONFIG_ZIGBEE_FOTA_ENDPOINT=5`,
  `CONFIG_ZIGBEE_FOTA_HW_VERSION=52`,
  `CONFIG_DFU_TARGET_MCUBOOT=y` (blocks land in slot 1 via
  the MCUboot DFU target), `CONFIG_ZIGBEE_FOTA_PROGRESS_EVT=y`
  (progress becomes log lines). Version stamps bumped
  together to **1.2.0**, as always.
- **`main.c`:** the device context becomes
  `ZBOSS_DECLARE_DEVICE_CTX_2_EP` with the library's own
  `zigbee_fota_client_ep` registered beside ours;
  `zigbee_fota_init(ota_evt_handler)` and
  `ZB_ZCL_REGISTER_DEVICE_CB(zcl_device_cb)` run before the
  context goes live; every stack signal is forwarded to
  `zigbee_fota_signal_handler()`; the ZCL device callback
  routes `ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID` to
  `zigbee_fota_zcl_cb()`; and the event handler's
  `ZIGBEE_FOTA_EVT_FINISHED` does the only dramatic thing
  in the step — `sys_reboot()`, handing the completed
  download to Chapter 9's swap.

## The endpoint collision (a two-minute lesson in defaults)

The library's default endpoint is **10** — our product's
endpoint, chosen in Chapter 6 and used by every shell
command since. Two endpoints cannot share a number, and
the library was not going to win: its endpoint moves to 5
(`CONFIG_ZIGBEE_FOTA_ENDPOINT=5`), exactly as Nordic's
sample moves it for its own endpoint. File the pattern:
*libraries ship defaults tuned for their samples, not for
your product; the integration work is noticing where the
defaults collide with decisions you made chapters ago.*

## The silent failure (this step's real tuition)

Build once with only the `prj.conf` changes and everything
above compiles, links, and runs — app 434,088 B of the slot
(46.7%, the library costs ~12 KB over Chapter 9). The
build even produces `dfu_application.zip`. What it does
**not** produce is the `.zigbee` file, and the only clue is
one warning buried mid-log:

```
CMake Warning at ncs-zigbee/sysbuild/CMakeLists.txt:
  No Zigbee FOTA image format selected. Please enable
  either legacy or the multi-image format.
```

The mechanism, once dissected: the add-on's sysbuild hook
wraps a **DFU multi-image package** in the OTA header — but
only if sysbuild is told to build that package.
`CONFIG_ZIGBEE_FOTA`'s Kconfig `imply`s a symbol
(`DFU_MULTI_IMAGE_PACKAGE_BUILD`) that exists only in the
*sysbuild* Kconfig tree, so the imply evaporates without a
sound — Chapter 9's vanishing-`CONFIG_MCUMGR` trick, second
verse. The fix lives in `sysbuild.conf`, following NCS's
own `dfu_multi_image` sample:

```
SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_BUILD=y
SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_APP=y
```

With those two lines, the build's last step runs the
add-on's `zb_add_ota_header.py` over `dfu_multi_image.bin`
and writes the package next to the build directory:

```
127F-0141-01020000-occupancy.zigbee   (434,324 B)
```

Parse its header in the lab (the lesson gives the layout)
and check every field against its source: file identifier
`0x0BEEF11E` (the OTA magic), manufacturer `0x127F`,
image type `0x0141`, **file version `0x01020000`** — the
imgtool sign stamp 1.2.0, BCD-packed — the comment string
from `CONFIG_ZIGBEE_FOTA_COMMENT`, total size equal to the
file's actual size, and one sub-element (tag `0x0000`)
whose 434,262-byte payload *is* `dfu_multi_image.bin`,
which wraps Chapter 9's signed image. Three nested
envelopes: OTA header → multi-image package → MCUboot
signed image. Chapter 9's signature survives the journey
untouched, checked at the end of it by the bootloader, as
always.

Rule, third formulation, for the wall: **a green build is
a hypothesis. The artifact list is the evidence.** If a
step promises a file, the step is not done until the file
exists and parses.

## Build, flash, and watch it wait

```bash
west build -p always -b nrf52840dk/nrf52840 \
    course/repo/ch10/step-01-fota-client -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
west flash
```

Desk check (expected output): the product boots as
v1.2.0, joins, confirms itself (Chapter 9's policy,
unchanged) — and then, periodically, the FOTA client does
its quiet job: it has broadcast for an OTA server (Match
Descriptor), found none on this network yet, and will ask
again. Nothing else happens, and *nothing else happening*
is the correct behaviour for a client with no server: the
occupancy loop is untouched, the bulb obeys, and the
device is one server away from updatable. From the
coordinator you can verify the new endpoint exists:
`zdo active_ep` on our device now lists **5 and 10**, and
endpoint 5's descriptor carries the OTA Upgrade cluster
(`0x0019`) as a client.

Also note what did *not* change on the wire home: the
cable door from Chapter 9 still works — mcumgr is still
compiled in (the FOTA Kconfig implies it, and we never
removed it). Two update transports, one slot machine.
Products keep the cable path for exactly the day the radio
path is the thing that's broken.

## The ideas to take away

- **The radio is the last place update logic should live.**
  Everything that decides an update's fate — signature,
  trial, confirmation, rollback — sits below the transport,
  in machinery Chapter 9 already proved. OTA changed *how
  bytes arrive* and nothing about *how they are judged*.
  If your OTA design needs its own security story, it is
  telling you the layering is wrong.
- **A client with no server is a finished state, not an
  error.** Discovery and periodic querying are the
  library's problem; the application's obligation ends at
  correct integration and at not letting update chatter
  disturb the product's real job — watch the occupancy
  loop's timing during Step 2's download and judge whether
  that obligation was met.
- **Defaults are decisions someone else made.** Endpoint
  10, block size 64, HW version 52: each default in this
  stack was reasonable for the sample it shipped with.
  Integration is the discipline of auditing defaults
  against *your* decisions.

## Exercises

1. Parse the `.zigbee` header with the tool of your choice
   and write out every field with the Kconfig or build
   input that produced it. One field — the stack version —
   has no input in our configuration at all. Where did it
   come from?
2. Change `CONFIG_ZIGBEE_FOTA_COMMENT` and the version to
   1.2.1, rebuild, and diff the two packages' headers byte
   by byte. Which bytes changed, which *must* never change
   between two packages for the same product, and who
   enforces that — build system, server, or client?
3. Remove the two `SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_*`
   lines and rebuild. Time yourself: how long does it take
   you to prove the build is broken, and what is the
   *fastest* check that catches it? (You are designing the
   CI assertion this step deserves. Write it down as one
   shell command.)

## Why this matters

You have now integrated a vendor update library the way it
is actually done in industry: mirror the reference sample,
audit the defaults, verify the artifacts, and treat the
one silent failure as the real curriculum. Step 2 builds
the other half of the conversation — the server, the
package it serves, and the version policy that decides
whether "newer" is 
...[truncated 1166 chars]