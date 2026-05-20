cmake_minimum_required(VERSION 3.20.0)

set(APP_IDENTITY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/project.env")

function(app_load_identity file_path)
    file(STRINGS "${file_path}" identity_lines REGEX "^[A-Za-z_][A-Za-z0-9_]*=")

    foreach(identity_line IN LISTS identity_lines)
        string(REGEX REPLACE "^([^=]+)=(.*)$" "\\1" identity_key "${identity_line}")
        string(REGEX REPLACE "^([^=]+)=(.*)$" "\\2" identity_value "${identity_line}")
        string(STRIP "${identity_value}" identity_value)

        if(identity_value MATCHES "^\"(.*)\"$")
            set(identity_value "${CMAKE_MATCH_1}")
        endif()

        set(${identity_key} "${identity_value}" PARENT_SCOPE)
    endforeach()
endfunction()

function(app_read_version_var variable_name default_value output_variable)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/${APP_VERSION_FILE}" version_line
         REGEX "^${variable_name}[ \t]*=")

    if(version_line)
        list(GET version_line 0 version_line)
        string(REGEX REPLACE "^${variable_name}[ \t]*=[ \t]*(.*)$" "\\1" version_value
                             "${version_line}")
        string(REGEX REPLACE "[ \t#].*$" "" version_value "${version_value}")
        string(STRIP "${version_value}" version_value)
    else()
        set(version_value "${default_value}")
    endif()

    set(${output_variable} "${version_value}" PARENT_SCOPE)
endfunction()

app_load_identity("${APP_IDENTITY_FILE}")

app_read_version_var(VERSION_MAJOR 0 APP_VERSION_MAJOR)
app_read_version_var(VERSION_MINOR 0 APP_VERSION_MINOR)
app_read_version_var(PATCHLEVEL 0 APP_PATCHLEVEL)
app_read_version_var(VERSION_TWEAK 0 APP_VERSION_TWEAK)
app_read_version_var(EXTRAVERSION "" APP_EXTRAVERSION)

set(APP_VERSION_STRING "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_PATCHLEVEL}")
if(NOT APP_VERSION_TWEAK STREQUAL "0")
    string(APPEND APP_VERSION_STRING ".${APP_VERSION_TWEAK}")
endif()
if(NOT APP_EXTRAVERSION STREQUAL "")
    string(APPEND APP_VERSION_STRING "-${APP_EXTRAVERSION}")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(${APP_CMAKE_PROJECT_NAME} VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_PATCHLEVEL}
        LANGUAGES C CXX)

set(APP_BUILD_PROFILE "${APP_BUILD_PROFILE}" CACHE STRING "Application build profile")
set(APP_BOARD_PROFILE "${APP_BOARD_PROFILE}" CACHE STRING "Friendly board profile")
set(APP_BOOT_MODE "${APP_BOOT_MODE}" CACHE STRING "Boot mode: no-mcuboot or mcuboot")
set(APP_DISPLAY_MODE "${APP_DISPLAY_MODE}" CACHE STRING "Display mode: on, off, or auto")

if(NOT APP_BUILD_PROFILE)
    set(APP_BUILD_PROFILE "unknown")
endif()
if(NOT APP_BOARD_PROFILE)
    set(APP_BOARD_PROFILE "${BOARD}")
endif()
if(NOT APP_BOARD_PROFILE)
    set(APP_BOARD_PROFILE "unknown")
endif()
if(NOT APP_BOOT_MODE)
    set(APP_BOOT_MODE "unknown")
endif()
if(NOT APP_DISPLAY_MODE)
    set(APP_DISPLAY_MODE "auto")
endif()

set(APP_GIT_COMMIT "unknown")
set(APP_GIT_DIRTY_VALUE 0)
find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE git_commit_result
        OUTPUT_VARIABLE git_commit_output
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(git_commit_result EQUAL 0 AND NOT git_commit_output STREQUAL "")
        set(APP_GIT_COMMIT "${git_commit_output}")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" status --porcelain
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE git_status_result
        OUTPUT_VARIABLE git_status_output
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(git_status_result EQUAL 0 AND NOT git_status_output STREQUAL "")
        set(APP_GIT_DIRTY_VALUE 1)
    endif()
endif()

string(TIMESTAMP APP_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/firmware_metadata.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/include/platform/board/firmware_metadata.h"
    @ONLY
)

target_include_directories(app PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated/include"
)

add_subdirectory(platform)
add_subdirectory(services)
add_subdirectory(commands)
add_subdirectory(app)
