cmake_minimum_required(VERSION 2.8.12)

# Create Daemon executable

set(daemon_sources
  "src/neato_daemon.cpp"
)

source_group("Source" FILES ${daemon_sources})

add_executable(neato_daemon
  ${daemon_sources}
)

target_link_libraries(neato_daemon neato_server)
