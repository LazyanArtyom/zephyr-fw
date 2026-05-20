#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SCRIPT_PATH="${SCRIPT_DIR}/run-clang-tidy.sh"
TIDY_DB_TO_REMOVE=""
CONTAINER_ROOT="${CONTAINER_PROJECTS_ROOT:-/home/artyom/Documents/projects}"
HOST_ROOT="$(cd "${PROJECT_ROOT}/.." && pwd)"

die() {
    echo "clang-tidy: $*" >&2
    exit 1
}

usage() {
    cat <<EOF
Usage:
  ${SCRIPT_PATH} [--container-root <dir>] [--host-root <dir>] <build-dir> [files...]

Options:
  --container-root <dir>  Container projects root (default: /home/artyom/Documents/projects)
  --host-root <dir>       Host projects root (default: parent of this repo)
EOF
}

resolve_path() {
    local path_value="$1"

    if [[ "${path_value}" = /* ]]; then
        printf '%s\n' "${path_value}"
    else
        printf '%s/%s\n' "${PROJECT_ROOT}" "${path_value#./}"
    fi
}

cleanup() {
    if [[ -n "${TIDY_DB_TO_REMOVE}" ]]; then
        rm -rf "${TIDY_DB_TO_REMOVE}"
    fi
}

make_tidy_database() {
    local compile_commands="$1"
    local temp_dir

    temp_dir="$(mktemp -d "${TMPDIR:-/tmp}/zephyr-fw-clang-tidy.XXXXXX")"
    python3 - "${compile_commands}" "${temp_dir}/compile_commands.json" "${CONTAINER_ROOT}" "${HOST_ROOT}" <<'PY'
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
    printf '%s\n' "${temp_dir}"
}

collect_project_sources() {
    local compile_commands="$1"
    sed -nE 's/^[[:space:]]*"file"[[:space:]]*:[[:space:]]*"([^"]+)".*/\1/p' "${compile_commands}" \
        | while IFS= read -r file_path; do
            [[ -n "${file_path}" ]] || continue
            case "${file_path}" in
                "${PROJECT_ROOT}/app/"*|\
                "${PROJECT_ROOT}/commands/"*|\
                "${PROJECT_ROOT}/platform/"*|\
                "${PROJECT_ROOT}/services/"*|\
                "${PROJECT_ROOT}/tests/"*)
                    ;;
                *)
                    continue
                    ;;
            esac
            [[ "${file_path}" =~ \.(c|cc|cpp|cxx)$ ]] || continue
            [[ "${file_path}" =~ /build/ ]] && continue
            printf '%s\n' "${file_path}"
        done \
        | sort -u
}

run_single_file() {
    local tidy_dir="$1"
    local file_path="$2"
    local header_filter

    header_filter="^${PROJECT_ROOT}/(app|commands|platform|services|tests)/"

    clang-tidy \
        "${file_path}" \
        -p "${tidy_dir}" \
        "--config-file=${PROJECT_ROOT}/.clang-tidy" \
        "--header-filter=${header_filter}" \
        2>&1 \
        | sed -E '/^[0-9]+ warnings generated\.$/d; /^Suppressed [0-9]+ warnings .*$/d; /^Use -header-filter=.*$/d; /^Found compiler error\(s\)\.$/d'
}

main() {
    local build_dir_arg=""
    local build_dir
    local compile_commands
    local tidy_dir
    local tidy_compile_commands
    local jobs
    local files=()

    while [[ $# -gt 0 ]]; do
        case "$1" in
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
                build_dir_arg="$1"
                shift
                break
                ;;
        esac
    done

    [[ -n "${build_dir_arg}" ]] || die "usage: ${SCRIPT_PATH} <build-dir> [files...]"

    command -v clang-tidy >/dev/null 2>&1 || die "clang-tidy not found"
    command -v python3 >/dev/null 2>&1 || die "python3 not found"

    build_dir="$(resolve_path "${build_dir_arg}")"
    compile_commands="${build_dir}/compile_commands.json"
    [[ -f "${compile_commands}" ]] || die "compile database not found: ${compile_commands}"

    tidy_dir="$(make_tidy_database "${compile_commands}")"
    tidy_compile_commands="${tidy_dir}/compile_commands.json"
    TIDY_DB_TO_REMOVE="${tidy_dir}"
    trap cleanup EXIT

    if [[ $# -gt 0 ]]; then
        while [[ $# -gt 0 ]]; do
            files+=("$(resolve_path "$1")")
            shift
        done
    else
        while IFS= read -r file_path; do
            files+=("${file_path}")
        done < <(collect_project_sources "${tidy_compile_commands}")
    fi

    ((${#files[@]} > 0)) || die "no project source files found"

    jobs="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || printf '4')"
    export TIDY_DIR="${tidy_dir}"
    export SCRIPT_PATH

    echo "clang-tidy: checking ${#files[@]} file(s)"
    printf '%s\0' "${files[@]}" | xargs -0 -n 1 -P "${jobs}" bash -c \
        '"${SCRIPT_PATH}" --single "${TIDY_DIR}" "$1"' _
}

if [[ "${1:-}" == "--single" ]]; then
    shift
    run_single_file "$1" "$2"
else
    main "$@"
fi
