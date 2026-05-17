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
hello_world
app status
app version
board
system uptime
system reboot
```

The shell prompt is generated from `project.env` by `scripts/build.sh`.
