#!/usr/bin/env python3

import argparse
import json
import pathlib
import sys

from board_metadata import (
    PROJECT_ROOT,
    BoardMetadataError,
    iter_board_profiles,
    print_shell_environment,
    require_valid_board_profile,
    validate_board_profile,
)
from firmware_package import (
    VALID_BOOT_MODES,
    VALID_BUILD_PROFILES,
    PackageError,
    PackageOptions,
    create_package,
)


BOARDS_DIR = PROJECT_ROOT / "boards"


def cmd_boards_list(args: argparse.Namespace) -> int:
    boards = list(iter_board_profiles())
    if args.enabled_only:
        boards = [board for board in boards if board.get("status", "unknown") == "enabled"]

    if args.names:
        for board in boards:
            print(board.profile)
        return 0

    print("Supported board profiles:")
    print()

    found = False
    for board in boards:
        found = True
        board_dir = board.board_dir
        zephyr_board = board.get("zephyr_board") or "not configured"
        print(f"  {board.profile}")
        print(f"    Name          : {board.get('display_name', 'unknown')}")
        print(f"    Zephyr target : {zephyr_board}")
        print(f"    Board dir     : {board_dir.relative_to(PROJECT_ROOT)}")
        print(f"    Status        : {board.get('status', 'unknown')}")
        print(f"    Display       : {board.get('default_display', 'off')}")
        print(f"    Settings      : {board.get('default_settings', 'off')}")
        print(f"    Default boot  : {board.get('default_boot', 'no-mcuboot')}")
        print(f"    Production    : {'production.yml' if (board_dir / 'production.yml').is_file() else 'missing'}")
        print()

    if not found:
        print("  No boards/<vendor>/<board>/metadata.yml files found.")
        print()

    print("Examples:")
    print()
    print("  ./scripts/build.sh --board esp32_oled --profile debug")
    print("  ./scripts/build.sh --board esp32_oled --profile release --mode incremental")
    print("  ./scripts/build.sh --board esp32_oled --profile production --boot mcuboot")
    print("  ./scripts/build.sh --board esp32_oled --profile service --boot no-mcuboot")
    return 0


def cmd_boards_validate(_: argparse.Namespace) -> int:
    failures = 0
    for board_profile in iter_board_profiles():
        issues = validate_board_profile(board_profile)
        failures += len(issues)
        for issue in issues:
            print(issue, file=sys.stderr)

    if failures:
        return 1

    print("Board profiles validated.")
    return 0


def cmd_boards_env(args: argparse.Namespace) -> int:
    try:
        board = require_valid_board_profile(args.profile)
    except BoardMetadataError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    print_shell_environment(board)
    return 0


def normalize_build_dir(build_dir: pathlib.Path) -> pathlib.Path:
    build_dir = build_dir.expanduser()
    if not build_dir.is_absolute():
        build_dir = PROJECT_ROOT / build_dir
    build_dir = build_dir.resolve()
    if build_dir.name == "zephyr":
        build_dir = build_dir.parent
    return build_dir


def infer_build_coordinates(build_dir: pathlib.Path) -> tuple[str, str, str]:
    parts = build_dir.parts
    if len(parts) < 3:
        raise PackageError(
            f"cannot infer board/profile/boot from build directory: {build_dir}"
        )

    board_profile, build_profile, boot_mode = parts[-3], parts[-2], parts[-1]
    if build_profile not in VALID_BUILD_PROFILES or boot_mode not in VALID_BOOT_MODES:
        raise PackageError(
            "build directory must end with <board>/<profile>/<boot>, for example "
            "build/esp32_oled/production/no-mcuboot"
        )
    return board_profile, build_profile, boot_mode


def discover_build_dirs() -> list[pathlib.Path]:
    build_root = PROJECT_ROOT / "build"
    if not build_root.is_dir():
        return []

    candidates: list[pathlib.Path] = []
    for board_dir in sorted(path for path in build_root.iterdir() if path.is_dir()):
        if board_dir.name.startswith(".") or board_dir.name == "generated-configs":
            continue
        for profile_dir in sorted(path for path in board_dir.iterdir() if path.is_dir()):
            if profile_dir.name not in VALID_BUILD_PROFILES:
                continue
            for boot_dir in sorted(path for path in profile_dir.iterdir() if path.is_dir()):
                if boot_dir.name not in VALID_BOOT_MODES:
                    continue
                app_image = boot_dir / "zephyr" / "zephyr.bin"
                if boot_dir.name == "mcuboot":
                    domains_file = boot_dir / "domains.yaml"
                    default_domain = "zephyr-fw"
                    if domains_file.is_file():
                        for line in domains_file.read_text(encoding="utf-8", errors="replace").splitlines():
                            stripped = line.strip()
                            if stripped.startswith("default:"):
                                default_domain = stripped.split(":", 1)[1].strip().strip('"') or default_domain
                                break
                    app_image = boot_dir / default_domain / "zephyr" / "zephyr.bin"
                if app_image.is_file():
                    candidates.append(boot_dir)
    return candidates


