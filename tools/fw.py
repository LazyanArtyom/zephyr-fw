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
    for board_yml in sorted(BOARDS_DIR.glob("*/board.yml")):
        boards.append((board_yml, read_flat_yaml(board_yml)))
    return boards


def cmd_boards_list(_: argparse.Namespace) -> int:
    for board_yml, board in iter_board_profiles():
        profile = board.get("profile", board_yml.parent.name)
        print(f"{profile}")
        print(f"  name          : {board.get('display_name', 'unknown')}")
        print(f"  status        : {board.get('status', 'unknown')}")
        print(f"  zephyr board  : {board.get('zephyr_board', 'not configured')}")
        print(f"  default boot  : {board.get('default_boot', 'no-mcuboot')}")
        print(f"  display       : {board.get('default_display', 'off')}")
    return 0


def cmd_boards_validate(_: argparse.Namespace) -> int:
    failures = 0
    for board_yml, board in iter_board_profiles():
        missing = [field for field in REQUIRED_BOARD_FIELDS if not board.get(field)]
        if missing:
            failures += 1
            print(f"{board_yml}: missing {', '.join(missing)}", file=sys.stderr)
            continue
        if board["status"] == "enabled" and not board.get("zephyr_board"):
            failures += 1
            print(f"{board_yml}: enabled boards must set zephyr_board", file=sys.stderr)
        if board["default_display"] not in ("on", "off"):
            failures += 1
            print(f"{board_yml}: default_display must be on or off", file=sys.stderr)
        if board["default_boot"] not in ("no-mcuboot", "mcuboot"):
            failures += 1
            print(f"{board_yml}: default_boot must be no-mcuboot or mcuboot", file=sys.stderr)

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
