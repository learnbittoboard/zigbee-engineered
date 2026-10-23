# Chapter 5 · Step 1 — The coordinator is a dongle running Nordic's shell

**What you do:** Build the Zigbee add-on's **shell sample** — unmodified,
Nordic's own code — for the nRF52840 Dongle, flash it, and form this
course's Zigbee network from its command line. No code of ours exists
in this step, and that is the lesson: a coordinator is a *role*, and
for development you borrow it.

**What this step teaches:** The coordinator half of commissioning.
Role is a runtime choice (`bdb role zc`), network formation is one
command (`bdb start`), and everything the stack decided — PAN ID,
extended PAN ID, channel — is readable back from the same shell.
Chapter 6's devices will join the network you form here.

## Build (from your NCS workspace root)

The sample lives in the add-on, not in this repo, so the build names
it directly. Two flags matter: the add-on is passed as an extra
Zephyr module (the Chapter 1 headless integration), and
`FILE_SUFFIX=usb` selects the sample's USB configuration — the dongle
has no UART header worth using; its shell rides the USB CDC port.

```bash
west build -p always -b nrf52840dongle/nrf52840 \
    ncs-zigbee/samples/shell -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee \
    -DFILE_SUFFIX=usb
```

(`<workspace>` is your NCS folder from Chapter 1 — the folder that
contains `ncs-zigbee/`, `nrf/` and `zephyr/`.)

Expected tail of a good build (numbers verified against this exact
stack — yours should match byte for byte):

```
Memory region         Used Size  Region Size  %age Used
           FLASH:      480516 B       1020 KB     46.01%
            SRAM:       88432 B        256 KB     33.73%
```

If your sizes differ, you are not building the pinned stack — stop
and fix that first; every later chapter assumes these numbers.

## Flash the dongle

The dongle has no debugger chip. It carries a USB bootloader, and
you ask it politely:

1. Plug the dongle in (or re-insert it) while holding its small
   side **RESET** button until the red LED breathes/pulses — that is
   the bootloader listening.
2. Run `west flash` from the same build directory. West packages
   the image and hands it to the bootloader over USB.

Expected behaviour (you will verify this on your own desk — this
course's build checks prove the image links, not your USB cable):
after flashing, the dongle reboots into the shell sample, and a new
USB serial port appears on your machine — on Linux typically
`/dev/ttyACM0`, on macOS `/dev/tty.usbmodem…`, on Windows a COM port.

3. Open that port in your serial terminal (115200 baud is nominal —
   USB CDC ignores it) and press **Enter**. The prompt answers:

```
uart:~$
```

That prompt is Chapter 1's Zephyr shell, wearing a new uniform:
type `help` and the list now includes `bdb`, `zdo`, `zcl`, `nbr`
and `nvram` next to the familiar `kernel` and `log`.

## Form the network

Type the two commands the whole chapter exists for:

```
uart:~$ bdb role zc
Coordinator set
Done

uart:~$ bdb start
Started coordinator
Done
```

`bdb role zc` says *this device is the coordinator* — a runtime
decision, stored by the stack, not a compile flag. `bdb start`
runs Base Device Behaviour top-level commissioning; for a
coordinator that means: pick a channel, invent a PAN ID and an
extended PAN ID, generate a network key, and open the network.

## Read back what the stack decided

Never trust a network you cannot describe. Ask:

```
uart:~$ bdb panid
1A2B
Done

uart:~$ bdb extpanid
0b010eaafd745dfa
Done

uart:~$ bdb channel
Primary channel(s): 11
Secondary channel(s): 11
Done

uart:~$ zdo short
0000
Done
```

(Hex values will differ on your desk — the stack randomises PAN ID
and extended PAN ID per formation; channel 11 is the common pick,
not a promise. The *shapes* are the contract: PAN ID is 16-bit,
extended PAN ID is 64-bit, channel is 11–26, and the coordinator's
short address is **always `0x0000`** — it assigns itself the first
address in its own network. Remember that address; Chapter 6 aims
commands at it.)

Write your four values down. They are your network's identity card,
and Chapter 11 will make you find the same numbers inside a
Wireshark capture.

## The ideas to take away

- **The coordinator is borrowed, on purpose.** Nordic maintains this
  sample against the same ZBOSS build our device firmware links.
  Any behaviour difference between "the network" and "our device"
  is therefore *our* bug to find — the cleanest possible debugging
  position, and the reason this course writes no coordinator code.
- **`bdb start` is not "join".** On the coordinator it *forms*; on
  a router or end device it *steers* — same command, role-dependent
  meaning. Chapter 6 lives inside that sentence.
- **Formation is stored.** Reset the dongle and it comes back as
  the coordinator of the *same* network (the parameters live in
  NVRAM), it does not invent a new one. To truly start over you must
  say so: `bdb factory_reset` — which Chapter 6 will use on purpose
  and by accident, in that order of preference.

## Exercises

1. Reboot the dongle (unplug, replug, reconnect the terminal).
   Before typing anything, predict: what will `bdb role` (no
   argument) and `bdb panid` report? Run them and reconcile.
2. Ask the shell for the coordinator's own long address:
   `zdo eui64`. Compare it with `bdb extpanid`'s answer and explain,
   in one sentence, why the extended PAN ID of a fresh network so
   often resembles the coordinator's EUI-64. (Hint: who invents it?)
3. Set an explicit channel before forming — factory-reset, then
   `bdb channel 25`, `bdb role zc`, `bdb start`, and read the
   channel back. Chapter 5's lesson explains why a network
   *next to a busy Wi-Fi router* might prefer 25 — and why it
   might not help.

## Why this matters

Every Zigbee product team owns exactly the skill you just used:
stand up a *known-good* network with tools you did not write, then
develop your device against it. When (not if) a join fails in
Chapter 6, this split is what turns "Zigbee is broken" into a
one-afternoon diagnosis: the network side can be interrogated,
command by command, from a shell you already trust.
