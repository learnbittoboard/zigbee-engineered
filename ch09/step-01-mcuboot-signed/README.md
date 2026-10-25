# Chapter 9 · Step 1 — The product, behind MCUboot: slots and signatures

**What you do:** Put a bootloader in front of the Chapter 8
product. Sysbuild now builds **two** images — MCUboot and the
app — the flash map gains a boot partition and a second slot,
every app image leaves the build **signed**, and the app
carries a version stamp the bootloader can read. The firmware
itself does not change by one line of behaviour: occupancy
still senses, reports, and switches the bulb. Everything this
step changes is *around* the firmware — which is exactly where
updates live or die.

**What this step teaches:** The geography of safe updates.
Two slots, one small strict bootloader that owns the reset
vector, and a cryptographic rule: **nothing boots that was
not signed.** You will prove the signature exists — in the
build artifacts, not in the documentation's say-so.

## The new flash map

Until now the app owned the whole flash from address 0. The
board overlay now includes a second partition file — the
FOTA layout Nordic ships for this board
(`ext_flash/nrf52840dk_partitions.dtsi`) — which re-carves
the map:

| Partition | Where | Size | Address |
|---|---|---|---|
| `mcuboot` (boot) | internal flash | 64 KB | 0x00000 |
| `image-0` (slot 0) | internal flash | 916 KB | 0x10000 |
| `image-1` (slot 1) | external QSPI flash (MX25R64) | 916 KB | ext 0x0 |

Three facts in that table run the whole chapter:

- **The app moved.** It now links at 0x10000, not 0x0. If you
  flash only the app (no bootloader) to a bare board, nothing
  runs — there is no code at the reset vector. First flash of
  this step is always the *merged* result sysbuild produces:
  bootloader + signed app in slot 0.
- **The slots are equal.** Slot 1 mirrors slot 0's 916 KB, so
  MCUboot uses **swap-using-move**: on upgrade it exchanges
  the two slots' contents sector by sector, no scratch
  partition needed. (Unequal slots would force a different
  mode; the lesson works through the trade-offs.)
- **Slot 1 lives in external flash.** The DK's QSPI chip —
  decoration in earlier chapters — is now load-bearing.
  MCUboot needs its QSPI NOR driver to read and swap from
  slot 1, which is why `sysbuild/mcuboot/boards/` carries a
  `.conf` enabling it (and disabling `FPROTECT`, whose ACL
  setup fails on this chip/layout combination — an upstream
  quirk the add-on's own FOTA samples work around the same
  way).

Both images include the *same* partition files
(`sysbuild/mcuboot/` has its own overlay). Bootloader and app
disagreeing about the map is the classic way to brick a board
from a build that "worked".

## Build — and read the artifacts

```bash
west build -p always -b nrf52840dk/nrf52840 \
    course/repo/ch09/step-01-mcuboot-signed -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
west flash
```

The build runs the app build, then the MCUboot build, then
**imgtool** over the app binary. Verified sizes from this
step's build: the app takes 422,352 B of its 916 KB slot
(45.4%) and 76,728 B RAM; MCUboot takes 24,088 B of its
64 KB (36.8%). Now prove the signing happened — three
artifacts, three different witnesses:

1. `step-01-mcuboot-signed/zephyr/zephyr.signed.bin` (and
   `.signed.hex`) exist next to the unsigned `zephyr.bin`.
   The signed file is 151 bytes of header + TLVs larger:
   a 512-byte image header and the signature trailer.
2. The build directory's `dfu_application.zip` contains
   exactly that signed binary plus a manifest — this is the
   package every later update path (Steps 2–3, Chapter 10)
   ships around.
3. Open the signed binary's header (the lesson shows the
   layout). Its fields read: magic `0x96f3b83d`, load
   address **0x00010000** — slot 0, not 0 — header size 512,
   and image version **1.0.0+0**.

The signature itself is ECDSA-P256 (NCS's default). Note the
build log's warning while you're there: *using the default
MCUboot signing key, for debug use only.* That default key is
published in the MCUboot repository — anyone on earth can
sign images your bootloader will accept. Fine for this lab,
a firing offence in a product; the lesson closes with key
management.

## Two stamps, one truth

The app carries its version **twice**, and the chapter's
smallest discipline is keeping them in agreement:

- The `VERSION` file (1.0.0) becomes `APP_VERSION_STRING`.
  It is what the app *says*: watch the console at boot —

  ```
  <inf> app: Starting Zigbee, Engineered - occupancy product, firmware v1.0.0
  ```

  (Expected output at your desk; this line is compiled in,
  and the string is verifiably present in the signed binary.)
- `CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION="1.0.0"` in prj.conf
  is what imgtool writes into the signed header. It is what
  the *bootloader* believes — the version rollback and
  anti-downgrade decisions are made from, and the one
  Chapter 10's OTA machinery will compare.

Change one and not the other and you have built a device
that lies about its own version to exactly the audience
(update machinery) that acts on the number. Every step from
here bumps both, together.

## The cable door

This step also opens the update path Steps 2 and 3 will use:
an **mcumgr SMP server on the console UART**
(`CONFIG_MCUMGR=y` with the UART transport and the image +
OS groups; `zephyr,uart-mcumgr = &uart0` in the board
overlay). One warning from this step's own build log, worth
learning here: Kconfig **silently drops** symbols whose
dependencies are unmet — `CONFIG_MCUMGR=y` alone vanished
from the generated `.config` until its dependency symbols
(NET_BUF, ZCBOR, and the transport's BASE64/CRC) were named
too. Always verify a feature in `.config`, never in
`prj.conf`. (This cost the course a fix entry; it will cost
you a debugging evening someday if you skip it.)

On your host, install the mcumgr CLI (Apache Mynewt's tool —
`go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest`,
or your platform's package) and look through the door:

```bash
mcumgr --conntype serial --connstring portdev=<your port>,baud=115200 image list
```

Expected shape of the answer: slot 0 holds version 1.0.0,
flagged **active**; slot 1 is empty. Leave the flags alone
for now — Step 2 exists to make those flags interesting.

One behaviour to notice before moving on: this v1 never
calls the image-confirm API, and nothing bad happens —
because it arrived by debugger, not by swap. An image only
owes confirmation when MCUboot *test-swapped* it in.
Steps 2 and 3 are that sentence, made into an experiment.

## The ideas to take away

- **The bootloader is a border guard, not a feature.**
  24 KB that does three things — verify signature, manage
  the swap, boot — and nothing else. Every line of
  functionality you add to it is attack surface that cannot
  itself be updated safely.
- **Signatures answer "who authorised this?", not "is it
  secret?"** The signed image is perfectly readable; signing
  is authentication, not encryption. (MCUboot *can* encrypt;
  the lesson explains why most products shouldn't.)
- **A slot layout is a product decision made in a header
  file.** External flash for slot 1 bought this design
  equal slots and move-swap on a 1 MB part. The partition
  files *are* the update architecture.

## Exercises

1. Bump the version to 1.0.1 — in the `VERSION` file only.
   Rebuild, and inspect: the boot log string, the signed
   header's version, and the zip's manifest. Which audiences
   now disagree about the device's version? Fix it properly
   and verify all three agree.
2. With the board running this step, ask it for its slot
   state (`mcumgr --conntype serial --connstring
   portdev=<your port>,baud=115200 image list`). Write down,
   in your own words: which slot is active, which flags it
   carries, and why slot 1 is empty. Then answer the
   chapter's smallest exam question: this image never calls
   the image-confirm API, so why is it not on trial? (If
   your answer is about how the image *arrived*, you have
   it.)
...[truncated 1357 chars]