add_executable (helloDemo demo.c)

target_link_libraries (helloDemo LINK_PUBLIC cgsl )

add_test(just_run helloDemo)

# Check for leaks
add_test(valgrind valgrind --leak-check=full --error-exitcode=1 ${CMAKE_CURRENT_BINARY_DIR}/helloDemo)