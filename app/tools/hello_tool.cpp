#include "hello_tool.h"

#include <app/board_info.h>

#if defined(CONFIG_APP_DISPLAY)
#include "display_service.h"
#endif

namespace app {

int RunHelloTool(const shell* shell) {
    shell_print(shell, "Hello World from %s!", GetDisplayName());
    shell_print(shell, "Board profile: %s", GetBoardProfile());
    shell_print(shell, "Zephyr board: %s", GetZephyrBoardTarget());
    shell_print(shell, "Version: %s%s", GetAppVersion(), IsGitDirty() ? "-dirty" : "");
    shell_print(shell, "Build profile: %s", GetBuildProfile());
    shell_print(shell, "Boot mode: %s", GetBootMode());

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
