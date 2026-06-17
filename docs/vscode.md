# VS Code

VS Code settings and tasks are committed under:

```text
.vscode/
```

Important tasks:

```text
build: esp32 debug no-mcuboot
build: esp32 release no-mcuboot
build: esp32 debug mcuboot
build: esp32 release mcuboot
export compile_commands: esp32 debug no-mcuboot
package: esp32 debug no-mcuboot
flash: esp32
monitor: esp32 uart
check: format
check: clang-tidy
kb: build debug no-mcuboot
kb: build release no-mcuboot
kb: build debug mcuboot
kb: build release mcuboot
```

After building inside Docker, run:

```bash
./scripts/export_compile_commands.sh --board esp32_oled --profile debug --boot no-mcuboot
```

This creates a host-path-adjusted and clangd-friendly `compile_commands.json`.

## clangd

Use the clangd extension as the primary C/C++ language server. The Microsoft
C/C++ extension IntelliSense and automatic clang-tidy runner are disabled in
workspace settings so diagnostics come from one place.
The exported compile database can rewrite Docker paths when needed. Pass
`--container-root` and `--host-root` if the build path inside Docker differs
from the host path.

It also strips Zephyr SDK flags that host clangd/clang-tidy do not understand.

## Known Analyzer Limitations

Zephyr and ESP-IDF headers use dense macro layers for devicetree, logging, SoC
registers, and Xtensa-specific compiler behavior. Host clangd/clang-tidy can
misread those macro expansions even when the firmware builds correctly with the
Zephyr SDK.

Project policy is to keep Zephyr logging and devicetree APIs in application
code. Do not make firmware code less idiomatic just to satisfy a host analyzer
false positive from Zephyr internals. Prefer one of these fixes instead:

- tune `.clangd` or `.clang-tidy` for known macro false positives
- keep `HeaderFilterRegex` scoped to project code
- fix the compile database export script when an SDK flag confuses host clang
- add a narrow `NOLINT` only when the warning is in project code and understood

Current intentionally disabled checks include Zephyr-macro-prone bugprone
checks for macro parentheses, reserved identifiers, sizeof expressions, logging
macro arithmetic, and a few broad style-only checks that conflict with the
existing firmware/Zephyr API style.

## Hotkeys

VS Code keybindings are user-profile settings. Open `Preferences: Open Keyboard
Shortcuts (JSON)` and add:

```json
[
  { "key": "f6", "command": "workbench.action.tasks.runTask", "args": "kb: build debug no-mcuboot" },
  { "key": "shift+f6", "command": "workbench.action.tasks.runTask", "args": "kb: build release no-mcuboot" },
  { "key": "f7", "command": "workbench.action.tasks.runTask", "args": "kb: build debug mcuboot" },
  { "key": "shift+f7", "command": "workbench.action.tasks.runTask", "args": "kb: build release mcuboot" }
]
```

Key meanings:

| Key | Action |
| --- | --- |
| `F6` | Debug no-MCUboot build and refresh `compile_commands.json` |
| `Shift+F6` | Release no-MCUboot build and refresh `compile_commands.json` |
| `F7` | Debug MCUboot build and refresh `compile_commands.json` |
| `Shift+F7` | Release MCUboot build and refresh `compile_commands.json` |

Some keyboards require `Fn + Fx` depending on system settings.

## Host Serial Port

The flash task prompts for a host serial device. Use `/dev/cu.usbserial-*`
on macOS and `/dev/ttyUSB*`, `/dev/ttyACM*`, or `/dev/serial/by-id/*` on Linux.
Run `./scripts/flash.sh --list-ports` from the host terminal to see likely
ports.
