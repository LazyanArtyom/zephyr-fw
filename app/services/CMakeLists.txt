target_sources_ifdef(CONFIG_APP_HEARTBEAT_SERVICE app PRIVATE
    heartbeat_service.cpp
)

target_sources_ifdef(CONFIG_APP_DISPLAY app PRIVATE
    display_service.cpp
)
