cmake_minimum_required(VERSION 2.8)

option(USE_LACEWING "Use lacewing instead of zeromq?" OFF)

if (USE_LACEWING)
    add_definitions(-DUSE_LACEWING)
endif()

# Download/install protobuf, umfpack as needed
if(WIN32)
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
        CONFIGURE_COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/msvc2013/protobuf-2.6.1 ${PROTOBUF_DIR}
        BINARY_DIR "${PROTOBUF_DIR}/vsprojects"
        BUILD_COMMAND msbuild protobuf.sln
        INSTALL_COMMAND ""
    )

    include_directories("${PROTOBUF_DIR}/src")
    link_directories("${PROTOBUF_DIR}/vsprojects/Debug")
    set(PROTOBUF_BIN_DIR "${PROTOBUF_DIR}/vsprojects/Debug")

    set(SUITESPARSE_COMMIT "232acd3f6c2ab2539167043e31c22a1ac1f599b8")
    set(SUITESPARSE_DIR "${CMAKE_CURRENT_BINARY_DIR}/suitesparse-metis-for-windows-${SUITESPARSE_COMMIT}")
    set(SUITESPARSE_URL "https://github.com/jlblancoc/suitesparse-metis-for-windows/archive/${SUITESPARSE_COMMIT}.zip")
    set(SUITESPARSE_MD5 "1e9b5fcdc38ec0c2aebe3d3cbd1ee8cd")

    ExternalProject_Add(suitesparse-ext
        PREFIX ${SUITESPARSE_DIR}
        URL ${SUITESPARSE_URL}
        URL_MD5 ${SUITESPARSE_MD5}
    )

    include_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/include/suitesparse)
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib)
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows)
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib64)

    set(LACEWING_COMMIT "93c825a")
    set(LACEWING_URL "https://github.com/udp/lacewing/zipball/0.5.4/udp-lacewing-0.5.4-0-g${LACEWING_COMMIT}.zip")
    set(LACEWING_DIR "${CMAKE_CURRENT_BINARY_DIR}/udp-lacewing-${LACEWING_COMMIT}")
    set(LACEWING_MD5 "953a6be9b5ced6cec3e31a199bcd6c4e")
    set(LACEWING_BIN_DIR "${LACEWING_DIR}/src/lacewing-ext/msvc")

    ExternalProject_Add(lacewing-ext
        PREFIX ${LACEWING_DIR}
        URL "${LACEWING_URL}"
        URL_MD5 "${LACEWING_MD5}"
        CONFIGURE_COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/msvc2013/udp-lacewing-${LACEWING_COMMIT} ${LACEWING_BIN_DIR}/..
        BINARY_DIR "${LACEWING_BIN_DIR}"
        BUILD_COMMAND msbuild liblacewing.sln
        INSTALL_COMMAND cmake -E copy ${LACEWING_BIN_DIR}/Debug/liblacewing.lib ${LACEWING_BIN_DIR}/Debug/lacewing.lib
    )

    link_directories(${LACEWING_BIN_DIR}/Debug)
    include_directories("${LACEWING_DIR}/src/lacewing-ext/include")
else(WIN32)
    # TODO: tell the user what's needed?
endif(WIN32)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fmi-tcp/src/fmitcp/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/strong-coupling-core/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/FMILibrary-2.0b2)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/FMILibrary-2.0b2/src/CAPI/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/FMILibrary-2.0b2/src/Import/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/FMILibrary-2.0b2/src/Util/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/FMILibrary-2.0b2/src/XML/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/FMILibrary-2.0b2/src/ZIP/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/FMILibrary-2.0b2/ThirdParty/FMI/default)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-tcp/lib)
add_subdirectory(strong-coupling-core)
add_dependencies(sc suitesparse-ext)
add_subdirectory(FMILibrary-2.0b2)
add_subdirectory(fmi-tcp)
add_dependencies(fmitcp fmilib)
add_subdirectory(fmi-tcp-master)