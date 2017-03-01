CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

FIND_PACKAGE(Threads REQUIRED)

if (NOT WIN32)
  # cl.exe has a different set of flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -Wall -Werror -Wno-unused-variable -Wno-unused-function -Wextra -Wno-unused-parameter")
  FIND_PACKAGE(MPI REQUIRED)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
endif ()

INCLUDE_DIRECTORIES(${FMIL_INCLUDE_DIR} ${STRONG_COUPLING_INCLUDE_DIR} ${FMITCP_INCLUDE_DIR} include include/fmitcp src/fmitcp)
LINK_DIRECTORIES   (${FMIL_LIBS_DIR}    ${STRONG_COUPLING_LIBS_DIR}    ${FMITCP_LIBS_DIR})

if (WIN32)
    link_directories($ENV{MSMPI_LIB32})
endif()

set(FMITCP_SRCS
    src/fmitcp/Client.cpp
    src/fmitcp/common.cpp
    src/fmitcp/fmitcp.pb.cc
    src/fmitcp/Logger.cpp
    src/fmitcp/serialize.cpp
    src/fmitcp/Server.cpp
)

set(FMITCP_HEADERS
    include/fmitcp/Client.h
    include/fmitcp/common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h
    include/fmitcp/Logger.h
    include/fmitcp/serialize.h
    include/fmitcp/Server.h
)

SET(MASTER_SRCS
    src/master/StrongConnector.cpp
    src/master/main.cpp
    src/master/FMIClient.cpp
    src/master/WeakConnection.cpp
    src/common/common.cpp
    src/master/BaseMaster.cpp
    src/master/parseargs.cpp
    src/master/StrongMaster.cpp
    ${FMITCP_SRCS}
    src/master/control.pb.cc
)
SET(MASTER_HEADERS
    include/master/FMIClient.h
    include/master/StrongConnector.h
    include/master/WeakConnection.h
    include/master/WeakMasters.h
    include/common/common.h
    ${FMITCP_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/master/control.pb.h
)

if(WIN32)
    set(MASTER_SRCS    ${MASTER_SRCS}    src/master/getopt.cpp)
    set(MASTER_HEADERS ${MASTER_HEADERS} include/master/getopt.h)
endif()

SET(TCP_MASTER_NAME fmi-tcp-master)
SET(MPI_MASTER_NAME fmi-mpi-master)
set(TCP_SERVER_NAME fmi-tcp-server)
set(MPI_SERVER_NAME fmi-mpi-server)

if (USE_GPL)
  set(EXTRALIBS gsl gslcblas )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_GPL")

  if (WIN32)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../umit-fmus/wingsl/lib)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../umit-fmus/wingsl/include)
    set(CMAKE_SHARED_LINKER_FLAGS "/SAFESEH:NO")
    set(CMAKE_EXE_LINKER_FLAGS "/SAFESEH:NO")
  endif ()
endif ()

ADD_EXECUTABLE(${TCP_MASTER_NAME} ${MASTER_HEADERS} ${MASTER_SRCS})
ADD_EXECUTABLE(${TCP_SERVER_NAME}  src/server/fmi-tcp-server.cpp src/common/common.cpp ${FMITCP_SRCS})

add_executable(${MPI_SERVER_NAME}  src/server/fmi-mpi-server.cpp  src/common/common.cpp src/common/mpi_tools.cpp ${FMITCP_SRCS})
add_executable(${MPI_MASTER_NAME} ${MASTER_HEADERS} ${MASTER_SRCS}                      src/common/mpi_tools.cpp )
set_target_properties(${MPI_MASTER_NAME} PROPERTIES COMPILE_FLAGS -DUSE_MPI)

IF(WIN32)
    SET(WINLIBS
        fmilib
        shlwapi
        ws2_32
        mswsock
        crypt32
        secur32
        mpr
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        libprotobuf
        libhdf5
        libhdf5_hl
        libzmq
        ${CMAKE_THREAD_LIBS_INIT})
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(WINLIBS ${WINLIBS} ${EXTRALIBS} sc libumfpackd libamdd libblas libcholmodd libcolamdd libccolamdd libcamdd metisd suitesparseconfigd)
    else()
        set(WINLIBS ${WINLIBS} ${EXTRALIBS} sc libumfpack  libamd  libblas libcholmod  libcolamd  libccolamd  libcamd  metis  suitesparseconfig)
    endif()
    include_directories($ENV{MSMPI_INC} $ENV{MSMPI_INC}x86)
    TARGET_LINK_LIBRARIES(${TCP_MASTER_NAME} ${WINLIBS})
    TARGET_LINK_LIBRARIES(${TCP_SERVER_NAME}  ${WINLIBS})
    target_link_libraries(${MPI_SERVER_NAME} ${WINLIBS} msmpi)
    target_link_libraries(${MPI_MASTER_NAME} ${WINLIBS} msmpi)
    INSTALL (FILES
        ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows/libblas.dll
        ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows/libgcc_s_dw2-1.dll
        ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows/libgfortran-3.dll
        ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows/libquadmath-0.dll
        DESTINATION bin)
    INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../3dparty/zeromq/4.2.0-master/cl32-11/libzmq.dll DESTINATION bin)
    set(PROTOC ${PROTOBUF_BIN_DIR}/protoc)
