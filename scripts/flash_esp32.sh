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

read_yaml_value() {
    local file_path="$1"
    local key="$2"

    awk -v wanted_key="${key}" '
        BEGIN { FS = ":" }
        $1 == wanted_key {
            value = substr($0, index($0, ":") + 1)
            gsub(/^[ \t]+|[ \t]+$/, "", value)
            gsub(/^"|"$/, "", value)
            print value
            exit
        }
    ' "${file_path}"
}

find_board_metadata() {
    local profile="$1"
    local metadata_path
    local metadata_profile

    while IFS= read -r metadata_path; do
        metadata_profile="$(read_yaml_value "${metadata_path}" profile)"
        if [[ -z "${metadata_profile}" ]]; then
            metadata_profile="$(basename "$(dirname "${metadata_path}")")"
        fi

        if [[ "${metadata_profile}" == "${profile}" ]]; then
            printf '%s\n' "${metadata_path}"
            return 0
        fi
    done < <(find "${PROJECT_ROOT}/boards" -mindepth 3 -maxdepth 3 -type f -name metadata.yml | sort)

    return 1
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

BOARD_METADATA="$(find_board_metadata "${BOARD_PROFILE}")" \
    || die "board metadata not found for '${BOARD_PROFILE}' under boards/<vendor>/<board>/metadata.yml"
BOARD_DIR="$(dirname "${BOARD_METADATA}")"

FLASH_RUNNER="$(read_yaml_value "${BOARD_METADATA}" flash_runner)"
BOARD_FLASH_CHIP="$(read_yaml_value "${BOARD_METADATA}" flash_chip)"
BOARD_FLASH_OFFSET="$(read_yaml_value "${BOARD_METADATA}" flash_offset)"
SERIAL_BAUD="$(read_yaml_value "${BOARD_METADATA}" serial_baud)"

[[ "${FLASH_RUNNER}" == "esp32_esptool" ]] || die "board profile ${BOARD_PROFILE} does not use esp32_esptool flashing"

FLASH_CHIP="${BOARD_FLASH_CHIP:-}"
FLASH_OFFSET="${BOARD_FLASH_OFFSET:-}"
FLASH_CONF="${BOARD_DIR}/flash.conf"
if [[ -f "${FLASH_CONF}" ]]; then
    # shellcheck disable=SC1090
    source "${FLASH_CONF}"
fi

FLASH_CHIP="${FLASH_CHIP:-esp32}"
FLASH_OFFSET="${FLASH_OFFSET:-0x1000}"
FLASH_BAUD="${CLI_FLASH_BAUD:-${FLASH_BAUD:-460800}}"
FLASH_MODE="${FLASH_MODE:-dio}"
FLASH_FREQ="${FLASH_FREQ:-40m}"
FLASH_SIZE="${FLASH_SIZE:-detect}"
SERIAL_BAUD="${SERIAL_BAUD:-115200}"

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
