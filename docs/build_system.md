# Build System

The primary build entry point is:

```bash
./scripts/build.sh --board esp32_oled --profile debug --boot no-mcuboot
```

## Main Options

```text
--board      Board profile from boards/<vendor>/<board>/metadata.yml
--profile    debug, release, production
--mode       auto, clean, pristine, incremental, no
--boot       no-mcuboot, mcuboot
```

Board hardware is selected by Zephyr board-root files under
`boards/<vendor>/<board>/`. Application policy is selected by `metadata.yml`,
`board.conf`, and profile/config fragments. Board metadata and board-local
paths are parsed through `tools/board_metadata.py`, exposed to shell entry
points by `tools/fw.py boards env`. Use `scripts/package.sh` after a successful
build when you need distributable artifacts.

## Output Layout

```text
build/<board>/<profile>/<boot>/
```

Examples:

```text
build/esp32_oled/debug/no-mcuboot/zephyr/zephyr.bin
build/esp32_oled/release/no-mcuboot/zephyr/zephyr.elf
```

## Config Ordering

The build script composes config fragments in this order:

```text
prj.conf
boards/<vendor>/<board>/board.conf
configs/profiles/<profile>.conf
boards/<vendor>/<board>/<profile>.conf
configs/boot/<boot>.conf
configs/features/display.conf or no_display.conf
configs/features/settings.conf or no_settings.conf
configs/features/shell.conf or no_shell.conf
generated shell prompt config
configs/features/no_asserts.conf
```

Later fragments intentionally override earlier fragments.

The build also passes `-DBOARD_ROOT=<project root>`, so custom boards in this
repository are resolved directly by Zephyr.

## Feature Fragments

Feature fragments are intentionally small policy fragments. Keep board
hardware capabilities such as I2C in `board.conf`; keep feature-specific driver
and application selections in `configs/features/`. Always-on application defaults,
including logging backend defaults, belong in `prj.conf`. The `no_*` fragments are kept
so metadata-selected features can be disabled deterministically even when a board
or profile fragment enables them earlier in the config order.
