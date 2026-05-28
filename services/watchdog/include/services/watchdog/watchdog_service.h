#ifndef SERVICES_WATCHDOG_WATCHDOG_SERVICE_H_
#define SERVICES_WATCHDOG_WATCHDOG_SERVICE_H_

#include <platform/core/string_view.h>

namespace services::watchdog {

struct WatchdogStatus {
    bool configured{false};
    bool device_ready{false};
    platform::StringView device_name;
    platform::StringView mode;
};

class WatchdogService final {
   public:
    [[nodiscard]] static WatchdogStatus Status();
};

}  // namespace services::watchdog

#endif  // SERVICES_WATCHDOG_WATCHDOG_SERVICE_H_
