cmake_minimum_required(VERSION 2.8.12)

# Create Daemon executable

set(daemon_sources
  "src/picam_daemon.cpp"
)

source_group("Source" FILES ${daemon_sources})

add_executable(picam_daemon
  ${daemon_sources}
)

target_link_libraries(picam_daemon picam_server)
