cmake_minimum_required(VERSION 2.8)

#enable testing
enable_testing()
# Includes which tests that should be skipped
configure_file(${CMAKE_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_BINARY_DIR})

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
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3dparty/zeromq/4.2.0-master/include) #zmq.hpp
    link_directories("${ZMQ_PREFIX}/src/zmq-ext-build/lib/${CMAKE_BUILD_TYPE}")

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3dparty/zeromq/4.2.0-master/include)
else(WIN32)
    # TODO: tell the user what's needed?
	include(FindProtobuf)
	find_package(Protobuf REQUIRED)
	include_directories(${PROTOBUF_INCLUDE_DIR})
	link_directories(${PROTOBUF_LIBRARY_DIR})

    include_directories(/usr/include/hdf5/serial)
    include_directories(/usr/include/suitesparse)
    link_directories(/usr/lib/x86_64-linux-gnu/hdf5/serial)
endif(WIN32)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/strong-coupling-core/include)

set(FMILIBRARY_VERSION FMILibrary-2.0.1)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/${FMILIBRARY_VERSION})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/src/CAPI/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/src/Import/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/src/Util/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/src/XML/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/src/ZIP/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/ThirdParty/FMI/default)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/ThirdParty/Expat/expat-2.1.0/lib)

if (APPLE)
    # For Macports
  
    if (EXISTS /opt/local/include)
        include_directories(/opt/local/include)
    endif()

    if (EXISTS /opt/local/lib)
        link_directories(/opt/local/lib)
    endif()

endif()

add_subdirectory(strong-coupling-core)
add_subdirectory(${FMILIBRARY_VERSION})

if (USE_GPL)
  if (WIN32)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/umit-fmus/wingsl/lib)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/umit-fmus/wingsl/include)
    set(CMAKE_SHARED_LINKER_FLAGS "/SAFESEH:NO")
    set(CMAKE_EXE_LINKER_FLAGS "/SAFESEH:NO")
  endif ()

  # We should have umit-fmus checked out
  # Use its cgsl regardless of whether BUILD_FMUS is ON or not
  add_subdirectory(umit-fmus/templates/cgsl)
endif ()

add_subdirectory(fmigo)
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
    add_subdirectory(umit-fmus)
endif ()

add_subdirectory(ssp)

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
