#!/usr/bin/env python3

import argparse
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
from firmware_package import PackageError, PackageOptions, create_package


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


def cmd_package(args: argparse.Namespace) -> int:
    try:
        package_dir = create_package(
            PackageOptions(
                board_profile=args.board,
                build_profile=args.profile,
                boot_mode=args.boot,
                build_dir=pathlib.Path(args.build_dir).resolve() if args.build_dir else None,
                dist_dir=pathlib.Path(args.dist_dir).resolve(),
            )
        )
    except (PackageError, RuntimeError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    print("Package created:")
    print(f"  {package_dir}")
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

    package = subcommands.add_parser("package")
    package.add_argument("--board", default="esp32_oled")
    package.add_argument("--profile", default="debug")
    package.add_argument("--boot", default="no-mcuboot")
    package.add_argument("--build-dir")
    package.add_argument("--dist-dir", default=str(PROJECT_ROOT / "dist"))
    package.set_defaults(func=cmd_package)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
