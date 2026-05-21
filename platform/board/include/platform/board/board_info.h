#ifndef PLATFORM_BOARD_BOARD_INFO_H_
#define PLATFORM_BOARD_BOARD_INFO_H_

#include <platform/core/string_view.h>

namespace platform {

class BoardInfo final {
   public:
    constexpr BoardInfo() = default;

    [[nodiscard]] static const BoardInfo& Current();

    [[nodiscard]] StringView display_name() const;
    [[nodiscard]] StringView firmware_name() const;
    [[nodiscard]] StringView firmware_slug() const;
    [[nodiscard]] StringView vendor_name() const;
    [[nodiscard]] StringView board_name() const;
    [[nodiscard]] StringView board_profile() const;
    [[nodiscard]] StringView board_status() const;
    [[nodiscard]] StringView board_serial_baud() const;
    [[nodiscard]] StringView board_description() const;
    [[nodiscard]] StringView zephyr_board_target() const;
    [[nodiscard]] StringView flash_runner() const;
    [[nodiscard]] StringView flash_chip() const;
    [[nodiscard]] StringView flash_offset() const;
    [[nodiscard]] StringView firmware_version() const;
    [[nodiscard]] StringView build_profile() const;
    [[nodiscard]] StringView boot_mode() const;
    [[nodiscard]] StringView display_mode() const;
    [[nodiscard]] StringView git_commit() const;
    [[nodiscard]] StringView build_timestamp() const;
    [[nodiscard]] bool is_git_dirty() const;

    [[nodiscard]] StringView zephyr_version() const;
    [[nodiscard]] StringView toolchain_variant() const;
    [[nodiscard]] StringView zephyr_sdk_install_dir() const;
    [[nodiscard]] StringView compiler_id() const;
    [[nodiscard]] StringView compiler_version() const;

    [[nodiscard]] StringView architecture() const;
    [[nodiscard]] StringView soc_name() const;
    [[nodiscard]] StringView soc_series() const;
    [[nodiscard]] StringView soc_family() const;
    [[nodiscard]] StringView soc_part_number() const;

    [[nodiscard]] bool shell_enabled() const;
    [[nodiscard]] bool display_enabled() const;
    [[nodiscard]] bool i2c_enabled() const;
    [[nodiscard]] bool settings_enabled() const;
    [[nodiscard]] bool flash_enabled() const;
    [[nodiscard]] bool mcuboot_enabled() const;
    [[nodiscard]] StringView settings_backend() const;
    [[nodiscard]] StringView settings_backend_status() const;
    [[nodiscard]] StringView storage_partition_status() const;

    void LogBootSummary() const;
};

}  // namespace platform

#endif  // PLATFORM_BOARD_BOARD_INFO_H_
