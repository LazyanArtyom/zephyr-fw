target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/services
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
)

target_sources(app PRIVATE
    src/main.cpp
    src/app_context.cpp
    src/board_info.cpp
)

add_subdirectory(services)
add_subdirectory(tools)
add_subdirectory(shell)
