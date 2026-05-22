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
  --container-root <dir>  Container projects root (default: parent of this repo)
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
HOST_ROOT="$(cd "${PROJECT_ROOT}/.." && pwd)"
CONTAINER_ROOT="${CONTAINER_PROJECTS_ROOT:-${HOST_ROOT}}"

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
command -v python3 >/dev/null 2>&1 || die "python3 not found"

mkdir -p "$(dirname "${OUTPUT_FILE}")"

python3 - "${COMPILE_COMMANDS}" "${OUTPUT_FILE}" "${CONTAINER_ROOT}" "${HOST_ROOT}" <<'PY'
import json
import os
import platform
import shlex
import subprocess
import sys

source_path, dest_path, container_root, host_root = sys.argv[1:5]


def discover_darwin_sdk_include():
    if platform.system() != "Darwin":
        return ""

    def sdk_include_from_path(sdk_path):
        if os.path.basename(sdk_path) == "include" and os.path.isdir(sdk_path):
            return sdk_path
        sdk_include = os.path.join(sdk_path, "usr", "include")
        return sdk_include if os.path.isdir(sdk_include) else ""

    sdkroot = os.environ.get("SDKROOT", "")
    if sdkroot:
        return sdk_include_from_path(sdkroot)

    try:
        sdk_path = subprocess.check_output(["xcrun", "--show-sdk-path"], text=True).strip()
    except (FileNotFoundError, subprocess.CalledProcessError):
        return ""

    return sdk_include_from_path(sdk_path)


darwin_sdk_include = discover_darwin_sdk_include()
analysis_target = os.environ.get("CLANGD_ANALYSIS_TARGET", "i386-unknown-elf")

unsupported_exact = {
    "-fmodules-ts",
    "-mlongcalls",
    "-fno-defer-pop",
    "-fno-reorder-functions",
    "-fstrict-volatile-bitfields",
    "-mtext-section-literals",
    "-fno-pic",
    "-fno-pie",
    "-fno-asynchronous-unwind-tables",
    "-fno-printf-return-value",
}

unsupported_prefixes = (
    "-fmodule-mapper=",
    "-fdeps-format=",
    "-specs=",
    "--sysroot=",
    "--param=",
    "-fmacro-prefix-map=",
)

unsupported_with_value = {
    "-fmodule-mapper",
    "-fdeps-format",
    "--sysroot",
    "--param",
}


def map_path_text(value):
    return value.replace(container_root, host_root)


def sanitize(args):
    result = []
    skip_next = False
    for arg in args:
        if skip_next:
            skip_next = False
            continue
        arg = map_path_text(arg)
        if arg in unsupported_exact:
            continue
        if arg in unsupported_with_value:
            skip_next = True
            continue
        if arg.startswith(unsupported_prefixes):
            continue
        result.append(arg)
    return add_host_analysis_flags(normalize_driver(result))


def normalize_driver(args):
    if not args:
        return args

    driver = os.path.basename(args[0])
    if "++" in driver or driver.endswith(("g++", "cxx")):
        args[0] = "clang++"
    elif driver.endswith(("gcc", "cc")):
        args[0] = "clang"
    return args


def add_host_analysis_flags(args):
    if platform.system() != "Darwin":
        return args

    extra_args = []

    has_target = any(arg == "-target" or arg.startswith(("-target=", "--target=")) for arg in args)
    if analysis_target and not has_target:
        extra_args.extend(["-target", analysis_target])

    has_sdk_include = darwin_sdk_include and darwin_sdk_include in args
    if darwin_sdk_include and not has_sdk_include:
        extra_args.extend(["-idirafter", darwin_sdk_include])

    for define in ("-D__CLANGD__", "-D__XTENSA__", "-D__xtensa__"):
        if define not in args:
            extra_args.append(define)

    if extra_args:
        args[1:1] = extra_args
    return args


with open(source_path, "r", encoding="utf-8") as source:
    database = json.load(source)

for entry in database:
    for key in ("directory", "file", "output"):
        if isinstance(entry.get(key), str):
            entry[key] = map_path_text(entry[key])

    if "arguments" in entry:
        entry["arguments"] = sanitize(entry["arguments"])

    if "command" in entry:
        entry["command"] = shlex.join(sanitize(shlex.split(map_path_text(entry["command"]))))

with open(dest_path, "w", encoding="utf-8") as dest:
    json.dump(database, dest, indent=2)
    dest.write("\n")
PY

echo "Exported compile database:"
echo "  ${OUTPUT_FILE}"
