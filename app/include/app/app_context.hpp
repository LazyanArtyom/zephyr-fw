#ifndef APP_APP_CONTEXT_HPP_
#define APP_APP_CONTEXT_HPP_

#include <app/status.hpp>

namespace app {

class AppContext {
public:
    AppContext() = default;

    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;

    [[nodiscard]] Status Initialize();
    [[nodiscard]] bool IsInitialized() const;

private:
    bool initialized_{false};
};

AppContext& GetAppContext();

}  // namespace app

#endif  // APP_APP_CONTEXT_HPP_
