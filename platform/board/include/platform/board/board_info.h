#ifndef PLATFORM_BOARD_BOARD_INFO_H_
#define PLATFORM_BOARD_BOARD_INFO_H_

namespace platform {

class BoardInfo final {
   public:
    constexpr BoardInfo() = default;

    [[nodiscard]] static const BoardInfo& Current();

    [[nodiscard]] const char* display_name() const;
    [[nodiscard]] const char* firmware_name() const;
    [[nodiscard]] const char* firmware_slug() const;
    [[nodiscard]] const char* vendor_name() const;
    [[nodiscard]] const char* board_name() const;
    [[nodiscard]] const char* board_profile() const;
    [[nodiscard]] const char* zephyr_board_target() const;
    [[nodiscard]] const char* firmware_version() const;
    [[nodiscard]] const char* build_profile() const;
    [[nodiscard]] const char* boot_mode() const;
    [[nodiscard]] const char* display_mode() const;
    [[nodiscard]] const char* git_commit() const;
    [[nodiscard]] const char* build_timestamp() const;
    [[nodiscard]] bool is_git_dirty() const;

    [[nodiscard]] const char* zephyr_version() const;
    [[nodiscard]] const char* toolchain_variant() const;
    [[nodiscard]] const char* zephyr_sdk_install_dir() const;
    [[nodiscard]] const char* compiler_id() const;
    [[nodiscard]] const char* compiler_version() const;

    [[nodiscard]] const char* architecture() const;
    [[nodiscard]] const char* soc_name() const;
    [[nodiscard]] const char* soc_series() const;
    [[nodiscard]] const char* soc_family() const;
    [[nodiscard]] const char* soc_part_number() const;

    [[nodiscard]] bool shell_enabled() const;
    [[nodiscard]] bool display_enabled() const;
    [[nodiscard]] bool i2c_enabled() const;
    [[nodiscard]] bool settings_enabled() const;
    [[nodiscard]] bool flash_enabled() const;
    [[nodiscard]] bool mcuboot_enabled() const;
    [[nodiscard]] const char* settings_backend() const;
    [[nodiscard]] const char* settings_backend_status() const;
    [[nodiscard]] const char* storage_partition_status() const;

    void LogBootSummary() const;
};

}  // namespace platform

#endif  // PLATFORM_BOARD_BOARD_INFO_H_
