project(fmigo)

cmake_minimum_required(VERSION 3.10.2)

find_package(Threads REQUIRED)

#enable testing
enable_testing()
# Includes which tests that should be skipped
configure_file(${CMAKE_SOURCE_DIR}/Buildstuff/CTestCustom.cmake ${CMAKE_BINARY_DIR})

if(CMAKE_VERSION VERSION_GREATER "3.3")
    cmake_policy(SET CMP0058 NEW)
endif()

# cmake_policy(SET CMP0045 OLD)
if(CMAKE_VERSION VERSION_GREATER "3.20")
    cmake_policy(SET CMP0115 NEW)
endif()

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
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
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
option(USE_TRACEANALYZER "Add instrumentation for Intel Traceanalyzer?" OFF)
option(ENABLE_SC "Enable strong coupling master? Disabling this removes the dependency on suitesparse" ON)
option(ENABLE_HDF5_HACK "Enable HDF5 column name hack? Necessary for perftest.py in fmifast" OFF)

if (WIN32)
    option(USE_INTEL_MPI "Use Intel MPI instead of MS-MPI?" OFF)
    option(INTEL_MPI_DIR "Location of Intel MPI install, including version number" "C:\\Program Files (x86)\\Intel\\oneAPI\\mpi\\2021.12")
    option(USE_EXTERNAL_LIBZMQ "Use external libzmq?" ON)
    option(USE_EXTERNAL_LIBPROTOBUF "Use external libprotobuf?" ON)
else ()
    option(USE_EXTERNAL_LIBZMQ "Use external libzmq?" OFF)
    option(USE_EXTERNAL_LIBPROTOBUF "Use external libprotobuf?" OFF)
endif ()
# This is only useful during development
option(USE_PROTOC "Use protoc to regenerate source files as needed?" OFF)

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

find_program(PYTHON3 python3)

set(FMILIB_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/fmi-library-install)
include(ExternalProject)
ExternalProject_Add(fmi-library-ext
    GIT_REPOSITORY https://github.com/modelon-community/fmi-library.git
    GIT_TAG 29fe5c4b36e44814fcf3dd69bdcad2667fe51478 # 3.0a4
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX:STRING=${FMILIB_INSTALL_DIR};-DFMILIB_BUILD_STATIC_LIB=ON;-DFMILIB_BUILD_SHARED_LIB=ON;-DFMILIB_BUILD_TESTS=OFF;-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER};-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER};-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    PATCH_COMMAND "git;reset;--hard;&&;git;apply;--ignore-whitespace;${CMAKE_CURRENT_SOURCE_DIR}/Buildstuff/0001-Fix-isatty-and-fileno-on-Windows-also-for-FMI3.patch;&&;git;apply;--ignore-whitespace;${CMAKE_CURRENT_SOURCE_DIR}/Buildstuff/0001-Pass-CMAKE_BUILD_TYPE-to-zlibext.patch;&&;git;apply;--ignore-whitespace;${CMAKE_CURRENT_SOURCE_DIR}/Buildstuff/0001-miniunz.c-Add-missing-include-sys-stat.h.patch"
    INSTALL_COMMAND "${CMAKE_COMMAND}" --build . --target install
)
link_directories(${FMILIB_INSTALL_DIR}/lib)
include_directories(${FMILIB_INSTALL_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-library-ext-prefix/src/fmi-library-ext/src/XML)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-library-ext-prefix/src/fmi-library-ext/src/XML/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-library-ext-prefix/src/fmi-library-ext-build/ExpatEx/install/include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-library-ext-prefix/src/fmi-library-ext-build/zlibext/Debug)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-library-ext-prefix/src/fmi-library-ext-build/zlibext/Release)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-library-ext-prefix/src/fmi-library-ext-build/zlibext)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-library-ext-prefix/src/fmi-library-ext-build)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-library-ext-prefix/src/fmi-library-ext-build/ExpatEx)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-library-ext-prefix/src/fmi-library-ext-build/Config.cmake/Minizip)

