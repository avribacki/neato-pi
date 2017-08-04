cmake_minimum_required(VERSION 2.8.12)

# Create Protocol Library

set(protocol_headers
  "${CMAKE_CURRENT_SOURCE_DIR}/include/neato_protocol.hpp"
)

# As Neato protocol contains only headers, declare it as interface.
add_library(neato_protocol INTERFACE)

# Expose dependency on jaw_network
target_link_libraries(neato_protocol INTERFACE jaw_network)

target_include_directories(neato_protocol
  INTERFACE ${Neato_INCLUDE_DIRS}
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

# Save headers in CACHE so they can be included by other projects.
set(neato_protocol_headers ${protocol_headers} CACHE INTERNAL "Neato Protocol Headers")
