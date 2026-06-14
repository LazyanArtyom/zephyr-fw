#include <services/manufacturing/manufacturing_service.h>

namespace services::manufacturing {

ManufacturingValue ManufacturingService::BoardSerial() {
    return platform::BoardIdentityStore::BoardSerial();
}

platform::Status ManufacturingService::SetBoardSerial(platform::StringView value) {
    return platform::BoardIdentityStore::SetBoardSerial(value);
}

ManufacturingValue ManufacturingService::BoardHardwareRevision() {
    return platform::BoardIdentityStore::BoardHardwareRevision();
}

platform::Status ManufacturingService::SetBoardHardwareRevision(platform::StringView value) {
    return platform::BoardIdentityStore::SetBoardHardwareRevision(value);
}

}  // namespace services::manufacturing
