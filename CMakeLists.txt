add_library(pico_pcf8523 INTERFACE)

target_sources(pico_pcf8523 INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/pico_pcf8523.cpp
)

target_include_directories(pico_pcf8523 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_LIST_DIR}/include)

# Pull in pico libraries that we need
target_link_libraries(pico_pcf8523 INTERFACE pico_stdlib hardware_i2c)
