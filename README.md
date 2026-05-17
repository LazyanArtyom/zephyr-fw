# Zephyr Golden FW

A professional, generic Zephyr firmware reference project.

This project is intentionally not tied to drones, swarms, or any product-specific domain. It is a reusable golden base for embedded Zephyr applications.

## Goals

- Clean multi-board structure
- Easy board selection
- Easy future STM32 support
- Board-specific peripherals via Devicetree overlays
- Feature selection via Kconfig/conf fragments
- Zephyr shell/CLI enabled
- Background service pattern
- Optional future MCUboot support

## Current board

Friendly project board name:

```text
esp32_oled
```

Zephyr board target:

```text
esp32_devkitc/esp32/procpu
```

## Build

```bash
./scripts/build.sh esp32_oled debug
```

## Flash from macOS

```bash
./scripts/flash_esp32_mac.sh /dev/cu.usbserial-210
```

## Serial terminal

```bash
picocom -b 115200 /dev/cu.usbserial-210
```

Then run:

```text
hello_world
app status
app version
board
system uptime
```

## Design rule

Do not hardcode board-specific pins or peripherals in application code.

Use Devicetree for hardware, Kconfig/config fragments for software features, board profiles for board selection, shell modules for commands, and services for background work.
