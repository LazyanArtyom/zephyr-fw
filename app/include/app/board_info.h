#ifndef APP_BOARD_INFO_H_
#define APP_BOARD_INFO_H_

namespace app {

const char* GetDisplayName();
const char* GetFirmwareName();
const char* GetFirmwareSlug();
const char* GetBoardName();
const char* GetBoardProfile();
const char* GetZephyrBoardTarget();
const char* GetAppVersion();
const char* GetBuildProfile();
const char* GetBootMode();
const char* GetAppProfile();
const char* GetGitCommit();
const char* GetBuildTimestamp();
bool IsGitDirty();

}  // namespace app

#endif  // APP_BOARD_INFO_H_
