#include <platform/board/board_info.h>
#include <platform/core/device_ref.h>
#include <services/display/display_service.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_service, CONFIG_LOG_DEFAULT_LEVEL);

namespace services::display {
namespace {

platform::DeviceRef GetDisplayDevice() {
    return platform::DeviceRef{DEVICE_DT_GET(DT_ALIAS(display0))};
}

}  // namespace

bool InitializeDisplay() {
    const platform::DeviceRef display = GetDisplayDevice();

    if (!display.is_ready()) {
        LOG_ERR("display device is not ready");
        return false;
    }

    const device* display_handle = display.native_handle();

    if (cfb_framebuffer_init(display_handle) != 0) {
        LOG_ERR("failed to initialize character framebuffer");
        return false;
    }

    cfb_framebuffer_clear(display_handle, true);
    cfb_framebuffer_invert(display_handle);
    cfb_framebuffer_finalize(display_handle);

    LOG_INF("display initialized");
    return true;
}

bool ShowBootSummary() {
    const platform::DeviceRef display = GetDisplayDevice();

    if (!display.is_ready()) {
        LOG_ERR("display device is not ready");
        return false;
    }

    const device* display_handle = display.native_handle();
    const platform::BoardInfo& board_info = platform::BoardInfo::Current();

    cfb_framebuffer_clear(display_handle, false);

    cfb_print(display_handle, board_info.display_name(), 0, 0);
    cfb_print(display_handle, board_info.board_profile(), 0, 16);
    cfb_print(display_handle, board_info.firmware_version(), 0, 32);
    cfb_print(display_handle, board_info.build_profile(), 0, 48);

    cfb_framebuffer_finalize(display_handle);
    return true;
}

}  // namespace services::display
