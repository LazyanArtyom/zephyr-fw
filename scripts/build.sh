#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# shellcheck disable=SC1091
source "${PROJECT_ROOT}/project.env"

usage() {
    cat <<EOF
Usage:
  $0 --board <profile> [options]
  $0 <profile> [debug|release|production] [auto|clean|incremental]

Options:
  --board, -b <profile>       Board profile from boards/<profile>/board.yml
  --profile, -p <profile>     Build profile: debug, release, production (default: debug)
  --mode, -m <mode>           Build mode: auto, clean, pristine, incremental, no (default: auto)
  --boot <mode>               Boot mode: no-mcuboot, mcuboot (default: board.yml)
  --app <profile>             Application profile label (default: main)
  --display <mode>            Display feature: auto, on, off (default: auto)
  --shell <mode>              Shell feature: auto, on, off (default: auto)
  --asserts <mode>            Global Zephyr asserts: auto, on, off (default: auto)
  --package                   Package artifacts after a successful build
  --list-boards               Show available board profiles
  --help                      Show this help

Examples:
  $0 --board esp32_oled --profile debug --boot no-mcuboot
  $0 --board esp32_oled --profile production --boot mcuboot --display off --shell off --package
  $0 esp32_oled debug incremental
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

join_by_semicolon() {
    local IFS=";"
    echo "$*"
}

add_conf_if_exists() {
    local file_path="$1"

    if [[ -f "${file_path}" ]]; then
        CONF_FILES+=("${file_path}")
    fi
}

escape_kconfig_string() {
    printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

BOARD_PROFILE=""
BUILD_PROFILE="debug"
BUILD_MODE="auto"
BOOT_MODE=""
APP_PROFILE="main"
DISPLAY_MODE="auto"
SHELL_MODE="auto"
ASSERTS_MODE="auto"
DO_PACKAGE=0

if [[ $# -gt 0 && "${1}" != --* && "${1}" != -* ]]; then
    BOARD_PROFILE="$1"
    shift
    if [[ $# -gt 0 && "${1}" != --* && "${1}" != -* ]]; then
        BUILD_PROFILE="$1"
        shift
    fi
    if [[ $# -gt 0 && "${1}" != --* && "${1}" != -* ]]; then
        BUILD_MODE="$1"
        shift
    fi
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --board|-b)
            BOARD_PROFILE="${2:-}"
            shift 2
            ;;
        --profile|-p)
            BUILD_PROFILE="${2:-}"
            shift 2
            ;;
        --mode|-m)
            BUILD_MODE="${2:-}"
            shift 2
            ;;
        --boot)
            BOOT_MODE="${2:-}"
            shift 2
            ;;
        --app)
            APP_PROFILE="${2:-}"
            shift 2
            ;;
        --display)
            DISPLAY_MODE="${2:-}"
            shift 2
            ;;
        --shell)
            SHELL_MODE="${2:-}"
            shift 2
            ;;
        --asserts)
            ASSERTS_MODE="${2:-}"
            shift 2
            ;;
        --package)
            DO_PACKAGE=1
            shift
            ;;
        --list-boards)
            "${PROJECT_ROOT}/scripts/list_boards.sh"
            exit 0
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

if [[ -z "${BOARD_PROFILE}" ]]; then
    usage
    echo
    "${PROJECT_ROOT}/scripts/list_boards.sh"
    exit 1
fi

BOARD_DIR="${PROJECT_ROOT}/boards/${BOARD_PROFILE}"
BOARD_YML="${BOARD_DIR}/board.yml"
[[ -f "${BOARD_YML}" ]] || die "board profile metadata not found: ${BOARD_YML}"

BOARD_STATUS="$(read_yaml_value "${BOARD_YML}" status)"
if [[ "${BOARD_STATUS}" != "enabled" ]]; then
    die "${BOARD_PROFILE} is not enabled yet (status: ${BOARD_STATUS:-unknown})"
fi

ZEPHYR_BOARD="$(read_yaml_value "${BOARD_YML}" zephyr_board)"
[[ -n "${ZEPHYR_BOARD}" ]] || die "zephyr_board is missing in ${BOARD_YML}"

