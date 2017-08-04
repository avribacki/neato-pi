cmake_minimum_required(VERSION 2.8.12)

# Create client library that implements neato_api.h

set(client_sources
  "src/neato_client.cpp"
)

source_group("Include" FILES ${neato_headers} ${neato_protocol_headers})
source_group("Source" FILES ${client_sources})

add_library(neato_client STATIC
  ${neato_headers}
  ${neato_protocol_headers}
  ${client_sources}
)

target_link_libraries(neato_client PRIVATE neato_protocol)

target_include_directories(neato_client
  PUBLIC ${Neato_INCLUDE_DIRS}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
