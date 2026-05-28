#include <services/health/health_service.h>

#include <platform/settings/settings_store.h>
#include <platform/storage/storage_info.h>

namespace {

services::health::ManufacturingValue ReadManufacturingValue(platform::StringView key) {
    services::health::ManufacturingValue output{};
    output.key = key;
#if defined(CONFIG_SETTINGS)
    const platform::Result<platform::SettingValue> value = platform::SettingsStore::GetString(key);
    if (value.ok()) {
        output.value.append(value.value().view());
        output.present = true;
    } else {
        output.value.append("not-set");
        output.present = false;
    }
#else
    output.value.append("settings-disabled");
    output.present = false;
#endif
    return output;
}

platform::Status WriteManufacturingValue(platform::StringView key, platform::StringView value) {
#if defined(CONFIG_SETTINGS)
    if (value.empty()) {
        return platform::Status::InvalidArgument("value must not be empty");
    }
    return platform::SettingsStore::SetString(key, value);
#else
    return platform::Status::NotSupported("settings are disabled");
#endif
}

}  // namespace

namespace services::health {

platform::Status HealthService::OverallStatus() {
#if defined(CONFIG_SETTINGS)
    const platform::Status settings_status = platform::SettingsStore::Initialize();
    if (!settings_status.ok()) {
        return settings_status;
    }
#endif

    if (platform::StorageInfo::PersistentSettingsEnabled()) {
        const platform::Result<platform::StoragePartitionInfo> storage =
            platform::StorageInfo::SettingsPartition();
        if (!storage.ok()) {
            return storage.status();
        }
    }

    return platform::Status::Ok();
}

ManufacturingValue ManufacturingService::BoardSerial() {
    return ReadManufacturingValue("board/serial");
}

platform::Status ManufacturingService::SetBoardSerial(platform::StringView value) {
    return WriteManufacturingValue("board/serial", value);
}

ManufacturingValue ManufacturingService::BoardHardwareRevision() {
    return ReadManufacturingValue("board/hw-rev");
}

platform::Status ManufacturingService::SetBoardHardwareRevision(platform::StringView value) {
    return WriteManufacturingValue("board/hw-rev", value);
}

}  // namespace services::health
