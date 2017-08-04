cmake_minimum_required(VERSION 2.8.12)

# Create Demo executable

set(demo_sources
  "src/picam_demo.cpp"
)

source_group("Source" FILES ${demo_sources})

include_directories(
  ${PROJECT_SOURCE_DIR}/include/
)

add_executable(picam_demo
  ${demo_sources}
)

target_link_libraries(picam_demo picam_client)

# Enable optional OpenCV suppport

find_package(OpenCV)

if (OpenCV_FOUND)
  message("Using OpenCV")
  target_link_libraries(picam_demo jaw_common)
  target_link_libraries(picam_demo ${OpenCV_LIBS})
  target_link_libraries(picam_demo ${CMAKE_THREAD_LIBS_INIT})
  target_compile_definitions(picam_demo PRIVATE USE_OPENCV)
else()
  message("Not using OpenCV")
endif()
