#include "hello_tool.hpp"

#include <app/board_info.hpp>

#if defined(CONFIG_APP_DISPLAY)
#include "display_service.hpp"
#endif

namespace app {

int RunHelloTool(const shell* shell)
{
    shell_print(shell, "Hello World from Zephyr FW!");
    shell_print(shell, "Board: %s", GetBoardName());
    shell_print(shell, "Version: %s", GetAppVersion());
    shell_print(shell, "Build profile: %s", GetBuildProfile());

#if defined(CONFIG_APP_DISPLAY)
    if (ShowHelloWorldOnDisplay()) {
        shell_print(shell, "OLED: message displayed");
    } else {
        shell_error(shell, "OLED: failed to display message");
        return -1;
    }
#endif

    return 0;
}

}  // namespace app