DEFAULT_DISPLAY="$(read_yaml_value "${BOARD_YML}" default_display)"
DEFAULT_BOOT="$(read_yaml_value "${BOARD_YML}" default_boot)"
BOOT_MODE="${BOOT_MODE:-${DEFAULT_BOOT:-no-mcuboot}}"

case "${BUILD_PROFILE}" in
    debug|release|production)
        PROFILE_CONF="${PROJECT_ROOT}/configs/profiles/${BUILD_PROFILE}.conf"
        [[ -f "${PROFILE_CONF}" ]] || die "profile config not found: ${PROFILE_CONF}"
        ;;
    *)
        die "unknown build profile: ${BUILD_PROFILE} (valid: debug, release, production)"
        ;;
esac

case "${BUILD_MODE}" in
    auto)
        PRISTINE_ARGS=(-p auto)
        ;;
    clean|pristine)
        PRISTINE_ARGS=(-p always)
        ;;
    incremental|no)
        PRISTINE_ARGS=(-p never)
        ;;
    *)
        die "unknown build mode: ${BUILD_MODE} (valid: auto, clean, pristine, incremental, no)"
        ;;
esac

case "${BOOT_MODE}" in
    no-mcuboot|mcuboot)
        BOOT_CONF="${PROJECT_ROOT}/configs/boot/${BOOT_MODE}.conf"
        [[ -f "${BOOT_CONF}" ]] || die "boot config not found: ${BOOT_CONF}"
        ;;
    *)
        die "unknown boot mode: ${BOOT_MODE} (valid: no-mcuboot, mcuboot)"
        ;;
esac

case "${DISPLAY_MODE}" in
    auto)
        RESOLVED_DISPLAY_MODE="${DEFAULT_DISPLAY:-off}"
        ;;
    on|off)
        RESOLVED_DISPLAY_MODE="${DISPLAY_MODE}"
        ;;
    *)
        die "unknown display mode: ${DISPLAY_MODE} (valid: auto, on, off)"
        ;;
esac

case "${RESOLVED_DISPLAY_MODE}" in
    on)
        DISPLAY_CONF="${PROJECT_ROOT}/configs/features/display.conf"
        ;;
    off)
        DISPLAY_CONF="${PROJECT_ROOT}/configs/features/no_display.conf"
        ;;
    *)
        die "board default_display must be on or off, got: ${RESOLVED_DISPLAY_MODE}"
        ;;
esac

case "${SHELL_MODE}" in
    auto)
        if [[ "${BUILD_PROFILE}" == "production" ]]; then
            RESOLVED_SHELL_MODE="off"
        else
            RESOLVED_SHELL_MODE="on"
        fi
        ;;
    on|off)
        RESOLVED_SHELL_MODE="${SHELL_MODE}"
        ;;
    *)
        die "unknown shell mode: ${SHELL_MODE} (valid: auto, on, off)"
        ;;
esac

case "${ASSERTS_MODE}" in
    auto)
        RESOLVED_ASSERTS_MODE="off"
        ;;
    on|off)
        RESOLVED_ASSERTS_MODE="${ASSERTS_MODE}"
        ;;
    *)
        die "unknown asserts mode: ${ASSERTS_MODE} (valid: auto, on, off)"
        ;;
esac

if [[ "${APP_PROFILE}" != "main" ]]; then
    APP_CONF="${PROJECT_ROOT}/configs/apps/${APP_PROFILE}.conf"
    [[ -f "${APP_CONF}" ]] || die "app profile config not found: ${APP_CONF}"
fi

BUILD_DIR="${PROJECT_ROOT}/build/${BOARD_PROFILE}/${BUILD_PROFILE}/${BOOT_MODE}"
GENERATED_CONF_DIR="${PROJECT_ROOT}/build/generated-configs/${BOARD_PROFILE}/${BUILD_PROFILE}/${BOOT_MODE}"
mkdir -p "${GENERATED_CONF_DIR}"

GENERATED_SHELL_PROMPT_CONF="${GENERATED_CONF_DIR}/shell_prompt.conf"
printf 'CONFIG_SHELL_PROMPT_UART="%s"\n' "$(escape_kconfig_string "${APP_SHELL_PROMPT}")" \
    > "${GENERATED_SHELL_PROMPT_CONF}"

