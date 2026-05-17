# VS Code

VS Code settings and tasks are committed under:

```text
.vscode/
```

Important tasks:

```text
build: esp32 debug
build: esp32 debug clean
build: esp32 release
export compile_commands
package: esp32 debug
flash: esp32 mac
monitor: esp32 uart
check: format
check: clang-tidy
```

After building inside Docker, run:

```bash
./scripts/export_compile_commands.sh --board esp32_oled --profile debug --boot no-mcuboot
```

This creates a host-path-adjusted `compile_commands.json` for clangd.
