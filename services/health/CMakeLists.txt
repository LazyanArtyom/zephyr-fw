target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources_ifdef(CONFIG_FW_HEALTH_HEARTBEAT app PRIVATE
    src/heartbeat_service.cpp
)
