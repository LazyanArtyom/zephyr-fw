#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
    cat <<EOF
Usage:
  $0 --port <serial_port> [options]
  $0 <serial_port> [options]
  $0 --list-ports

Package flashing:
  $0 --port /dev/cu.usbserial-11120
  $0 --package path/to/package --port /dev/ttyUSB0

Raw image flashing:
  $0 --image build/esp32_oled/debug/no-mcuboot/zephyr/zephyr.bin --port /dev/ttyUSB0 --chip esp32 --offset 0x1000

Options:
  --port <serial_port>     Host serial device to program, for example:
                             macOS: /dev/cu.usbserial-11120
                             Linux: /dev/ttyUSB0 or /dev/serial/by-id/...
  --package <dir>          Firmware package directory (default: current dir or script dir)
  --image <file>           Explicit image path; overrides package image selection
  --chip <chip>            Flash chip for raw-image mode, for example esp32
  --offset <offset>        Flash offset for raw-image mode, for example 0x1000
  --baud <baud>            Flash baud override
  --flash-mode <mode>      Flash mode override
  --flash-freq <freq>      Flash frequency override
  --flash-size <size>      Flash size override
  --serial-baud <baud>     Monitor baud override
  --python <python>        Python executable that has esptool installed
  --erase                  Erase flash before writing
  --monitor                Open a serial monitor after flashing
  --list-ports             List likely serial ports visible on this host

Notes:
  Run this script from the host OS that owns the USB serial device. Docker on
  macOS usually will not show /dev/cu.* devices without USB passthrough.
  On macOS, prefer /dev/cu.* over /dev/tty.* for outbound flashing.
EOF
}

die() {
    echo "Error: $*" >&2
    exit 1
}

