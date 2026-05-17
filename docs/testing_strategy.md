# Testing Strategy

Near-term testing:

```text
Build all enabled board profiles
Build debug and release profiles
Build display on/off
Run hello_world shell command
Run app version and board shell commands
Verify OLED message on esp32_oled
Verify package metadata and checksums
```

Future testing:

```text
Move pure logic into unit-testable modules
Add native/sim tests where Zephyr APIs are not required
Add hardware smoke checklist per board
Add shell command smoke tests through serial automation
Add twister tests when app modules become testable
```
