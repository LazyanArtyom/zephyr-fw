#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# shellcheck disable=SC1091
source "${PROJECT_ROOT}/project.env"

usage() {
    cat <<EOF
Usage:
  $0 --board <profile> [options]
  $0 <profile> [debug|release|production] [auto|clean|pristine|incremental|no]

Options:
  --board, -b <profile>       Board profile from boards/<vendor>/<board>/metadata.yml
  --profile, -p <profile>     Build profile: debug, release, production (default: debug)
  --mode, -m <mode>           Build mode: auto, clean, pristine, incremental, no (default: auto)
  --boot <mode>               Boot mode: no-mcuboot, mcuboot (default: metadata.yml)
  --list-boards               Show available board profiles
  --help                      Show this help

Examples:
  $0 --board esp32_oled --profile debug --boot no-mcuboot
  $0 --board esp32_oled --profile production --boot mcuboot
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

read_cmake_cache_value() {
    local cache_file="$1"
    local key="$2"

    awk -F= -v wanted_key="${key}" '
        $1 ~ "^" wanted_key ":" {
            print substr($0, index($0, "=") + 1)
            exit
        }
    ' "${cache_file}"
}

remove_build_dir() {
    local build_dir="$1"
    local reason="$2"

    case "${build_dir}" in
        "${PROJECT_ROOT}/build/"*)
            ;;
        *)
            die "refusing to remove build directory outside project build tree: ${build_dir}"
            ;;
    esac

    if [[ -d "${build_dir}" ]]; then
        rm -rf -- "${build_dir}"
        BUILD_DIR_RESET_REASON="${reason}"
    fi
}

west_supports_build() {
    "${WEST_EXE}" "$@" build -h >/dev/null 2>&1
}

west_supports_build_in_dir() {
    local workdir="$1"
    shift

    (cd "${workdir}" && "${WEST_EXE}" "$@" build -h >/dev/null 2>&1)
}

resolve_west_executable() {
    if [[ -n "${WEST_BIN:-}" ]]; then
        if [[ -x "${WEST_BIN}" ]]; then
            WEST_EXE="${WEST_BIN}"
            return
        fi
        die "WEST_BIN is not executable: ${WEST_BIN}"
    fi

    if command -v west >/dev/null 2>&1; then
        WEST_EXE="$(command -v west)"
        return
    fi

    die "west not found. Install west in PATH or set WEST_BIN=/path/to/west."
}

resolve_west_command() {
    resolve_west_executable

    if [[ -n "${ZEPHYR_BASE:-}" && -d "${ZEPHYR_BASE}" ]]; then
        if west_supports_build -z "${ZEPHYR_BASE}"; then
            WEST_CMD=("${WEST_EXE}" -z "${ZEPHYR_BASE}")
            return
        fi

        local workspace_dir
        workspace_dir="$(dirname "${ZEPHYR_BASE}")"
        if [[ -d "${workspace_dir}/.west" ]] && west_supports_build_in_dir "${workspace_dir}"; then
            WEST_CMD=("${WEST_EXE}")
            WEST_WORKDIR="${workspace_dir}"
            return
        fi
    fi

    local workspace_dir
    if workspace_dir="$("${WEST_EXE}" topdir 2>/dev/null)"; then
        if [[ -d "${workspace_dir}/zephyr" ]] && west_supports_build_in_dir "${workspace_dir}"; then
            WEST_CMD=("${WEST_EXE}")
            WEST_WORKDIR="${workspace_dir}"
            export ZEPHYR_BASE="${workspace_dir}/zephyr"
            return
        fi
    fi

    die "west build is not available. Set ZEPHYR_BASE to the Zephyr checkout used by this toolchain."
}

