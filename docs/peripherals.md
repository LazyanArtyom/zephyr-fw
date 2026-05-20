# Peripherals

Peripherals should be described in devicetree overlays and selected through
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

Display support is selected by board profile metadata and board config
fragments. Keep application modules buildable when display support is disabled.
