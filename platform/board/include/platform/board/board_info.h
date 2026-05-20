#ifndef PLATFORM_BOARD_BOARD_INFO_H_
#define PLATFORM_BOARD_BOARD_INFO_H_

namespace platform::board {

const char* GetDisplayName();
const char* GetFirmwareName();
const char* GetFirmwareSlug();
const char* GetVendorName();
const char* GetBoardName();
const char* GetBoardProfile();
const char* GetZephyrBoardTarget();
const char* GetFirmwareVersion();
const char* GetBuildProfile();
const char* GetBootMode();
const char* GetDisplayMode();
const char* GetGitCommit();
const char* GetBuildTimestamp();
bool IsGitDirty();

}  // namespace platform::board

#endif  // PLATFORM_BOARD_BOARD_INFO_H_
