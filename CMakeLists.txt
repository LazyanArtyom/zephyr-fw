cmake_minimum_required(VERSION 3.20.0)

set(FW_IDENTITY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/project.env")

function(fw_load_identity file_path)
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

function(fw_read_version_var variable_name default_value output_variable)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/${FW_VERSION_FILE}" version_line
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

fw_load_identity("${FW_IDENTITY_FILE}")

fw_read_version_var(VERSION_MAJOR 0 FW_VERSION_MAJOR)
fw_read_version_var(VERSION_MINOR 0 FW_VERSION_MINOR)
fw_read_version_var(PATCHLEVEL 0 FW_PATCHLEVEL)
fw_read_version_var(VERSION_TWEAK 0 FW_VERSION_TWEAK)
fw_read_version_var(EXTRAVERSION "" FW_EXTRAVERSION)

list(APPEND BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(FW_VERSION_STRING "${FW_VERSION_MAJOR}.${FW_VERSION_MINOR}.${FW_PATCHLEVEL}")
if(NOT FW_VERSION_TWEAK STREQUAL "0")
    string(APPEND FW_VERSION_STRING ".${FW_VERSION_TWEAK}")
endif()
if(NOT FW_EXTRAVERSION STREQUAL "")
    string(APPEND FW_VERSION_STRING "-${FW_EXTRAVERSION}")
endif()

find_package(Zephyr REQUIRED HINTS "$ENV{ZEPHYR_BASE}")
project(${FW_CMAKE_PROJECT_NAME} VERSION ${FW_VERSION_MAJOR}.${FW_VERSION_MINOR}.${FW_PATCHLEVEL}
        LANGUAGES C CXX)

set(FW_BUILD_PROFILE "${FW_BUILD_PROFILE}" CACHE STRING "Application build profile")
set(FW_BOARD_PROFILE "${FW_BOARD_PROFILE}" CACHE STRING "Board profile")
set(FW_BOARD_DISPLAY_NAME "${FW_BOARD_DISPLAY_NAME}" CACHE STRING "Board display name")
set(FW_BOARD_STATUS "${FW_BOARD_STATUS}" CACHE STRING "Board profile status")
set(FW_BOARD_SERIAL_BAUD "${FW_BOARD_SERIAL_BAUD}" CACHE STRING "Default board serial baud rate")
set(FW_BOARD_FLASH_RUNNER "${FW_BOARD_FLASH_RUNNER}" CACHE STRING "Board flash runner")
set(FW_BOARD_FLASH_CHIP "${FW_BOARD_FLASH_CHIP}" CACHE STRING "Board flash chip")
set(FW_BOARD_FLASH_OFFSET "${FW_BOARD_FLASH_OFFSET}" CACHE STRING "Board flash offset")
set(FW_BOARD_DESCRIPTION "${FW_BOARD_DESCRIPTION}" CACHE STRING "Board profile description")
set(FW_BOOT_MODE "${FW_BOOT_MODE}" CACHE STRING "Boot mode: no-mcuboot or mcuboot")
set(FW_DISPLAY_MODE "${FW_DISPLAY_MODE}" CACHE STRING "Display mode: on, off, or auto")

if(NOT FW_BUILD_PROFILE)
    set(FW_BUILD_PROFILE "unknown")
endif()
if(NOT FW_BOARD_PROFILE)
    set(FW_BOARD_PROFILE "${BOARD}")
endif()
if(NOT FW_BOARD_PROFILE)
    set(FW_BOARD_PROFILE "unknown")
endif()
if(NOT FW_BOARD_DISPLAY_NAME)
    set(FW_BOARD_DISPLAY_NAME "${FW_BOARD_PROFILE}")
endif()
if(NOT FW_BOARD_STATUS)
    set(FW_BOARD_STATUS "unknown")
endif()
if(NOT FW_BOARD_SERIAL_BAUD)
    set(FW_BOARD_SERIAL_BAUD "unknown")
endif()
if(NOT FW_BOARD_FLASH_RUNNER)
    set(FW_BOARD_FLASH_RUNNER "unknown")
endif()
if(NOT FW_BOARD_FLASH_CHIP)
    set(FW_BOARD_FLASH_CHIP "unknown")
endif()
if(NOT FW_BOARD_FLASH_OFFSET)
    set(FW_BOARD_FLASH_OFFSET "unknown")
endif()
if(NOT FW_BOARD_DESCRIPTION)
    set(FW_BOARD_DESCRIPTION "")
endif()
if(NOT FW_BOOT_MODE)
    set(FW_BOOT_MODE "unknown")
endif()
if(NOT FW_DISPLAY_MODE)
    set(FW_DISPLAY_MODE "auto")
endif()

set(FW_TOOLCHAIN_VARIANT "$ENV{ZEPHYR_TOOLCHAIN_VARIANT}")
if(NOT FW_TOOLCHAIN_VARIANT)
    set(FW_TOOLCHAIN_VARIANT "unknown")
endif()

set(FW_ZEPHYR_SDK_INSTALL_DIR "$ENV{ZEPHYR_SDK_INSTALL_DIR}")
if(NOT FW_ZEPHYR_SDK_INSTALL_DIR AND DEFINED ZEPHYR_SDK_INSTALL_DIR)
    set(FW_ZEPHYR_SDK_INSTALL_DIR "${ZEPHYR_SDK_INSTALL_DIR}")
endif()
if(NOT FW_ZEPHYR_SDK_INSTALL_DIR)
    set(FW_ZEPHYR_SDK_INSTALL_DIR "unknown")
endif()

set(FW_CXX_COMPILER_ID "${CMAKE_CXX_COMPILER_ID}")
if(NOT FW_CXX_COMPILER_ID)
    set(FW_CXX_COMPILER_ID "unknown")
endif()

set(FW_CXX_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
if(NOT FW_CXX_COMPILER_VERSION)
    set(FW_CXX_COMPILER_VERSION "unknown")
endif()

set(FW_GIT_COMMIT "unknown")
set(FW_GIT_DIRTY_VALUE 0)
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
        set(FW_GIT_COMMIT "${git_commit_output}")
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
        set(FW_GIT_DIRTY_VALUE 1)
    endif()
endif()

string(TIMESTAMP FW_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/firmware_metadata.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/include/platform/board/firmware_metadata.h"
    @ONLY
)

target_include_directories(app PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated/include"
)

target_compile_features(app PRIVATE cxx_std_20)
target_compile_options(app PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

add_subdirectory(platform)
add_subdirectory(services)
add_subdirectory(commands)
add_subdirectory(app)
