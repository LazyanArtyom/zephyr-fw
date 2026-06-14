#ifndef SERVICES_MANUFACTURING_MANUFACTURING_SERVICE_H_
#define SERVICES_MANUFACTURING_MANUFACTURING_SERVICE_H_

#include <platform/board_identity/board_identity_store.h>
#include <platform/core/status.h>
#include <platform/core/string_view.h>

namespace services::manufacturing {

using ManufacturingValue = platform::BoardIdentityValue;

class ManufacturingService final {
   public:
    [[nodiscard]] static ManufacturingValue BoardSerial();
    [[nodiscard]] static platform::Status SetBoardSerial(platform::StringView value);
    [[nodiscard]] static ManufacturingValue BoardHardwareRevision();
    [[nodiscard]] static platform::Status SetBoardHardwareRevision(platform::StringView value);
};

}  // namespace services::manufacturing

#endif  // SERVICES_MANUFACTURING_MANUFACTURING_SERVICE_H_
