if (WIN32)
  # Needed due to SOURCE_SUBDIR
  cmake_minimum_required(VERSION 3.7.2)
else ()
  cmake_minimum_required(VERSION 2.8)
endif ()

find_package(Threads REQUIRED)

#enable testing
enable_testing()
# Includes which tests that should be skipped
configure_file(${CMAKE_SOURCE_DIR}/Buildstuff/CTestCustom.cmake ${CMAKE_BINARY_DIR})

if(CMAKE_VERSION VERSION_GREATER "3.3")
    #suppress warning about libexpat.a
    cmake_policy(SET CMP0058 OLD)
endif()

# cmake_policy(SET CMP0045 OLD)

if (CMAKE_VERSION VERSION_GREATER "3.0")
    # CMake Warning (dev):
    #   Policy CMP0042 is not set: MACOSX_RPATH is enabled by default.  Run "cmake
    #   --help-policy CMP0042" for policy details.  Use the cmake_policy command to
    #   set the policy and suppress this warning.
    cmake_policy(SET CMP0042 NEW)
endif ()

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMake build type set to RELEASE.")
    set(CMAKE_BUILD_TYPE Release CACHE String "" FORCE)
endif()

SET(FMIGO_MAJOR_VERSION 0)
SET(FMIGO_MINOR_VERSION 5)
SET(FMIGO_PATCH_VERSION 0)
SET(FMIGO_VERSION ${FMIGO_MAJOR_VERSION}.${FMIGO_MINOR_VERSION}.${FMIGO_PATCH_VERSION} )

option(BUILD_FMUS "Build example FMUs in umit-fmus/?" ON)
option(USE_GPL "Enable GPL? Needed for GNU GSL support in master (algebraic loop solver, ModelExchange integrator)" ON)

# Enable libmatio if we find it
find_library(MATIO_LIBRARY matio)
if (MATIO_LIBRARY)
    set(MATIO_DEFAULT ON)
else ()
    set(MATIO_DEFAULT OFF)
endif ()
option(USE_MATIO "Enable Matlab output via libmatio?" ${MATIO_DEFAULT})

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install)

# Need to be able to zip FMUs. Recent versions of CMake can do this with the "tar" sub-command.
# For older CMakes we rely on "zip" being installed on the system
# We also need to be able to copy multiple files using the "copy" sub-command, or the "cp" commands must be installed
# There may also be some CMake versions between 2.8.12.2 and 3.5.0 that have have the correct behavior for "copy" or "tar" (but not both)
if (BUILD_FMUS AND CMAKE_VERSION VERSION_LESS "3.5.0")
    find_program(ZIP_PROGRAM zip)
    find_program(CP_PROGRAM  cp)
    if (NOT ZIP_PROGRAM)
        message(FATAL_ERROR "CMake < 3.5.0 and zip not installed on the system -> can't pack FMUs")
    endif ()
    if (NOT CP_PROGRAM)
        # We could maybe fall back to "copy" on Windows
        message(FATAL_ERROR "CMake < 3.5.0 and cp not installed on the system -> can't pack FMUs")
    endif ()
endif ()

find_program(PYTHON2 python2)
find_program(PYTHON3 python3)

