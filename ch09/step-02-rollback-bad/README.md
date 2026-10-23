# Chapter 9 · Step 2 — The bad update: watch MCUboot revert it

**What you do:** Ship a firmware update that deserves to
die. This v1.1.0 is the healthy product with one deliberate
defect: **it never confirms its image**, and 20 seconds
after boot it reboots itself. You install it over the cable
through Step 1's mcumgr door as a *test* image, reboot — and
watch MCUboot swap it in, let it run, watch it fail to
confirm, and **swap v1.0.0 back** on the next boot. Nobody
touches the board. That is rollback, and it is the entire
reason the bootloader exists.

**What this step teaches:** Confirmation is a contract.
MCUboot's deal with every updated image: *you get one boot
to prove yourself; if you reset before confirming, I assume
the update was bad and the old image returns.* A valid
signature gets an image **booted**; only confirmation gets
it **kept**.

## Before you start

Your DK runs Step 1: MCUboot + signed v1.0.0, flashed as the
merged image, occupancy loop working. This step never
touches the debugger again — if the experiment goes wrong,
that is a course bug, not a lab step.

Build the bad v2 (verified build: app 422,624 B of the
916 KB slot, 45.5%; signed header reads version **1.1.0+0**,
load address 0x10000 — both version stamps bumped together,
as Step 1 demanded):

```bash
west build -p always -b nrf52840dk/nrf52840 \
    course/repo/ch09/step-02-rollback-bad -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
```

The image you ship is the signed binary from the build's
`dfu_application.zip` (extract it) — the same bytes Chapter
10 will later wrap for the air. Find the defect before you
ship it; it's labelled, because a lab defect should be:

```c
/* main.c — the whole crime, in one work handler */
static void doom_handler(struct k_work *work)
{
	LOG_WRN("BAD v2: rebooting WITHOUT confirming this image - "
		"MCUboot should now revert to v1.0.0");
	k_sleep(K_MSEC(200));
	sys_reboot(SYS_REBOOT_COLD);
}
/* scheduled 20 s after boot; boot_write_img_confirmed()
 * appears nowhere in this file. */
```

Note what the defect is *not*: the image is properly
signed, the right size, the right version. It boots. It
would join and sense and switch. Its only sin is dying
unconfirmed — which in production is what a crash loop, a
watchdog bite, or a power cut mid-bring-up all look like
from the bootloader's side of the wall.

## The update, over the cable

All commands below are the expected desk procedure; the
mcumgr conversation is the standard NCS cable-DFU path
(SMP over the console UART you configured in Step 1):

```bash
# 1. Upload the signed v1.1.0 into slot 1
mcumgr --conntype serial --connstring portdev=<port>,baud=115200 \
    image upload step-02-rollback-bad.signed.bin

# 2. Look at the slots
mcumgr --conntype serial --connstring portdev=<port>,baud=115200 \
    image list
```

Expected `image list` shape after upload:

```
slot 0: version 1.0.0, active
slot 1: version 1.1.0, (no flags)
```

```bash
# 3. Mark slot 1 as the test image (hash from image list)
mcumgr ... image test <hash of slot 1>

# 4. Reboot into the test
mcumgr ... reset
```

## What you watch happen

In order, on the console — every line of this is the
mechanism, not decoration:

1. **A long pause, then boot.** MCUboot found the test flag
   and is performing the swap: with swap-using-move, ~916 KB
   of sectors trade places between internal and external
   flash. Swaps cost time in proportion to image size;
   products budget for this pause, and so should your
   expectations.
2. **v1.1.0 boots and confesses.**
   `firmware v1.1.0`, followed immediately by its warning:
   this build is deliberately bad and reboots in 20 s. The
   bootloader already knows none of this — it verified a
   valid signature and kept its side of the deal.
3. **20 seconds.** The occupancy firmware is fully alive —
   it had time to join, sense, and switch. Then the doom
   handler fires and the board resets **without confirming**.
4. **A second long pause.** MCUboot inspects the image
   state: slot 0's image booted as a test and reset
   unconfirmed. Verdict: revert. The slots swap back.
5. **v1.0.0 boots.** `image list` now shows slot 0 =
   1.0.0, **active, confirmed**; slot 1 holds the rejected
   1.1.0. Your product survived shipping a broken update,
   and the only evidence is in the slot table and your logs.

The board is running the exact firmware it ran before the
update. No JTAG, no rescue image, no human.

## The ideas to take away

- **Rollback is not rejection.** The bad image was never
  refused — it was *tried*, under supervision, with the old
  image held safely in the other slot. Signature failure is
  rejection (Step 1's border guard). Unconfirmed reset is
  rollback (this step's probation officer). Products need
  both, for different failure species.
- **Any reset is the trigger.** This lab used a polite
  software reboot as a stand-in. A crash loop would produce
  the identical outcome — MCUboot cannot tell *why* you
  reset, only *that* you reset unconfirmed. Design
  confirmation as "the device proved its core functions",
  never as "main() reached line 40": a firmware that
  confirms early and crash-loops later has signed its own
  death certificate *permanently*.
- **Confirmation is one-way.** Once an image is confirmed,
  MCUboot will not revert it, ever, on its own. (Step 3's
  exercises explore what that means when the confirmed
  image turns out to be subtly wrong.)
- **The swap is the price.** Two slots double the flash an
  updateable product must carry, and move-swap spends
  seconds and flash wear on every exchange. Boring,
  predictable, and cheaper than one truck roll to a bricked
  installation.

## Exercises

1. Change the doom delay to 120 s, rebuild, re-upload, test.
   This time, while v1.1.0 is running, confirm it yourself
   from the host: `mcumgr ... image confirm`. Then let the
   doom reboot happen. Which version boots, and why? You
   have just proven that *the pending state, not the app*,
   is the gate — and that a host (or an attacker with UART
   access) holds a key to it. Where does that leave your
   product's physical-access threat model?
2. Upload again, but mark the image **permanent** before
   reset (`image confirm <hash>` on slot 1, instead of
   `image test`). Run the experiment. What changed, which
   safety property did you just waive, and for what kind of
   update is waiving it the right call?
3. Time both swaps (test-swap in, revert out) with a
   stopwatch against the boot log timestamps. Swap time
   scales with image size: at what image size would this
   board's swap pause break a product requirement you can
   invent plausibly (alarm panel? door lock? medical
   sensor?) — and which MCUboot mode or layout change
   answers it?

## Why this matters

Every senior firmware engineer has a story about the update
that bricked the fleet — or would have, without exactly
this mechanism. You now own the pattern end to end: signed
image, test swap, supervised first boot, automatic revert.
Step 3 ships the same v1.1.0 with the one line this step
omitted, and the update sticks.
