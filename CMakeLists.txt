cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr_golden_fw LANGUAGES C CXX)

# Project version shown by app shell commands.
# Keep this synchronized with VERSION.
set(APP_VERSION_STRING "0.1.0")

target_compile_definitions(app PRIVATE
    APP_VERSION="${APP_VERSION_STRING}"
    APP_BUILD_PROFILE="${APP_BUILD_PROFILE}"
)

add_subdirectory(app)
