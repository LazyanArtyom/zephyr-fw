#include "display_service.hpp"

#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_service, CONFIG_LOG_DEFAULT_LEVEL);

namespace app {
namespace {

const device* GetDisplayDevice()
{
    return DEVICE_DT_GET(DT_ALIAS(display0));
}

}  // namespace

bool InitializeDisplay()
{
    const device* display = GetDisplayDevice();

    if (!device_is_ready(display)) {
        LOG_ERR("display device is not ready");
        return false;
    }

    if (cfb_framebuffer_init(display) != 0) {
        LOG_ERR("failed to initialize character framebuffer");
        return false;
    }

    cfb_framebuffer_clear(display, true);
    cfb_framebuffer_invert(display);
    cfb_framebuffer_finalize(display);

    LOG_INF("display initialized");
    return true;
}

bool ShowHelloWorldOnDisplay()
{
    const device* display = GetDisplayDevice();

    if (!device_is_ready(display)) {
        LOG_ERR("display device is not ready");
        return false;
    }

    cfb_framebuffer_clear(display, false);

    cfb_print(display, "Zephyr FW", 0, 0);
    cfb_print(display, "Hello World!", 0, 16);
    cfb_print(display, "ESP32 OLED", 0, 32);
    cfb_print(display, "v0.1.0 debug", 0, 48);

    cfb_framebuffer_finalize(display);
    return true;
}

}  // namespace app