if (USE_EXTERNAL_LIBZMQ)
    set(ZMQ_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/zeromq-4.3.5")

    ExternalProject_Add(zmq-ext
        PREFIX "${ZMQ_PREFIX}"
        GIT_REPOSITORY "https://github.com/zeromq/libzmq.git"
        GIT_TAG 622fc6dde99ee172ebaa9c8628d85a7a1995a21d # v4.3.5
        CMAKE_ARGS "-DBUILD_SHARED=ON;-DBUILD_STATIC=OFF;-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE};-DWITH_TLS=OFF;-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER};-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        INSTALL_COMMAND ""
        BUILD_COMMAND "${CMAKE_COMMAND}" --build . --config ${CMAKE_BUILD_TYPE}
    )

    include_directories("${ZMQ_PREFIX}/src/zmq-ext/include")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zeromq/4.2.0-master/include) #zmq.hpp
    link_directories("${ZMQ_PREFIX}/src/zmq-ext-build/lib/Debug")
    link_directories("${ZMQ_PREFIX}/src/zmq-ext-build/lib/Release")
    link_directories("${ZMQ_PREFIX}/src/zmq-ext-build/lib")
endif ()

if (USE_EXTERNAL_LIBPROTOBUF)
    add_definitions(-D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS) # required by protobuf-ext
    add_definitions(-DPROTOBUF_USE_DLLS) # required to use protobuf, now that we've switched to DLLs

    set(PROTOBUF_CXX_FLAGS "")
    if (WIN32)
        if (MSVC)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /FS")
            set(PROTOBUF_CXX_FLAGS "/bigobj")
        endif ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DPROTOBUF_USE_DLLS")
    endif ()

    set(PROTOBUF_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/protobuf-21.12")

    ExternalProject_Add(protobuf-ext
        PREFIX "${PROTOBUF_PREFIX}"
        # 25.6 is the last version to support C++14
        # 21.12 is the last version to not require abseil-cpp
        GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
        GIT_TAG f0dc78d7e6e331b8c6bb2d5283e06aa26883ca7c # v21.12
        CONFIGURE_COMMAND "${CMAKE_COMMAND}" "${PROTOBUF_PREFIX}/src/protobuf-ext" -G "${CMAKE_GENERATOR}" -Dprotobuf_BUILD_SHARED_LIBS=ON -DCMAKE_CXX_FLAGS=${PROTOBUF_CXX_FLAGS} -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_EXAMPLES=OFF -Dprotobuf_BUILD_CONFORMANCE=OFF -Dprotobuf_WITH_ZLIB=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        INSTALL_COMMAND ""
        PATCH_COMMAND git reset --hard && git apply --ignore-whitespace "${CMAKE_CURRENT_SOURCE_DIR}/Buildstuff/0001-Remove-bigobj.patch"
    )

    include_directories("${PROTOBUF_PREFIX}/src/protobuf-ext/src")
    link_directories("${PROTOBUF_PREFIX}/src/protobuf-ext-build/Debug")
    link_directories("${PROTOBUF_PREFIX}/src/protobuf-ext-build/Release")
    link_directories("${PROTOBUF_PREFIX}/src/protobuf-ext-build")
    set(PROTOBUF_BIN_DIR "${PROTOBUF_PREFIX}/src/protobuf-ext-build/${CMAKE_BUILD_TYPE}")
    set(PROTOBUF_LIBRARY protobuf)
else ()
    include(FindProtobuf)
    find_package(Protobuf REQUIRED)
    include_directories(${PROTOBUF_INCLUDE_DIR})
    link_directories(${PROTOBUF_LIBRARY_DIR})
endif ()

# Download/install protobuf, umfpack as needed
if(WIN32)
    if (MSVC14)
        set(MSVC_TOOLCHAIN_VERSION "/p:VisualStudioVersion=14.0")
    elseif (MSV12)
        set(MSVC_TOOLCHAIN_VERSION "/p:VisualStudioVersion=12.0")
    else (MSVC14)
        # Assume there is only one toolchain installed and it is compatible with MSVC12
        set(MSVC_TOOLCHAIN_VERSION "")
    endif (MSVC14)

    if (ENABLE_SC)
        set(SUITESPARSE_DIR "${CMAKE_CURRENT_BINARY_DIR}/suitesparse-metis-for-windows")

        ExternalProject_Add(suitesparse-ext
            PREFIX ${SUITESPARSE_DIR}
            URL https://github.com/jlblancoc/suitesparse-metis-for-windows/archive/232acd3f6c2ab2539167043e31c22a1ac1f599b8.zip
            URL_MD5 1e9b5fcdc38ec0c2aebe3d3cbd1ee8cd
        )

        if (CMAKE_GENERATOR MATCHES "^.*Win64") # Maybe there is a better way to detect 64-bit builds?
            set(SUITESPARSE_LIBDIR ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib64)
        else ()
            set(SUITESPARSE_LIBDIR ${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib)
        endif ()

        include_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/include)
        include_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/include/suitesparse)
        link_directories(${SUITESPARSE_LIBDIR})
        link_directories(${SUITESPARSE_LIBDIR}/lapack_blas_windows)
        link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib64) # libmetis.lib always ends up in lib64
    endif ()

    set(HDF5_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/hdf5-install)
    ExternalProject_Add(hdf5-ext
        GIT_REPOSITORY https://github.com/HDFGroup/hdf5.git
        GIT_TAG 7bf340440909d468dbb3cf41f0ea0d87f5050cea # hdf5-1.14.6
        CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX:STRING=${HDF5_INSTALL_DIR};-DBUILD_SHARED_LIBS:BOOL=OFF;-DHDF5_BUILD_TOOLS:BOOL=OFF;-DHDF5_BUILD_UTILS:BOOL=OFF;-DBUILD_TESTING:BOOL=OFF;-DHDF5_BUILD_EXAMPLES:BOOL=OFF;-DHDF5_BUILD_JAVA:BOOL=OFF;-DHDF5_BUILD_FORTRAN:BOOL=OFF;-DHDF5_BUILD_CPP_LIB:BOOL=OFF;-DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=OFF;-DHDF5_ENABLE_SZIP_SUPPORT:BOOL=OFF;-DHDF5_PACKAGE_EXTLIBS:BOOL=ON;-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    )

    set(HDF5_RELEASE_LIBS libhdf5   libhdf5_hl   shlwapi zlib)
    set(HDF5_DEBUG_LIBS   libhdf5_D libhdf5_hl_D shlwapi zlibd)
    link_directories(${HDF5_INSTALL_DIR}/lib)
    include_directories(${HDF5_INSTALL_DIR}/include)
