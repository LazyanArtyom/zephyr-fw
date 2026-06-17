#include <platform/board/board_info.h>
#include <services/health/heartbeat_service.h>
#include <services/watchdog/watchdog_service.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_FW_DISPLAY)
#include <services/display/display_service.h>
#endif

#if defined(CONFIG_SETTINGS)
#include <platform/settings/settings_store.h>
#endif

#if defined(CONFIG_FW_SERVICE_DIAGNOSTICS)
#include <services/diagnostics/diagnostics_service.h>
#endif

LOG_MODULE_REGISTER(app_main, CONFIG_FW_APP_MAIN_LOG_LEVEL);

int main() {
    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    board_info.LogBootSummary();

#if defined(CONFIG_SETTINGS)
    const platform::Status settings_status = platform::SettingsStore::Load();
    if (!settings_status.ok()) {
        LOG_ERR("Settings load failed: %s (%s)", platform::ToString(settings_status.code()).c_str(),
                settings_status.message().c_str());
    }
#endif

#if defined(CONFIG_FW_SERVICE_DIAGNOSTICS)
    const platform::Status diagnostics_status =
        services::diagnostics::DiagnosticsService::Initialize();
    if (!diagnostics_status.ok()) {
        LOG_ERR("Diagnostics initialization failed: %s (%s)",
                platform::ToString(diagnostics_status.code()).c_str(),
                diagnostics_status.message().c_str());
    }
#endif

#if defined(CONFIG_FW_SERVICE_WATCHDOG)
    const platform::Status watchdog_status = services::watchdog::WatchdogService::Initialize();
    if (!watchdog_status.ok()) {
        LOG_ERR("Watchdog initialization failed: %s (%s)",
                platform::ToString(watchdog_status.code()).c_str(),
                watchdog_status.message().c_str());
    }
#endif

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
