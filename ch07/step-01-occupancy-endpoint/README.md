# Chapter 7 · Step 1 — The Occupancy Sensing endpoint, declared

**What you do:** Replace the skeleton's range-extender placeholder
with a real product endpoint — an HA **Occupancy Sensor** whose
server side carries the **Occupancy Sensing cluster (0x0406)** —
then prove it from the coordinator shell: descriptor, attribute
reads, and an attribute that flips when you press DK Button 1.

**What this step teaches:** The grammar of a Zigbee device. A
device is *endpoints*; an endpoint is *clusters*; a cluster is
*attributes and commands* — and none of it is invented per product.
"Occupancy sensor" is a standard device type with a standard
cluster, which is why a coordinator it has never met can read it
the minute it joins. This step is the declaration of that grammar
in ZBOSS's macro dialect; the sensor itself arrives in Step 2.

## Build, flash, observe

Same chassis as Chapter 6, same invocation:

```bash
west build -p always -b nrf52840dk/nrf52840 \
    course/repo/ch07/step-01-occupancy-endpoint -- \
    -DZEPHYR_EXTRA_MODULES=<workspace>/ncs-zigbee
west flash
```

Join it to the Chapter 5 network exactly as in Chapter 6 (steering
is unchanged — check the join line's Extended PAN ID against your
identity card, as always). Then, on the DK log:

```
<inf> app: Starting Zigbee, Engineered - occupancy endpoint (simulated sensor)
...
<inf> app: Endpoint 10 live: Basic + Identify + Occupancy Sensing. Button 1 flips the Occupancy attribute
```

Press **Button 1**. Each press schedules one line:

```
<inf> app: Occupancy attribute -> OCCUPIED
```

Press again: `UNOCCUPIED`. That is the entire simulation — your
thumb is a stand-in sensor, and the point of the stand-in is that
everything *downstream* of the attribute can now be tested for
real.

## See it from the coordinator's chair

Find the device's short address (`zdo mgmt_lqi 0x0000`, as
before), then interrogate the endpoint you just declared:

```
uart:~$ zdo simple_desc_req 0x<short> 10
uart:~$ zcl attr read 0x<short> 10 0x0406 0x0104 0x0000
uart:~$ zcl attr read 0x<short> 10 0x0406 0x0104 0x0001
uart:~$ zcl attr read 0x<short> 10 0x0406 0x0104 0x0002
```

What you are looking at:

- The **simple descriptor** now reports device ID **0x0107**
  (Occupancy Sensor) and lists `0x0406` among the endpoint's
  *server* clusters, beside Basic and Identify. Nothing about the
  radio, the join, or the firmware's startup changed — one
  declaration did all of it.
- Attribute **0x0000 (Occupancy)** reads back the bitmap your
  thumb controls: press Button 1, read again, watch it flip
  between `0x00` and `0x01` — the read crosses the network and
  lands in *your* `dev_ctx` storage, because the declaration
  pointed the stack at it.
- Attribute **0x0001 (OccupancySensorType)** reads **1 —
  Ultrasonic**. Not a typo, and not a lie that matters: read the
  long comment in `app_clusters_attr_init()` before you judge it.
  The ZCL enum predates consumer mmWave radar and offers four
  values; an active transmit-and-listen wave sensor belongs to
  the ultrasonic family, and the passive alternative (PIR) would
  be the actual falsehood. **0x0002** (the type *bitmap*) says
  the same thing as a bit flag — bit 1 set.

## The ideas to take away

- **You declare; the stack performs.** There is no
  `send_read_response()` anywhere in this application. Reads,
  descriptor answers, identify blinking — all of it is the stack
  walking tables *you declared at compile time*: attribute lists
  of pointers into your storage, cluster lists, one endpoint
  descriptor. Your runtime code only *changes values*
  (`ZB_ZCL_SET_ATTRIBUTE`) and reacts to signals.
- **Server vs client is a property of the cluster instance, not
  the cluster.** Occupancy Sensing sits on this device as a
  *server* (we hold the attributes; others read them). The same
  cluster ID on a wall controller would be a *client* (it holds
  none; it reads and subscribes). Chapter 8 adds a client cluster
  to this very endpoint — watch how different the two sides feel
  to declare.
- **Storage is yours; meaning is the spec's.** `dev_ctx.occupancy`
  is three bytes of your RAM. What makes them *the* Occupancy
  attribute — ID 0x0000, bitmap8, bit 0 = occupied — is the ZCL
  specification, not your firmware. That is the entire bargain of
  standard clusters: your bytes, everyone's dictionary.
- **The sensor-type enum is a lesson in spec archaeology.** Specs
  freeze vocabularies; technology keeps moving. Reporting
  ULTRASONIC for a radar sensor is the honest mapping into the
  frozen vocabulary — and being able to *defend* the mapping,
  from the mechanism outward, is a professional skill this course
  will keep asking for.

## Exercises

1. Read attribute 0x0000 *before* ever pressing Button 1. What
   value does the network see, and which line of
   `app_clusters_attr_init()` is responsible? Why is "the device
   boots unoccupied" a safety property, not just a default?
2. From the coordinator, try reading attribute 0x0010
   (PIROccupiedToUnoccupiedDelay — it exists in the spec). What
   comes back, and why? Then find, in
   `zb_zcl_occupancy_sensing.h`, the exact list of attributes
   the stack's declare macro actually created for us. Spec
   *defines*; macro *decides*.
3. Change the declared sensor type to PIR (0) in the init
   function, rebuild, and re-read 0x0001 and 0x0002 — both must
   change together to stay honest. Then change it back, and
   write one sentence in your notebook about which of the two
   attributes a strict interoperability checker would trust more,
   and why. (There is a defensible answer; it is not "both".)

## Why this matters

This is the first moment the course's device stops being a
Nordic demo and becomes a *product a stranger's hub can
understand*. Home Assistant, SmartThings, a vendor gateway —
none of them will ever run your code, and all of them can read
endpoint 10 today, because you spoke the standard instead of
inventing a protocol. Step 2 keeps every byte of that and swaps
the thumb for a radar.
