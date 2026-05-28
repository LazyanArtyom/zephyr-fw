# Peripherals

Peripherals should be described in board-root devicetree and selected through
Kconfig/config fragments.

Current ESP32 OLED profile:

```text
Display: SSD1306 128x64
Bus:     I2C
Address: 0x3c
SDA:     GPIO21
SCL:     GPIO22
Alias:   display0
```

Display support is selected by board metadata and app config
fragments. Keep application modules buildable when display support is disabled.


## Persistent Settings

Settings are selected through board metadata and feature fragments:

```text
configs/features/settings.conf
configs/features/no_settings.conf
```

The ESP32 OLED profile enables NVS-backed settings on the Zephyr
`storage_partition`. Future boards should set `default_settings: on` only when
their devicetree provides a validated flash storage partition.
