target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources_ifdef(CONFIG_I2C app PRIVATE
    src/i2c_scanner.cpp
)
