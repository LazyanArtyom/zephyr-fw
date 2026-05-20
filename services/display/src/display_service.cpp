#include <platform/board/board_info.h>
#include <services/display/display_service.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_service, CONFIG_LOG_DEFAULT_LEVEL);

namespace services::display {
namespace {

const device* GetDisplayDevice() {
    return DEVICE_DT_GET(DT_ALIAS(display0));
}

}  // namespace

bool InitializeDisplay() {
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

bool ShowBootSummary() {
    const device* display = GetDisplayDevice();

    if (!device_is_ready(display)) {
        LOG_ERR("display device is not ready");
        return false;
    }

    cfb_framebuffer_clear(display, false);

    cfb_print(display, platform::board::GetDisplayName(), 0, 0);
    cfb_print(display, platform::board::GetBoardProfile(), 0, 16);
    cfb_print(display, platform::board::GetFirmwareVersion(), 0, 32);
    cfb_print(display, platform::board::GetBuildProfile(), 0, 48);

    cfb_framebuffer_finalize(display);
    return true;
}

}  // namespace services::display
