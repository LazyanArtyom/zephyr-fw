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
Production policy: production.yml and ../../../../partitions/esp32_oled.md
```

This is a real out-of-tree Zephyr board definition. Hardware belongs in the
devicetree files here; application build policy belongs in `metadata.yml`, `board.conf`, and
the profile config fragments.
