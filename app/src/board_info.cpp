#include <app/app_metadata.h>
#include <app/board_info.h>

namespace app {

const char* GetDisplayName() {
    return APP_META_DISPLAY_NAME;
}

const char* GetFirmwareName() {
    return APP_META_FIRMWARE_NAME;
}

const char* GetFirmwareSlug() {
    return APP_META_SLUG;
}

const char* GetBoardName() {
    return GetBoardProfile();
}

const char* GetBoardProfile() {
    return APP_META_BOARD_PROFILE;
}

const char* GetZephyrBoardTarget() {
    return CONFIG_BOARD_TARGET;
}

const char* GetAppVersion() {
    return APP_META_VERSION_STRING;
}

const char* GetBuildProfile() {
    return APP_META_BUILD_PROFILE;
}

const char* GetBootMode() {
    return APP_META_BOOT_MODE;
}

const char* GetAppProfile() {
    return APP_META_APP_PROFILE;
}

const char* GetGitCommit() {
    return APP_META_GIT_COMMIT;
}

const char* GetBuildTimestamp() {
    return APP_META_BUILD_TIMESTAMP;
}

bool IsGitDirty() {
    return APP_META_GIT_DIRTY != 0;
}

}  // namespace app
