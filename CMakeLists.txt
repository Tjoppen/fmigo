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
        BUILD_COMMAND msbuild protobuf.sln  /p:Configuration=${CMAKE_BUILD_TYPE}
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
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib)
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib/lapack_blas_windows)
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-ext-build/install/lib64)

    if (USE_LACEWING)
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
            BUILD_COMMAND msbuild liblacewing.sln  /p:Configuration=${CMAKE_BUILD_TYPE}
            INSTALL_COMMAND cmake -E copy ${LACEWING_BIN_DIR}/${CMAKE_BUILD_TYPE}/liblacewing.lib ${LACEWING_BIN_DIR}/${CMAKE_BUILD_TYPE}/lacewing.lib
        )

        link_directories(${LACEWING_BIN_DIR}/${CMAKE_BUILD_TYPE})
        include_directories("${LACEWING_DIR}/src/lacewing-ext/include")
    else()
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3dparty/zeromq/4.2.0-master/include)
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3dparty/zeromq/4.2.0-master/cl32-11)
    endif()

    set(JSONCPP_DIR "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp")
    set(JSONCPP_URL "https://github.com/open-source-parsers/jsoncpp/archive/0.10.5.zip")
    set(JSONCPP_MD5 "7ebcc0530b77cd25b2d6920dd19c2d53")

    ExternalProject_Add(jsoncpp-ext
        PREFIX ${JSONCPP_DIR}
        URL ${JSONCPP_URL}
        URL_MD5 ${JSONCPP_MD5}
        BUILD_COMMAND msbuild jsoncpp.sln  /p:Configuration=${CMAKE_BUILD_TYPE}
        INSTALL_COMMAND ""
    )

    include_directories(${JSONCPP_DIR}/src/jsoncpp-ext/include)
    link_directories(${JSONCPP_DIR}/src/jsoncpp-ext-build/src/lib_json/${CMAKE_BUILD_TYPE})

else(WIN32)
    # TODO: tell the user what's needed?
	include(FindProtobuf)
	find_package(Protobuf REQUIRED)
	include_directories(${PROTOBUF_INCLUDE_DIR})
	link_directories(${PROTOBUF_LIBRARY_DIR})

    include_directories(/usr/include/hdf5/serial)
    link_directories(/usr/lib/x86_64-linux-gnu/hdf5/serial)
endif(WIN32)

if(APPLE)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3dparty/zeromq/4.2.0-master/include)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fmi-tcp/src/fmitcp/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/strong-coupling-core/include)

set(FMILIBRARY_VERSION FMILibrary-2.0.1)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/${FMILIBRARY_VERSION})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/src/CAPI/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/src/Import/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/src/Util/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/src/XML/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/src/ZIP/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${FMILIBRARY_VERSION}/ThirdParty/FMI/default)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/fmi-tcp/lib)
add_subdirectory(strong-coupling-core)
add_dependencies(sc suitesparse-ext)
add_subdirectory(${FMILIBRARY_VERSION})
add_subdirectory(fmi-tcp-master)
add_dependencies(fmi-tcp-master fmilib jsoncpp-ext protobuf-ext)