COMMON_CONF="${PROJECT_ROOT}/prj.conf"
LOGGING_CONF="${PROJECT_ROOT}/configs/features/logging.conf"
BOARD_CONF="${BOARD_DIR}/board.conf"
BOARD_PROFILE_CONF="${BOARD_DIR}/${BUILD_PROFILE}.conf"
BOARD_OVERLAY="${BOARD_DIR}/board.overlay"

CONF_FILES=()
add_conf_if_exists "${COMMON_CONF}"
add_conf_if_exists "${LOGGING_CONF}"
add_conf_if_exists "${BOARD_CONF}"
add_conf_if_exists "${PROFILE_CONF}"
add_conf_if_exists "${BOARD_PROFILE_CONF}"
add_conf_if_exists "${BOOT_CONF}"
add_conf_if_exists "${DISPLAY_CONF}"
if [[ "${APP_PROFILE}" != "main" ]]; then
    add_conf_if_exists "${APP_CONF}"
fi
if [[ "${RESOLVED_SHELL_MODE}" == "on" ]]; then
    add_conf_if_exists "${PROJECT_ROOT}/configs/features/shell.conf"
    add_conf_if_exists "${GENERATED_SHELL_PROMPT_CONF}"
else
    add_conf_if_exists "${PROJECT_ROOT}/configs/features/no_shell.conf"
fi
if [[ "${RESOLVED_ASSERTS_MODE}" == "on" ]]; then
    add_conf_if_exists "${PROJECT_ROOT}/configs/features/asserts.conf"
else
    add_conf_if_exists "${PROJECT_ROOT}/configs/features/no_asserts.conf"
fi

CONF_FILE_ARG="$(join_by_semicolon "${CONF_FILES[@]}")"

WEST_ARGS=(build)
if [[ "${BOOT_MODE}" == "mcuboot" ]]; then
    WEST_ARGS+=(--sysbuild)
fi

echo "Project root   : ${PROJECT_ROOT}"
echo "Firmware       : ${APP_FIRMWARE_NAME}"
echo "Board profile  : ${BOARD_PROFILE}"
echo "Zephyr board   : ${ZEPHYR_BOARD}"
echo "Build profile  : ${BUILD_PROFILE}"
echo "Build mode     : ${BUILD_MODE}"
echo "Boot mode      : ${BOOT_MODE}"
echo "App profile    : ${APP_PROFILE}"
echo "Display        : ${RESOLVED_DISPLAY_MODE}"
echo "Shell          : ${RESOLVED_SHELL_MODE}"
echo "Asserts        : ${RESOLVED_ASSERTS_MODE}"
echo "Build dir      : ${BUILD_DIR}"
echo "Conf files     : ${CONF_FILE_ARG}"
echo "Overlay        : ${BOARD_OVERLAY}"
echo

command -v west >/dev/null 2>&1 || die "west not found. Run this inside the Zephyr Docker/west environment."

west "${WEST_ARGS[@]}" "${PRISTINE_ARGS[@]}" \
    -b "${ZEPHYR_BOARD}" \
    -d "${BUILD_DIR}" \
    "${PROJECT_ROOT}" \
    -- \
    -DCONF_FILE="${CONF_FILE_ARG}" \
    -DDTC_OVERLAY_FILE="${BOARD_OVERLAY}" \
    -DAPP_BUILD_PROFILE="${BUILD_PROFILE}" \
    -DAPP_BOARD_PROFILE="${BOARD_PROFILE}" \
    -DAPP_BOOT_MODE="${BOOT_MODE}" \
    -DAPP_APP_PROFILE="${APP_PROFILE}" \
    -DAPP_DISPLAY_MODE="${RESOLVED_DISPLAY_MODE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo
echo "Build complete:"
echo "  ${BUILD_DIR}/zephyr/zephyr.bin"
echo "  ${BUILD_DIR}/zephyr/zephyr.elf"

if [[ "${DO_PACKAGE}" -eq 1 ]]; then
    "${PROJECT_ROOT}/scripts/package.sh" \
        --board "${BOARD_PROFILE}" \
        --profile "${BUILD_PROFILE}" \
        --boot "${BOOT_MODE}" \
        --build-dir "${BUILD_DIR}"
fi
