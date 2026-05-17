# Zephyr FW

A professional, generic Zephyr firmware reference project.

This project is intentionally not tied to drones, swarms, UAVs, or any product-specific domain. It is a reusable base for embedded Zephyr applications.

## Goals

- Clean multi-board project structure
- Friendly board profile names
- Future STM32, ESP32, nRF, and other board support
- Board-specific peripherals through Devicetree overlays
- Software features through Kconfig and `.conf` fragments
- Zephyr shell/CLI over UART
- Background service pattern
- Optional future MCUboot support
- Clean separation between app code, tools, services, board profiles, and scripts

## Current validated board

Project board profile:

```text
esp32_oled
```

Zephyr board target:

```text
esp32_devkitc/esp32/procpu
```

Current tested hardware:

```text
ESP32-WROOM-32 development board
USB serial: /dev/cu.usbserial-210
Baud rate: 115200
```

## Project layout

```text
zephyr-fw/
├── README.md
├── VERSION
├── CMakeLists.txt
├── Kconfig
├── prj.conf
├── app/
│   ├── include/app/
│   ├── src/
│   ├── shell/
│   ├── services/
│   └── tools/
├── boards/
│   ├── esp32_oled/
│   └── stm32_template/
├── configs/
├── scripts/
└── docs/
```

## Build workflow

Build happens inside the Ubuntu Zephyr Docker environment.

Flash and serial monitoring happen from macOS.

```text
Docker:
  build firmware

macOS:
  flash ESP32
  open serial terminal
```

## Build inside Docker

Enter Docker, then go to the project:

```bash
cd ~/Documents/projects/zephyr-fw
```

Debug build:

```bash
./scripts/build.sh esp32_oled debug
```

Release build:

```bash
./scripts/build.sh esp32_oled release
```

Clean debug build:

```bash
./scripts/build.sh esp32_oled debug clean
```

Incremental debug build:

```bash
./scripts/build.sh esp32_oled debug incremental
```

Default build mode is `auto`:

```bash
./scripts/build.sh esp32_oled debug
```

## Build modes

```text
auto
  Let west decide whether pristine rebuild is needed.

incremental
  Do not force pristine rebuild. Best for normal .cpp/.hpp edits.

clean
  Force full pristine rebuild.

pristine
  Same as clean.
```

Recommended usage:

```text
Changed .cpp/.hpp:
  ./scripts/build.sh esp32_oled debug incremental

Changed .conf/.overlay/Kconfig/CMakeLists.txt/board settings:
  ./scripts/build.sh esp32_oled debug auto

Strange build issue:
  ./scripts/build.sh esp32_oled debug clean
```

## Firmware output

Debug firmware:

```text
build/esp32_oled/debug/zephyr/zephyr.bin
```

Release firmware:

```text
build/esp32_oled/release/zephyr/zephyr.bin
```

## macOS esptool setup

Homebrew Python is externally managed, so use a dedicated virtual environment:

```bash
python3 -m venv ~/.venvs/esptool
source ~/.venvs/esptool/bin/activate
python -m pip install --upgrade pip
python -m pip install esptool
python -m esptool version
```

For future flashing sessions:

```bash
source ~/.venvs/esptool/bin/activate
```

To leave the environment:

```bash
deactivate
```

## Flash from macOS

From macOS terminal:

```bash
cd ~/Documents/projects/zephyr-fw
source ~/.venvs/esptool/bin/activate
./scripts/flash_esp32_mac.sh /dev/cu.usbserial-210 esp32_oled debug
```

Flash release firmware:

```bash
./scripts/flash_esp32_mac.sh /dev/cu.usbserial-210 esp32_oled release
```

List available serial ports:

```bash
ls /dev/cu.*
```

Successful flashing should show:

```text
Hash of data verified.
Hard resetting via RTS pin...
```

## Serial terminal from macOS

Open serial terminal:

```bash
picocom -b 115200 /dev/cu.usbserial-210
```

Alternative:

```bash
screen /dev/cu.usbserial-210 115200
```

Exit `picocom`:

```text
Ctrl-a
Ctrl-x
```

## Shell prompt

The UART shell prompt is configured in:

```text
configs/shell.conf
```

Prompt setting:

```conf
CONFIG_SHELL_PROMPT_UART="zephyr-fw:~$ "
```

After changing shell config, rebuild and flash again:

```bash
./scripts/build.sh esp32_oled debug clean
```

## Available shell commands

```text
hello_world
app status
app version
board
system uptime
system reboot
```

Example:

```text
zephyr-fw:~$ hello_world
Hello World from Zephyr FW!
Board: esp32_devkitc/esp32/procpu
Version: 0.1.0
Build profile: debug
```

## Versioning

Project version file:

```text
VERSION
```

Current format:

```text
VERSION_MAJOR = 0
VERSION_MINOR = 1
PATCHLEVEL = 0
VERSION_TWEAK = 0
EXTRAVERSION =
```

Runtime version string is currently defined in:

```text
CMakeLists.txt
```

Current value:

```cmake
set(APP_VERSION_STRING "0.1.0")
```

Keep both synchronized when changing versions.

## Board profiles

List supported board profiles:

```bash
./scripts/list_boards.sh
```

Current profiles:

```text
esp32_oled
  Enabled. Maps to esp32_devkitc/esp32/procpu.

stm32_template
  Placeholder for future STM32 support.
```

## Design rules

Do not hardcode board-specific pins or peripherals in application code.

Use:

```text
Devicetree overlays
  Hardware description: pins, buses, displays, sensors, LEDs.

Kconfig and .conf fragments
  Software feature selection.

Board profiles
  Friendly names and board-specific build configuration.

Shell modules
  User-facing commands.

Services
  Background work.

Tools
  Manually triggered utilities.
```

Application logic should stay portable across boards.

## Important files

```text
CMakeLists.txt
  Top-level Zephyr application CMake file.

Kconfig
  Root project Kconfig. Includes Zephyr Kconfig and project options.

VERSION
  Zephyr-compatible project version file.

prj.conf
  Common application configuration.

configs/shell.conf
  UART shell configuration.

configs/debug.conf
  Debug build configuration.

configs/release.conf
  Release build configuration.

boards/esp32_oled/board.conf
  ESP32 board profile configuration.

boards/esp32_oled/board.overlay
  ESP32 board-specific Devicetree overlay.

scripts/build.sh
  Main build script.

scripts/flash_esp32_mac.sh
  ESP32 flashing script for macOS.

app/shell/
  Zephyr shell command implementations.

app/services/
  Background services.

app/tools/
  Reusable command/tool logic.
```

## MCUboot

Current flashing mode is direct application flashing:

```text
ESP32 ROM bootloader -> Zephyr application
```

Future production-style mode:

```text
ESP32 ROM bootloader -> MCUboot -> signed Zephyr application
```

Do not enable MCUboot until the base firmware structure is stable.

## Clean project

Remove build outputs:

```bash
./scripts/clean.sh
```