BOARD_PROFILE=""
BUILD_PROFILE="debug"
BUILD_MODE="auto"
BOOT_MODE=""
WEST_EXE=""
WEST_CMD=()
WEST_WORKDIR=""

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
            [[ $# -ge 2 && -n "${2:-}" ]] || die "$1 requires a board profile"
            BOARD_PROFILE="${2:-}"
            shift 2
            ;;
        --profile|-p)
            [[ $# -ge 2 && -n "${2:-}" ]] || die "$1 requires a build profile"
            BUILD_PROFILE="${2:-}"
            shift 2
            ;;
        --mode|-m)
            [[ $# -ge 2 && -n "${2:-}" ]] || die "$1 requires a build mode"
            BUILD_MODE="${2:-}"
            shift 2
            ;;
        --boot)
            [[ $# -ge 2 && -n "${2:-}" ]] || die "$1 requires a boot mode"
            BOOT_MODE="${2:-}"
            shift 2
            ;;
        --list-boards)
            "${PROJECT_ROOT}/tools/fw.py" boards list
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
    "${PROJECT_ROOT}/tools/fw.py" boards list
    exit 1
fi

BOARD_METADATA="$(find_board_metadata "${BOARD_PROFILE}")" \
    || die "board profile metadata not found for '${BOARD_PROFILE}' under boards/<vendor>/<board>/metadata.yml"
BOARD_DIR="$(dirname "${BOARD_METADATA}")"
BOARD_ROOT="${PROJECT_ROOT}"
BOARD_ZEPHYR_YML="${BOARD_DIR}/board.yml"
[[ -f "${BOARD_ZEPHYR_YML}" ]] || die "Zephyr board.yml not found: ${BOARD_ZEPHYR_YML}"

BOARD_STATUS="$(read_yaml_value "${BOARD_METADATA}" status)"
if [[ "${BOARD_STATUS}" != "enabled" ]]; then
    die "${BOARD_PROFILE} is not enabled yet (status: ${BOARD_STATUS:-unknown})"
fi

ZEPHYR_BOARD="$(read_yaml_value "${BOARD_METADATA}" zephyr_board)"
[[ -n "${ZEPHYR_BOARD}" ]] || die "zephyr_board is missing in ${BOARD_METADATA}"

BOARD_DISPLAY_NAME="$(read_yaml_value "${BOARD_METADATA}" display_name)"
BOARD_SERIAL_BAUD="$(read_yaml_value "${BOARD_METADATA}" serial_baud)"
BOARD_FLASH_RUNNER="$(read_yaml_value "${BOARD_METADATA}" flash_runner)"
BOARD_FLASH_CHIP="$(read_yaml_value "${BOARD_METADATA}" flash_chip)"
BOARD_FLASH_OFFSET="$(read_yaml_value "${BOARD_METADATA}" flash_offset)"
BOARD_DESCRIPTION="$(read_yaml_value "${BOARD_METADATA}" description)"
DEFAULT_DISPLAY="$(read_yaml_value "${BOARD_METADATA}" default_display)"
DEFAULT_SETTINGS="$(read_yaml_value "${BOARD_METADATA}" default_settings)"
DEFAULT_BOOT="$(read_yaml_value "${BOARD_METADATA}" default_boot)"
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

case "${DEFAULT_DISPLAY:-off}" in
    on)
        DISPLAY_CONF="${PROJECT_ROOT}/configs/features/display.conf"
        DISPLAY_MODE="on"
        ;;
    off)
        DISPLAY_CONF="${PROJECT_ROOT}/configs/features/no_display.conf"
        DISPLAY_MODE="off"
        ;;
    *)
        die "board default_display must be on or off, got: ${DEFAULT_DISPLAY}"
        ;;
esac

case "${DEFAULT_SETTINGS:-off}" in
    on)
        SETTINGS_CONF="${PROJECT_ROOT}/configs/features/settings.conf"
        SETTINGS_MODE="on"
        ;;
    off)
        SETTINGS_CONF="${PROJECT_ROOT}/configs/features/no_settings.conf"
        SETTINGS_MODE="off"
        ;;
    *)
        die "board default_settings must be on or off, got: ${DEFAULT_SETTINGS}"
        ;;
esac

if [[ "${BUILD_PROFILE}" == "production" ]]; then
    SHELL_MODE="off"
else
    SHELL_MODE="on"
fi

ASSERTS_MODE="off"

BUILD_DIR="${PROJECT_ROOT}/build/${BOARD_PROFILE}/${BUILD_PROFILE}/${BOOT_MODE}"
GENERATED_CONF_DIR="${PROJECT_ROOT}/build/generated-configs/${BOARD_PROFILE}/${BUILD_PROFILE}/${BOOT_MODE}"
mkdir -p "${GENERATED_CONF_DIR}"

# Keep tool-generated caches inside the project build tree. Docker images, host
# shells, and CI runners can disagree about HOME ownership; project-local caches
# make ccache and Zephyr's CMake package cache deterministic and writable.
BUILD_CACHE_DIR="${PROJECT_ROOT}/build/.cache"
BUILD_CCACHE_DIR="${PROJECT_ROOT}/build/.ccache"
mkdir -p "${BUILD_CACHE_DIR}" "${BUILD_CCACHE_DIR}/tmp"

