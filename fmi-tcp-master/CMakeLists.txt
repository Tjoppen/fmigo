CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Protobuf
INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(Threads REQUIRED)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

INCLUDE_DIRECTORIES(${FMIL_INCLUDE_DIR} ${STRONG_COUPLING_INCLUDE_DIR} ${FMITCP_INCLUDE_DIR} include include/fmitcp src/fmitcp)
LINK_DIRECTORIES   (${FMIL_LIBS_DIR}    ${STRONG_COUPLING_LIBS_DIR}    ${FMITCP_LIBS_DIR})

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
    src/master/Connection.cpp
    src/master/StrongConnector.cpp
    src/master/main.cpp
    src/master/FMIClient.cpp
    src/master/WeakConnection.cpp
    src/common/url_parser.c
    src/common/common.cpp
    src/master/BaseMaster.cpp
    src/master/parseargs.cpp
    src/master/StrongMaster.cpp
    ${FMITCP_SRCS}
)
SET(MASTER_HEADERS
    include/master/Connection.h
    include/master/FMIClient.h
    include/master/StrongConnector.h
    include/master/WeakConnection.h
    include/common/common.h
    include/common/url_parser.h
    ${FMITCP_HEADERS}
)

if(WIN32)
    set(MASTER_SRCS    ${MASTER_SRCS}    src/master/getopt.cpp)
    set(MASTER_HEADERS ${MASTER_HEADERS} include/master/getopt.h)
endif()

SET(TCP_MASTER_NAME fmi-tcp-master)
SET(MPI_MASTER_NAME fmi-mpi-master)
set(TCP_SERVER_NAME fmi-tcp-server)
set(MPI_SERVER_NAME fmi-mpi-server)

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/bin")

ADD_EXECUTABLE(${TCP_MASTER_NAME} ${MASTER_HEADERS} ${MASTER_SRCS})
ADD_EXECUTABLE(${TCP_SERVER_NAME}  src/server/fmi-tcp-server.cpp ${FMITCP_SRCS})

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
        jsoncpp
        libzmq
        ${CMAKE_THREAD_LIBS_INIT})
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(WINLIBS ${WINLIBS} sc libumfpackd libamdd libblas libcholmodd libcolamdd libccolamdd libcamdd metisd suitesparseconfigd)
    else()
        set(WINLIBS ${WINLIBS} sc libumfpack  libamd  libblas libcholmod  libcolamd  libccolamd  libcamd  metis  suitesparseconfig)
    endif()
    TARGET_LINK_LIBRARIES(${TCP_MASTER_NAME} ${WINLIBS})
    TARGET_LINK_LIBRARIES(${TCP_SERVER_NAME}  ${WINLIBS})
    INSTALL (FILES
        ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows/libblas.dll
        ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows/libgcc_s_dw2-1.dll
        ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows/libgfortran-3.dll
        ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows/libquadmath-0.dll
        DESTINATION bin)
    INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../3dparty/zeromq/4.2.0-master/cl32-11/libzmq.dll DESTINATION bin)
ELSE(WIN32)
    SET(LINUXLIBS
        fmilib
        dl
        ##sc m umfpack amd blas atlas cholmod colamd suitesparseconfig # For strong coupling
        sc m umfpack amd blas cholmod colamd suitesparseconfig # For strong coupling
        jsoncpp
        hdf5
        hdf5_hl
        zmq
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${PROTOBUF_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT})
    TARGET_LINK_LIBRARIES(${TCP_MASTER_NAME} ${LINUXLIBS})
    TARGET_LINK_LIBRARIES(${TCP_SERVER_NAME}  ${LINUXLIBS})

    # MPI stuff. Linux only for now
    # NOTE: switching to MPI compilers. This seems to work fine, surprisingly
    set(CMAKE_C_COMPILER   mpicc)
    set(CMAKE_CXX_COMPILER mpicxx)
    add_executable(${MPI_SERVER_NAME}  src/server/fmi-mpi-server.cpp   src/common/mpi_tools.cpp ${FMITCP_SRCS})
    add_executable(${MPI_MASTER_NAME} ${MASTER_HEADERS} ${MASTER_SRCS} src/common/mpi_tools.cpp )
    set_target_properties(${MPI_MASTER_NAME} PROPERTIES COMPILE_FLAGS -DUSE_MPI)
    target_link_libraries(${MPI_SERVER_NAME} ${LINUXLIBS})
    target_link_libraries(${MPI_MASTER_NAME} ${LINUXLIBS})
    install(TARGETS ${MPI_MASTER_NAME} ${MPI_SERVER_NAME} DESTINATION bin)
ENDIF(WIN32)

# Compile proto
IF(WIN32)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.cc
        COMMAND ${PROTOBUF_BIN_DIR}/protoc --cpp_out=. fmitcp.proto
        WORKING_DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp
        DEPENDS libprotobuf ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.proto)
ELSE(WIN32)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.cc
        COMMAND protoc --cpp_out=. fmitcp.proto
        WORKING_DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp
        DEPENDS             ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.proto)
ENDIF(WIN32)

add_dependencies(${TCP_MASTER_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h)
add_dependencies(${TCP_SERVER_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h)
if(NOT WIN32)
    add_dependencies(${MPI_MASTER_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h)
    add_dependencies(${MPI_SERVER_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h)
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

SET(FMU_SRCS
  fmu/fmu_interface.c
)
SET(FMU_HEADERS
  fmu/include/fmiFunctions.h
  fmu/include/fmiFunctionTypes.h
  fmu/include/fmiTypesPlatform.h
  fmu/include/fmu_interface.h
)

# create the fmu.dll
SET(CMAKE_SHARED_LIBRARY_PREFIX "")
ADD_LIBRARY(vanilla SHARED ${FMU_HEADERS} ${FMU_SRCS})

SET_TARGET_PROPERTIES(vanilla
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin"
)

# remove the exisitng fmu and directories
EXECUTE_PROCESS(COMMAND rm -r -f "${CMAKE_CURRENT_LIST_DIR}/bin/binaries")
EXECUTE_PROCESS(COMMAND rm -f "${CMAKE_CURRENT_LIST_DIR}/bin/vanilla.fmu")

if ( 0 )
# create the needed directories
EXECUTE_PROCESS(COMMAND mkdir -p "${CMAKE_CURRENT_LIST_DIR}/bin/binaries/win32")

# copy the fmu.dll to binaries & modelDescription.xml to bin
EXECUTE_PROCESS(COMMAND cp "${CMAKE_CURRENT_LIST_DIR}/bin/vanilla.dll" "${CMAKE_CURRENT_LIST_DIR}/bin/binaries/win32")
EXECUTE_PROCESS(COMMAND cp "${CMAKE_CURRENT_LIST_DIR}/fmu/modelDescription.xml" "${CMAKE_CURRENT_LIST_DIR}/bin")

# make a zip file
EXECUTE_PROCESS(COMMAND zip -r "vanilla.fmu" "binaries" "modelDescription.xml"
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

endif ( 0 )
