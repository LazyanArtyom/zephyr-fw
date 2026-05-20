#include <platform/board/board_info.h>
#include <services/health/heartbeat_service.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_FW_DISPLAY)
#include <services/display/display_service.h>
#endif

LOG_MODULE_REGISTER(firmware_main, CONFIG_LOG_DEFAULT_LEVEL);

int main() {
    LOG_INF("%s booted", platform::board::GetDisplayName());
    LOG_INF("Board profile: %s", platform::board::GetBoardProfile());
    LOG_INF("Zephyr board: %s", platform::board::GetZephyrBoardTarget());
    LOG_INF("Version: %s%s", platform::board::GetFirmwareVersion(),
            platform::board::IsGitDirty() ? "-dirty" : "");
    LOG_INF("Build profile: %s, boot: %s", platform::board::GetBuildProfile(),
            platform::board::GetBootMode());

#if defined(CONFIG_FW_DISPLAY)
    if (!services::display::InitializeDisplay()) {
        LOG_WRN("display initialization failed");
    } else if (!services::display::ShowBootSummary()) {
        LOG_WRN("display boot summary failed");
    }
#endif

#if defined(CONFIG_FW_HEALTH_HEARTBEAT)
    services::health::StartHeartbeatService();
#endif

    return 0;
}
