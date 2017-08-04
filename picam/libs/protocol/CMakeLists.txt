cmake_minimum_required(VERSION 2.8.12)

# Create Protocol Library

set(protocol_headers
  "${CMAKE_CURRENT_SOURCE_DIR}/include/picam_protocol.hpp"
)

# As PiCam protocol contains only headers, declare it as interface.
add_library(picam_protocol INTERFACE)

# Expose dependency on jaw_network
target_link_libraries(picam_protocol INTERFACE jaw_network)

target_include_directories(picam_protocol
  INTERFACE ${PiCam_INCLUDE_DIRS}
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

# Save headers in CACHE so they can be included by other projects.
set(picam_protocol_headers ${protocol_headers} CACHE INTERNAL "PiCam Protocol Headers")
