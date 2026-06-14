# ESP32 OLED

Zephyr board target:

```text
esp32_oled/esp32/procpu
```

Hardware:

```text
ESP32-WROOM-32 class development board
UART shell: 115200 baud on UART0
OLED: SSD1306 128x64 on I2C0, address 0x3c
I2C pins: SDA GPIO21, SCL GPIO22
BOOT button: GPIO0, active low
Settings: NVS on the Zephyr storage partition
```

## Board Policy Files

This board is the reference out-of-tree Zephyr board for the project. Keep
hardware description, build defaults, flash parameters, and production policy in
this directory so board behavior is auditable in one place.

```text
metadata.yml       Board profile name, Zephyr target, defaults, flash metadata.
board.conf         Board application defaults and hardware-backed capabilities.
board.overlay      Application devicetree overlay for aliases and peripherals.
debug.conf         Board-specific debug profile overrides.
release.conf       Board-specific release profile overrides.
production.conf    Board-specific production profile overrides.
flash.conf         ESP32 flashing parameters consumed by flash/package tooling.
production.yml     Canonical slot, settings, signing, rollback, and recovery policy.
```

`production.yml` is schema-validated by `./tools/fw.py boards validate` and is
checked against the generated `zephyr.dts` during packaging. The partition notes
under `partitions/` intentionally point back here instead of duplicating offsets
or sizes.

Hardware belongs in the Zephyr board files and `board.overlay`. Application
feature policy belongs in `board.conf`, profile fragments, and `metadata.yml`.
