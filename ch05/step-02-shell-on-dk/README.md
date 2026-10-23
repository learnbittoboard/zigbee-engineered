# Chapter 5 · Step 2 — The same shell on the DK: watch a device join

**What you do:** Build the *same* shell sample — still unmodified —
for the nRF52840 DK, and join it to the network your dongle formed
in Step 1. One sample, two boards, two roles: the difference between
"the network" and "a device" is about to stop being abstract.

**What this step teaches:** Steering from the joiner's side. The DK
boots as a **router** (the sample's default role — check its
`prj.conf`: `CONFIG_ZIGBEE_ROLE_ROUTER=y`), and `bdb start` on a
device that owns no network means *find one and join it*. You will
watch the join land from both ends: the DK's own shell and LED, and
the coordinator's view of a new neighbor.

## Build (from your NCS workspace root)

No `FILE_SUFFIX` this time: on the DK the sample's default
configuration puts the shell on the UART, which the DK's debugger
chip conveniently bridges to USB. Same add-on module flag as Step 1.

```bash
west build -p always -b nrf52840dk/nrf52840 \
    ncs-zigbee/samples/shell -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
west flash
```

## Join

1. Dongle first: make sure your Step 1 coordinator is running
   (reconnect its terminal; `bdb role` should answer `zc`).
2. DK second: connect to the DK's serial port, press Enter for the
   `uart:~$` prompt, and steer:

```
uart:~$ bdb role
zr
Done

uart:~$ bdb start
Started router
Done
```

3. Within a few seconds the DK's **LED 3 turns on** — the
   sample's "I am in a network" light (the same convention our own
   Chapter 6 firmware uses). Ask the DK who it became:

```
uart:~$ zdo short
2485
Done
```

Your hex will differ — that is the point to absorb. `0x0000` was
*assigned by definition* (the coordinator is always first); this
address was *assigned by the coordinator during the join*, from its
address pool, and it is the name the network will use for this
device until it leaves or is reset. The device's EUI-64
(`zdo eui64`) is its birth name; the short address is its local
name. Zigbee traffic uses the local one.

## Prove it from the coordinator's chair

Back on the dongle's terminal, look at the network you now own:

```
uart:~$ zdo mgmt_lqi 0x0000
```

Expected behaviour: the coordinator returns its neighbor table and
the DK appears in it — a router neighbor at the short address you
just read off the DK, with a link-quality (LQI) number that drops
if you walk the DK to another room. Then talk to it:

```
uart:~$ zcl ping 0x2485 10
Ping time: 20 ms
Done
```

(use *your* DK's short address). A ping reply is the full stack —
radio, network key, addressing, ZCL plumbing — proven in one line.
Behind the reply, the endpoint logger also narrates the packet it
received; Chapter 11 teaches you to read that line properly, and
`log disable zigbee.eprxzcl` quiets it when it gets chatty.

## The ideas to take away

- **Role is data, not code.** One firmware image became coordinator
  on the dongle (because someone typed `bdb role zc`) and router on
  the DK (because the role defaulted to `zr`). When Chapter 6 builds
  *our* router firmware, the role is chosen in `prj.conf` instead —
  same decision, made at build time because a product cannot ask
  its user to type shell commands.
- **Joining is an exchange, not a subscription.** The DK asked
  (beacon request), the coordinator offered (beacon), the DK
  associated, and the coordinator *transported the network key* to
  it, encrypted under a key both sides could derive. The lesson
  draws the whole sequence; Step 1 and this step are its two ends.
- **You now own the diagnostic split.** Every later failure gets
  bisected the way you just worked: the coordinator side is
  interrogable (`mgmt_lqi`, `nbr`, `zcl ping`), the device side is
  interrogable (`zdo short`, LED, logs) — and "Zigbee doesn't work"
  is never an acceptable stopping point again.

## Exercises

1. On the DK, run `bdb factory_reset`, confirm LED 3 goes dark and
   `zdo short` no longer reports a joined address. Then `bdb start`
   and rejoin. Did the DK get the *same* short address back?
   Explain why or why not using the phrase "address assignment is
   the coordinator's bookkeeping".
2. From the coordinator, read the DK's Basic cluster the way
   Chapter 7 will read our sensor: `zdo active_ep 0x2485` first
   (find the shell endpoint — it is 64 in this sample), then
   `zcl attr read 0x2485 64 0x0000 0x0104 0x0000` (Basic cluster,
   ZCLVersion attribute). Write down the reply format; you will
   meet it again.
3. Power the DK off mid-conversation. Back on the coordinator,
   `zcl ping` the silent address and `zdo mgmt_lqi 0x0000` again:
   how does the network distinguish "left" from "just quiet", and
   how long do you think the neighbor entry lingers? Then try the
   other neighbor-table lens: `nbr monitor on` on the *coordinator*
   makes it log every neighbor-table change as it happens —
   `nbr monitor trigger` replays the current table on demand.
   Power the DK back on and watch which of the three views
   (mgmt_lqi, ping, nbr monitor) notices first.

## Why this matters

You have now stood in both chairs of a Zigbee network: the chair
that forms and admits, and the chair that asks and joins. Chapter 6
throws the borrowed router away and puts *your* firmware in the
joining chair — but every diagnostic move you will need was
rehearsed here, on stock code that cannot be blamed.
