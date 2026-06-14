#include <errno.h>
#include <platform/core/status.h>
#include <platform/shell/console.h>
#include <platform/storage/storage_info.h>
#include <services/factory_reset/factory_reset_service.h>
#include <services/health/health_service.h>
#include <services/watchdog/watchdog_service.h>
#include <zephyr/shell/shell.h>

namespace {

int PrintUsage(const platform::shell::Console& output) {
    output.line("Usage:");
    output.line("  health status");
    output.line("  health storage info");
    output.line("  health factory reset");
    output.line("  health watchdog status");
    return 0;
}

int CmdHealthStatus(const platform::shell::Console& output) {
    const platform::Status status = services::health::HealthService::OverallStatus();
    if (!status.ok()) {
        return platform::shell::PrintStatusError(output, "health check failed", status);
    }
    output.field("Health", "ok");
    return 0;
}

int CmdStorageInfo(const platform::shell::Console& output) {
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

int CmdFactoryReset(const platform::shell::Console& output) {
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

int CmdWatchdogStatus(const platform::shell::Console& output) {
    const services::watchdog::WatchdogStatus status = services::watchdog::WatchdogService::Status();
    output.feature("watchdog", status.configured);
    output.feature("device ready", status.device_ready);
    output.field("Device", status.device_name);
    output.field("Mode", status.mode);
    return 0;
}

int CmdHealth(const shell* shell, size_t argc, char** argv) {
    const platform::shell::Console output(shell);
    const platform::shell::Arguments arguments(argc, argv);

    if (arguments.size() < 2 || arguments.at(1).equals("--help") || arguments.at(1).equals("-h")) {
        return PrintUsage(output);
    }

    const platform::StringView command = arguments.at(1);
    if (command.equals("status")) {
        return CmdHealthStatus(output);
    }
    if (command.equals("factory-reset")) {
        return CmdFactoryReset(output);
    }
    if (command.equals("factory") && arguments.size() >= 3 && arguments.at(2).equals("reset")) {
        return CmdFactoryReset(output);
    }
    if (command.equals("storage") && arguments.size() >= 3 && arguments.at(2).equals("info")) {
        return CmdStorageInfo(output);
    }
    if (command.equals("watchdog") && arguments.size() >= 3 && arguments.at(2).equals("status")) {
        return CmdWatchdogStatus(output);
    }

    output.error_value("unknown health command", command);
    return -EINVAL;
}

}  // namespace

SHELL_CMD_ARG_REGISTER(health, NULL, "Health and manufacturing support commands.", CmdHealth, 1, 3);
