# Diagnostics

The firmware owns a persistent diagnostics record under `fw/diagnostics` in the
settings backend. It records boot count, reset cause, firmware version, git
commit, build profile, panic count, last fatal reason, and the last watchdog
reason recorded by firmware code.

Fatal handling uses a two-stage pattern. The fatal handler stores only a compact
marker in noinit RAM, flushes Zephyr logs, and reboots. On the next clean boot,
`services::diagnostics::DiagnosticsService` persists that marker to settings.
This avoids flash writes from corrupted fatal context while still making the
crash visible after reboot.

Shell commands:

```text
diag status
diag crash
diag reset-cause
diag threads
diag stacks
diag clear
```

`diag threads` and `diag stacks` require thread diagnostics options, so they are
intended for `debug` and `service` images. Normal production images keep the
persistent crash record and reset cause, but do not expose shell diagnostics when
the production shell policy disables the shell.

## Watchdog Resets

The watchdog service records hardware watchdog reset detection from
`platform::ResetInfo::Current()` when diagnostics storage is available. Runtime
status is exposed through `health watchdog status`, and a manual feed check is
available as `health watchdog feed` in shell-enabled builds.
