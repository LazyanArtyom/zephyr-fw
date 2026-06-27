# Testing Strategy

Near-term testing:

```text
Validate board metadata and production policy
Build all enabled board profiles
Build debug, release, and production profiles
Build display on/off feature combinations
Build no-MCUboot and MCUboot boot modes
Package debug and production outputs
Verify manifest.json, firmware.sha256, and partition_summary.txt
Run fw version, fw build, and board info shell smoke tests
Run settings get/set/reset on a disposable test key
Run health status, health storage info, and health watchdog status
Verify board serial and board hw-rev commands with test values
Verify OLED splash on esp32_oled hardware
```

Host tests:

```sh
./scripts/run_tests.sh
```

Current host coverage checks shell command argument contracts, config policy, and
firmware package DTS partition parsing.

Future testing:

```text
Move pure logic into unit-testable modules
Add native/sim tests where Zephyr APIs are not required
Add hardware smoke checklist per board
Add shell command smoke tests through serial automation
Add package contract tests with known-good fixture builds
Add twister tests when app modules become testable
```
