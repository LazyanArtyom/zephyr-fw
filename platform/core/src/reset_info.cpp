#include <platform/core/reset_info.h>
#include <zephyr/sys/reboot.h>

namespace platform {

ResetInfo ResetInfo::Current() {
    return ResetInfo{};
}

void ResetInfo::RequestColdReboot() {
    sys_reboot(SYS_REBOOT_COLD);
}

StringView ResetInfo::reason_text() const {
    switch (reason_) {
        case Reason::kUnknown:
            return "unknown";
    }

    return "unknown";
}

}  // namespace platform
