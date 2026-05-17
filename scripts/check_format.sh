#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format not found" >&2
    exit 1
fi

files=()
while IFS= read -r file_path; do
    files+=("${file_path}")
done < <(
    find "${PROJECT_ROOT}/app" \
        -type f \( -name '*.h' -o -name '*.cpp' -o -name '*.c' \) \
        | sort
)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No source files found."
    exit 0
fi

clang-format --dry-run --Werror "${files[@]}"
echo "Format check passed (${#files[@]} files)."
