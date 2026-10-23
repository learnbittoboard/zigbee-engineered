# Chapter 2 · Step 3 — Kconfig: configure software without touching C

**What you do:** Build the same source three ways — different blink period,
a feature compiled in or out, one module loud and the rest quiet — changing
only configuration files.

**What this step teaches:** the Kconfig/prj.conf split (define vs set),
custom application options, `IS_ENABLED()`, and per-module log levels.

## Build, flash, observe

```bash
west build -p always -b nrf52840dk/nrf52840
west flash
```

```
[00:00:00.211,000] <inf> app: kconfig demo: period = 250 ms, heartbeat feature ON
[00:00:00.215,000] <dbg> sim: raw sample: t=215, distance=312 mm
[00:00:00.465,000] <dbg> sim: raw sample: t=465, distance=1427 mm
[00:00:00.465,500] <inf> app: heartbeat 1: distance = 1427 mm
```

Notice: `<dbg>` lines exist **only** for module `sim`. The `app` module
never prints debug output, because nobody raised *its* level.

## The ideas to take away

- **Devicetree = hardware, Kconfig = software.** If it exists on the board,
  it belongs to the devicetree. If it is a choice about behaviour, it
  belongs to Kconfig.
- **`Kconfig` defines, `prj.conf` sets.** Our `./Kconfig` defines
  `ROOM_BLINK_PERIOD_MS` (default 1000); `prj.conf` sets it to 250. The C
  code reads only `CONFIG_ROOM_BLINK_PERIOD_MS` and cannot tell where the
  value came from.
- **A Kconfig bool tested with `IS_ENABLED()` is not a runtime flag.** Off
  means the code is *absent* from the firmware — this is how one source
  tree ships feature variants.
- **Log levels are per module.** `CONFIG_SIM_LOG_LEVEL_DBG=y` in prj.conf
  raises one module. In the field you will do this to exactly the subsystem
  you suspect, and leave the product's other logs alone. Fine print worth
  knowing: that symbol exists because our `Kconfig` instantiates Zephyr's
  log-config template for the module name `SIM` — per-module symbols are
  created by Kconfig files, not by the C registration alone.

## Try it (10 minutes)

1. Delete the `CONFIG_SIM_LOG_LEVEL_DBG=y` line from prj.conf, rebuild
   pristine, reflash. The `<dbg> sim:` lines vanish — the C code is
   byte-for-byte identical. Where did the messages go? (They were never
   compiled in.)
2. Add `CONFIG_ROOM_FEATURE_HEARTBEAT_LOG=n` to prj.conf and rebuild. The
   boot line reports the feature OFF and no heartbeat lines appear. Compare
   the FLASH usage printed by the build with the previous build — the
   compiled-out feature took its bytes with it.
3. Run `west build -t menuconfig` in the build directory and find
   "Room sensor demo". This menu is generated from our `Kconfig` file —
   help text included.

## Why this matters

Chapter 9 turns MCUboot signing on with options like these; Chapter 7 sets
Zigbee role and buffer sizes the same way. Every Nordic sample you will ever
read is steered from prj.conf — this step is the decoder ring.