# Download/install protobuf, umfpack as needed
if(WIN32)
    add_definitions(-D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS) # required by protobuf-ext
    add_definitions(-DPROTOBUF_USE_DLLS) # required to use protobuf, now that we've switched to DLLs

    if (MSVC14)
        set(MSVC_TOOLCHAIN_VERSION "/p:VisualStudioVersion=14.0")
    elseif (MSV12)
        set(MSVC_TOOLCHAIN_VERSION "/p:VisualStudioVersion=12.0")
    else (MSVC14)
        # Assume there is only one toolchain installed and it is compatible with MSVC12
        set(MSVC_TOOLCHAIN_VERSION "")
    endif (MSVC14)

    include(ExternalProject)

    set(PROTOBUF_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/protobuf-3.2.0")

    ExternalProject_Add(protobuf-ext
        PREFIX "${PROTOBUF_PREFIX}"
        URL https://github.com/google/protobuf/releases/download/v3.2.0/protobuf-cpp-3.2.0.zip
        URL_MD5 a6c76f8fd0d393370bf4b32a29273114
        SOURCE_SUBDIR cmake
        CONFIGURE_COMMAND ${CMAKE_COMMAND} ${PROTOBUF_PREFIX}/src/protobuf-ext/cmake -G ${CMAKE_GENERATOR} -DBUILD_SHARED_LIBS=ON
        INSTALL_COMMAND ""
    )

    include_directories("${PROTOBUF_PREFIX}/src/protobuf-ext/src")
    include_directories("${PROTOBUF_PREFIX}/src/ex-protobuf-ext1234/protobuf-3.2.0/src")
    link_directories("${PROTOBUF_PREFIX}/src/protobuf-ext-build/${CMAKE_BUILD_TYPE}")
    set(PROTOBUF_BIN_DIR "${PROTOBUF_PREFIX}/src/protobuf-ext-build/${CMAKE_BUILD_TYPE}")

    set(SUITESPARSE_DIR "${CMAKE_CURRENT_BINARY_DIR}/suitesparse-metis-for-windows")

    ExternalProject_Add(suitesparse-ext
        PREFIX ${SUITESPARSE_DIR}
        URL https://github.com/jlblancoc/suitesparse-metis-for-windows/archive/232acd3f6c2ab2539167043e31c22a1ac1f599b8.zip
        URL_MD5 1e9b5fcdc38ec0c2aebe3d3cbd1ee8cd
    )

    if (CMAKE_GENERATOR MATCHES "^.*Win64") # Maybe there is a better way to detect 64-bit builds?
        link_directories($ENV{MSMPI_LIB64})
        set(SUITESPARSE_LIBDIR ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib64)
    else ()
        link_directories($ENV{MSMPI_LIB32})
        set(SUITESPARSE_LIBDIR ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib)
    endif ()

    include_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/include)
    include_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/include/suitesparse)
    link_directories(${SUITESPARSE_LIBDIR})
    link_directories(${SUITESPARSE_LIBDIR}/lapack_blas_windows)
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib64) # libmetis.lib always ends up in lib64

    set(HDF5_DIR "${CMAKE_CURRENT_BINARY_DIR}/hdf5")
    ExternalProject_Add(hdf5-ext
        PREFIX  ${HDF5_DIR}
        URL     "https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8/hdf5-1.8.17/src/CMake-hdf5-1.8.17.zip"
        URL_MD5 "8cc32abfbe9cc8815386372783301257"
        CONFIGURE_COMMAND ${CMAKE_COMMAND}          ${HDF5_DIR}/src/hdf5-ext/hdf5-1.8.17 -G ${CMAKE_GENERATOR}
        BUILD_COMMAND     ${CMAKE_COMMAND} -E chdir ${HDF5_DIR}/src/hdf5-ext-build msbuild HDF5.sln /p:Configuration=${CMAKE_BUILD_TYPE} ${MSVC_TOOLCHAIN_VERSION}
        INSTALL_COMMAND   ""
    )

    include_directories(${HDF5_DIR}/src/hdf5-ext/hdf5-1.8.17/src)
    include_directories(${HDF5_DIR}/src/hdf5-ext/hdf5-1.8.17/hl/src)
    include_directories(${HDF5_DIR}/src/hdf5-ext-build)
    link_directories(${HDF5_DIR}/src/hdf5-ext-build/bin/${CMAKE_BUILD_TYPE})

    set(ZMQ_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/zeromq-4.2.3")

    ExternalProject_Add(zmq-ext
        PREFIX "${ZMQ_PREFIX}"
        GIT_REPOSITORY "https://github.com/zeromq/libzmq.git"
        GIT_TAG 44f96a36521a8410378ddc83d7c4ff258da5d399
        INSTALL_COMMAND ""
    )

    include_directories("${ZMQ_PREFIX}/src/zmq-ext/include")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zeromq/4.2.0-master/include) #zmq.hpp
    link_directories("${ZMQ_PREFIX}/src/zmq-ext-build/lib/${CMAKE_BUILD_TYPE}")

else(WIN32)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zeromq/4.2.0-master/include)
    # TODO: tell the user what's needed?
    include(FindProtobuf)
    find_package(Protobuf REQUIRED)
    include_directories(${PROTOBUF_INCLUDE_DIR})
    link_directories(${PROTOBUF_LIBRARY_DIR})

    if (NOT APPLE)
        include_directories(/usr/include/hdf5/serial)
        include_directories(/usr/include/suitesparse)
        link_directories(/usr/lib/x86_64-linux-gnu/hdf5/serial)
    endif()
endif(WIN32)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tools/strong-coupling-core/include)

set(FMILIBRARY_DIR 3rdparty/FMILibrary-2.0.1)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/${FMILIBRARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_DIR}/src/CAPI/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_DIR}/src/Import/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_DIR}/src/Util/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_DIR}/src/XML/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_DIR}/src/ZIP/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_DIR}/ThirdParty/FMI/default)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_DIR}/ThirdParty/Expat/expat-2.1.0/lib)

