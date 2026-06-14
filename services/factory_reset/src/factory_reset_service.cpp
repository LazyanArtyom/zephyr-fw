#include <platform/settings/settings_store.h>
#include <services/factory_reset/factory_reset_service.h>

namespace {

constexpr platform::StringView kResetKeys[] = {
    "fw/factory-reset-pending", "user/profile", "network/ssid", "network/psk", "display/contrast",
};

}  // namespace

namespace services::factory_reset {

FactoryResetPolicy FactoryResetService::Policy() {
    return {"settings-only",
            "bootloader slots, firmware images, board serial, and hardware revision"};
}

platform::Status FactoryResetService::ResetSettings() {
#if defined(CONFIG_SETTINGS)
    const platform::Status init_status = platform::SettingsStore::Initialize();
    if (!init_status.ok()) {
        return init_status;
    }

    for (platform::StringView key : kResetKeys) {
        const platform::Status status = platform::SettingsStore::Reset(key);
        if (!status.ok() && status.code() != platform::StatusCode::kNotFound) {
            return status;
        }
    }

    return platform::SettingsStore::Save();
#else
    return platform::Status::NotSupported("settings are disabled");
#endif
}

}  // namespace services::factory_reset
