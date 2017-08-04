cmake_minimum_required(VERSION 2.8.12)

project(NeatoPi)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# Remove 'lib' prefix for shared libraries on Windows
if (WIN32)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    message (FATAL_ERROR "GCC version must be at least 4.9!")
  endif()
endif()

set(CMAKE_CXX_STANDARD 14)

find_package(Threads)

add_subdirectory(jaw)
add_subdirectory(picam)
add_subdirectory(neato)