if (APPLE)
    # For Macports
  
    if (EXISTS /opt/local/include)
        include_directories(/opt/local/include)
    endif()

    if (EXISTS /opt/local/lib)
        link_directories(/opt/local/lib)
    endif()

    if (EXISTS /usr/local/lib)
        link_directories(/usr/local/lib)
    endif()

endif()

add_subdirectory(tools/strong-coupling-core)
add_subdirectory(${FMILIBRARY_DIR})

if (USE_GPL)
  if (WIN32)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wingsl/lib)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wingsl/include)
    set(CMAKE_SHARED_LINKER_FLAGS "/SAFESEH:NO")
    set(CMAKE_EXE_LINKER_FLAGS "/SAFESEH:NO")
  endif ()

  # Use cgsl regardless of whether BUILD_FMUS is ON or not
  add_subdirectory(tools/cgsl)
endif ()

if (NOT WIN32)
  # cl.exe has a different set of flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Werror -Wno-unused-variable -Wno-unused-function -Wextra -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDEBUG")
  FIND_PACKAGE(MPI REQUIRED)
  FIND_PACKAGE(Protobuf REQUIRED)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
else ()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DDEBUG")
endif ()

include_directories(${FMIL_INCLUDE_DIR} ${STRONG_COUPLING_INCLUDE_DIR} ${FMITCP_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/fmitcp ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp)
LINK_DIRECTORIES   (${FMIL_LIBS_DIR}    ${STRONG_COUPLING_LIBS_DIR}    ${FMITCP_LIBS_DIR})

set(COMMON_SRCS
    src/fmitcp/Client.cpp
    src/fmitcp/fmitcp-common.cpp
    src/fmitcp/fmitcp.pb.cc
    src/fmitcp/serialize.cpp
    src/fmitcp/Server.cpp
    src/master/globals.cpp
    src/common/timer.cpp
)

set(COMMON_HEADERS
    include/fmitcp/Client.h
    include/fmitcp/fmitcp-common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h
    include/fmitcp/serialize.h
    include/fmitcp/Server.h
    include/common/timer.h
)

SET(MASTER_SRCS
    src/master/StrongConnector.cpp
    src/master/main.cpp
    src/master/FMIClient.cpp
    src/master/WeakConnection.cpp
    src/common/common.cpp
    src/common/CSV-parser.cpp
    src/master/BaseMaster.cpp
    src/master/parseargs.cpp
    src/master/StrongMaster.cpp
    src/master/modelExchange.cpp
    ${COMMON_SRCS}
    src/master/control.pb.cc
    src/master/globals.cpp
)
SET(MASTER_HEADERS
    include/master/FMIClient.h
    include/master/StrongConnector.h
    include/master/WeakConnection.h
    include/master/WeakMasters.h
    include/master/modelExchange.h
    include/master/BaseMaster.h
    include/master/parseargs.h
    include/master/StrongMaster.h
    include/common/common.h
    include/common/CSV-parser.h
    ${COMMON_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/master/control.pb.h
    include/master/globals.h
)

if(WIN32)
    set(MASTER_SRCS    ${MASTER_SRCS}    src/master/getopt.cpp)
    set(MASTER_HEADERS ${MASTER_HEADERS} include/master/getopt.h)
endif()

SET(TCP_MASTER_NAME fmigo-master)
SET(MPI_MASTER_NAME fmigo-mpi)
set(TCP_SERVER_NAME fmigo-server)

if (USE_GPL)
  set(EXTRALIBS cgsl )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_GPL")

  set(MASTER_SRCS ${MASTER_SRCS}
    src/common/fmigo_storage.cpp
  )

  set(MASTER_HEADERS ${MASTER_HEADERS}
    include/common/fmigo_storage.h
  )

  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tools/cgsl/include)
endif ()

if (USE_MATIO)
  set(EXTRALIBS ${EXTRALIBS} matio)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_MATIO")
endif ()

ADD_EXECUTABLE(${TCP_MASTER_NAME} ${MASTER_HEADERS} ${MASTER_SRCS})
ADD_EXECUTABLE(${TCP_SERVER_NAME}  src/server/fmigo-server.cpp src/common/common.cpp ${COMMON_SRCS})

add_executable(${MPI_MASTER_NAME} ${MASTER_HEADERS} ${MASTER_SRCS})
set_target_properties(${MPI_MASTER_NAME} PROPERTIES COMPILE_FLAGS -DUSE_MPI)

IF(WIN32)
    # Multi-threade
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
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
        ${CMAKE_THREAD_LIBS_INIT})
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(WINLIBS ${WINLIBS} ${EXTRALIBS} sc libprotobufd libzmq-${CMAKE_VS_PLATFORM_TOOLSET}-mt-gd-4_2_3 libhdf5_hl_D libumfpackd libhdf5_D
            libamdd libblas libcholmodd libcolamdd libccolamdd libcamdd metisd suitesparseconfigd)
    else()
        set(WINLIBS ${WINLIBS} ${EXTRALIBS} sc libprotobuf  libzmq-${CMAKE_VS_PLATFORM_TOOLSET}-mt-4_2_3 libhdf5_hl      libumfpack  libhdf5
            libamd  libblas libcholmod  libcolamd  libccolamd  libcamd  metis  suitesparseconfig)
    endif()
    include_directories($ENV{MSMPI_INC} $ENV{MSMPI_INC}x86)

    if (CMAKE_GENERATOR MATCHES "^.*Win64") # Maybe there is a better way to detect 64-bit builds?
        install(FILES ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libgcc_s_sjlj-1.dll DESTINATION bin)
    else ()
        install(FILES ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libgcc_s_dw2-1.dll DESTINATION bin)
    endif ()

    TARGET_LINK_LIBRARIES(${TCP_MASTER_NAME} ${WINLIBS})
    TARGET_LINK_LIBRARIES(${TCP_SERVER_NAME}  ${WINLIBS})
    target_link_libraries(${MPI_MASTER_NAME} ${WINLIBS} msmpi)

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    INSTALL (FILES
        ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libblas.dll
        ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libgfortran-3.dll
        ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libquadmath-0.dll
        ${PROTOBUF_PREFIX}/src/protobuf-ext-build/${CMAKE_BUILD_TYPE}/libprotobufd.dll
        ${ZMQ_PREFIX}/src/zmq-ext-build/bin/${CMAKE_BUILD_TYPE}/libzmq-${CMAKE_VS_PLATFORM_TOOLSET}-mt-gd-4_2_3.dll
        DESTINATION bin)
    else()
    INSTALL (FILES
        ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libblas.dll
        ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libgfortran-3.dll
        ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libquadmath-0.dll
        ${PROTOBUF_PREFIX}/src/protobuf-ext-build/${CMAKE_BUILD_TYPE}/libprotobuf.dll
        ${ZMQ_PREFIX}/src/zmq-ext-build/bin/${CMAKE_BUILD_TYPE}/libzmq-${CMAKE_VS_PLATFORM_TOOLSET}-mt-4_2_3.dll
        DESTINATION bin)
    endif()
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
    # mpicc/cxx uses gcc/g++, which needs -Wno-literal-suffix on trusty
    # Sometimes clang gets picked up which lacks -Wno-literal-suffix on trusty, hence -Wno-unknown-warning-option
    target_compile_options(${MPI_MASTER_NAME} PRIVATE -Wno-unknown-warning-option -Wno-literal-suffix)
    set(CMAKE_C_COMPILER   mpicc)
    set(CMAKE_CXX_COMPILER mpicxx)
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
add_dependencies(${TCP_SERVER_NAME} fmitcp_pb control_pb)
add_dependencies(${MPI_MASTER_NAME} fmitcp_pb control_pb)


