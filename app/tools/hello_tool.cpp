#include "hello_tool.hpp"

#include <app/board_info.hpp>

namespace app {

int RunHelloTool(const shell* shell)
{
    shell_print(shell, "Hello World from Zephyr Golden FW!");
    shell_print(shell, "Board: %s", GetBoardName());
    shell_print(shell, "Version: %s", GetAppVersion());
    shell_print(shell, "Build profile: %s", GetBuildProfile());
    return 0;
}

}  // namespace app
