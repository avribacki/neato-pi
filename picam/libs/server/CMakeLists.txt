cmake_minimum_required(VERSION 2.8.12)

# Create Server Library

set(server_sources
  "src/picam_server.cpp"
)

source_group("Include" FILES ${picam_protocol_headers})
source_group("Source" FILES ${server_sources})

add_library(picam_server STATIC
  ${picam_protocol_headers}
  ${server_sources}
)

target_link_libraries(picam_server picam_protocol)
target_link_libraries(picam_server picam_core)

target_include_directories(picam_server
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
