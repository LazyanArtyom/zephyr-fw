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

Display support is optional:

```bash
./scripts/build.sh --board esp32_oled --display on
./scripts/build.sh --board esp32_oled --display off
```

Application modules must compile when display support is disabled.
