#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <app/board_info.hpp>

namespace {

int CmdBoard(const shell* shell, size_t argc, char** argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Board: %s", app::GetBoardName());
    return 0;
}

}  // namespace

SHELL_CMD_REGISTER(board, nullptr, "Show board information.", CmdBoard);
