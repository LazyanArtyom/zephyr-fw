# Board Porting

Add a board profile under:

```text
boards/<board_profile>/
```

Required files:

```text
board.yml
board.conf
board.overlay
debug.conf
release.conf
flash.conf
README.md
```

Minimum `board.yml`:

```yaml
profile: my_board
display_name: My Board
status: enabled
zephyr_board: vendor_board/qualifier
default_display: off
default_boot: no-mcuboot
serial_baud: 115200
flash_runner:
flash_chip:
flash_offset:
description: Short hardware description.
```

Rules:

```text
Pins and buses          -> board.overlay
Driver/peripheral Kconfig -> board.conf
Debug/release tweaks    -> debug.conf / release.conf
Flash behavior          -> flash.conf and board.yml
Application features    -> configs/features or board/profile fragments
```

Do not hardcode board pins in C++ code.
