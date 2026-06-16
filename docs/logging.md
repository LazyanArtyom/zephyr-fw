# Logging Policy

This project uses Zephyr logging as the only normal application logging path.
Do not replace application logs with `printk`. `printk` remains available for
very early boot or emergency breadcrumbs, and `CONFIG_LOG_PRINTK=y` routes those
messages through the logging subsystem when logging is running.

## Build Profiles

```text
debug:      compile debug logs, start runtime filtering at info
release:    compile warning/error logs by default, allow selected modules at info
production: emit errors by default, compile warnings only where modules opt in
service:    controlled support image with shell and verbose runtime logging
```

Normal builds use deferred logging:

```text
CONFIG_LOG_MODE_DEFERRED=y
CONFIG_LOG_BACKEND_UART=y
CONFIG_LOG_RUNTIME_FILTERING=y
CONFIG_LOG_PRINTK=y
```

Shell-enabled builds also enable Zephyr logging shell commands:

```text
CONFIG_LOG_CMDS=y
```

Production/no-shell builds explicitly disable shell log commands while keeping
runtime filtering available for future non-shell control paths.

## Module Log Levels

Each project module that logs must register its own Zephyr logging module and
must use a project Kconfig log-level symbol, not `CONFIG_LOG_DEFAULT_LEVEL`
directly.

Current project-owned log modules:

```text
app_main                CONFIG_FW_APP_MAIN_LOG_LEVEL
platform_board_info     CONFIG_FW_PLATFORM_BOARD_LOG_LEVEL
display_service         CONFIG_FW_SERVICE_DISPLAY_LOG_LEVEL
heartbeat_service       CONFIG_FW_SERVICE_HEALTH_HEARTBEAT_LOG_LEVEL
```

These symbols are defined in `app/logging/Kconfig` using Zephyr's standard
`subsys/logging/Kconfig.template.log_config` template. Each module therefore has
normal Kconfig choices for off, error, warning, info, debug, or default.

## Adding Logging To A Module

1. Add the logging include near the other Zephyr includes:

```cpp
#include <zephyr/logging/log.h>
```

2. Register one module in exactly one `.cpp` file:

```cpp
LOG_MODULE_REGISTER(my_service, CONFIG_FW_SERVICE_MY_SERVICE_LOG_LEVEL);
```

3. Add a matching entry to `app/logging/Kconfig`:

```text
module = FW_SERVICE_MY_SERVICE
module-str = services/my_service
source "subsys/logging/Kconfig.template.log_config"
```

4. Use Zephyr log macros in implementation code:

```cpp
LOG_ERR("failed to start: %d", rc);
LOG_WRN("configuration missing, using defaults");
LOG_INF("service started");
LOG_DBG("state=%u", state);
```

5. Avoid logging secrets, private keys, credentials, tokens, or raw user data.
For production firmware, prefer stable event text and small numeric context over
large formatted payloads.

## Runtime Control

In debug, release, and service builds the UART shell is enabled. Zephyr's log
shell commands are available through the `log` command, so support images can
raise or lower runtime log levels without rebuilding firmware.

Recommended workflow:

```text
build debug/service image
flash it
open serial shell
inspect available log command help with: log help
raise one noisy module only while debugging
restore the level before collecting final logs
```

Do not make production images verbose just to debug a field issue. Build a
`service` image instead, or enable one specific module level through Kconfig for
a controlled diagnostic build.
