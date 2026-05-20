# Build System

The primary build entry point is:

```bash
./scripts/build.sh --board esp32_oled --profile debug --boot no-mcuboot
```

## Main Options

```text
--board      Friendly board profile from boards/<profile>/board.yml
--profile    debug, release, production
--mode       auto, clean, pristine, incremental, no
--boot       no-mcuboot, mcuboot
```

Board features are selected by `boards/<board>/board.yml`,
`boards/<board>/board.conf`, and profile/config fragments. Use
`scripts/package.sh` after a successful build when you need distributable
artifacts.

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
configs/features/logging.conf
boards/<board>/board.conf
configs/profiles/<profile>.conf
boards/<board>/<profile>.conf
configs/boot/<boot>.conf
configs/features/display.conf or no_display.conf
configs/features/shell.conf or no_shell.conf
generated shell prompt config
configs/features/no_asserts.conf
```

Later fragments intentionally override earlier fragments.