def prompt_for_build_dir() -> pathlib.Path:
    if not sys.stdin.isatty():
        raise PackageError(
            "build directory is required. Example: "
            "./scripts/package.sh build/esp32_oled/production/no-mcuboot"
        )

    candidates = discover_build_dirs()
    if candidates:
        print("Build directories:")
        for index, candidate in enumerate(candidates, start=1):
            print(f"  {index}. {candidate.relative_to(PROJECT_ROOT)}")
        answer = input("Select build directory number or enter a path: ").strip()
        if answer.isdigit():
            selected = int(answer)
            if 1 <= selected <= len(candidates):
                return candidates[selected - 1]
            raise PackageError(f"selection out of range: {answer}")
        if answer:
            return normalize_build_dir(pathlib.Path(answer))
        raise PackageError("build directory selection is required")

    answer = input("Build directory to package: ").strip()
    if not answer:
        raise PackageError("build directory is required")
    return normalize_build_dir(pathlib.Path(answer))


def resolve_package_request(args: argparse.Namespace) -> PackageOptions:
    build_arg = args.build_path or args.build_dir
    build_dir = normalize_build_dir(pathlib.Path(build_arg)) if build_arg else None

    if build_dir is None and not (args.board and args.profile and args.boot):
        if args.board or args.profile or args.boot:
            raise PackageError(
                "provide all of --board, --profile, and --boot, or pass a build directory"
            )
        build_dir = prompt_for_build_dir()

    inferred: tuple[str, str, str] | None = None
    if build_dir is not None:
        inferred = infer_build_coordinates(build_dir)

    board_profile = args.board or (inferred[0] if inferred else None)
    build_profile = args.profile or (inferred[1] if inferred else None)
    boot_mode = args.boot or (inferred[2] if inferred else None)

    if board_profile is None or build_profile is None or boot_mode is None:
        raise PackageError(
            "package target is incomplete; pass a build directory or --board/--profile/--boot"
        )

    return PackageOptions(
        board_profile=board_profile,
        build_profile=build_profile,
        boot_mode=boot_mode,
        build_dir=build_dir,
        dist_dir=pathlib.Path(args.dist_dir).resolve(),
    )


def cmd_package(args: argparse.Namespace) -> int:
    try:
        package_dir = create_package(resolve_package_request(args))
    except (PackageError, RuntimeError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    manifest_file = package_dir / "manifest.json"
    manifest = json.loads(manifest_file.read_text(encoding="utf-8"))
    print("Package created:")
    print(f"  {package_dir}")
    print(f"  Build profile : {manifest.get('build_profile', 'unknown')}")
    print(f"  Boot mode     : {manifest.get('boot_mode', 'unknown')}")
    shell_enabled = manifest.get("runtime", {}).get("shell_enabled", False)
    print(f"  Shell         : {'enabled' if shell_enabled else 'disabled'}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(prog="fw.py")
    subcommands = parser.add_subparsers(dest="command", required=True)

    boards_parser = subcommands.add_parser("boards")
    boards_subcommands = boards_parser.add_subparsers(dest="boards_command", required=True)

    boards_list = boards_subcommands.add_parser("list")
    boards_list.add_argument("--names", action="store_true", help="Print only board profile names")
    boards_list.add_argument("--enabled-only", action="store_true", help="Only include enabled boards")
    boards_list.set_defaults(func=cmd_boards_list)

    boards_validate = boards_subcommands.add_parser("validate")
    boards_validate.set_defaults(func=cmd_boards_validate)

    boards_env = boards_subcommands.add_parser("env")
    boards_env.add_argument("profile")
    boards_env.set_defaults(func=cmd_boards_env)

    package = subcommands.add_parser(
        "package",
        description="Create a package from a build directory.",
        epilog=(
            "Examples:\n"
            "  ./scripts/package.sh build/esp32_oled/production/no-mcuboot\n"
            "  ./scripts/package.sh build/esp32_oled/debug/mcuboot\n"
            "  ./scripts/package.sh --board esp32_oled --profile production --boot mcuboot\n"
            "  ./scripts/package.sh --board esp32_oled --profile service --boot no-mcuboot"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    package.add_argument("build_path", nargs="?", help="Build directory: build/<board>/<profile>/<boot>")
    package.add_argument("--board", help="Board profile; inferred from build directory when omitted")
    package.add_argument("--profile", help="Build profile; inferred from build directory when omitted")
    package.add_argument("--boot", help="Boot mode; inferred from build directory when omitted")
    package.add_argument("--build-dir", help="Build directory: build/<board>/<profile>/<boot>")
    package.add_argument("--dist-dir", default=str(PROJECT_ROOT / "dist"))
    package.set_defaults(func=cmd_package)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
