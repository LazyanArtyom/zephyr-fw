#ifndef SERVICES_HEALTH_HEALTH_SERVICE_H_
#define SERVICES_HEALTH_HEALTH_SERVICE_H_

#include <platform/core/status.h>
#include <platform/core/string_view.h>
#include <platform/settings/settings_store.h>

namespace services::health {

struct ManufacturingValue {
    platform::StringView key;
    platform::SettingValue value;
    bool present{false};
};

class HealthService final {
   public:
    [[nodiscard]] static platform::Status OverallStatus();
};

class ManufacturingService final {
   public:
    [[nodiscard]] static ManufacturingValue BoardSerial();
    [[nodiscard]] static platform::Status SetBoardSerial(platform::StringView value);
    [[nodiscard]] static ManufacturingValue BoardHardwareRevision();
    [[nodiscard]] static platform::Status SetBoardHardwareRevision(platform::StringView value);
};

}  // namespace services::health

#endif  // SERVICES_HEALTH_HEALTH_SERVICE_H_
