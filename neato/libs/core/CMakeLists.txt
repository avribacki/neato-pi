cmake_minimum_required(VERSION 2.8.12)

# Create Core library

set(core_headers
  "include/neato_robot.hpp"
  "include/neato_serial_port.hpp"
)

set(core_sources
  "src/neato_core.cpp"
  "src/neato_robot.cpp"
  "src/neato_serial_port.cpp"
)

source_group("Include" FILES ${neato_headers} ${core_headers})
source_group("Source" FILES ${core_sources})

add_library(neato_core STATIC
  ${neato_headers}
  ${core_headers}
  ${core_sources}
)

target_link_libraries(neato_core PRIVATE jaw_common)

target_include_directories(neato_core
  PUBLIC ${Neato_INCLUDE_DIRS}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
