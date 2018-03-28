add_executable (cgsl_demo demo.c)

target_link_libraries (cgsl_demo LINK_PUBLIC cgsl )

if (WIN32)
  target_link_libraries (cgsl_demo LINK_PUBLIC gsl gslcblas)
  add_definitions(-D_USE_MATH_DEFINES)  # For M_PI
endif ()

add_test(ctest_cgsl_run cgsl_demo)

# No valgrind on Windows
# Also don't run if using any -fsanitize since that interferes with valgrind
if (NOT WIN32 AND NOT "${CMAKE_C_FLAGS}" MATCHES "-fsanitize")
  # Check for leaks
  add_test(ctest_cgsl_valgrind valgrind --leak-check=full --error-exitcode=1 ${CMAKE_CURRENT_BINARY_DIR}/cgsl_demo)
endif ()
