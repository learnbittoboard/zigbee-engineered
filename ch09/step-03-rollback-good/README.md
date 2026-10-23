# Chapter 9 · Step 3 — The good update: confirm on evidence, and it sticks

**What you do:** Ship v1.1.0 again — the same version
number as Step 2's bad image — with the one behavioural
difference that decides everything: once this firmware has
**joined the Zigbee network**, it calls
`boot_write_img_confirmed()` and tells the bootloader "I am
healthy; keep me." You install it through the same cable
door, as the same kind of test image, and this time the
update **sticks**: reboots come and go, and v1.1.0 remains.

**What this step teaches:** *Where* confirmation belongs.
Not at the top of `main()` — that proves nothing. Not never
— that was Step 2. Confirmation is the app signing its own
health certificate, and the signature should be earned by
evidence the product defines: here, *the device booted,
started its stack, and joined its network* — the minimum
viable proof that this firmware can do its job.

## The whole diff that matters

Step 3 is Step 1 plus one policy block, inside the signal
handler you have known since Chapter 5 (verified build: app
422,532 B of the slot, 45.5%; signed header **1.1.0+0** —
yes, identical version stamps to the bad image; read the
ideas section before you call that a typo):

```c
#include <zephyr/dfu/mcuboot.h>

static bool image_confirmed;

void zboss_signal_handler(zb_bufid_t bufid)
{
	/* ... LED update, default signal handler ... */

	if (!image_confirmed && ZB_JOINED()) {
		int err = boot_write_img_confirmed();

		if (err == 0) {
			LOG_INF("Firmware v%s: joined and healthy - "
				"image CONFIRMED (permanent)",
				APP_VERSION_STRING);
		} else {
			LOG_WRN("boot_write_img_confirmed() failed (%d) - "
				"this image is still on trial", err);
		}
		image_confirmed = true;
	}
	/* ... buffer release ... */
}
```

Three design choices in those lines, each deliberate:

- **The trigger is `ZB_JOINED()`** — a signal that only
  arrives after the radio, the stack, commissioning state,
  and the network have all worked at least once. A build
  with a broken radio driver or corrupted network settings
  *cannot* produce this signal — it stays on trial and
  Step 2's machinery saves the device. (Nordic's own FOTA
  sample confirms on the same class of evidence.)
- **The flag is once-per-boot, the effect is once-ever.**
  `image_confirmed` stops the app asking repeatedly;
  `boot_write_img_confirmed()` itself writes persistent
  image state in flash — after it succeeds, no reboot,
  watchdog, or power cut can bring v1.0.0 back. The
  bootloader has closed the case.
- **Failure is loud, not fatal.** If the confirm call
  errors, the app logs it and keeps running — on trial,
  meaning the *next* reset reverts. That asymmetry is
  correct: an unconfirmed-but-running device is a problem
  you can still fix remotely; a brick is not.

## The procedure — same door, different ending

Build, then repeat Step 2's cable update with this step's
signed binary (from its `dfu_application.zip`):

```bash
west build -p always -b nrf52840dk/nrf52840 \
    course/repo/ch09/step-03-rollback-good -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee

mcumgr ... image upload step-03-rollback-good.signed.bin
mcumgr ... image test <hash of slot 1>
mcumgr ... reset
```

What you watch happen (expected output at your desk):

1. The swap pause, then `firmware v1.1.0` boots — the same
   opening as the bad image. Nothing yet distinguishes them.
2. The stack joins — and this time the log gains the line
   Step 2 never printed:
   `Firmware v1.1.0: joined and healthy - image CONFIRMED (permanent)`.
3. `mcumgr ... image list`: slot 0 = 1.1.0, **active,
   confirmed**. Slot 1 holds v1.0.0 — now merely the
   previous tenant, with no path back short of a new update.
4. **Reset the board. Twice. Pull the power once.** v1.1.0
   boots every time. The trial is over; the update is
   simply *the firmware* now.
5. Close the loop that proves health was real, not
   ceremonial: walk into the room. The bulb still obeys —
   the confirmed image is the full Chapter 8 product,
   because of course it is; an update that breaks the
   product's one job would have failed its own confirmation
   evidence in any design that checks the right things.

## The ideas to take away

- **Version is a label; behaviour is the verdict.** Steps 2
  and 3 carry byte-identical version stamps (1.1.0) and
  opposite fates. MCUboot never compared version numbers in
  this chapter — it compared *outcomes*: confirmed, or not.
  Version policy still matters (Chapter 10 makes it a
  gatekeeper for downloads), but rollback runs on
  confirmation, full stop.
- **Confirmation criteria are a product specification.**
  "Joined the network" is this product's bar. A sensor
  might require a first successful measurement; a lock, a
  self-test pass; a gateway, a WAN connection. Write the
  criteria down, make them *falsifiable by real failure
  modes*, and review them like safety code — because a
  wrongly-confirmed bad image is worse than a brick: it is
  a brick that reports success.
- **The old image's retirement is the point of no return.**
  The moment slot 0 confirms, slot 1 stops being a safety
  net and becomes scratch space for the *next* update. That
  is why products confirm late-but-not-too-late: long enough
  to earn it, soon enough that a latent crash-loop defect
  still meets an unconfirmed image.

## Exercises

1. Move the confirmation trigger earlier: confirm on the
   first signal of any kind, before joining. Rebuild as a
   test image (keep version 1.1.0), install, and then break
   joining on purpose — e.g. power down the coordinator
   before the test reset. Does the broken-network build
   stick now? State, in one sentence, the failure class
   your early confirmation just legalised.
2. Design this product's real confirmation checklist for
   production: at least four pieces of evidence (join,
   sensor self-read, a bound-bulb command acknowledged or
   reported, persistent-storage sanity…), each with the
   failure it exists to catch and a timeout after which the
   device should *deliberately not* confirm and take the
   revert. Where in the code would each check live?
3. **The downgrade drill.** Slot 1 still holds v1.0.0.
   `image test` it, reset, and let v1.0.0 boot. It is, by
   Step 1's design, an image that never confirms — so what
   happens at its *next* reset, and what does the slot
   table look like after the dust settles? You have just
   discovered why "confirm your image" must be a property
   of *every* release a fleet might land on, including the
   old ones you roll back *to*.

## Why this matters

Chapter 9's contract is now complete on your desk: signed
images (authenticity), two slots (a way back), test swaps
(supervision), confirmation on evidence (a verdict), and
rollback (the sentence for failing it). Chapter 10 removes
the cable: the same signed images, the same slot machinery,
the same confirmation policy — delivered through the
Zigbee network while the product keeps working.
