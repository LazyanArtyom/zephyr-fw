# Zephyr FW

Generic Zephyr firmware boilerplate for reusable embedded projects.

The project is intentionally generic and not tied to any product-specific
domain. It is a small BSP/application platform that can be copied into future
ESP32, STM32, nRF, or custom-board projects.

## Identity

Project identity is centralized in:

```text
project.env
```

Current identity:

```text
Display name:     Zephyr FW
Slug:             zephyr-fw
Firmware name:    zephyr-fw
CMake project:    zephyr_fw
Shell prompt:     zephyr-fw:~$
C++ namespace:    app
Kconfig prefix:   APP_
Version source:   VERSION
Artifact pattern: zephyr-fw_<version>_<board>_<profile>_<boot>
```

## Current Board

Friendly board profile:

```text
esp32_oled
```

Zephyr board target:

```text
esp32_devkitc/esp32/procpu
```

Validated hardware:

```text
ESP32-WROOM-32 development board
USB serial: /dev/cu.usbserial-210
UART shell: 115200 baud
OLED: SSD1306, 128x64, I2C address 0x3c, SDA GPIO21, SCL GPIO22
```

## Layout

```text
zephyr-fw/
├── project.env
├── VERSION
├── CMakeLists.txt
├── Kconfig
├── prj.conf
├── app/
│   ├── include/app/
│   ├── src/
│   ├── services/
│   ├── shell/
│   └── tools/
├── boards/
│   └── <board_profile>/
│       ├── board.yml
│       ├── board.conf
│       ├── board.overlay
│       ├── debug.conf
│       ├── release.conf
│       └── flash.conf
├── configs/
│   ├── boot/
│   ├── features/
│   └── profiles/
├── scripts/
├── docs/
├── keys/
└── partitions/
```

## Build

Builds are intended to run inside the Ubuntu Zephyr Docker environment.

Debug development build:

```bash
./scripts/build.sh --board esp32_oled --profile debug --boot no-mcuboot
```

Debug build with global Zephyr assertions enabled:

```bash
./scripts/build.sh --board esp32_oled --profile debug --boot no-mcuboot --asserts on
```

Incremental build:

```bash
./scripts/build.sh --board esp32_oled --profile debug --mode incremental
```

Production-style MCUboot build scaffold:

```bash
./scripts/build.sh --board esp32_oled --profile production --boot mcuboot
```

List board profiles:

```bash
./scripts/list_boards.sh
```

## Flash From macOS

Flashing is done from macOS because Docker Desktop does not cleanly expose the
CH340 serial device into Linux containers.

The flash script auto-detects `~/.venvs/esptool/bin/python` and falls back to
`python3`.

```bash
./scripts/flash_esp32_mac.sh \
  --port /dev/cu.usbserial-210 \
  --board esp32_oled \
  --profile debug \
  --boot no-mcuboot
```

Erase and then flash:

```bash
./scripts/flash_esp32_mac.sh --port /dev/cu.usbserial-210 --erase
```

Flash and open monitor:

```bash
./scripts/flash_esp32_mac.sh --port /dev/cu.usbserial-210 --monitor
```

## Serial Monitor

```bash
picocom -b 115200 /dev/cu.usbserial-210
```

Exit `picocom` with `Ctrl-a`, then `Ctrl-x`.

## Package

Create a deterministic package under `dist/`:

```bash
./scripts/package.sh --board esp32_oled --profile debug --boot no-mcuboot
```

Example output:

```text
dist/zephyr-fw_0.1.0_esp32_oled_debug_no-mcuboot/
├── zephyr.bin
├── zephyr.elf
├── zephyr.map
├── firmware.meta.json
├── firmware.sha256
├── flash.sh
└── README.txt
```

## Tooling

VS Code tasks are in `.vscode/tasks.json`.

Useful tasks:

```text
build: esp32 debug no-mcuboot
build: esp32 release no-mcuboot
build: esp32 debug mcuboot
build: esp32 release mcuboot
export compile_commands: esp32 debug no-mcuboot
package: esp32 debug no-mcuboot
flash: esp32 mac
monitor: esp32 uart
check: format
check: clang-tidy
kb: build debug no-mcuboot
kb: build release no-mcuboot
kb: build debug mcuboot
kb: build release mcuboot
```

After a Docker build, export a host-friendly compile database for clangd:

```bash
./scripts/export_compile_commands.sh --board esp32_oled --profile debug --boot no-mcuboot
```

Recommended VS Code user keybindings:

```json
[
  { "key": "f6", "command": "workbench.action.tasks.runTask", "args": "kb: build debug no-mcuboot" },
  { "key": "shift+f6", "command": "workbench.action.tasks.runTask", "args": "kb: build release no-mcuboot" },
  { "key": "f7", "command": "workbench.action.tasks.runTask", "args": "kb: build debug mcuboot" },
  { "key": "shift+f7", "command": "workbench.action.tasks.runTask", "args": "kb: build release mcuboot" }
]
```

VS Code stores keybindings in the user profile, not in workspace settings. The
same snippet is committed at `.vscode/keybindings.json` as a project reference.

## Shell Commands

Initial commands:

```text
hello_world
app status
app version
board
system uptime
system reboot
```

## More Docs

Start with:

```text
docs/architecture.md
docs/build_system.md
docs/board_porting.md
docs/boot_modes.md
docs/packaging.md
docs/versioning.md
docs/vscode.md
docs/coding_standard.md
docs/release_process.md
```
