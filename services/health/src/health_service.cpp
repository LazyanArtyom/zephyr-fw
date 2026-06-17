#include <platform/settings/settings_store.h>
#include <platform/storage/storage_info.h>
#include <services/health/health_service.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_BOOTLOADER_MCUBOOT) && defined(CONFIG_MCUBOOT_IMG_MANAGER)
#include <zephyr/dfu/mcuboot.h>
#endif

LOG_MODULE_REGISTER(health_service, CONFIG_FW_SERVICE_HEALTH_LOG_LEVEL);

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

platform::Status HealthService::ConfirmBootIfHealthy() {
    const platform::Status health_status = OverallStatus();
    if (!health_status.ok()) {
        return health_status;
    }

#if defined(CONFIG_BOOTLOADER_MCUBOOT) && defined(CONFIG_MCUBOOT_IMG_MANAGER)
    if (boot_is_img_confirmed()) {
        LOG_DBG("MCUboot image already confirmed");
        return platform::Status::Ok();
    }

    const int rc = boot_write_img_confirmed();
    if (rc != 0) {
        LOG_ERR("MCUboot image confirmation failed: %d", rc);
        return platform::Status::InternalError("MCUboot image confirmation failed");
    }

    LOG_INF("MCUboot image confirmed after health checks");
#endif

    return platform::Status::Ok();
}

}  // namespace services::health
