#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# shellcheck disable=SC1091
source "${PROJECT_ROOT}/project.env"

usage() {
    cat <<EOF
Usage:
  $0 --board <profile> [--profile debug|release|production] [--boot no-mcuboot|mcuboot]

Options:
  --board <profile>       Board profile (default: esp32_oled)
  --profile <profile>     Build profile (default: debug)
  --boot <mode>           Boot mode (default: no-mcuboot)
  --build-dir <dir>       Explicit build directory
  --dist-dir <dir>        Distribution root (default: ./dist)
EOF
}

die() {
    echo "Error: $*" >&2
    exit 1
}

read_version_var() {
    local key="$1"
    local default_value="$2"
    local value

    value="$(awk -v wanted_key="${key}" '
        $1 == wanted_key {
            for (i = 3; i <= NF; ++i) {
                if ($i ~ /^#/) {
                    break
                }
                if (i > 3) {
                    printf " "
                }
                printf "%s", $i
            }
            exit
        }
    ' "${PROJECT_ROOT}/${APP_VERSION_FILE}")"

    if [[ -z "${value}" ]]; then
        printf '%s\n' "${default_value}"
    else
        printf '%s\n' "${value}"
    fi
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

read_optional_yaml_value() {
    local file_path="$1"
    local key="$2"
    local default_value="$3"

    if [[ -f "${file_path}" ]]; then
        local value
        value="$(read_yaml_value "${file_path}" "${key}")"
        if [[ -n "${value}" ]]; then
            printf '%s\n' "${value}"
            return
        fi
    fi

    printf '%s\n' "${default_value}"
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

json_escape() {
    printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

sha256_file() {
    if command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1"
    elif command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1"
    else
        die "shasum or sha256sum is required"
    fi
}

BOARD_PROFILE="esp32_oled"
BUILD_PROFILE="debug"
BOOT_MODE="no-mcuboot"
BUILD_DIR=""
DIST_DIR="${PROJECT_ROOT}/dist"

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
        --dist-dir)
            DIST_DIR="${2:-}"
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

BOARD_METADATA="$(find_board_metadata "${BOARD_PROFILE}")" \
    || die "board metadata not found for '${BOARD_PROFILE}' under boards/<vendor>/<board>/metadata.yml"
BOARD_DIR="$(dirname "${BOARD_METADATA}")"
PRODUCTION_POLICY_FILE="${BOARD_DIR}/production.yml"
[[ -d "${BUILD_DIR}" ]] || die "build directory not found: ${BUILD_DIR}"

VERSION_MAJOR="$(read_version_var VERSION_MAJOR 0)"
VERSION_MINOR="$(read_version_var VERSION_MINOR 0)"
VERSION_PATCH="$(read_version_var PATCHLEVEL 0)"
VERSION_TWEAK="$(read_version_var VERSION_TWEAK 0)"
EXTRAVERSION="$(read_version_var EXTRAVERSION "")"

APP_VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
if [[ "${VERSION_TWEAK}" != "0" ]]; then
    APP_VERSION="${APP_VERSION}.${VERSION_TWEAK}"
fi
if [[ -n "${EXTRAVERSION}" ]]; then
    APP_VERSION="${APP_VERSION}-${EXTRAVERSION}"
fi

PACKAGE_NAME="${APP_SLUG}_${APP_VERSION}_${BOARD_PROFILE}_${BUILD_PROFILE}_${BOOT_MODE}"
PACKAGE_DIR="${DIST_DIR}/${PACKAGE_NAME}"
ZEPHYR_DIR="${BUILD_DIR}/zephyr"

[[ -f "${ZEPHYR_DIR}/zephyr.bin" ]] || die "firmware image not found: ${ZEPHYR_DIR}/zephyr.bin"

rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}"

copy_if_exists() {
    local source_path="$1"
    local dest_name="${2:-$(basename "${source_path}")}"

    if [[ -f "${source_path}" ]]; then
        cp -f "${source_path}" "${PACKAGE_DIR}/${dest_name}"
    fi
}

generate_partition_summary() {
    local output_path="$1"
    local source_dts="${ZEPHYR_DIR}/zephyr.dts"

    {
        echo "Partition summary"
        echo
        echo "Board profile: ${BOARD_PROFILE}"
        echo "Boot mode: ${BOOT_MODE}"
        echo "Production policy: ${PRODUCTION_POLICY_FILE}"
        echo
        if [[ -f "${source_dts}" ]]; then
            awk '
                /partition@[0-9a-fA-F]+[ 	]*{/ { in_partition = 1; node = $1; label = ""; reg = "" }
                in_partition && /label =/ { label = $0; gsub(/^[ 	]+label = "|";$/, "", label) }
                in_partition && /reg =/ { reg = $0; gsub(/^[ 	]+reg = <|>;$/, "", reg) }
                in_partition && /};/ {
                    if (node != "") {
                        printf "- %s label=%s reg=<%s>\n", node, label, reg
                    }
                    in_partition = 0
                }
            ' "${source_dts}"
        else
            echo "zephyr.dts was not present in the build output."
        fi
    } > "${output_path}"
}

copy_if_exists "${ZEPHYR_DIR}/zephyr.bin"
copy_if_exists "${ZEPHYR_DIR}/zephyr.elf"
copy_if_exists "${ZEPHYR_DIR}/zephyr.map"
copy_if_exists "${ZEPHYR_DIR}/zephyr.hex"
copy_if_exists "${ZEPHYR_DIR}/zephyr.signed.bin"
copy_if_exists "${ZEPHYR_DIR}/app_update.bin"
copy_if_exists "${ZEPHYR_DIR}/zephyr.dts"
copy_if_exists "${ZEPHYR_DIR}/.config" "zephyr.config"
copy_if_exists "${BUILD_DIR}/build_info.yml"
copy_if_exists "${BUILD_DIR}/compile_commands.json"
copy_if_exists "${PRODUCTION_POLICY_FILE}" "production.yml"
copy_if_exists "${PROJECT_ROOT}/partitions/${BOARD_PROFILE}.md" "partition_policy.md"
generate_partition_summary "${PACKAGE_DIR}/partition_summary.txt"

if [[ -f "${BUILD_DIR}/domains.yaml" ]]; then
    copy_if_exists "${BUILD_DIR}/domains.yaml"
fi
if [[ -f "${BUILD_DIR}/mcuboot/zephyr/zephyr.bin" ]]; then
    copy_if_exists "${BUILD_DIR}/mcuboot/zephyr/zephyr.bin" "mcuboot.bin"
fi

GIT_COMMIT="unknown"
GIT_DIRTY=false
if command -v git >/dev/null 2>&1 && git -C "${PROJECT_ROOT}" rev-parse --git-dir >/dev/null 2>&1; then
    GIT_COMMIT="$(git -C "${PROJECT_ROOT}" rev-parse --short=12 HEAD 2>/dev/null || printf 'unknown')"
    if [[ -n "$(git -C "${PROJECT_ROOT}" status --porcelain)" ]]; then
        GIT_DIRTY=true
    fi
fi

BOARD_DISPLAY_NAME="$(read_yaml_value "${BOARD_METADATA}" display_name)"
ZEPHYR_BOARD="$(read_yaml_value "${BOARD_METADATA}" zephyr_board)"
FLASH_RUNNER="$(read_yaml_value "${BOARD_METADATA}" flash_runner)"
FLASH_CHIP="$(read_yaml_value "${BOARD_METADATA}" flash_chip)"
FLASH_OFFSET="$(read_yaml_value "${BOARD_METADATA}" flash_offset)"
MCUBOOT_PARTITION_LAYOUT="$(read_optional_yaml_value "${PRODUCTION_POLICY_FILE}" mcuboot_partition_layout "unknown")"
SLOT0_SIZE="$(read_optional_yaml_value "${PRODUCTION_POLICY_FILE}" slot0_size "unknown")"
SLOT1_SIZE="$(read_optional_yaml_value "${PRODUCTION_POLICY_FILE}" slot1_size "unknown")"
SCRATCH_POLICY="$(read_optional_yaml_value "${PRODUCTION_POLICY_FILE}" scratch_policy "unknown")"
SETTINGS_PARTITION="$(read_optional_yaml_value "${PRODUCTION_POLICY_FILE}" settings_partition "unknown")"
FACTORY_RESET_BEHAVIOR="$(read_optional_yaml_value "${PRODUCTION_POLICY_FILE}" factory_reset_behavior "settings-only")"
SIGNING_KEY_POLICY="$(read_optional_yaml_value "${PRODUCTION_POLICY_FILE}" signing_key_policy "external")"
ROLLBACK_POLICY="$(read_optional_yaml_value "${PRODUCTION_POLICY_FILE}" rollback_policy "manual-confirm")"
RECOVERY_PROCESS="$(read_optional_yaml_value "${PRODUCTION_POLICY_FILE}" recovery_process "board-specific")"

cat > "${PACKAGE_DIR}/firmware.meta.json" <<EOF
{
  "display_name": "$(json_escape "${APP_DISPLAY_NAME}")",
  "slug": "$(json_escape "${APP_SLUG}")",
  "firmware_name": "$(json_escape "${APP_FIRMWARE_NAME}")",
  "version": "$(json_escape "${APP_VERSION}")",
  "board_profile": "$(json_escape "${BOARD_PROFILE}")",
  "board_display_name": "$(json_escape "${BOARD_DISPLAY_NAME}")",
  "zephyr_board": "$(json_escape "${ZEPHYR_BOARD}")",
  "build_profile": "$(json_escape "${BUILD_PROFILE}")",
  "boot_mode": "$(json_escape "${BOOT_MODE}")",
  "git_commit": "$(json_escape "${GIT_COMMIT}")",
  "git_dirty": ${GIT_DIRTY},
  "created_utc": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
  "flash": {
    "runner": "$(json_escape "${FLASH_RUNNER}")",
    "chip": "$(json_escape "${FLASH_CHIP}")",
    "offset": "$(json_escape "${FLASH_OFFSET}")"
  },
  "production": {
    "mcuboot_partition_layout": "$(json_escape "${MCUBOOT_PARTITION_LAYOUT}")",
    "slot0_size": "$(json_escape "${SLOT0_SIZE}")",
    "slot1_size": "$(json_escape "${SLOT1_SIZE}")",
    "scratch_policy": "$(json_escape "${SCRATCH_POLICY}")",
    "settings_partition": "$(json_escape "${SETTINGS_PARTITION}")",
    "factory_reset_behavior": "$(json_escape "${FACTORY_RESET_BEHAVIOR}")",
    "signing_key_policy": "$(json_escape "${SIGNING_KEY_POLICY}")",
    "rollback_policy": "$(json_escape "${ROLLBACK_POLICY}")",
    "recovery_process": "$(json_escape "${RECOVERY_PROCESS}")"
  },
  "artifacts": {
    "firmware_image": "zephyr.bin",
    "signed_image": "zephyr.signed.bin when present",
    "manifest": "firmware.meta.json",
    "sha256": "firmware.sha256",
    "zephyr_config": "zephyr.config",
    "devicetree": "zephyr.dts",
    "flash_helper": "flash.sh",
    "partition_summary": "partition_summary.txt"
  }
}
EOF
cp -f "${PACKAGE_DIR}/firmware.meta.json" "${PACKAGE_DIR}/manifest.json"

cat > "${PACKAGE_DIR}/README.txt" <<EOF
${APP_DISPLAY_NAME} firmware package

Package: ${PACKAGE_NAME}
Version: ${APP_VERSION}
Board profile: ${BOARD_PROFILE}
Zephyr board: ${ZEPHYR_BOARD}
Build profile: ${BUILD_PROFILE}
Boot mode: ${BOOT_MODE}

Primary image:
  zephyr.bin

Metadata:
  firmware.meta.json
  manifest.json
  firmware.sha256
  partition_summary.txt
  production.yml

For ESP32 development flashing with esptool:
  ./flash.sh /dev/ttyUSB0
EOF

if [[ "${FLASH_RUNNER}" == "esp32_esptool" ]]; then
    cat > "${PACKAGE_DIR}/flash.sh" <<EOF
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
PORT="\${1:-}"

if [[ -z "\${PORT}" ]]; then
    echo "Usage: \$0 <serial_port>"
    exit 1
fi

PYTHON="\${ESPTOOL_PYTHON:-}"
if [[ -z "\${PYTHON}" ]]; then
    if [[ -x "\${HOME}/.venvs/esptool/bin/python" ]]; then
        PYTHON="\${HOME}/.venvs/esptool/bin/python"
    else
        PYTHON="python3"
    fi
fi

IMAGE="\${SCRIPT_DIR}/zephyr.bin"
if [[ -f "\${SCRIPT_DIR}/zephyr.signed.bin" ]]; then
    IMAGE="\${SCRIPT_DIR}/zephyr.signed.bin"
fi

"\${PYTHON}" -m esptool \\
    --chip "${FLASH_CHIP:-esp32}" \\
    --port "\${PORT}" \\
    --baud 460800 \\
    write-flash \\
    --flash-mode dio \\
    --flash-freq 40m \\
    --flash-size detect \\
    "${FLASH_OFFSET:-0x1000}" "\${IMAGE}"
EOF
else
    cat > "${PACKAGE_DIR}/flash.sh" <<EOF
#!/usr/bin/env bash
set -euo pipefail
echo "No self-contained flash helper is available for board profile: ${BOARD_PROFILE}" >&2
exit 1
EOF
fi
chmod +x "${PACKAGE_DIR}/flash.sh"

(
    cd "${PACKAGE_DIR}"
    : > firmware.sha256
    for artifact in *; do
        [[ -f "${artifact}" ]] || continue
        [[ "${artifact}" == "firmware.sha256" ]] && continue
        sha256_file "${artifact}" >> firmware.sha256
    done
)

echo "Package created:"
echo "  ${PACKAGE_DIR}"
