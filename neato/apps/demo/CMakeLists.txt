cmake_minimum_required(VERSION 2.8.12)

# Create Demo executable

set(demo_sources
  "src/neato_demo.cpp"
)

source_group("Source" FILES ${demo_sources})

add_executable(neato_demo
  ${demo_sources}
)

target_link_libraries(neato_demo neato_client)
