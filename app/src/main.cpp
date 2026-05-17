#include <app/app_context.h>
#include <app/board_info.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "heartbeat_service.h"

#if defined(CONFIG_APP_DISPLAY)
#include "display_service.h"
#endif

LOG_MODULE_REGISTER(app_main, CONFIG_LOG_DEFAULT_LEVEL);

int main() {
    auto& context = app::GetAppContext();
    const app::Status status = context.Initialize();

    if (status != app::Status::kOk) {
        LOG_ERR("Application initialization failed");
        return -1;
    }

    LOG_INF("%s booted", app::GetDisplayName());
    LOG_INF("Board profile: %s", app::GetBoardProfile());
    LOG_INF("Zephyr board: %s", app::GetZephyrBoardTarget());
    LOG_INF("Version: %s%s", app::GetAppVersion(), app::IsGitDirty() ? "-dirty" : "");
    LOG_INF("Build profile: %s, boot: %s", app::GetBuildProfile(), app::GetBootMode());

#if defined(CONFIG_APP_DISPLAY)
    if (!app::InitializeDisplay()) {
        LOG_WRN("display initialization failed");
    }
#endif

#if defined(CONFIG_APP_HEARTBEAT_SERVICE)
    app::StartHeartbeatService();
#endif

    return 0;
}