else(WIN32)
    set(HDF5_RELEASE_LIBS hdf5_serial_hl hdf5_serial)
    set(HDF5_DEBUG_LIBS ${HDF5_RELEASE_LIBS})
    if (NOT APPLE)
        include_directories(/usr/include/hdf5/serial)
        if (ENABLE_SC)
            include_directories(/usr/include/suitesparse)
        endif ()
        link_directories(/usr/lib/x86_64-linux-gnu/hdf5/serial)
    endif()
endif(WIN32)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tools/strong-coupling-core/include)

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

if (ENABLE_SC)
    add_subdirectory(tools/strong-coupling-core)
endif ()

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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall ${WERROR} -Wno-unused-variable -Wno-unused-function -Wextra -Wno-unused-parameter")
  if (ENABLE_SC)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_SC")
  endif ()
  if (ENABLE_HDF5_HACK)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_HDF5_HACK")
  endif ()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDEBUG")
else ()
  if (ENABLE_SC)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DENABLE_SC")
  endif ()
  if (ENABLE_HDF5_HACK)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DENABLE_HDF5_HACK")
  endif ()
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
    include/common/mpi_tools.h
)

SET(MASTER_SRCS
    src/master/main.cpp
    src/master/FMIClient.cpp
    src/master/WeakConnection.cpp
    src/common/common.cpp
    src/common/CSV-parser.cpp
    src/master/BaseMaster.cpp
    src/master/parseargs.cpp
    src/master/modelExchange.cpp
    ${COMMON_SRCS}
    src/master/control.pb.cc
    src/master/globals.cpp
)
if (ENABLE_SC)
    set(MASTER_SRCS ${MASTER_SRCS} src/master/StrongConnector.cpp src/master/StrongMaster.cpp)
endif ()
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

