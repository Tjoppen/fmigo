add_executable (fmigoStorage testStorage.cpp ../src/common/fmigo_storage.cpp)

#cmake_policy(SET CMP0045 OLD)
#target_link_libraries (fmigoStorage LINK_PUBLIC MASTERS_SRCS)

add_test(ctest_fmigoStorage_run fmigoStorage)
add_test(ctest_fmigoStorage_valgrind valgrind --leak-check=full --error-exitcode=1 ${CMAKE_CURRENT_BINARY_DIR}/fmigoStorage)
