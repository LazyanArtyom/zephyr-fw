#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

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

echo "Supported board profiles:"
echo

found=0
for board_yml in "${PROJECT_ROOT}"/boards/*/board.yml; do
    [[ -f "${board_yml}" ]] || continue
    found=1

    profile="$(read_yaml_value "${board_yml}" profile)"
    display_name="$(read_yaml_value "${board_yml}" display_name)"
    zephyr_board="$(read_yaml_value "${board_yml}" zephyr_board)"
    status="$(read_yaml_value "${board_yml}" status)"
    default_display="$(read_yaml_value "${board_yml}" default_display)"
    default_boot="$(read_yaml_value "${board_yml}" default_boot)"

    echo "  ${profile}"
    echo "    Name          : ${display_name:-unknown}"
    echo "    Zephyr target : ${zephyr_board:-not configured}"
    echo "    Status        : ${status:-unknown}"
    echo "    Display       : ${default_display:-off}"
    echo "    Default boot  : ${default_boot:-no-mcuboot}"
    echo
done

if [[ "${found}" -eq 0 ]]; then
    echo "  No boards/<profile>/board.yml files found."
    echo
fi

cat <<EOF
Examples:

  ./scripts/build.sh --board esp32_oled --profile debug
  ./scripts/build.sh --board esp32_oled --profile release --mode incremental
  ./scripts/build.sh --board esp32_oled --profile production --boot mcuboot --package
EOF