if (WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O3")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif ()
ADD_EXECUTABLE(${TCP_MASTER_NAME} ${MASTER_HEADERS} ${MASTER_SRCS})
ADD_EXECUTABLE(${TCP_SERVER_NAME}  src/server/fmigo-server.cpp src/common/common.cpp ${COMMON_SRCS})

add_executable(${MPI_MASTER_NAME} ${MASTER_HEADERS} ${MASTER_SRCS})
set_target_properties(${MPI_MASTER_NAME} PROPERTIES COMPILE_FLAGS -DUSE_MPI)

IF(WIN32)
    # Multi-threade
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
    SET(WINLIBS
        shlwapi
        ws2_32
        mswsock
        crypt32
        secur32
        mpr
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT})

    if (CMAKE_GENERATOR MATCHES "Ninja")
        set(WINLIBS ${WINLIBS} fmiimport fmicapi jmutils fmixml libexpat fmizip minizip)
    else ()
        set(WINLIBS ${WINLIBS} fmilib)
    endif ()

    # TODO: don't depend on CMAKE_BUILD_TYPE here
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(WINLIBS ${WINLIBS} ${EXTRALIBS} libprotobufd ${HDF5_DEBUG_LIBS})
        if (CMAKE_GENERATOR MATCHES "Ninja")
            # icx via ninja
            set(WINLIBS ${WINLIBS} libzmq-mt-gd-4_3_5)
        else ()
            set(WINLIBS ${WINLIBS} libzmq-${CMAKE_VS_PLATFORM_TOOLSET}-mt-gd-4_3_5)
        endif ()
        if (ENABLE_SC)
            set(WINLIBS ${WINLIBS} sc libumfpackd libamdd libblas libcholmodd libcolamdd libccolamdd libcamdd metisd suitesparseconfigd)
        endif ()
    else()
        set(WINLIBS ${WINLIBS} ${EXTRALIBS} libprotobuf ${HDF5_RELEASE_LIBS})
        if (CMAKE_GENERATOR MATCHES "Ninja")
            set(WINLIBS ${WINLIBS} libzmq-mt-4_3_5)
        else ()
            set(WINLIBS ${WINLIBS} libzmq-${CMAKE_VS_PLATFORM_TOOLSET}-mt-4_3_5)
        endif ()
        if (ENABLE_SC)
            set(WINLIBS ${WINLIBS} sc libumfpack  libamd  libblas libcholmod  libcolamd  libccolamd  libcamd  metis  suitesparseconfig)
        endif ()
    endif()
    if (USE_INTEL_MPI)
        include_directories("${INTEL_MPI_DIR}\\include")
        link_directories("${INTEL_MPI_DIR}\\lib")
    else()
        include_directories($ENV{MSMPI_INC} $ENV{MSMPI_INC}x86)
    endif ()

    if (ENABLE_SC)
        if (CMAKE_GENERATOR MATCHES "^.*Win64") # Maybe there is a better way to detect 64-bit builds?
            install(FILES ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libgcc_s_sjlj-1.dll DESTINATION bin)
        else ()
            install(FILES ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libgcc_s_dw2-1.dll DESTINATION bin)
        endif ()
    endif ()

    TARGET_LINK_LIBRARIES(${TCP_MASTER_NAME} ${WINLIBS})
    TARGET_LINK_LIBRARIES(${TCP_SERVER_NAME}  ${WINLIBS})

    if (USE_INTEL_MPI)
        target_link_libraries(${MPI_MASTER_NAME} ${WINLIBS} impi)
    else ()
        # Only support 64-bit MSMPI
        # Matching "Win64" doesn't work on MSVC 2022
        # Also for some reason link_directories("$ENV{MSMPI_LIB64}") doesn't work
        # Copying msmpi.lib into the build directory does work, so do that
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy "$ENV{MSMPI_LIB64}/msmpi.lib" "${CMAKE_CURRENT_BINARY_DIR}")
        link_directories("${CMAKE_CURRENT_BINARY_DIR}")
        target_link_libraries(${MPI_MASTER_NAME} ${WINLIBS} msmpi)
    endif ()

    if (ENABLE_SC)
        install (FILES
            ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libblas.dll
            ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libgfortran-3.dll
            ${SUITESPARSE_LIBDIR}/lapack_blas_windows/libquadmath-0.dll
            DESTINATION bin
        )
    endif ()
    if (CMAKE_GENERATOR MATCHES "Ninja")
        # icx via ninja
        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            INSTALL (FILES
                    ${PROTOBUF_PREFIX}/src/protobuf-ext-build/libprotobufd.dll
                    ${ZMQ_PREFIX}/src/zmq-ext-build/bin/libzmq-mt-gd-4_3_5.dll
                    DESTINATION bin)
        else ()
            INSTALL (FILES
                ${PROTOBUF_PREFIX}/src/protobuf-ext-build/libprotobuf.dll
                ${ZMQ_PREFIX}/src/zmq-ext-build/bin/libzmq-mt-4_3_5.dll
                DESTINATION bin)
        endif ()
    else ()
        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            INSTALL (FILES
                ${PROTOBUF_PREFIX}/src/protobuf-ext-build/${CMAKE_BUILD_TYPE}/libprotobufd.dll
                ${ZMQ_PREFIX}/src/zmq-ext-build/bin/${CMAKE_BUILD_TYPE}/libzmq-${CMAKE_VS_PLATFORM_TOOLSET}-mt-gd-4_3_5.dll
                DESTINATION bin)
        else()
            INSTALL (FILES
                ${PROTOBUF_PREFIX}/src/protobuf-ext-build/${CMAKE_BUILD_TYPE}/libprotobuf.dll
                ${ZMQ_PREFIX}/src/zmq-ext-build/bin/${CMAKE_BUILD_TYPE}/libzmq-${CMAKE_VS_PLATFORM_TOOLSET}-mt-4_3_5.dll
                DESTINATION bin)
        endif()
    endif ()
    set(PROTOC ${PROTOBUF_BIN_DIR}/protoc)
