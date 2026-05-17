#!/usr/bin/env bash
set -euo pipefail

cat <<EOF
Supported board profiles:

  esp32_oled
    Zephyr target:
      esp32_devkitc/esp32/procpu
    Status:
      enabled

  stm32_template
    Zephyr target:
      not configured yet
    Status:
      placeholder

Examples:

  ./scripts/build.sh esp32_oled debug
  ./scripts/build.sh esp32_oled release
EOF
