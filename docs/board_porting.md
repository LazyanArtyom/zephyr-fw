# Board Porting

Add a board profile under:

```text
boards/<vendor>/<board>/
```

Required files:

```text
board.yml
Kconfig.<board>
Kconfig.defconfig
<board>_<qualifiers>.dts
<board>_<qualifiers>_defconfig
<board>_<qualifiers>.yaml
board.cmake
support/openocd.cfg
metadata.yml
app.conf
debug.conf
release.conf
flash.conf
README.md
```

Minimum Zephyr `board.yml`:

```yaml
board:
  name: my_board
  full_name: My Board
  vendor: others
  socs:
    - name: stm32u5a9xx
```

Minimum application `metadata.yml`:

```yaml
profile: my_board
display_name: My Board
status: enabled
zephyr_board: my_board
default_display: off
default_boot: mcuboot
serial_baud: 115200
flash_runner: openocd
flash_chip:
flash_offset:
description: Short hardware description.
```

For multi-core or qualified SoCs, use Zephyr's normalized filename rules:

```text
target: esp32_oled/esp32/procpu
files:  esp32_oled_procpu.dts
        esp32_oled_procpu_defconfig
        esp32_oled_procpu.yaml
```

Rules:

```text
Pins and buses             -> <board>_<qualifiers>.dts
SoC/CPU board selection    -> Kconfig.<board>
Board hardware defaults    -> <board>_<qualifiers>_defconfig
App feature defaults       -> app.conf
Debug/release tweaks       -> debug.conf / release.conf
Flash/debug runners        -> board.cmake and support/openocd.cfg
Package/flash metadata     -> metadata.yml and flash.conf
Shared application features -> configs/features or profile fragments
```

Keep placeholders outside `boards/` until they describe real hardware. Use
`templates/zephyr_board/stm32_custom/` as the starting point for STM32 custom
hardware.

Do not hardcode board pins in C++ code.
