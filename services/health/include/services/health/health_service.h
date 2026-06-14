#ifndef SERVICES_HEALTH_HEALTH_SERVICE_H_
#define SERVICES_HEALTH_HEALTH_SERVICE_H_

#include <platform/core/status.h>
namespace services::health {

class HealthService final {
   public:
    [[nodiscard]] static platform::Status OverallStatus();
};

}  // namespace services::health

#endif  // SERVICES_HEALTH_HEALTH_SERVICE_H_
