#include <platform/core/device_ref.h>
#include <zephyr/device.h>

namespace platform {

bool DeviceRef::is_ready() const {
    return device_ != nullptr && device_is_ready(device_);
}

}  // namespace platform
