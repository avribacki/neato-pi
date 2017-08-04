cmake_minimum_required(VERSION 2.8.12)

project(PiCam)

# picam_api.h is implemented by picam_core and picam_client libraries
# picam_server.h is implemented by picam_server library
set(picam_headers
  "${PROJECT_SOURCE_DIR}/include/picam_api.h"
  "${PROJECT_SOURCE_DIR}/include/picam_server.h"
  "${PROJECT_SOURCE_DIR}/include/picam_defines.h"
)

set(PiCam_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)

# Create libraries that compose PiCam Library

add_subdirectory(libs/core)
add_subdirectory(libs/protocol)
add_subdirectory(libs/client)
add_subdirectory(libs/server)

add_subdirectory(apps/daemon)
add_subdirectory(apps/demo)

