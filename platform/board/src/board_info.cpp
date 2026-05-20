#include <platform/board/board_info.h>
#include <platform/board/firmware_metadata.h>

namespace platform::board {

const char* GetDisplayName() {
    return FW_META_DISPLAY_NAME;
}

const char* GetFirmwareName() {
    return FW_META_FIRMWARE_NAME;
}

const char* GetFirmwareSlug() {
    return FW_META_SLUG;
}

const char* GetVendorName() {
    return FW_META_VENDOR;
}

const char* GetBoardName() {
    return GetBoardProfile();
}

const char* GetBoardProfile() {
    return FW_META_BOARD_PROFILE;
}

const char* GetZephyrBoardTarget() {
    return CONFIG_BOARD_TARGET;
}

const char* GetFirmwareVersion() {
    return FW_META_VERSION_STRING;
}

const char* GetBuildProfile() {
    return FW_META_BUILD_PROFILE;
}

const char* GetBootMode() {
    return FW_META_BOOT_MODE;
}

const char* GetDisplayMode() {
    return FW_META_DISPLAY_MODE;
}

const char* GetGitCommit() {
    return FW_META_GIT_COMMIT;
}

const char* GetBuildTimestamp() {
    return FW_META_BUILD_TIMESTAMP;
}

bool IsGitDirty() {
    return FW_META_GIT_DIRTY != 0;
}

}  // namespace platform::board
