#include <platform/board/board_info.h>
#include <platform/board/firmware_metadata.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(platform_board_info, CONFIG_LOG_DEFAULT_LEVEL);

namespace {

constexpr platform::BoardInfo kCurrentBoardInfo{};

}  // namespace

namespace platform {

const BoardInfo& BoardInfo::Current() {
    return kCurrentBoardInfo;
}

const char* BoardInfo::display_name() const {
    return FW_META_DISPLAY_NAME;
}

const char* BoardInfo::firmware_name() const {
    return FW_META_FIRMWARE_NAME;
}

const char* BoardInfo::firmware_slug() const {
    return FW_META_SLUG;
}

const char* BoardInfo::vendor_name() const {
    return FW_META_VENDOR;
}

const char* BoardInfo::board_name() const {
    return board_profile();
}

const char* BoardInfo::board_profile() const {
    return FW_META_BOARD_PROFILE;
}

const char* BoardInfo::zephyr_board_target() const {
    return CONFIG_BOARD_TARGET;
}

const char* BoardInfo::firmware_version() const {
    return FW_META_VERSION_STRING;
}

const char* BoardInfo::build_profile() const {
    return FW_META_BUILD_PROFILE;
}

const char* BoardInfo::boot_mode() const {
    return FW_META_BOOT_MODE;
}

const char* BoardInfo::display_mode() const {
    return FW_META_DISPLAY_MODE;
}

const char* BoardInfo::git_commit() const {
    return FW_META_GIT_COMMIT;
}

const char* BoardInfo::build_timestamp() const {
    return FW_META_BUILD_TIMESTAMP;
}

bool BoardInfo::is_git_dirty() const {
    return FW_META_GIT_DIRTY != 0;
}

void BoardInfo::LogBootSummary() const {
    LOG_INF("%s booted", display_name());
    LOG_INF("Board profile: %s", board_profile());
    LOG_INF("Zephyr board: %s", zephyr_board_target());
    LOG_INF("Version: %s%s", firmware_version(), is_git_dirty() ? "-dirty" : "");
    LOG_INF("Build profile: %s, boot: %s", build_profile(), boot_mode());
}

}  // namespace platform
