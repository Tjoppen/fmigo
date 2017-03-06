add_executable (cgsl_demo demo.c)

target_link_libraries (cgsl_demo LINK_PUBLIC cgsl )

add_test(ctest_cgsl_run cgsl_demo)

# Check for leaks
add_test(ctest_cgsl_valgrind valgrind --leak-check=full --error-exitcode=1 ${CMAKE_CURRENT_BINARY_DIR}/cgsl_demo)
