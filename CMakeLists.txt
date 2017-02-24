CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
enable_testing()
project(CGSL)

add_library(cgsl
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gsl-interface.c
  )
target_include_directories (cgsl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries (cgsl LINK_PUBLIC -lgsl -lgslcblas )

add_subdirectory(demo)

