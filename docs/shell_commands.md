# Shell Commands

Shell support is controlled by:

```text
configs/features/shell.conf
configs/features/no_shell.conf
```

Default behavior:

```text
debug/release: shell on
production:    shell off
```

Commands:

```text
fw info
fw version
fw build
board_info
board info
board caps
system uptime
system reset-reason
system reboot
settings list [subtree]
settings get <key>
settings set <key> <value>
settings reset <key>
settings save
settings load
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
