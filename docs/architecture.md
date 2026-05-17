# Architecture

Zephyr FW is structured as a small reusable firmware platform.

## Layers

Hardware description:

```text
boards/<board_profile>/board.overlay
```

Board profile policy:

```text
boards/<board_profile>/board.yml
boards/<board_profile>/board.conf
boards/<board_profile>/<profile>.conf
boards/<board_profile>/flash.conf
```

Feature selection:

```text
configs/features/
configs/profiles/
configs/boot/
```

Application code:

```text
app/src/
app/services/
app/shell/
app/tools/
```

Build/deploy workflow:

```text
scripts/
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
differences visible through devicetree, Kconfig, and board profile metadata.