list_serial_ports() {
    shopt -s nullglob
    local ports=(
        /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial* /dev/cu.usbmodem*
        /dev/ttyUSB* /dev/ttyACM* /dev/serial/by-id/*
    )
    shopt -u nullglob

    if [[ ${#ports[@]} -eq 0 ]]; then
        echo "No likely serial ports found on this host."
        echo "macOS examples: /dev/cu.usbserial-* /dev/cu.usbmodem*"
        echo "Linux examples: /dev/ttyUSB* /dev/ttyACM* /dev/serial/by-id/*"
        return 1
    fi

    printf '%s\n' "${ports[@]}"
}

resolve_python() {
    if [[ -n "${PYTHON_BIN}" ]]; then
        return
    fi
    if [[ -n "${ESPTOOL_PYTHON:-}" ]]; then
        PYTHON_BIN="${ESPTOOL_PYTHON}"
    elif [[ -x "${HOME}/.venvs/esptool/bin/python" ]]; then
        PYTHON_BIN="${HOME}/.venvs/esptool/bin/python"
    else
        PYTHON_BIN="python3"
    fi
}

require_esptool() {
    if ! "${PYTHON_BIN}" -m esptool version >/dev/null 2>&1; then
        die "esptool is not available for ${PYTHON_BIN}. Install it on the host with: ${PYTHON_BIN} -m pip install esptool"
    fi
}

manifest_value() {
    local manifest_file="$1"
    local dotted_key="$2"
    "${PYTHON_BIN}" - "${manifest_file}" "${dotted_key}" <<'MANIFEST_PY'
import json
import sys

manifest_path, dotted_key = sys.argv[1], sys.argv[2]
with open(manifest_path, encoding="utf-8") as handle:
    value = json.load(handle)
for part in dotted_key.split('.'):
    if not isinstance(value, dict) or part not in value:
        sys.exit(2)
    value = value[part]
if value is None:
    sys.exit(2)
print(value)
MANIFEST_PY
}

maybe_manifest_value() {
    local manifest_file="$1"
    local dotted_key="$2"
    manifest_value "${manifest_file}" "${dotted_key}" 2>/dev/null || true
}

manifest_flash_segments() {
    local manifest_file="$1"
    "${PYTHON_BIN}" - "${manifest_file}" <<'SEGMENTS_PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as handle:
    manifest = json.load(handle)
segments = manifest.get("flash", {}).get("segments")
if not isinstance(segments, list) or not segments:
    sys.exit(2)
for segment in segments:
    offset = segment.get("offset")
    image = segment.get("image")
    if not offset or not image:
        sys.exit(2)
    print(f"{offset}	{image}")
SEGMENTS_PY
}

open_monitor() {
    if command -v picocom >/dev/null 2>&1; then
        exec picocom -b "${SERIAL_BAUD}" "${PORT}"
    fi
    if command -v screen >/dev/null 2>&1; then
        exec screen "${PORT}" "${SERIAL_BAUD}"
    fi
    die "no serial monitor found; install picocom or use: screen ${PORT} ${SERIAL_BAUD}"
}

PORT=""
PACKAGE_DIR=""
IMAGE_FILE=""
FLASH_CHIP=""
FLASH_OFFSET=""
FLASH_BAUD=""
FLASH_MODE=""
FLASH_FREQ=""
FLASH_SIZE=""
SERIAL_BAUD=""
PYTHON_BIN=""
FLASH_SEGMENTS=()
ERASE=0
MONITOR=0
LIST_PORTS=0

if [[ $# -gt 0 && "${1}" != --* && "${1}" != -* ]]; then
    PORT="$1"
    shift
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --port)
            PORT="${2:-}"
            shift 2
            ;;
        --package)
            PACKAGE_DIR="${2:-}"
            shift 2
            ;;
        --image)
            IMAGE_FILE="${2:-}"
            shift 2
            ;;
        --chip)
            FLASH_CHIP="${2:-}"
            shift 2
            ;;
        --offset)
            FLASH_OFFSET="${2:-}"
            shift 2
            ;;
        --baud)
            FLASH_BAUD="${2:-}"
            shift 2
            ;;
        --flash-mode)
            FLASH_MODE="${2:-}"
            shift 2
            ;;
        --flash-freq)
            FLASH_FREQ="${2:-}"
            shift 2
            ;;
        --flash-size)
            FLASH_SIZE="${2:-}"
            shift 2
            ;;
        --serial-baud)
            SERIAL_BAUD="${2:-}"
            shift 2
            ;;
        --python)
            PYTHON_BIN="${2:-}"
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
        --list-ports)
            LIST_PORTS=1
            shift
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

if [[ "${LIST_PORTS}" -eq 1 ]]; then
    list_serial_ports
    exit $?
fi

[[ -n "${PORT}" ]] || {
    usage
    exit 1
}
[[ -e "${PORT}" ]] || die "serial port not found on this host: ${PORT}. Run: $0 --list-ports"

resolve_python

if [[ -z "${PACKAGE_DIR}" ]]; then
    if [[ -f "${PWD}/manifest.json" ]]; then
        PACKAGE_DIR="${PWD}"
    elif [[ -f "${SCRIPT_DIR}/manifest.json" ]]; then
        PACKAGE_DIR="${SCRIPT_DIR}"
    fi
fi

MANIFEST_FILE=""
if [[ -n "${PACKAGE_DIR}" ]]; then
    PACKAGE_DIR="$(cd "${PACKAGE_DIR}" && pwd)"
    MANIFEST_FILE="${PACKAGE_DIR}/manifest.json"
    [[ -f "${MANIFEST_FILE}" ]] || die "package manifest not found: ${MANIFEST_FILE}"

    FLASH_RUNNER="$(maybe_manifest_value "${MANIFEST_FILE}" flash.runner)"
    [[ "${FLASH_RUNNER}" == "esp32_esptool" ]] || die "unsupported package flash runner: ${FLASH_RUNNER:-missing}"

    FLASH_CHIP="${FLASH_CHIP:-$(manifest_value "${MANIFEST_FILE}" flash.chip)}"
    FLASH_OFFSET="${FLASH_OFFSET:-$(manifest_value "${MANIFEST_FILE}" flash.offset)}"
    FLASH_BAUD="${FLASH_BAUD:-$(manifest_value "${MANIFEST_FILE}" flash.baud)}"
    FLASH_MODE="${FLASH_MODE:-$(manifest_value "${MANIFEST_FILE}" flash.mode)}"
    FLASH_FREQ="${FLASH_FREQ:-$(manifest_value "${MANIFEST_FILE}" flash.freq)}"
    FLASH_SIZE="${FLASH_SIZE:-$(manifest_value "${MANIFEST_FILE}" flash.size)}"
    SERIAL_BAUD="${SERIAL_BAUD:-$(maybe_manifest_value "${MANIFEST_FILE}" flash.serial_baud)}"

    if [[ -z "${IMAGE_FILE}" ]]; then
        while IFS=$'	' read -r segment_offset segment_image; do
            [[ -n "${segment_offset}" && -n "${segment_image}" ]] || continue
            FLASH_SEGMENTS+=("${segment_offset}" "${PACKAGE_DIR}/${segment_image}")
        done < <(manifest_flash_segments "${MANIFEST_FILE}" || true)

        if [[ ${#FLASH_SEGMENTS[@]} -eq 0 ]]; then
            if [[ -f "${PACKAGE_DIR}/zephyr.signed.bin" ]]; then
                IMAGE_FILE="${PACKAGE_DIR}/zephyr.signed.bin"
            else
                IMAGE_FILE="${PACKAGE_DIR}/zephyr.bin"
            fi
        fi
    fi
fi

FLASH_BAUD="${FLASH_BAUD:-${ESPTOOL_BAUD:-460800}}"
FLASH_MODE="${FLASH_MODE:-dio}"
FLASH_FREQ="${FLASH_FREQ:-40m}"
FLASH_SIZE="${FLASH_SIZE:-detect}"
SERIAL_BAUD="${SERIAL_BAUD:-115200}"

if [[ ${#FLASH_SEGMENTS[@]} -eq 0 ]]; then
    [[ -n "${IMAGE_FILE}" ]] || die "no image selected; run from a package directory, pass --package, or pass --image"
    [[ -f "${IMAGE_FILE}" ]] || die "firmware image not found: ${IMAGE_FILE}"
    [[ -n "${FLASH_OFFSET}" ]] || die "flash offset is required outside package mode; pass --offset"
    FLASH_SEGMENTS=("${FLASH_OFFSET}" "${IMAGE_FILE}")
else
    for ((index = 1; index < ${#FLASH_SEGMENTS[@]}; index += 2)); do
        [[ -f "${FLASH_SEGMENTS[index]}" ]] || die "firmware image not found: ${FLASH_SEGMENTS[index]}"
    done
fi
[[ -n "${FLASH_CHIP}" ]] || die "flash chip is required outside package mode; pass --chip"

require_esptool

echo "Firmware flash"
echo "  Port       : ${PORT}"
if [[ -n "${PACKAGE_DIR}" ]]; then
    echo "  Package    : ${PACKAGE_DIR}"
fi
echo "  Python     : ${PYTHON_BIN}"
echo "  Chip       : ${FLASH_CHIP}"
echo "  Baud       : ${FLASH_BAUD}"
if [[ ${#FLASH_SEGMENTS[@]} -eq 2 ]]; then
    echo "  Image      : ${FLASH_SEGMENTS[1]}"
    echo "  Offset     : ${FLASH_SEGMENTS[0]}"
else
    echo "  Segments   :"
    for ((index = 0; index < ${#FLASH_SEGMENTS[@]}; index += 2)); do
        echo "    ${FLASH_SEGMENTS[index]} ${FLASH_SEGMENTS[index + 1]}"
    done
fi
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
    "${FLASH_SEGMENTS[@]}"

if [[ "${MONITOR}" -eq 1 ]]; then
    open_monitor
fi
