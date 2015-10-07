cmake_minimum_required(VERSION 2.8)

# Download/install protobuf, umfpack as needed
if(WIN32)
    set(PROTOC_URL "https://github.com/google/protobuf/releases/download/v2.6.1/protoc-2.6.1-win32.zip")
    set(PROTOC_MD5 "b057f86ef83835010bb227eb2d82de04")
    set(PROTOC_DOWNLOAD_PATH "${CMAKE_CURRENT_BINARY_DIR}/protoc-2.6.1-win32.zip")

    if (NOT EXISTS "${PROTOC_DOWNLOAD_PATH}")
        file(DOWNLOAD "${PROTOC_URL}" "${PROTOC_DOWNLOAD_PATH}" EXPECTED_MD5 ${PROTOC_MD5})
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar x ${PROTOC_DOWNLOAD_PATH}
            DEPENDS "${PROTOC_DOWNLOAD_PATH}"
        )
    endif()

    set(PROTOBUF_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})

    set(PROTOBUF_URL "https://github.com/google/protobuf/releases/download/v2.6.1/protobuf-2.6.1.zip")
    set(PROTOBUF_MD5 "b057f86ef83835010bb227eb2d82de04")
    set(PROTOBUF_DOWNLOAD_PATH "${CMAKE_CURRENT_BINARY_DIR}/protobuf-2.6.1.zip")

    if (NOT EXISTS "${PROTOBUF_DOWNLOAD_PATH}")
        file(DOWNLOAD "${PROTOBUF_URL}" "${PROTOBUF_DOWNLOAD_PATH}" EXPECTED_MD5 ${PROTOBUF_MD5})
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar x ${PROTOBUF_DOWNLOAD_PATH}
            DEPENDS "${PROTOBUF_DOWNLOAD_PATH}"
        )
    endif()

    include_directories(${CMAKE_CURRENT_BINARY_DIR}/protobuf-2.6.1/src)

    set(SUITESPARSE_COMMIT "232acd3f6c2ab2539167043e31c22a1ac1f599b8")
    set(SUITESPARSE_DIR "${CMAKE_CURRENT_BINARY_DIR}/suitesparse-metis-for-windows-${SUITESPARSE_COMMIT}")
    set(SUITESPARSE_URL "https://github.com/jlblancoc/suitesparse-metis-for-windows/archive/${SUITESPARSE_COMMIT}.zip")
    set(SUITESPARSE_MD5 "1e9b5fcdc38ec0c2aebe3d3cbd1ee8cd")

    include(ExternalProject)
    ExternalProject_Add(suitesparse
        PREFIX ${SUITESPARSE_DIR}
        URL ${SUITESPARSE_URL}
        URL_MD5 ${SUITESPARSE_MD5}
        #CMAKE_ARGS ""
        #UPDATE_COMMAND ""
    )

    include_directories(${SUITESPARSE_DIR}/src/suitesparse-build/install/include/suitesparse)
    link_directories(${SUITESPARSE_DIR}/src/suitesparse-build/install/lib)
else(WIN32)
    # TODO: tell the user what's needed?
endif(WIN32)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fmi-tcp/src/fmitcp/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/strong-coupling-core/include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-tcp/lib)
add_subdirectory(strong-coupling-core)
#add_dependencies(sc suitesparse)
add_subdirectory(FMILibrary-2.0b2)
add_subdirectory(fmi-tcp)
add_subdirectory(fmi-tcp-master)