#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app/app_context.hpp>
#include <app/board_info.hpp>

#include "heartbeat_service.hpp"

#if defined(CONFIG_APP_DISPLAY)
#include "display_service.hpp"
#endif

LOG_MODULE_REGISTER(app_main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
    auto& context = app::GetAppContext();
    const app::Status status = context.Initialize();

    if (status != app::Status::kOk) {
        LOG_ERR("Application initialization failed");
        return -1;
    }

    LOG_INF("Zephyr FW booted");
    LOG_INF("Board: %s", app::GetBoardName());
    LOG_INF("Version: %s", app::GetAppVersion());
    LOG_INF("Build profile: %s", app::GetBuildProfile());

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
