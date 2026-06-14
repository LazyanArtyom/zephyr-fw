#ifndef PLATFORM_BOARD_IDENTITY_BOARD_IDENTITY_STORE_H_
#define PLATFORM_BOARD_IDENTITY_BOARD_IDENTITY_STORE_H_

#include <platform/core/status.h>
#include <platform/core/string_view.h>
#include <platform/settings/settings_store.h>

namespace platform {

struct BoardIdentityValue {
    StringView key;
    SettingValue value;
    bool present{false};
};

class BoardIdentityStore final {
   public:
    [[nodiscard]] static BoardIdentityValue BoardSerial();
    [[nodiscard]] static Status SetBoardSerial(StringView value);
    [[nodiscard]] static BoardIdentityValue BoardHardwareRevision();
    [[nodiscard]] static Status SetBoardHardwareRevision(StringView value);
};

}  // namespace platform

#endif  // PLATFORM_BOARD_IDENTITY_BOARD_IDENTITY_STORE_H_
