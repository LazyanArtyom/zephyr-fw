target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources_ifdef(CONFIG_FW_SERVICE_DIAGNOSTICS app PRIVATE
    src/diagnostics_service.cpp
)
