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
board.conf
board.overlay
debug.conf
release.conf
production.conf
production.yml
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
default_settings: off
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
App feature defaults       -> board.conf
App devicetree overrides   -> board.overlay
Debug/release/production   -> debug.conf / release.conf / production.conf
Flash/debug runners        -> board.cmake and support/openocd.cfg
Package/flash metadata     -> metadata.yml and flash.conf
Signing/rollback/recovery  -> production.yml
Shared application features -> configs/features or profile fragments
```

Keep required board policy files present, even when the first version only documents that no board-specific override is needed. Use
`boards/espressif/esp32_oled/` as the reference implementation when adding a
new board.

Do not hardcode board pins in C++ code.
