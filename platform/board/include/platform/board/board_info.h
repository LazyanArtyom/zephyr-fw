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

    void LogBootSummary() const;
};

}  // namespace platform

#endif  // PLATFORM_BOARD_BOARD_INFO_H_
