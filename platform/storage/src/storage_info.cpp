#include <errno.h>
#include <platform/storage/storage_info.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

namespace platform {

Result<StoragePartitionInfo> StorageInfo::SettingsPartition() {
#if defined(CONFIG_FLASH_MAP) && DT_NODE_EXISTS(DT_NODELABEL(storage_partition))
    const struct flash_area* area = nullptr;
    const int rc = flash_area_open(PARTITION_ID(storage_partition), &area);
    if (rc != 0 || area == nullptr) {
        return Result<StoragePartitionInfo>::FromStatus(
            Status::Unavailable("settings storage partition is not available"));
    }

    StoragePartitionInfo info{};
    info.label = "storage";
    info.offset = static_cast<std::uint32_t>(area->fa_off);
    info.size = static_cast<std::uint32_t>(area->fa_size);
    info.erase_block_size = 0;
    info.available = true;
    flash_area_close(area);
    return Result<StoragePartitionInfo>::FromValue(info);
#else
    return Result<StoragePartitionInfo>::FromStatus(
        Status::NotSupported("settings storage partition is not defined"));
#endif
}

StringView StorageInfo::BackendName() {
#if defined(CONFIG_SETTINGS_NVS)
    return "nvs";
#elif defined(CONFIG_SETTINGS_ZMS)
    return "zms";
#elif defined(CONFIG_SETTINGS_FCB)
    return "fcb";
#elif defined(CONFIG_SETTINGS_FILE)
    return "file";
#elif defined(CONFIG_SETTINGS)
    return "custom";
#else
    return "disabled";
#endif
}

bool StorageInfo::PersistentSettingsEnabled() {
#if defined(CONFIG_SETTINGS)
    return true;
#else
    return false;
#endif
}

}  // namespace platform
