# ESP32 OLED Production Partition Review

The canonical production partition policy lives in:

```text
boards/espressif/esp32_oled/production.yml
```

The board includes Zephyr's Espressif `partitions_0x1000_amp_4M` layout through
`boards/espressif/esp32_oled/esp32_oled_partitions.dtsi`. Package generation
validates the built `zephyr.dts` partition names, offsets, and sizes against
`production.yml` before writing release metadata.

Keep this file as the human release-review pointer; do not duplicate slot sizes
or offsets here.