export XDG_CACHE_HOME="${XDG_CACHE_HOME:-${BUILD_CACHE_DIR}}"
export ZEPHYR_CACHE_DIR="${ZEPHYR_CACHE_DIR:-${BUILD_CACHE_DIR}/zephyr}"
export CCACHE_DIR="${CCACHE_DIR:-${BUILD_CCACHE_DIR}}"
export CCACHE_TEMPDIR="${CCACHE_TEMPDIR:-${BUILD_CCACHE_DIR}/tmp}"

GENERATED_SHELL_PROMPT_CONF="${GENERATED_CONF_DIR}/shell_prompt.conf"
if [[ "${SHELL_MODE}" == "on" ]]; then
    printf 'CONFIG_SHELL_PROMPT_UART="%s"\n' "$(escape_kconfig_string "${APP_SHELL_PROMPT}")" \
        > "${GENERATED_SHELL_PROMPT_CONF}"
fi

COMMON_CONF="${PROJECT_ROOT}/prj.conf"
LOGGING_CONF="${PROJECT_ROOT}/configs/features/logging.conf"
BOARD_CONF="${BOARD_DIR}/board.conf"
LEGACY_BOARD_CONF="${BOARD_DIR}/app.conf"
BOARD_PROFILE_CONF="${BOARD_DIR}/${BUILD_PROFILE}.conf"
BOARD_OVERLAY="${BOARD_DIR}/board.overlay"
LEGACY_BOARD_OVERLAY="${BOARD_DIR}/app.overlay"

CONF_FILES=()
add_conf_if_exists "${COMMON_CONF}"
add_conf_if_exists "${LOGGING_CONF}"
if [[ -f "${BOARD_CONF}" ]]; then
    add_conf_if_exists "${BOARD_CONF}"
else
    add_conf_if_exists "${LEGACY_BOARD_CONF}"
fi
add_conf_if_exists "${PROFILE_CONF}"
add_conf_if_exists "${BOARD_PROFILE_CONF}"
add_conf_if_exists "${BOOT_CONF}"
add_conf_if_exists "${DISPLAY_CONF}"
add_conf_if_exists "${SETTINGS_CONF}"
if [[ "${SHELL_MODE}" == "on" ]]; then
    add_conf_if_exists "${PROJECT_ROOT}/configs/features/shell.conf"
    add_conf_if_exists "${GENERATED_SHELL_PROMPT_CONF}"
else
    add_conf_if_exists "${PROJECT_ROOT}/configs/features/no_shell.conf"
fi
add_conf_if_exists "${PROJECT_ROOT}/configs/features/no_asserts.conf"

CONF_FILE_ARG="$(join_by_semicolon "${CONF_FILES[@]}")"

WEST_ARGS=(build)
if [[ "${BOOT_MODE}" == "mcuboot" ]]; then
    WEST_ARGS+=(--sysbuild)
fi

resolve_west_command

if [[ -z "${ZEPHYR_BASE:-}" || ! -f "${ZEPHYR_BASE}/cmake/modules/zephyr_default.cmake" ]]; then
    die "invalid ZEPHYR_BASE: ${ZEPHYR_BASE:-unset} (missing cmake/modules/zephyr_default.cmake)"
fi

AUTO_PRISTINE_REASON=""
BUILD_DIR_RESET_REASON=""
if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    CACHED_ZEPHYR_BASE="$(read_cmake_cache_value "${BUILD_DIR}/CMakeCache.txt" "ZEPHYR_BASE")"
    if [[ -n "${CACHED_ZEPHYR_BASE}" && "${CACHED_ZEPHYR_BASE}" != "${ZEPHYR_BASE}" ]]; then
        case "${BUILD_MODE}" in
            auto|clean|pristine)
                remove_build_dir "${BUILD_DIR}" \
                    "cached Zephyr base changed: ${CACHED_ZEPHYR_BASE} -> ${ZEPHYR_BASE}"
                PRISTINE_ARGS=(-p auto)
                if [[ "${BUILD_MODE}" == "auto" ]]; then
                    AUTO_PRISTINE_REASON="${BUILD_DIR_RESET_REASON}"
                fi
                ;;
            incremental|no)
                die "build cache uses ${CACHED_ZEPHYR_BASE}, but active ZEPHYR_BASE is ${ZEPHYR_BASE}. Re-run with --mode auto or --mode pristine."
                ;;
        esac
    fi
