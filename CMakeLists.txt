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

function(fw_resolve_metadata variable_name cache_description fallback_value)
    zephyr_get(resolved_value VAR "${variable_name}")
    if(NOT resolved_value)
        set(resolved_value "${fallback_value}")
    endif()

    set("${variable_name}" "${resolved_value}" CACHE STRING "${cache_description}" FORCE)
    set("${variable_name}" "${resolved_value}" PARENT_SCOPE)
endfunction()

fw_resolve_metadata(FW_BUILD_PROFILE "Application build profile" "unknown")
fw_resolve_metadata(FW_BOARD_PROFILE "Board profile" "${BOARD}")
fw_resolve_metadata(FW_BOARD_DISPLAY_NAME "Board display name" "${FW_BOARD_PROFILE}")
fw_resolve_metadata(FW_BOARD_STATUS "Board profile status" "unknown")
fw_resolve_metadata(FW_BOARD_SERIAL_BAUD "Default board serial baud rate" "unknown")
fw_resolve_metadata(FW_BOARD_FLASH_RUNNER "Board flash runner" "unknown")
fw_resolve_metadata(FW_BOARD_FLASH_CHIP "Board flash chip" "unknown")
fw_resolve_metadata(FW_BOARD_FLASH_OFFSET "Board flash offset" "unknown")
fw_resolve_metadata(FW_BOARD_DESCRIPTION "Board profile description" "")
fw_resolve_metadata(FW_BOOT_MODE "Boot mode: no-mcuboot or mcuboot" "unknown")
fw_resolve_metadata(FW_DISPLAY_MODE "Display mode: on, off, or auto" "auto")

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
