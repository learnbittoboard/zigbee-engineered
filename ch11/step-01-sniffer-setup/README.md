# Step 1 — Turn your spare dongle into a sniffer, and give Wireshark the keys

**Goal:** your second nRF52840 dongle becomes a passive radio
listener. Wireshark shows every frame your course network exchanges —
and, because you will put the network on a key *you* chose, Wireshark
can open the encrypted parts too.

Three pieces, in order:

1. Sniffer firmware on the dongle (Nordic's own sample, from the
   pinned SDK on your disk).
2. Nordic's Wireshark plugin (an *extcap* program) so Wireshark can
   talk to that firmware.
3. A fresh network formed on a network key you set yourself, plus
   that key typed into Wireshark.

Nothing here talks back to the network. A sniffer only listens —
which is exactly why it can be trusted as a witness.

## 1. Build and flash the sniffer firmware

The firmware is Nordic's `802154_sniffer` sample. It ships inside
your pinned NCS v3.4.1 tree, and its job is tiny: put the radio in
promiscuous receive mode and print every frame it hears, as text,
over USB serial.

From your NCS workspace:

```console
$ cd ~/ncs-course
$ west build -p always -b nrf52840dongle/nrf52840 -d build-sniffer \
    nrf/samples/peripheral/802154_sniffer
```

Expected tail (verified at course build time for this exact target):

```
Memory region         Used Size  Region Size  %age Used
           FLASH:      129180 B      1020 KB     12.37%
             RAM:       57656 B       256 KB     21.99%
```

Flash it with the dongle ritual from Chapter 5 — hold the dongle's
side **RESET** button until the red LED breathes, then, from the
build directory:

```console
$ west flash
```

After flashing, the dongle reboots into the sniffer. Two things to
know about it on the desk:

- It enumerates as a USB serial device with Nordic's USB IDs
  (VID `0x1915`, PID `0x154B`). That ID pair is how the Wireshark
  plugin recognises it later.
- Its LEDs report state: LED1 blinks slowly (2 s) while idle and
  fast (0.5 s) while capturing; LED4 toggles for every packet heard.

### Optional smoke test (no Wireshark yet)

Open the dongle's serial port in your terminal and type:

```
uart:~$ channel 15
uart:~$ receive
```

Expected output: one line per frame in the air around you, in the
sample's fixed format:

```
received: <hex bytes> power: <-42> lqi: <173> time: <...>
```

`sleep` stops the capture; `bootloader` (dongle only) reboots it
back into flash mode. You will not drive it by hand in the worksheet
— the Wireshark plugin sends these commands for you — but this is
the whole protocol: text in, text out.

## 2. Install Wireshark and Nordic's extcap plugin

You need **Wireshark 4.0 or newer** (Nordic's stated requirement).

The plugin is a single Python program, `nrf802154_sniffer.py`, from
Nordic's public `nRF-Sniffer-for-802.15.4` repository:

1. From that repository's `nrf802154_sniffer/` folder, take
   `nrf802154_sniffer.py` — plus `nrf802154_sniffer.bat` on Windows.
2. Copy the file(s) into Wireshark's **Personal Extcap** folder.
   Find it under **Help → About Wireshark → Folders**.
3. Install its one dependency: `pip install pyserial`.
4. On Linux/macOS make the script executable (`chmod +x`).
5. Restart Wireshark.

Now, *while the sniffer firmware is running on the dongle*, a new
capture interface appears in Wireshark: **nRF Sniffer for
802.15.4**. (No dongle or wrong firmware → no interface. That is
the first thing to check if it is missing.)

Two interface options matter when you start a capture:

- **Channel** (required): 11–26. A sniffer hears exactly **one**
  channel. Find yours first — ask your coordinator:
  `uart:~$ bdb channel`.
- **Metadata**: leave the default, *IEEE 802.15.4 TAP*. TAP is the
  wrapper that carries the radio facts — channel, RSSI, LQI — into
  Wireshark alongside each frame.

> Same folder, useful fallback: Nordic also ships ready-built
> firmware images there (`nrf802154_sniffer_nrf52840dongle.hex`).
> The course route builds from the pinned SDK, but the prebuilt hex
> is the same sample if a build ever fights you.

## 3. Put the network on a key you know, and tell Wireshark

Zigbee encrypts at the network layer, so a capture of a normal
network is mostly sealed envelopes. This course's trick: **form
the network on a key you choose.** The coordinator shell has a
command for exactly this — from the ZBOSS shell:

```
bdb nwkkey <h:key>   Set a pre-defined network key instead of a random one
```

with two rules, straight from the shell's behaviour:

- It only works **before** `bdb start`. Afterwards you get
  `Stack already started`.
- If the coordinator's NVRAM already holds a network, it refuses
  (`Can't change NWK key - NVRAM not empty`). Factory-reset the
  coordinator first (Chapter 5's NVRAM erase), reboot, then set
  the key.

Procedure — coordinator dongle, fresh start:

```
uart:~$ bdb nwkkey 00112233445566778899aabbccddeeff
uart:~$ bdb role zc
uart:~$ bdb start
```

(The example key above is the one in ZBOSS's own help text. Pick
your own 16 bytes if you like — just write them down; you will
type the same bytes into Wireshark, and the worksheet will make
you find them inside a captured frame.)

Then bring the other devices back so the *whole* course network
runs on your key:

- Sensor DK: factory-reset it and re-join (Chapter 6 procedure).
- IKEA bulb: the power-cycle pairing ritual from Chapter 6.

Chapter 8's reporting and binding lived on the sensor, so the
reset cleared them — Step 2's worksheet restores them with the
same two shell commands before decoding that traffic. Nothing is
lost; state has an address, and now you know it.

### Type the keys into Wireshark

**Edit → Preferences → Protocols → Zigbee:**

- **Security Level:** `AES-128 Encryption, 32-bit Integrity Protection`
- **Pre-configured Keys → Edit** — add two rows, byte order
  **Normal** for both:
  1. The Trust Center link key — the famous well-known one, ASCII
     `ZigBeeAlliance09` as bytes:
     `5A:69:67:42:65:65:41:6C:6C:69:61:6E:63:65:30:39`
  2. **Your** network key — the 16 bytes you just set with
     `bdb nwkkey`.

Why two keys? The lesson explains the design; the short version:
the first key lets Wireshark open the one frame that *carries*
your network key across the air during a join; the second opens
everything else.

> The other direction works too, and Step 2 points it out: with
> only the Trust Center key configured, start a capture, let a
> device join, and *read the network key out of the Transport Key
> frame itself* — then add it as above and Wireshark re-opens the
> whole capture. Setting the key yourself is deterministic, so the
> worksheet uses that path.

## First capture (smoke test)

1. Coordinator shell: `bdb channel` — note the number.
2. Wireshark: start the **nRF Sniffer for 802.15.4** interface with
   that channel.
3. Make traffic: reset the sensor DK, wave at the LD2410, wait a
   few seconds.

Expected: frames start flowing — beacons, commands, reports. If
the list stays empty, you are on the wrong channel or the dongle
is not running the sniffer firmware; nothing else is subtle at
this stage. Save the capture (`File → Save`) — Step 2 decodes it,
and every later target, field by field.

## Honest boundary

The firmware build above is compile-verified on the pinned stack
in the course's build environment (sizes quoted from that build).
This environment has no radios: the capture behaviour, LED
patterns, and serial output are the sample's *documented* behaviour
(Nordic's sample README and sniffer user guide), written here as
expected output for you to confirm on your desk — the same
discipline as Chapters 5–10.
