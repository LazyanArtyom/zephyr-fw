#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

mapfile -t BOARD_PROFILES < <("${PROJECT_ROOT}/tools/fw.py" boards list --names --enabled-only)
BUILD_PROFILES=("debug" "release")
BOOT_MODES=("no-mcuboot")
BUILD_MODE="auto"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --include-production)
            BUILD_PROFILES+=("production")
            shift
            ;;
        --include-mcuboot)
            BOOT_MODES+=("mcuboot")
            shift
            ;;
        --mode)
            BUILD_MODE="${2:-}"
            shift 2
            ;;
        --help|-h)
            cat <<EOF
Usage:
  $0 [--include-production] [--include-mcuboot] [--mode auto|clean|incremental]
EOF
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

if [[ ${#BOARD_PROFILES[@]} -eq 0 ]]; then
    echo "No enabled board profiles found." >&2
    exit 1
fi

for board in "${BOARD_PROFILES[@]}"; do
    for profile in "${BUILD_PROFILES[@]}"; do
        for boot in "${BOOT_MODES[@]}"; do
            echo
            echo "=== build: board=${board} profile=${profile} boot=${boot} ==="
            "${PROJECT_ROOT}/scripts/build.sh" \
                --board "${board}" \
                --profile "${profile}" \
                --boot "${boot}" \
                --mode "${BUILD_MODE}"
        done
    done
done
