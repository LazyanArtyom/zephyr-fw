target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources_ifdef(CONFIG_FW_DISPLAY app PRIVATE
    src/display_service.cpp
)
