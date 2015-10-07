cmake_minimum_required(VERSION 2.8)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fmi-tcp/src/fmitcp/include)
link_directories(${CMAKE_CURRENT_BIN_DIR}/fmi-tcp/lib)
add_subdirectory(fmi-tcp)
add_subdirectory(fmi-tcp-master)