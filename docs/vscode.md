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
