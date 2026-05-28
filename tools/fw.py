#!/usr/bin/env python3

import argparse
import pathlib
import sys


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
BOARDS_DIR = PROJECT_ROOT / "boards"


REQUIRED_BOARD_FIELDS = (
    "profile",
    "display_name",
    "status",
    "default_display",
    "default_settings",
    "default_boot",
    "serial_baud",
)


def read_flat_yaml(path: pathlib.Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or ":" not in stripped:
            continue
        key, value = stripped.split(":", 1)
        values[key.strip()] = value.strip().strip('"')
    return values


def iter_board_profiles() -> list[tuple[pathlib.Path, dict[str, str]]]:
    boards: list[tuple[pathlib.Path, dict[str, str]]] = []
    for metadata_yml in sorted(BOARDS_DIR.glob("*/*/metadata.yml")):
        boards.append((metadata_yml, read_flat_yaml(metadata_yml)))
    return boards


def cmd_boards_list(_: argparse.Namespace) -> int:
    print("Supported board profiles:")
    print()

    found = False
    for metadata_yml, board in iter_board_profiles():
        found = True
        board_dir = metadata_yml.parent
        profile = board.get("profile", board_dir.name)
        zephyr_board = board.get("zephyr_board") or "not configured"
        print(f"  {profile}")
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
    for metadata_yml, board in iter_board_profiles():
        board_dir = metadata_yml.parent
        missing = [field for field in REQUIRED_BOARD_FIELDS if not board.get(field)]
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


def main() -> int:
    parser = argparse.ArgumentParser(prog="fw.py")
    subcommands = parser.add_subparsers(dest="command", required=True)

    boards_parser = subcommands.add_parser("boards")
    boards_subcommands = boards_parser.add_subparsers(dest="boards_command", required=True)

    boards_list = boards_subcommands.add_parser("list")
    boards_list.set_defaults(func=cmd_boards_list)

    boards_validate = boards_subcommands.add_parser("validate")
    boards_validate.set_defaults(func=cmd_boards_validate)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
