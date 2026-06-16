# Architecture

Zephyr FW is structured as a small reusable firmware platform.

## Layers

Hardware description:

```text
boards/<vendor>/<board>/board.yml
boards/<vendor>/<board>/<board>_<qualifiers>.dts
boards/<vendor>/<board>/<board>_<qualifiers>_defconfig
boards/<vendor>/<board>/board.cmake
boards/<vendor>/<board>/support/openocd.cfg
```

Board profile and production policy:

```text
boards/<vendor>/<board>/metadata.yml
boards/<vendor>/<board>/board.conf
boards/<vendor>/<board>/board.overlay
boards/<vendor>/<board>/debug.conf
boards/<vendor>/<board>/release.conf
boards/<vendor>/<board>/production.conf
boards/<vendor>/<board>/flash.conf
boards/<vendor>/<board>/production.yml
```

Feature selection:

```text
configs/features/
configs/profiles/
configs/boot/
```

Application entry point:

```text
app/main/
```

Reusable platform modules:

```text
platform/board/
platform/settings/
platform/shell/
platform/storage/
```

Product services:

```text
services/display/
services/diagnostics/
services/factory_reset/
services/health/
services/manufacturing/
services/watchdog/
```

Shell command modules:

```text
commands/board/
commands/fw/
commands/diagnostics/
commands/health/
commands/settings/
commands/system/
```

Build/deploy workflow:

```text
scripts/
tools/
.vscode/
dist/
```

## Current App Model

There is one reusable application selected by Kconfig and config fragments. This
is the right model while board behavior differs mostly by peripherals and
features.

Add separate app directories later only when behavior truly diverges:

```text
apps/diagnostics/
apps/display_demo/
apps/sensor_node/
apps/gateway/
common/
```

Until then, keep common application logic board-independent and make board
differences visible through board-root devicetree, Kconfig, and board metadata.

## Domain Boundaries

Runtime health and manufacturing identity are separate domains. Board serial and
hardware revision are persisted by `platform::BoardIdentityStore` and exposed to
application code through `services::manufacturing`. Health checks may depend on
the same storage backend, but health services should not own manufacturing
identity data.
