add_executable (fmigoStorage
  testStorage.cpp
  ../src/common/fmigo_storage.cpp
  ../src/common/common.cpp
)
add_dependencies(fmigoStorage fmitcp_pb control_pb)

add_test(ctest_fmigoStorage_run fmigoStorage)
#add_test(ctest_fmigoStorage_valgrind valgrind --leak-check=full --error-exitcode=1 ${CMAKE_CURRENT_BINARY_DIR}/fmigoStorage)
set(FMU_PATH ${CMAKE_SOURCE_DIR}/umit-fmus/me )
add_test(ctest_me_springs
	mpiexec -np 2 fmigo-mpi -t 12 -m me ${FMU_PATH}/springs/springs.fmu )
