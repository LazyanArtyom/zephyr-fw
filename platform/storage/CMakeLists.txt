target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(app PRIVATE
    src/storage_info.cpp
)
