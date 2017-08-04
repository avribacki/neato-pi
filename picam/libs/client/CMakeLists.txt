cmake_minimum_required(VERSION 2.8.12)

# Create client library that implements picam_api.h

set(client_sources
  "src/picam_client.cpp"
)

source_group("Include" FILES ${picam_headers} ${picam_protocol_headers})
source_group("Source" FILES ${picam_sources})

add_library(picam_client STATIC
  ${picam_headers}
  ${picam_protocol_headers}
  ${client_sources}
)

target_link_libraries(picam_client PRIVATE picam_protocol)

target_include_directories(picam_client
  PUBLIC ${PiCam_INCLUDE_DIRS}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