# Manpage stuff
find_program(GZIP   gzip)
find_program(ICONV  iconv)
find_program(GROFF  groff)
find_program(SED    sed)

set(IN_MANPAGE ${CMAKE_CURRENT_LIST_DIR}/doc/fmigo.1)

if (GZIP)
  set(OUT_MANPAGE ${CMAKE_CURRENT_BINARY_DIR}/fmigo.1.gz)
  add_custom_command(OUTPUT ${OUT_MANPAGE} COMMAND ${GZIP} -c9 < ${IN_MANPAGE} > ${OUT_MANPAGE} DEPENDS ${IN_MANPAGE})
  add_custom_target(manpage ALL DEPENDS ${OUT_MANPAGE})
  install (FILES ${OUT_MANPAGE} DESTINATION man/man1)
endif ()

# groff on Mac is different
if (ICONV AND GROFF AND SED AND NOT APPLE)
  set(OUT_PDF ${CMAKE_CURRENT_LIST_DIR}/doc/fmigo.pdf)

  # The sed part strips out date so the PDF output is always the same (reproducible)
  add_custom_command(OUTPUT ${OUT_PDF} COMMAND
    ${ICONV} -t ISO-8859-15 -f UTF-8 < ${IN_MANPAGE} |
    ${GROFF} -T pdf -mandoc |
    ${SED} -e '/CreationDate/,/endobj/d' > ${OUT_PDF}
    DEPENDS ${IN_MANPAGE}
  )
  add_custom_target(manpage_pdf ALL DEPENDS ${OUT_PDF})
  install(FILES ${OUT_PDF} DESTINATION doc)
