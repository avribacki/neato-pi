cmake_minimum_required(VERSION 2.8.12)

# Create Server Library

set(server_sources
  "src/neato_server.cpp"
)

source_group("Include" FILES ${neato_protocol_headers})
source_group("Source" FILES ${server_sources})

add_library(neato_server STATIC
  ${neato_protocol_headers}
  ${server_sources}
)

target_link_libraries(neato_server neato_protocol)
target_link_libraries(neato_server neato_core)

target_include_directories(neato_server
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
