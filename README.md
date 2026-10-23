# Zigbee, Engineered — Course Repository

Companion code for **Zigbee, Engineered**, a project-based course that builds a
Zigbee router device end to end: an nRF52840 with an HLK-LD2410 mmWave presence
sensor that reports occupancy and controls a store-bought IKEA Zigbee bulb —
commissioning, a custom Occupancy Sensing endpoint, reporting and binding,
MCUboot + Zigbee OTA updates, and a Wireshark sniffer capstone.

## Structure

Each chapter's exercises live in their own folder, one folder per step:

```
ch01/step-01-hello-logging
ch01/step-02-shell-command
...
ch11/step-02-decode-your-network
```

Every step is a standalone Zephyr application with its own README: what to
build, what you should see, and why. Work through them in order — later steps
build on the concepts (and sometimes the code) of earlier ones.

## Pinned stack

| Piece | Version |
|---|---|
| nRF Connect SDK (NCS) | v3.4.1 |
| Zigbee add-on (ncs-zigbee / ZBOSS) | v1.4.0 |
| Zephyr | 4.4.2 (as carried by NCS v3.4.1) |
| Board | `nrf52840dk/nrf52840` (coordinator/sniffer steps use the nRF52840 dongle, as the lessons specify) |

## Getting started

Follow Chapter 1 of the course: install the toolchain, set up the NCS
workspace, then clone this repository into the **root of the workspace** and
name the folder `repo`:

```sh
git clone <course repository URL> repo
```

Build any step from inside the workspace, for example:

```sh
west build -p always -b nrf52840dk/nrf52840 -d build/ch01-step01 repo/ch01/step-01-hello-logging
```
