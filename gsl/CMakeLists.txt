add_library(CGSL
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gsl-interface.c
  )
target_include_directories (CGSL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries (CGSL LINK_PUBLIC -lgsl -lgslcblas )
