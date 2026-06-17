# Shell Commands

Shell support is controlled by:

```text
configs/features/shell.conf
configs/features/no_shell.conf
```

Default behavior:

```text
debug/release/service: shell on
production:            shell off
```

Commands:

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
health watchdog feed
i2cdetect -r -y 0
i2cdetect -y i2c0
i2cdetect -a -r -y 0
```

Settings keys are intentionally namespaced under:

```text
fw/*
board/*
user/*
network/*
display/*
```

The shell prompt is generated from `project.env` by `scripts/build.sh`.

Zephyr logging shell commands are enabled in shell builds through
`CONFIG_LOG_CMDS=y`. Use `log help` on the target to inspect runtime log-level
controls for the Zephyr version in use. Zephyr kernel shell commands are limited
to diagnostics builds through `configs/features/diagnostics.conf`.
