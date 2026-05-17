#include <app/board_info.hpp>

#ifndef APP_VERSION
#define APP_VERSION "0.1.0"
#endif

#ifndef APP_BUILD_PROFILE
#define APP_BUILD_PROFILE "unknown"
#endif

namespace app {

const char* GetBoardName()
{
    return CONFIG_BOARD_TARGET;
}

const char* GetAppVersion()
{
    return APP_VERSION;
}

const char* GetBuildProfile()
{
    return APP_BUILD_PROFILE;
}

}  // namespace app