endif ()

INSTALL(TARGETS ${TCP_MASTER_NAME} ${TCP_SERVER_NAME} ${MPI_MASTER_NAME} DESTINATION bin)

add_subdirectory(tests)

add_dependencies(fmigo-master fmilib)
add_dependencies(fmigo-mpi    fmilib)


if (WIN32)
    add_dependencies(sc suitesparse-ext)
    add_dependencies(fmigo-master protobuf-ext hdf5-ext zmq-ext)
    add_dependencies(fmigo-server protobuf-ext hdf5-ext zmq-ext)
    add_dependencies(fmigo-mpi    protobuf-ext hdf5-ext zmq-ext)
    add_dependencies(fmitcp_pb    protobuf-ext)
    add_dependencies(control_pb   protobuf-ext)
endif ()

if (BUILD_FMUS)
    include(Buildstuff/FmuBase.cmake)

    # GSL Co-Simulation FMUs
    add_fmu(tests/umit-fmus/gsl/clutch                      clutch                          "cgsl")

    # Non-GSL Co-Simulation FMUs (internal verlet integrator typically)
    add_fmu(tests/umit-fmus/kinematictruck/body             body                            "")
    add_fmu(tests/umit-fmus/kinematictruck/engine           engine                          "")
    add_fmu(tests/umit-fmus/kinematictruck/gearbox2         gearbox2                        "")
    add_fmu(tests/umit-fmus/forcevelocitytruck/fvbody       fvbody                          "")
    add_fmu(tests/umit-fmus/forcevelocitytruck/gearbox      gearbox                         "")
    add_fmu(tests/umit-fmus/tests/typeconvtest              typeconvtest                    "")
    add_fmu(tests/umit-fmus/tests/loopsolvetest/add         add                             "")
    add_fmu(tests/umit-fmus/tests/loopsolvetest/sub         sub                             "")
    add_fmu(tests/umit-fmus/tests/loopsolvetest/mul         mul                             "")
    add_fmu(tests/umit-fmus/tests/stringtest                stringtest                      "")
    add_fmu(tests/umit-fmus/tests/strange_variable_names    strange_variable_names          "")
    add_fmu(tests/umit-fmus/tests/alltypestest              alltypestest                    "")

    # Model-Exchange FMUs
    add_fmu(tests/umit-fmus/me/springs                      springs                         "")
    add_fmu(tests/umit-fmus/me/springs2                     springs2                        "")
    add_fmu(tests/umit-fmus/me/springs3                     springs3                        "")
    add_fmu(tests/umit-fmus/me/springs4                     springs4                        "")
    add_fmu(tests/umit-fmus/me/bouncingBall                 bouncingBall                    "")

    # Wrap Model-Exchange FMUs into Co-Simulation FMUs using GSL for integrating
    # The ME FMUs are taken as-is and put into resources/ in the outer FMU
    # The modelDescription file is found in one of two ways:
    #
    # - on disc, uncompressed (plain XML file)
    # - from inside the source FMU

    # These generate the modelDescription from XML
    add_wrapped(tests/umit-fmus/meWrapper/bouncingBall              bouncingBall)
    add_wrapped(tests/umit-fmus/meWrapper/springs                   springs)
    add_wrapped(tests/umit-fmus/meWrapper/springs2                  springs2)
    add_wrapped(tests/umit-fmus/meWrapper/springs3                  springs3)
    add_wrapped(tests/umit-fmus/meWrapper/springs4                  springs4)
    add_wrapped_filter(tests/umit-fmus/meWrapperFiltered/springs2                  springs2)


    # These generate the modelDescription from the source FMU
    add_wrapped_fmu(tests/umit-fmus/meWrapperFmu/bouncingBall       bouncingBall    "${bouncingBall_fmu}")
    add_wrapped_fmu(tests/umit-fmus/meWrapperFmu/springs2           springs2        "${springs2_fmu}")
endif ()

add_subdirectory(tools/ssp)

# CPack setup
set(CPACK_PACKAGE_NAME "fmigo")
# set(CPACK_PACKAGE_VENDOR "")
set(CPACK_PACKAGE_VERSION ${FMIGO_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${FMIGO_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${FMIGO_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${FMIGO_RELEASE_VERSION})
# set(CPACK_RESOURCE_FILE_LICENSE ${LICENSE_FILE})
set(CPACK_GENERATOR "ZIP")
include(CPack)
