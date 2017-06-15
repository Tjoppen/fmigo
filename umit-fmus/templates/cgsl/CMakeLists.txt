CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
enable_testing()
project(CGSL)

add_library(cgsl
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gsl-interface.c
)

# Need -fPIC when linking cgsl statically into FMU .so files
set_property(TARGET cgsl PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (cgsl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries (cgsl LINK_PUBLIC gsl gslcblas )

add_subdirectory(demo)

