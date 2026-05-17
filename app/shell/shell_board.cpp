#include <app/board_info.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

namespace {

int CmdBoard(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Board profile: %s", app::GetBoardProfile());
    shell_print(shell, "Zephyr board: %s", app::GetZephyrBoardTarget());
    return 0;
}

}  // namespace

SHELL_CMD_REGISTER(board, NULL, "Show board information.", CmdBoard);
