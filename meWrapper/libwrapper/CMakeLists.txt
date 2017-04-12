CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(WRAPPERLIB)

add_library(wrapperlib
  ${CMAKE_CURRENT_SOURCE_DIR}/src/modelExchange.c
)

# Need -fPIC when linking wrapperlib statically into FMU .so files
set_property(TARGET wrapperlib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (wrapperlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries (wrapperlib LINK_PUBLIC cgsl fmilib fmilib_shared)
