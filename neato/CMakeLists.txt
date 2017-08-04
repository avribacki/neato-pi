cmake_minimum_required(VERSION 2.8.12)

project(Neato)

# neato_api.h is implemented by neato_core and neato_client libraries
# neato_server.h is implemented by neato_server library
set(neato_headers
  "${PROJECT_SOURCE_DIR}/include/neato_api.h"
  "${PROJECT_SOURCE_DIR}/include/neato_server.h"
  "${PROJECT_SOURCE_DIR}/include/neato_defines.h"
)

set(Neato_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)

# Create libraries that compose Neato Firmware

add_subdirectory(libs/core)
add_subdirectory(libs/protocol)
add_subdirectory(libs/client)
add_subdirectory(libs/server)

add_subdirectory(apps/daemon)
add_subdirectory(apps/demo)

