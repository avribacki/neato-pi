cmake_minimum_required(VERSION 2.8.12)

# Create Network library

# In windows it's harder to simply install ZMQ and find it using CMake.
# For that reason we supply a static pre-compiled version.
if (WIN32)
  set(SPEC "win32")

  if (MSVC_VERSION EQUAL 1900)
    set(SPEC "${SPEC}-msvc2015")
  else()
    message(FATAL_ERROR "ZeroMQ currently not supported by MSVC ${MSVC_VERSION}." )
  endif()

  if (bitness EQUAL 32)
    set(SPEC "${SPEC}-x86")
  else()
    set(SPEC "${SPEC}-x64")
  endif()

  set(ZeroMQ_LIBRARY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/zmq/lib/${SPEC}-r/libzmq.lib")
  set(ZeroMQ_LIBRARY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/zmq/lib/${SPEC}-d/libzmq.lib")

  function(link_with_zeromq _target)
    target_link_libraries(${_target} debug ${ZeroMQ_LIBRARY_DEBUG} optimized ${ZeroMQ_LIBRARY_RELEASE})
    target_link_libraries(${_target} ws2_32.lib Iphlpapi.lib)
  endfunction(link_with_zeromq)

# For linux we assume that zmq is already installed in the system
elseif (UNIX)

  function(link_with_zeromq _target)
    target_link_libraries(${_target} zmq)
  endfunction(link_with_zeromq)

endif()

set(ZeroMQ_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/zmq/include")

set(network_headers
  "include/jaw_client.hpp"
  "include/jaw_server.hpp"
  "include/jaw_socket.hpp"
  "include/jaw_serialization.hpp"
)

set(network_sources
  "src/jaw_socket_impl.hpp"
  "src/jaw_socket_impl.cpp"
  "src/jaw_socket.cpp"
  "src/jaw_serialization.cpp"
)

source_group("Include" FILES ${network_headers})
source_group("Source" FILES ${network_sources})

add_definitions(-DZMQ_STATIC)

add_library(jaw_network STATIC
  ${network_headers}
  ${network_sources}
)

link_with_zeromq(jaw_network)
target_link_libraries(jaw_network jaw_common)
target_link_libraries(jaw_network ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(jaw_network
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/
  PRIVATE ${ZeroMQ_INCLUDE_DIRS}
)