fi

echo "Project root   : ${PROJECT_ROOT}"
echo "Firmware       : ${APP_FIRMWARE_NAME}"
echo "Board profile  : ${BOARD_PROFILE}"
echo "Board dir      : ${BOARD_DIR}"
echo "Board metadata : ${BOARD_METADATA}"
echo "Board root     : ${BOARD_ROOT}"
echo "Zephyr board   : ${ZEPHYR_BOARD}"
echo "Build profile  : ${BUILD_PROFILE}"
echo "Build mode     : ${BUILD_MODE}"
if [[ -n "${AUTO_PRISTINE_REASON}" ]]; then
    echo "Auto pristine  : ${AUTO_PRISTINE_REASON}"
elif [[ -n "${BUILD_DIR_RESET_REASON}" ]]; then
    echo "Build reset    : ${BUILD_DIR_RESET_REASON}"
fi
echo "Boot mode      : ${BOOT_MODE}"
echo "Display        : ${DISPLAY_MODE} (from metadata.yml)"
echo "Settings       : ${SETTINGS_MODE} (from metadata.yml)"
echo "Shell          : ${SHELL_MODE} (profile policy)"
echo "Asserts        : ${ASSERTS_MODE} (profile policy)"
echo "Build dir      : ${BUILD_DIR}"
echo "Conf files     : ${CONF_FILE_ARG}"
if [[ ! -f "${BOARD_OVERLAY}" && -f "${LEGACY_BOARD_OVERLAY}" ]]; then
    BOARD_OVERLAY="${LEGACY_BOARD_OVERLAY}"
fi

if [[ -f "${BOARD_OVERLAY}" ]]; then
    echo "Overlay        : ${BOARD_OVERLAY}"
else
    echo "Overlay        : <none>"
fi
echo "West command   : ${WEST_CMD[*]}"
if [[ -n "${WEST_WORKDIR}" ]]; then
    echo "West workdir   : ${WEST_WORKDIR}"
fi
echo

BUILD_COMMAND=(
    "${WEST_CMD[@]}" "${WEST_ARGS[@]}" "${PRISTINE_ARGS[@]}"
    -b "${ZEPHYR_BOARD}"
    -d "${BUILD_DIR}"
    "${PROJECT_ROOT}"
    --
    -DCONF_FILE="${CONF_FILE_ARG}"
    -DBOARD_ROOT="${BOARD_ROOT}"
    -DAPP_BUILD_PROFILE="${BUILD_PROFILE}"
    -DAPP_BOARD_PROFILE="${BOARD_PROFILE}"
    -DAPP_BOARD_DISPLAY_NAME="${BOARD_DISPLAY_NAME:-${BOARD_PROFILE}}"
    -DAPP_BOARD_STATUS="${BOARD_STATUS:-unknown}"
    -DAPP_BOARD_SERIAL_BAUD="${BOARD_SERIAL_BAUD:-unknown}"
    -DAPP_BOARD_FLASH_RUNNER="${BOARD_FLASH_RUNNER:-unknown}"
    -DAPP_BOARD_FLASH_CHIP="${BOARD_FLASH_CHIP:-unknown}"
    -DAPP_BOARD_FLASH_OFFSET="${BOARD_FLASH_OFFSET:-unknown}"
    -DAPP_BOARD_DESCRIPTION="${BOARD_DESCRIPTION:-}"
    -DAPP_BOOT_MODE="${BOOT_MODE}"
    -DAPP_DISPLAY_MODE="${DISPLAY_MODE}"
    -DZEPHYR_BASE="${ZEPHYR_BASE}"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

if [[ -f "${BOARD_OVERLAY}" ]]; then
    BUILD_COMMAND+=(-DDTC_OVERLAY_FILE="${BOARD_OVERLAY}")
fi

if [[ -n "${WEST_WORKDIR}" ]]; then
    (cd "${WEST_WORKDIR}" && "${BUILD_COMMAND[@]}")
else
    "${BUILD_COMMAND[@]}"
fi

echo
echo "Build complete:"
echo "  ${BUILD_DIR}/zephyr/zephyr.bin"
echo "  ${BUILD_DIR}/zephyr/zephyr.elf"
