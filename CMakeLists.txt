cmake_minimum_required(VERSION 2.8)

if(CMAKE_VERSION VERSION_GREATER "3.3")
    #suppress warning about libexpat.a
    cmake_policy(SET CMP0058 OLD)
endif()


# Set default build type
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "CMake build type set to RELEASE.")
  set(CMAKE_BUILD_TYPE Release CACHE String "" FORCE)
endif()


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

# Download/install protobuf, umfpack as needed
if(WIN32)
    add_definitions(-D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS) # required by protobuf-ext

    if (MSVC14)
        set(PROTOBUF_PROJECTS_DIR "msvc2015")
        set(MSVC_TOOLCHAIN_VERSION "/p:VisualStudioVersion=14.0")
    elseif (MSV12)
        set(PROTOBUF_PROJECTS_DIR "msvc2013")
        set(MSVC_TOOLCHAIN_VERSION "/p:VisualStudioVersion=12.0")
    else (MSVC14)
        # Assume there is only one toolchain installed and it is compatible with MSVC12
        set(MSVC_TOOLCHAIN_VERSION "")
        set(PROTOBUF_PROJECTS_DIR "msvc2013")
    endif (MSVC14)

    include(ExternalProject)

    set(PROTOBUF_URL "https://github.com/google/protobuf/releases/download/v2.6.1/protobuf-2.6.1.zip")
    set(PROTOBUF_MD5 "54e31efec42451ed73f99abc39f8c745")
    set(PROTOBUF_DOWNLOAD_PATH "${CMAKE_CURRENT_BINARY_DIR}/protobuf-2.6.1.zip")
    set(PROTOBUF_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/protobuf-2.6.1")
    set(PROTOBUF_DIR "${PROTOBUF_PREFIX}/src/protobuf-ext")

    ExternalProject_Add(protobuf-ext
        PREFIX "${PROTOBUF_PREFIX}"
        URL ${PROTOBUF_URL}
        URL_MD5 ${PROTOBUF_MD5}
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${PROTOBUF_PROJECTS_DIR}/protobuf-2.6.1 ${PROTOBUF_DIR}
        BINARY_DIR "${PROTOBUF_DIR}/vsprojects"
        # Only build required libs because test fails with MSVC14
        # BUILD_COMMAND msbuild protobuf.sln /p:Configuration=${CMAKE_BUILD_TYPE} ${MSVC_TOOLCHAIN_VERSION}
        BUILD_COMMAND msbuild libprotobuf.vcxproj /p:Configuration=${CMAKE_BUILD_TYPE} ${MSVC_TOOLCHAIN_VERSION}
              COMMAND msbuild libprotoc.vcxproj /p:Configuration=${CMAKE_BUILD_TYPE} ${MSVC_TOOLCHAIN_VERSION}
              COMMAND msbuild protoc.vcxproj /p:Configuration=${CMAKE_BUILD_TYPE} ${MSVC_TOOLCHAIN_VERSION}
        # BUILD_COMMAND msbuild libprotoc.vcxproj /p:Configuration=${CMAKE_BUILD_TYPE} ${MSVC_TOOLCHAIN_VERSION}
        # BUILD_COMMAND msbuild protoc.vcxproj /p:Configuration=${CMAKE_BUILD_TYPE} ${MSVC_TOOLCHAIN_VERSION}
        INSTALL_COMMAND ""
    )

    include_directories("${PROTOBUF_DIR}/src")
    link_directories("${PROTOBUF_DIR}/vsprojects/${CMAKE_BUILD_TYPE}")
    set(PROTOBUF_BIN_DIR "${PROTOBUF_DIR}/vsprojects/${CMAKE_BUILD_TYPE}")

    set(SUITESPARSE_COMMIT "232acd3f6c2ab2539167043e31c22a1ac1f599b8")
    set(SUITESPARSE_DIR "${CMAKE_CURRENT_BINARY_DIR}/suitesparse-metis-for-windows")
    set(SUITESPARSE_URL "https://github.com/jlblancoc/suitesparse-metis-for-windows/archive/${SUITESPARSE_COMMIT}.zip")
    set(SUITESPARSE_MD5 "1e9b5fcdc38ec0c2aebe3d3cbd1ee8cd")

    ExternalProject_Add(suitesparse-ext
        PREFIX ${SUITESPARSE_DIR}
        URL ${SUITESPARSE_URL}
        URL_MD5 ${SUITESPARSE_MD5}
    )

    include_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/include)
    include_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/include/suitesparse)
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib)
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows)
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib64)

        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3dparty/zeromq/4.2.0-master/include)
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3dparty/zeromq/4.2.0-master/cl32-11)

    set(HDF5_DIR "${CMAKE_CURRENT_BINARY_DIR}/hdf5")
    ExternalProject_Add(hdf5-ext
        PREFIX  ${HDF5_DIR}
        URL     "https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8.17/src/CMake-hdf5-1.8.17.zip"
        URL_MD5 "8cc32abfbe9cc8815386372783301257"
        CONFIGURE_COMMAND ${CMAKE_COMMAND}          ${HDF5_DIR}/src/hdf5-ext/hdf5-1.8.17 -G ${CMAKE_GENERATOR}
        BUILD_COMMAND     ${CMAKE_COMMAND} -E chdir ${HDF5_DIR}/src/hdf5-ext-build msbuild HDF5.sln /p:Configuration=${CMAKE_BUILD_TYPE} ${MSVC_TOOLCHAIN_VERSION}
        INSTALL_COMMAND   ""
    )

    include_directories(${HDF5_DIR}/src/hdf5-ext/hdf5-1.8.17/src)
    include_directories(${HDF5_DIR}/src/hdf5-ext/hdf5-1.8.17/hl/src)
    include_directories(${HDF5_DIR}/src/hdf5-ext-build)
    link_directories(${HDF5_DIR}/src/hdf5-ext-build/bin/${CMAKE_BUILD_TYPE})

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

add_subdirectory(strong-coupling-core)
add_subdirectory(${FMILIBRARY_VERSION})
add_subdirectory(fmigo)
add_dependencies(fmigo-master fmilib)
add_dependencies(fmigo-mpi    fmilib)

if (WIN32)
    add_dependencies(sc suitesparse-ext)
    add_dependencies(fmigo-master protobuf-ext hdf5-ext)
    add_dependencies(fmigo-mpi    protobuf-ext hdf5-ext)
endif ()

if (BUILD_FMUS)
    add_subdirectory(umit-fmus)
endif ()

add_subdirectory(ssp)
