cmake_minimum_required(VERSION 2.8.12)

# Create Core library

# TODO: Move me to FindMMAL.cmake
# If we are building for Raspberry, use local MMAL libraries
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES arm*)

  # Find required libraries and headers.
  find_library(MMAL_LIB NAMES mmal PATHS /opt/vc/lib)
  find_library(MMAL_CORE_LIB NAMES mmal_core PATHS /opt/vc/lib)
  find_library(MMAL_UTIL_LIB NAMES mmal_util PATHS /opt/vc/lib)
  find_library(MMAL_VC_CLIENT_LIB NAMES mmal_vc_client PATHS /opt/vc/lib)
  find_library(BCM_HOST_LIB NAMES bcm_host PATHS /opt/vc/lib)
  find_path(MMAL_INCLUDE_DIR NAMES interface/mmal/mmal.h PATHS /opt/vc/include)

  # Test only for MMAL_LIB. Assume that if it was found so was the others.
  # TODO: Maybe test everything just to be sure :)
  if((MMAL_LIB) AND (MMAL_INCLUDE_DIR))
    set(MMAL_LIBRARIES
       ${MMAL_LIB}
       ${MMAL_CORE_LIB}
       ${MMAL_UTIL_LIB}
       ${MMAL_VC_CLIENT_LIB}
       ${BCM_HOST_LIB}
    )
    set(MMAL_INCLUDE_DIRS
      ${MMAL_INCLUDE_DIR}
      ${MMAL_INCLUDE_DIR}/interface/vcos/pthreads
      ${MMAL_INCLUDE_DIR}/interface/vmcs_host/linux
    )
    set(MMAL_FOUND TRUE)
    message("Found MMAL: TRUE")
  else()
    message("Found MMAL: FALSE")
  endif()

endif()

set(core_headers
  "include/picam_camera.hpp"
)

set(core_sources
  "src/picam_core.cpp"
  "src/picam_camera.cpp"
  "src/picam_camera_impl.hpp"
)

# Use MMAL for camera implementation if it was found
if(MMAL_FOUND)
  set(core_sources ${core_sources}
    "src/picam_camera_mmal.cpp"
    "src/picam_camera_mmal.hpp"
  )
# Othewise, use dummy camera that generates random frames
else()
  set(core_sources ${core_sources}
    "src/picam_camera_dummy.cpp"
    "src/picam_camera_dummy.hpp"
  )
endif()

source_group("Include" FILES ${picam_headers} ${core_headers})
source_group("Source" FILES ${core_sources})

add_library(picam_core STATIC
  ${picam_headers}
  ${core_headers}
  ${core_sources}
)

target_link_libraries(picam_core PRIVATE jaw_common)

target_include_directories(picam_core
  PUBLIC ${PiCam_INCLUDE_DIRS}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

# Add extra MMAL dependencies
if(MMAL_FOUND)
  target_compile_definitions(picam_core PRIVATE USE_MMAL)
  target_link_libraries(picam_core PRIVATE ${MMAL_LIBRARIES})
  target_include_directories(picam_core PRIVATE ${MMAL_INCLUDE_DIRS})
endif()
