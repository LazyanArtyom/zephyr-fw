#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PORT="${1:-}"
BOARD_PROFILE="${2:-esp32_oled}"
BUILD_PROFILE="${3:-debug}"

if [[ -z "${PORT}" ]]; then
    echo "Usage: $0 <serial_port> [board_profile] [debug|release]"
    echo "Example: $0 /dev/cu.usbserial-210 esp32_oled debug"
    exit 1
fi

BIN_FILE="${PROJECT_ROOT}/build/${BOARD_PROFILE}/${BUILD_PROFILE}/zephyr/zephyr.bin"

if [[ ! -f "${BIN_FILE}" ]]; then
    echo "Error: firmware not found:"
    echo "  ${BIN_FILE}"
    echo
    echo "Build first:"
    echo "  ./scripts/build.sh ${BOARD_PROFILE} ${BUILD_PROFILE}"
    exit 1
fi

python -m esptool     --chip esp32     --port "${PORT}"     --baud 460800     write-flash     --flash-mode dio     --flash-freq 40m     --flash-size detect     0x1000 "${BIN_FILE}"
