#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SCRIPT_PATH="${SCRIPT_DIR}/run-clang-tidy.sh"
TIDY_DB_TO_REMOVE=""

die() {
    echo "clang-tidy: $*" >&2
    exit 1
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
    python3 - "${compile_commands}" "${temp_dir}/compile_commands.json" <<'PY'
import json
import shlex
import sys

unsupported_exact = {
    "-fmodules-ts",
    "-mlongcalls",
    "-fno-defer-pop",
    "-fno-reorder-functions",
    "-fstrict-volatile-bitfields",
    "-mtext-section-literals",
}

unsupported_prefixes = (
    "-fmodule-mapper=",
    "-fdeps-format=",
    "-specs=",
    "--param=",
)

unsupported_with_value = {
    "-fmodule-mapper",
    "-fdeps-format",
    "--param",
}


def sanitize(args):
    result = []
    skip_next = False
    for arg in args:
        if skip_next:
            skip_next = False
            continue
        if arg in unsupported_exact:
            continue
        if arg in unsupported_with_value:
            skip_next = True
            continue
        if arg.startswith(unsupported_prefixes):
            continue
        result.append(arg)
    return result


with open(sys.argv[1], "r", encoding="utf-8") as source:
    database = json.load(source)

for entry in database:
    if "arguments" in entry:
        entry["arguments"] = sanitize(entry["arguments"])
    if "command" in entry:
        entry["command"] = shlex.join(sanitize(shlex.split(entry["command"])))

with open(sys.argv[2], "w", encoding="utf-8") as dest:
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
            [[ "${file_path}" =~ /(app|apps|common|tests)/.*\.(c|cc|cpp|cxx)$ ]] || continue
            [[ "${file_path}" =~ /build/ ]] && continue
            printf '%s\n' "${file_path}"
        done \
        | sort -u
}

run_single_file() {
    local tidy_dir="$1"
    local file_path="$2"
    local header_filter

    header_filter="^.*/(app|apps|common|tests)/"

    clang-tidy \
        "${file_path}" \
        -p "${tidy_dir}" \
        "--config-file=${PROJECT_ROOT}/.clang-tidy" \
        "--header-filter=${header_filter}" \
        2>&1 \
        | sed -E '/^[0-9]+ warnings generated\.$/d; /^Suppressed [0-9]+ warnings .*$/d'
}

main() {
    local build_dir_arg="${1:-}"
    local build_dir
    local compile_commands
    local tidy_dir
    local jobs
    local files=()

    [[ -n "${build_dir_arg}" ]] || die "usage: ${SCRIPT_PATH} <build-dir> [files...]"
    shift || true

    command -v clang-tidy >/dev/null 2>&1 || die "clang-tidy not found"
    command -v python3 >/dev/null 2>&1 || die "python3 not found"

    build_dir="$(resolve_path "${build_dir_arg}")"
    compile_commands="${build_dir}/compile_commands.json"
    [[ -f "${compile_commands}" ]] || die "compile database not found: ${compile_commands}"

    tidy_dir="$(make_tidy_database "${compile_commands}")"
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
        done < <(collect_project_sources "${compile_commands}")
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
