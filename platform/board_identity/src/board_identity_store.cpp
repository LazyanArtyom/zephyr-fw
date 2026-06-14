#include <platform/board_identity/board_identity_store.h>

namespace {

platform::BoardIdentityValue ReadIdentityValue(platform::StringView key) {
    platform::BoardIdentityValue output{};
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

platform::Status WriteIdentityValue(platform::StringView key, platform::StringView value) {
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

namespace platform {

BoardIdentityValue BoardIdentityStore::BoardSerial() {
    return ReadIdentityValue("board/serial");
}

Status BoardIdentityStore::SetBoardSerial(StringView value) {
    return WriteIdentityValue("board/serial", value);
}

BoardIdentityValue BoardIdentityStore::BoardHardwareRevision() {
    return ReadIdentityValue("board/hw-rev");
}

Status BoardIdentityStore::SetBoardHardwareRevision(StringView value) {
    return WriteIdentityValue("board/hw-rev", value);
}

}  // namespace platform
