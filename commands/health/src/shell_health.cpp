#include <platform/core/status.h>
#include <platform/shell/console.h>
#include <platform/storage/storage_info.h>
#include <services/factory_reset/factory_reset_service.h>
#include <services/health/health_service.h>
#include <services/watchdog/watchdog_service.h>
#include <zephyr/shell/shell.h>

namespace {

int CmdHealthStatus(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const platform::shell::Console output(shell);
    const platform::Status status = services::health::HealthService::OverallStatus();
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "health check failed", status);
    }
    output.field("Health", "ok");
    return 0;
}

int CmdStorageInfo(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const platform::shell::Console output(shell);
    output.field("Settings backend", platform::StorageInfo::BackendName());
    output.feature("persistent settings", platform::StorageInfo::PersistentSettingsEnabled());

    const platform::Result<platform::StoragePartitionInfo> storage =
        platform::StorageInfo::SettingsPartition();
    if (!storage.ok()) {
        return platform::shell::PrintStatusError(output, "storage info failed", storage.status());
    }

    output.field("Partition", storage.value().label);
    output.integer_field("Offset", storage.value().offset, "bytes");
    output.integer_field("Size", storage.value().size, "bytes");
    return 0;
}

int CmdFactoryReset(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const platform::shell::Console output(shell);
    const services::factory_reset::FactoryResetPolicy policy =
        services::factory_reset::FactoryResetService::Policy();
    output.field("Factory reset behavior", policy.behavior);
    output.field("Preserves", policy.preserves);

    const platform::Status status = services::factory_reset::FactoryResetService::ResetSettings();
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "factory reset failed", status);
    }

    output.line("Factory reset complete.");
    return 0;
}

void PrintWatchdogStatus(const platform::shell::Console& output,
                         const services::watchdog::WatchdogStatus& status) {
    output.feature("watchdog", status.configured);
    output.feature("device ready", status.device_ready);
    output.feature("initialized", status.initialized);
    output.feature("last reset watchdog", status.last_reset_watchdog);
    output.field("Device", status.device_name);
    output.field("Mode", status.mode);
    output.integer_field("Channel", status.channel_id);
    output.integer_field("Timeout", status.timeout_ms, "ms");
    output.integer_field("Feed interval", status.feed_interval_ms, "ms");
    output.integer_field("Feed count", status.feed_count);
    output.integer_field("Last feed uptime", static_cast<std::int64_t>(status.last_feed_uptime_ms),
                         "ms");
    output.integer_field("Last error", status.last_error);
    output.feature("supervision", status.supervision_available);
    output.feature("supervision healthy", status.supervision_healthy);
    output.integer_field("Supervised tasks", status.supervised_task_count);
    if (!status.unhealthy_task.empty()) {
        output.field("Unhealthy task", status.unhealthy_task);
    }
}

int CmdWatchdogStatus(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    PrintWatchdogStatus(platform::shell::Console(shell),
                        services::watchdog::WatchdogService::Status());
    return 0;
}

int CmdWatchdogFeed(const shell* shell, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    const platform::shell::Console output(shell);
    const platform::Status status = services::watchdog::WatchdogService::Feed();
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "watchdog feed failed", status);
    }
    output.line("Watchdog fed.");
    return 0;
}

}  // namespace

SHELL_STATIC_SUBCMD_SET_CREATE(storage_subcommands,
                               SHELL_CMD(info, NULL, "Show storage information.", CmdStorageInfo),
                               SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(factory_subcommands,
                               SHELL_CMD(reset, NULL, "Reset persistent settings.",
                                         CmdFactoryReset),
                               SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(watchdog_subcommands,
                               SHELL_CMD(status, NULL, "Show watchdog status.", CmdWatchdogStatus),
                               SHELL_CMD(feed, NULL, "Feed the watchdog now.", CmdWatchdogFeed),
                               SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
    health_subcommands, SHELL_CMD(status, NULL, "Show health status.", CmdHealthStatus),
    SHELL_CMD(storage, &storage_subcommands, "Storage commands.", NULL),
    SHELL_CMD(factory, &factory_subcommands, "Factory commands.", NULL),
    SHELL_CMD(watchdog, &watchdog_subcommands, "Watchdog commands.", NULL), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(health, &health_subcommands, "Health support commands.", NULL);
