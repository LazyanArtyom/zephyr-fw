#include <services/watchdog/watchdog_service.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

namespace services::watchdog {

WatchdogStatus WatchdogService::Status() {
    WatchdogStatus status{};
#if defined(CONFIG_WATCHDOG) && DT_NODE_HAS_STATUS(DT_ALIAS(watchdog0), okay)
    const struct device* watchdog = DEVICE_DT_GET(DT_ALIAS(watchdog0));
    status.configured = true;
    status.device_ready = device_is_ready(watchdog);
    status.device_name = watchdog->name;
    status.mode = "hardware";
#elif defined(CONFIG_WATCHDOG)
    status.configured = true;
    status.device_ready = false;
    status.device_name = "watchdog0";
    status.mode = "missing-alias";
#else
    status.configured = false;
    status.device_ready = false;
    status.device_name = "disabled";
    status.mode = "disabled";
#endif
    return status;
}

}  // namespace services::watchdog
