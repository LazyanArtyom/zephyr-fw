# ESP32 OLED Production Partition Summary

The board currently includes Zephyr's Espressif `partitions_0x1000_amp_4M` layout.
Production policy is recorded in `boards/espressif/esp32_oled/production.yml`.

| Region | Offset | Size | Policy |
| --- | ---: | ---: | --- |
| MCUboot | `0x1000` | `60K` | Bootloader when MCUboot is enabled |
| System | `0x10000` | `64K` | Espressif system partition |
| Slot 0 | `0x20000` | `1344K` | Primary application image |
| Slot 1 | `0x170000` | `1344K` | Secondary signed update image |
| App CPU slot 0 | `0x2c0000` | `448K` | Reserved by AMP layout |
| App CPU slot 1 | `0x330000` | `448K` | Reserved by AMP layout |
| LP core slot 0 | `0x3a0000` | `32K` | Reserved by AMP layout |
| LP core slot 1 | `0x3a8000` | `32K` | Reserved by AMP layout |
| Settings storage | `0x3b0000` | `192K` | NVS settings and manufacturing values |
| Scratch | `0x3e0000` | `124K` | MCUboot scratch/swap policy |
| Coredump | `0x3ff000` | `4K` | Crash dump reservation |

Factory reset is settings-only. It clears mutable firmware/user/network/display
settings while preserving bootloader, slot0, slot1, recovery images, board
serial, and hardware revision.