ELSE(WIN32)
    SET(LINUXLIBS
        fmilib
        dl
        ##sc m umfpack amd blas atlas cholmod colamd suitesparseconfig # For strong coupling
        sc m umfpack amd blas cholmod colamd suitesparseconfig # For strong coupling
        hdf5
        hdf5_hl
        zmq
        ${MPI_LIBRARIES}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${PROTOBUF_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT})
    TARGET_LINK_LIBRARIES(${TCP_MASTER_NAME} ${LINUXLIBS} ${EXTRALIBS})
    TARGET_LINK_LIBRARIES(${TCP_SERVER_NAME}  ${LINUXLIBS})

    # MPI stuff. Linux only for now
    # NOTE: switching to MPI compilers. This seems to work fine, surprisingly
    set(CMAKE_C_COMPILER   mpicc)
    set(CMAKE_CXX_COMPILER mpicxx)
    target_link_libraries(${MPI_SERVER_NAME} ${LINUXLIBS})
    target_link_libraries(${MPI_MASTER_NAME} ${LINUXLIBS} ${EXTRALIBS})

    add_custom_target(libprotobuf)  #fake target
    set(PROTOC protoc)
ENDIF(WIN32)

# Compile proto
set(FMITCP_PROTOBUF_TARGETS ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp_pb2.py)
add_custom_command(OUTPUT ${FMITCP_PROTOBUF_TARGETS}
    COMMAND ${PROTOC} --cpp_out=. --python_out=. fmitcp.proto
    WORKING_DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp
    DEPENDS libprotobuf ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.proto)
#can't depend on the generated files themselves because CMake is awful. create a custom target instead
add_custom_target(fmitcp_pb DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.cc)

set(MASTER_CONTROL_PROTOBUF_TARGETS ${CMAKE_CURRENT_SOURCE_DIR}/src/master/control.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/src/master/control.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/src/master/control_pb2.py)
add_custom_command(OUTPUT ${MASTER_CONTROL_PROTOBUF_TARGETS}
    COMMAND ${PROTOC} --cpp_out=. --python_out=. control.proto
    WORKING_DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/src/master
    DEPENDS libprotobuf ${CMAKE_CURRENT_SOURCE_DIR}/src/master/control.proto)
#same here
add_custom_target(control_pb DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/master/control.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/src/master/control.pb.cc)

# Install compiled protbuf files 
install(FILES ${FMITCP_PROTOBUF_TARGETS} ${MASTER_CONTROL_PROTOBUF_TARGETS} DESTINATION protobuf)

add_dependencies(${TCP_MASTER_NAME} fmitcp_pb control_pb)
add_dependencies(${TCP_SERVER_NAME} fmitcp_pb)
if(NOT WIN32)
    add_dependencies(${MPI_MASTER_NAME} fmitcp_pb control_pb)
    add_dependencies(${MPI_SERVER_NAME} fmitcp_pb)
endif()

# TODO: Server manpage?
if(NOT WIN32)
    SET(IN_MANPAGE ${CMAKE_CURRENT_LIST_DIR}/doc/${TCP_MASTER_NAME}.1)
    SET(OUT_MANPAGE ${CMAKE_CURRENT_BINARY_DIR}/${TCP_MASTER_NAME}.1.gz)
    ADD_CUSTOM_TARGET(manpage COMMAND gzip -kc9 ${IN_MANPAGE} > ${OUT_MANPAGE} DEPENDS ${IN_MANPAGE} SOURCES ${IN_MANPAGE})
    ADD_DEPENDENCIES(${TCP_MASTER_NAME} manpage)
    INSTALL (FILES ${OUT_MANPAGE} DESTINATION man/man1)
endif()

INSTALL(TARGETS ${TCP_MASTER_NAME} ${TCP_SERVER_NAME} DESTINATION bin)
install(TARGETS ${MPI_MASTER_NAME} ${MPI_SERVER_NAME} DESTINATION bin)
