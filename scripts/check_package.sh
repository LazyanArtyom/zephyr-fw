#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

BOARD_PROFILE="${1:-esp32_oled}"
BUILD_PROFILE="${2:-debug}"
BOOT_MODE="${3:-no-mcuboot}"

"${PROJECT_ROOT}/scripts/package.sh" \
    --board "${BOARD_PROFILE}" \
    --profile "${BUILD_PROFILE}" \
    --boot "${BOOT_MODE}"

echo "Package check complete."
