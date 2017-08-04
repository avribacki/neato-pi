cmake_minimum_required(VERSION 2.8.12)

# Create Common library

set(common_headers
  "include/jaw_exception.hpp"
  "include/jaw_guid.hpp"
  "include/jaw_member_call.hpp"
  "include/jaw_protected_call.hpp"
  "include/jaw_queue.hpp"
)

set(common_sources
  "src/jaw_exception.cpp"
  "src/jaw_guid.cpp"
  "src/jaw_protected_call.cpp"
)

source_group("Include" FILES ${common_headers})
source_group("Source" FILES ${common_sources})

add_library(jaw_common STATIC
  ${common_headers}
  ${common_sources}
)

target_include_directories(jaw_common
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
