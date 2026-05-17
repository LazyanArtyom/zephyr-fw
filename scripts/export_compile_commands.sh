#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
    cat <<EOF
Usage:
  $0 --board <profile> [--profile debug|release|production] [--boot no-mcuboot|mcuboot]

Options:
  --board <profile>       Board profile (default: esp32_oled)
  --profile <profile>     Build profile (default: debug)
  --boot <mode>           Boot mode (default: no-mcuboot)
  --build-dir <dir>       Explicit build directory
  --output <file>         Output compile database (default: ./compile_commands.json)
  --container-root <dir>  Container projects root (default: /home/artyom/Documents/projects)
  --host-root <dir>       Host projects root (default: parent of this repo)
EOF
}

die() {
    echo "Error: $*" >&2
    exit 1
}

BOARD_PROFILE="esp32_oled"
BUILD_PROFILE="debug"
BOOT_MODE="no-mcuboot"
BUILD_DIR=""
OUTPUT_FILE="${PROJECT_ROOT}/compile_commands.json"
CONTAINER_ROOT="${CONTAINER_PROJECTS_ROOT:-/home/artyom/Documents/projects}"
HOST_ROOT="$(cd "${PROJECT_ROOT}/.." && pwd)"

while [[ $# -gt 0 ]]; do
    case "$1" in
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
        --build-dir)
            BUILD_DIR="${2:-}"
            shift 2
            ;;
        --output)
            OUTPUT_FILE="${2:-}"
            shift 2
            ;;
        --container-root)
            CONTAINER_ROOT="${2:-}"
            shift 2
            ;;
        --host-root)
            HOST_ROOT="${2:-}"
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

if [[ -z "${BUILD_DIR}" ]]; then
    BUILD_DIR="${PROJECT_ROOT}/build/${BOARD_PROFILE}/${BUILD_PROFILE}/${BOOT_MODE}"
fi

COMPILE_COMMANDS="${BUILD_DIR}/compile_commands.json"
[[ -f "${COMPILE_COMMANDS}" ]] || die "compile database not found: ${COMPILE_COMMANDS}"

container_root_escaped="$(printf '%s' "${CONTAINER_ROOT}" | sed 's/[.[\*^$()+?{|]/\\&/g')"
host_root_sed="$(printf '%s' "${HOST_ROOT}" | sed 's/[&/\]/\\&/g')"

sed "s#${container_root_escaped}#${host_root_sed}#g" "${COMPILE_COMMANDS}" > "${OUTPUT_FILE}"

echo "Exported compile database:"
echo "  ${OUTPUT_FILE}"
