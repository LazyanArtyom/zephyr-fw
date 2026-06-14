#include <platform/settings/settings_store.h>
#include <platform/storage/storage_info.h>
#include <services/health/health_service.h>

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

}  // namespace services::health
