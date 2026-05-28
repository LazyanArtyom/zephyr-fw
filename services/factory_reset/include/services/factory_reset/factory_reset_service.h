#ifndef SERVICES_FACTORY_RESET_FACTORY_RESET_SERVICE_H_
#define SERVICES_FACTORY_RESET_FACTORY_RESET_SERVICE_H_

#include <platform/core/status.h>
#include <platform/core/string_view.h>

namespace services::factory_reset {

struct FactoryResetPolicy {
    platform::StringView behavior;
    platform::StringView preserves;
};

class FactoryResetService final {
   public:
    [[nodiscard]] static FactoryResetPolicy Policy();
    [[nodiscard]] static platform::Status ResetSettings();
};

}  // namespace services::factory_reset

#endif  // SERVICES_FACTORY_RESET_FACTORY_RESET_SERVICE_H_
