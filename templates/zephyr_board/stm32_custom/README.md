# STM32 Custom Board Template

This template is intentionally outside `boards/` so it does not register a fake
board target. Copy it under `boards/<vendor>/<board_name>/` when real STM32
hardware exists, then replace the SoC include, pins, memory, partitions, and
runner settings.

Expected production board-root files:

```text
board.yml
Kconfig.<board_name>
Kconfig.defconfig
<board_name>.dts
<board_name>_defconfig
<board_name>.yaml
board.cmake
support/openocd.cfg
metadata.yml
app.conf
debug.conf
release.conf
flash.conf
```