ELSE(WIN32)
    SET(LINUXLIBS
        fmilib
        dl
        ${HDF5_RELEASE_LIBS}
        zmq
        ${MPI_LIBRARIES}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${PROTOBUF_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT})
    if (ENABLE_SC)
        set(LINUXLIBS ${LINUXLIBS} sc m umfpack amd blas cholmod colamd suitesparseconfig) # For strong coupling
    endif ()
    TARGET_LINK_LIBRARIES(${TCP_MASTER_NAME} ${LINUXLIBS} ${EXTRALIBS})
    TARGET_LINK_LIBRARIES(${TCP_SERVER_NAME}  ${LINUXLIBS})

    # mpicc/cxx uses gcc/g++, which needs -Wno-literal-suffix on trusty
    # Sometimes clang gets picked up which lacks -Wno-literal-suffix on trusty, hence -Wno-unknown-warning-option
    target_compile_options(${MPI_MASTER_NAME} PRIVATE -Wno-unknown-warning-option -Wno-literal-suffix)
    target_link_libraries(${MPI_MASTER_NAME} ${LINUXLIBS} ${EXTRALIBS})

    add_custom_target(libprotobuf)  #fake target
    set(PROTOC protoc)
ENDIF(WIN32)

if (USE_PROTOC)
    # Compile proto
    set(FMITCP_PROTOBUF_TARGETS ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp_pb2.py)
    add_custom_command(OUTPUT ${FMITCP_PROTOBUF_TARGETS}
        COMMAND ${PROTOC} --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp --python_out=${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp/fmitcp.proto --proto_path ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp
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
endif ()


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

add_dependencies(fmigo-master fmi-library-ext)
add_dependencies(fmigo-server fmi-library-ext)
add_dependencies(fmigo-mpi    fmi-library-ext)

if (USE_EXTERNAL_LIBZMQ)
    add_dependencies(fmigo-master zmq-ext)
    add_dependencies(fmigo-server zmq-ext)
    add_dependencies(fmigo-mpi    zmq-ext)
endif ()

if (USE_EXTERNAL_PROTOBUF)
    add_dependencies(fmigo-master protobuf-ext)
    add_dependencies(fmigo-server protobuf-ext)
    add_dependencies(fmigo-mpi    protobuf-ext)
    if (USE_PROTOC)
        add_dependencies(fmitcp_pb    protobuf-ext)
        add_dependencies(control_pb   protobuf-ext)
    endif ()
endif ()

if (WIN32)
    if (ENABLE_SC)
        add_dependencies(sc suitesparse-ext)
    endif ()
    add_dependencies(fmigo-master hdf5-ext)
    add_dependencies(fmigo-server hdf5-ext)
    add_dependencies(fmigo-mpi    hdf5-ext)
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

if (NOT WIN32)
    add_executable(mpi-speed-test src/mpi-speed-test.cpp)
    # OpenMPI on trusty won't compile with -std=c++11 or later
    # https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=796985
    set_target_properties(mpi-speed-test PROPERTIES COMPILE_FLAGS -std=c++03)
    install(TARGETS mpi-speed-test DESTINATION bin)

    if (USE_TRACEANALYZER)
        set_target_properties(mpi-speed-test PROPERTIES LINK_FLAGS -trace)
        set_target_properties(${MPI_MASTER_NAME} PROPERTIES LINK_FLAGS -trace)
    endif ()
endif ()


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
