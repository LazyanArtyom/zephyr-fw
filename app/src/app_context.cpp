#include <app/app_context.hpp>

namespace app {

Status AppContext::Initialize()
{
    initialized_ = true;
    return Status::kOk;
}

bool AppContext::IsInitialized() const
{
    return initialized_;
}

AppContext& GetAppContext()
{
    static AppContext context;
    return context;
}

}  // namespace app
