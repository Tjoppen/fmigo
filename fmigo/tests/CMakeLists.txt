add_executable (fmigoStorage testStorage.cpp ../src/common/fmigo_storage.cpp)
add_test(ctest_fmigoStorage_run fmigoStorage)
#add_test(ctest_fmigoStorage_valgrind valgrind --leak-check=full --error-exitcode=1 ${CMAKE_CURRENT_BINARY_DIR}/fmigoStorage)
set(FMI_MPI_MASTER ${CMAKE_BINARY_DIR}/install/bin/fmi-mpi-master)
set(FMI_MPI_SERVER ${CMAKE_BINARY_DIR}/install/bin/fmi-mpi-server)
set(FMU_PATH ${CMAKE_SOURCE_DIR}/umit-fmus/me )
add_test(ctest_me_springs
	mpiexec -np 1 ${FMI_MPI_MASTER} -t 12 -m me : -np 1 ${FMI_MPI_SERVER} ${FMU_PATH}/springs/springs.fmu )
