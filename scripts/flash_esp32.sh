#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
    cat <<EOF
Usage:
  $0 --port <serial_port> [options]
  $0 <serial_port> [board_profile] [debug|release|production]

Options:
  --port <serial_port>     Serial device, for example /dev/ttyUSB0
  --board <profile>        Board profile (default: esp32_oled)
  --profile <profile>      Build profile (default: debug)
  --boot <mode>            Boot mode (default: no-mcuboot)
  --image <file>           Explicit image path
  --erase                  Erase flash before writing
  --monitor                Start picocom after flashing
  --baud <baud>            Flash baud rate (default: board flash.conf or 460800)
  --python <python>        Python executable for esptool
EOF
}

die() {
    echo "Error: $*" >&2
    exit 1
}

load_board_env() {
    local profile="$1"
    local env_output

    if ! env_output="$("${PROJECT_ROOT}/tools/fw.py" boards env "${profile}")"; then
        die "board metadata not found for '${profile}' under boards/<vendor>/<board>/metadata.yml"
    fi

    eval "${env_output}"
}

PORT=""
BOARD_PROFILE="esp32_oled"
BUILD_PROFILE="debug"
BOOT_MODE="no-mcuboot"
IMAGE_FILE=""
ERASE=0
MONITOR=0
CLI_FLASH_BAUD=""
PYTHON_BIN=""

if [[ $# -gt 0 && "${1}" != --* && "${1}" != -* ]]; then
    PORT="$1"
    shift
    if [[ $# -gt 0 && "${1}" != --* && "${1}" != -* ]]; then
        BOARD_PROFILE="$1"
        shift
    fi
    if [[ $# -gt 0 && "${1}" != --* && "${1}" != -* ]]; then
        BUILD_PROFILE="$1"
        shift
    fi
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --port)
            PORT="${2:-}"
            shift 2
            ;;
        --board)
            BOARD_PROFILE="${2:-}"
            shift 2
            ;;
        --profile)
            BUILD_PROFILE="${2:-}"
            shift 2
            ;;
        --boot)
            BOOT_MODE="${2:-}"
            shift 2
            ;;
        --image)
            IMAGE_FILE="${2:-}"
            shift 2
            ;;
        --erase)
            ERASE=1
            shift
            ;;
        --monitor)
            MONITOR=1
            shift
            ;;
        --baud)
            CLI_FLASH_BAUD="${2:-}"
            shift 2
            ;;
        --python)
            PYTHON_BIN="${2:-}"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            die "unknown argument: $1"
            ;;
    esac
done

[[ -n "${PORT}" ]] || {
    usage
    exit 1
}

load_board_env "${BOARD_PROFILE}"

[[ "${BOARD_FLASH_RUNNER}" == "esp32_esptool" ]] || die "board profile ${BOARD_PROFILE} does not use esp32_esptool flashing"

FLASH_CHIP="${BOARD_FLASH_CHIP:-esp32}"
FLASH_OFFSET="${BOARD_FLASH_OFFSET:-0x1000}"
FLASH_BAUD="${CLI_FLASH_BAUD:-${BOARD_FLASH_BAUD:-460800}}"
FLASH_MODE="${BOARD_FLASH_MODE:-dio}"
FLASH_FREQ="${BOARD_FLASH_FREQ:-40m}"
FLASH_SIZE="${BOARD_FLASH_SIZE:-detect}"
SERIAL_BAUD="${BOARD_SERIAL_BAUD:-115200}"

if [[ -z "${IMAGE_FILE}" ]]; then
    IMAGE_FILE="${PROJECT_ROOT}/build/${BOARD_PROFILE}/${BUILD_PROFILE}/${BOOT_MODE}/zephyr/zephyr.bin"
fi

[[ -f "${IMAGE_FILE}" ]] || die "firmware image not found: ${IMAGE_FILE}"

if [[ -z "${PYTHON_BIN}" ]]; then
    if [[ -x "${HOME}/.venvs/esptool/bin/python" ]]; then
        PYTHON_BIN="${HOME}/.venvs/esptool/bin/python"
    else
        PYTHON_BIN="python3"
    fi
fi

echo "ESP32 flash"
echo "  Port       : ${PORT}"
echo "  Board      : ${BOARD_PROFILE}"
echo "  Profile    : ${BUILD_PROFILE}"
echo "  Boot       : ${BOOT_MODE}"
echo "  Image      : ${IMAGE_FILE}"
echo "  Python     : ${PYTHON_BIN}"
echo "  Chip       : ${FLASH_CHIP}"
echo "  Offset     : ${FLASH_OFFSET}"
echo

if [[ "${ERASE}" -eq 1 ]]; then
    "${PYTHON_BIN}" -m esptool \
        --chip "${FLASH_CHIP}" \
        --port "${PORT}" \
        --baud "${FLASH_BAUD}" \
        erase-flash
fi

"${PYTHON_BIN}" -m esptool \
    --chip "${FLASH_CHIP}" \
    --port "${PORT}" \
    --baud "${FLASH_BAUD}" \
    write-flash \
    --flash-mode "${FLASH_MODE}" \
    --flash-freq "${FLASH_FREQ}" \
    --flash-size "${FLASH_SIZE}" \
    "${FLASH_OFFSET}" "${IMAGE_FILE}"

if [[ "${MONITOR}" -eq 1 ]]; then
    if ! command -v picocom >/dev/null 2>&1; then
        die "picocom not found"
    fi
    exec picocom -b "${SERIAL_BAUD}" "${PORT}"
fi
