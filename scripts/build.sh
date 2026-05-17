#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

BOARD_PROFILE="${1:-}"
BUILD_PROFILE="${2:-debug}"
BUILD_MODE="${3:-auto}"

if [[ -z "${BOARD_PROFILE}" ]]; then
    echo "Usage: $0 <board_profile> <debug|release> [auto|clean|incremental]"
    echo
    "${PROJECT_ROOT}/scripts/list_boards.sh"
    exit 1
fi

case "${BOARD_PROFILE}" in
    esp32_oled)
        ZEPHYR_BOARD="esp32_devkitc/esp32/procpu"
        BOARD_DIR="${PROJECT_ROOT}/boards/esp32_oled"
        ;;
    stm32_template)
        echo "stm32_template is a placeholder. Add a real Zephyr board target before building."
        exit 1
        ;;
    *)
        echo "Unknown board profile: ${BOARD_PROFILE}"
        echo
        "${PROJECT_ROOT}/scripts/list_boards.sh"
        exit 1
        ;;
esac

case "${BUILD_PROFILE}" in
    debug)
        PROFILE_CONF="${PROJECT_ROOT}/configs/debug.conf"
        BOARD_PROFILE_CONF="${BOARD_DIR}/debug.conf"
        ;;
    release)
        PROFILE_CONF="${PROJECT_ROOT}/configs/release.conf"
        BOARD_PROFILE_CONF="${BOARD_DIR}/release.conf"
        ;;
    *)
        echo "Unknown build profile: ${BUILD_PROFILE}"
        echo "Valid profiles: debug, release"
        exit 1
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
        PRISTINE_ARGS=()
        ;;
    *)
        echo "Unknown build mode: ${BUILD_MODE}"
        echo "Valid build modes: auto, clean, pristine, incremental, no"
        exit 1
        ;;
esac

COMMON_CONF="${PROJECT_ROOT}/prj.conf"
SHELL_CONF="${PROJECT_ROOT}/configs/shell.conf"
LOGGING_CONF="${PROJECT_ROOT}/configs/logging.conf"
NO_DISPLAY_CONF="${PROJECT_ROOT}/configs/no_display.conf"
BOARD_CONF="${BOARD_DIR}/board.conf"
BOARD_OVERLAY="${BOARD_DIR}/board.overlay"

BUILD_DIR="${PROJECT_ROOT}/build/${BOARD_PROFILE}/${BUILD_PROFILE}"

CONF_FILES="${COMMON_CONF};${SHELL_CONF};${LOGGING_CONF};${NO_DISPLAY_CONF};${BOARD_CONF};${PROFILE_CONF};${BOARD_PROFILE_CONF}"

echo "Project root   : ${PROJECT_ROOT}"
echo "Board profile  : ${BOARD_PROFILE}"
echo "Zephyr board   : ${ZEPHYR_BOARD}"
echo "Build profile  : ${BUILD_PROFILE}"
echo "Build mode     : ${BUILD_MODE}"
echo "Build dir      : ${BUILD_DIR}"
echo "Conf files     : ${CONF_FILES}"
echo "Overlay        : ${BOARD_OVERLAY}"
echo

west build "${PRISTINE_ARGS[@]}" \
    -b "${ZEPHYR_BOARD}" \
    -d "${BUILD_DIR}" \
    "${PROJECT_ROOT}" \
    -- \
    -DCONF_FILE="${CONF_FILES}" \
    -DDTC_OVERLAY_FILE="${BOARD_OVERLAY}" \
    -DAPP_BUILD_PROFILE="${BUILD_PROFILE}"
