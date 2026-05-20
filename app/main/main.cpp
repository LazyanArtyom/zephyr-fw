#include <platform/board/board_info.h>
#include <services/health/heartbeat_service.h>

#if defined(CONFIG_FW_DISPLAY)
#include <services/display/display_service.h>
#endif

int main() {
    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    board_info.LogBootSummary();

#if defined(CONFIG_FW_DISPLAY)
    if (services::display::InitializeDisplay()) {
        (void)services::display::ShowBootSummary();
    }
#endif

#if defined(CONFIG_FW_HEALTH_HEARTBEAT)
    services::health::StartHeartbeatService();
#endif

    return 0;
}
