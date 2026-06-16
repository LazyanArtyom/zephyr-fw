# Zephyr FW

Production-oriented Zephyr firmware foundation for reusable embedded projects.

The project is intentionally product-neutral: board definitions, production
policy, packaging, shell commands, storage, manufacturing identity, and health
checks are organized so a real product can fork the repo without inheriting demo
app debt.

## Identity

Project identity is centralized in `project.env`.

```text
Display name:     Zephyr FW
Slug:             zephyr-fw
Firmware name:    zephyr-fw
CMake project:    zephyr_fw
Shell prompt:     zephyr-fw:~$
C++ namespace:    platform
Kconfig prefix:   FW_
Version source:   VERSION
Artifact pattern: zephyr-fw_<version>_<board>_<profile>_<boot>
```

## Current Board

```text
Board profile:    esp32_oled
Zephyr target:    esp32_oled/esp32/procpu
Hardware:         ESP32-WROOM-32 class development board
UART shell:       host serial port at 115200 baud
                  macOS example: /dev/cu.usbserial-11120
                  Linux example: /dev/ttyUSB0
Display:          SSD1306 128x64 OLED, I2C address 0x3c
I2C pins:         SDA GPIO21, SCL GPIO22
Settings:         NVS on the Zephyr storage partition
```

## Layout

```text
zephyr-fw/
├── app/                    # application entry point
│   └── main/
├── boards/                 # Zephyr board roots plus board-local policy
│   └── <vendor>/<board>/
├── commands/               # shell command modules
├── configs/                # boot, profile, and feature config fragments
├── docs/                   # architecture, build, release, and operating docs
├── keys/                   # development signing material only
├── partitions/             # partition notes that point to canonical policy
├── platform/               # reusable board, settings, shell, and storage APIs
├── scripts/                # stable developer and CI entry points
├── services/               # product services built on platform APIs
├── tools/                  # Python metadata, packaging, and validation logic
├── project.env
├── VERSION
├── Kconfig
├── CMakeLists.txt
└── prj.conf
```

Board production policy lives with the board. The ESP32 OLED reference board
contains `metadata.yml`, `board.conf`, `board.overlay`, `debug.conf`,
`release.conf`, `production.conf`, `flash.conf`, and `production.yml`.

## Build, Package, Flash

List supported board profiles:

```bash
./scripts/build.sh --list-boards
```

Build a debug image without MCUboot:

```bash
./scripts/build.sh --board esp32_oled --profile debug --boot no-mcuboot
```

Build incrementally after a successful configured build:

```bash
./scripts/build.sh --board esp32_oled --profile debug --boot no-mcuboot --mode incremental
```

Build a production MCUboot image:

```bash
./scripts/build.sh --board esp32_oled --profile production --boot mcuboot
```

Build a service diagnostics image with production-like optimization, shell access, verbose runtime logging, thread analysis, and coredump logging:

```bash
./scripts/build.sh --board esp32_oled --profile service --boot no-mcuboot
```

Production builds are quiet field builds: the UART shell is disabled. Use
`debug` for developer bring-up and `service` for controlled support diagnostics when you need an interactive shell.

Create a distributable package from a build directory under `dist/`:

```bash
./scripts/package.sh build/esp32_oled/debug/no-mcuboot
./scripts/package.sh build/esp32_oled/production/no-mcuboot
./scripts/package.sh build/esp32_oled/production/mcuboot
```

When run from a terminal without arguments, `package.sh` prompts for the build
directory to package.

Programming is a host-side step. Build/package inside Docker if that is your
Zephyr environment, then run programming commands from the macOS/Linux terminal
that can see the USB serial device. Docker on macOS usually will not show
`/dev/cu.*` devices without explicit USB passthrough.

List likely serial ports on the host:

```bash
./scripts/flash.sh --list-ports
```

Flash from the package on macOS using the callout device:

```bash
cd dist/zephyr-fw_0.1.0_esp32_oled_debug_no-mcuboot
./flash.sh --port /dev/cu.usbserial-11120
```

Flash from the package on Linux:

```bash
cd dist/zephyr-fw_0.1.0_esp32_oled_debug_no-mcuboot
./flash.sh --port /dev/ttyUSB0
```

Erase before flashing and attach a serial monitor:

```bash
./flash.sh --port /dev/cu.usbserial-11120 --erase --monitor
```

Flash a package without changing directories:

```bash
./scripts/flash.sh --package dist/zephyr-fw_0.1.0_esp32_oled_debug_no-mcuboot --port /dev/cu.usbserial-11120
```

Open a serial monitor manually on macOS:

```bash
screen /dev/cu.usbserial-11120 115200
```

Open a serial monitor manually on Linux:

```bash
picocom -b 115200 /dev/ttyUSB0
```

Exit `picocom` with `Ctrl-a`, then `Ctrl-x`. Detach `screen` with `Ctrl-a`, then `d`.

## Useful Commands

Validate board metadata and production policy:

```bash
./tools/fw.py boards validate
```

Print board build environment exported by the Python metadata parser:

```bash
./tools/fw.py boards env esp32_oled
```

Export a host-friendly compile database for clangd:

```bash
./scripts/export_compile_commands.sh --board esp32_oled --profile debug --boot no-mcuboot
```

Run formatting checks:

```bash
./scripts/check_format.sh
```

Run the configured build matrix:

```bash
./scripts/check_build_matrix.sh
```

Create and validate a package:

```bash
./scripts/package.sh build/esp32_oled/debug/no-mcuboot
```

## Package Contract

Packages contain a single metadata file, `manifest.json`, plus checksums,
flash tooling, production policy, and available Zephyr artifacts.

```text
dist/zephyr-fw_<version>_<board>_<profile>_<boot>/
├── zephyr.bin
├── zephyr.elf
├── zephyr.map
├── zephyr.config
├── zephyr.dts
├── manifest.json
├── firmware.sha256
├── partition_summary.txt
├── production.yml
├── flash.sh
└── README.txt
```

MCUboot packages also include signed/update/bootloader artifacts when Zephyr
produces them. Packaging rejects builds whose generated partition table
disagrees with the board `production.yml`.

## Logging

Application code uses Zephyr logging through per-module `LOG_MODULE_REGISTER`
entries and Kconfig-controlled module levels. Shell-enabled builds also expose
Zephyr's `log` command for runtime filtering. See `docs/logging.md`.

## Shell Commands

The debug, release, and service profiles enable the UART shell by default.
Production turns it off unless a board profile explicitly enables it.

```text
fw info
fw version
fw build
board info
board caps
board serial get
board serial set <value>
board hw-rev get
board hw-rev set <value>
system uptime
system reset-reason
system reboot
diag status
diag crash
diag reset-cause
diag threads
diag stacks
diag clear
settings list [subtree]
settings get <key>
settings set <key> <value>
settings reset <key>
settings save
settings load
health status
health storage info
health factory reset
health watchdog status
i2cdetect -r -y 0
i2cdetect -y i2c0
i2cdetect -a -r -y 0
```

## VS Code

Workspace tasks live in `.vscode/tasks.json`; clangd settings live in
`.vscode/settings.json`. See `docs/vscode.md` for optional user keybindings and
compile database export details.

## More Docs

```text
docs/architecture.md
docs/build_system.md
docs/board_porting.md
docs/boot_modes.md
docs/diagnostics.md
docs/display.md
docs/logging.md
docs/mcuboot.md
docs/packaging.md
docs/peripherals.md
docs/release_process.md
docs/shell_commands.md
docs/testing_strategy.md
docs/versioning.md
docs/vscode.md
```
