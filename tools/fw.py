#!/usr/bin/env python3

import argparse
import pathlib
import sys

from board_metadata import PROJECT_ROOT, REQUIRED_BOARD_FIELDS, iter_board_profiles, print_shell_environment, require_board_profile
from firmware_package import PackageError, PackageOptions, create_package


BOARDS_DIR = PROJECT_ROOT / "boards"


def cmd_boards_list(_: argparse.Namespace) -> int:
    print("Supported board profiles:")
    print()

    found = False
    for board in iter_board_profiles():
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
        metadata_yml = board_profile.metadata_path
        board = board_profile.metadata
        board_dir = board_profile.board_dir
        missing = [field for field in REQUIRED_BOARD_FIELDS if not board_profile.get(field)]
        if missing:
            failures += 1
            print(f"{metadata_yml}: missing {', '.join(missing)}", file=sys.stderr)
            continue
        if not (board_dir / "board.yml").is_file():
            failures += 1
            print(f"{board_dir}: missing Zephyr board.yml", file=sys.stderr)
        if not (board_dir / "board.conf").is_file():
            failures += 1
            print(f"{board_dir}: missing board.conf", file=sys.stderr)
        if not (board_dir / "board.overlay").is_file():
            failures += 1
            print(f"{board_dir}: missing board.overlay", file=sys.stderr)
        for profile_conf in ("debug.conf", "release.conf", "production.conf", "flash.conf", "production.yml", "README.md"):
            if not (board_dir / profile_conf).is_file():
                failures += 1
                print(f"{board_dir}: missing {profile_conf}", file=sys.stderr)
        if not any(board_dir.glob("*.dts")):
            failures += 1
            print(f"{board_dir}: missing Zephyr devicetree (*.dts)", file=sys.stderr)
        if not any(board_dir.glob("*_defconfig")):
            failures += 1
            print(f"{board_dir}: missing Zephyr board defconfig (*_defconfig)", file=sys.stderr)
        if board["status"] == "enabled" and not board.get("zephyr_board"):
            failures += 1
            print(f"{metadata_yml}: enabled boards must set zephyr_board", file=sys.stderr)
        if board["default_display"] not in ("on", "off"):
            failures += 1
            print(f"{metadata_yml}: default_display must be on or off", file=sys.stderr)
        if board["default_settings"] not in ("on", "off"):
            failures += 1
            print(f"{metadata_yml}: default_settings must be on or off", file=sys.stderr)
        if board["default_boot"] not in ("no-mcuboot", "mcuboot"):
            failures += 1
            print(f"{metadata_yml}: default_boot must be no-mcuboot or mcuboot", file=sys.stderr)

    if failures:
        return 1

    print("Board profiles validated.")
    return 0


def cmd_boards_env(args: argparse.Namespace) -> int:
    print_shell_environment(require_board_profile(args.profile))
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
