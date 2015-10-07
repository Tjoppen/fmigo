cmake_minimum_required(VERSION 2.8)

# Download/install protobuf, umfpack as needed
if(WIN32)
    set(PROTOBUF_URL "https://github.com/google/protobuf/releases/download/v2.6.1/protoc-2.6.1-win32.zip")
    set(PROTOBUF_MD5 "b057f86ef83835010bb227eb2d82de04")
    set(PROTOBUF_DOWNLOAD_PATH "${CMAKE_CURRENT_BINARY_DIR}/protoc-2.6.1-win32.zip")

    if (NOT EXISTS "${PROTOBUF_DOWNLOAD_PATH}")
        file(DOWNLOAD "${PROTOBUF_URL}" "${PROTOBUF_DOWNLOAD_PATH}" EXPECTED_MD5 ${PROTOBUF_MD5})
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar x ${PROTOBUF_DOWNLOAD_PATH}
            DEPENDS "${PROTOBUF_DOWNLOAD_PATH}"
        )
    endif()

    set(PROTOBUF_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
else(WIN32)
    # TODO: tell the user what's needed?
endif(WIN32)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fmi-tcp/src/fmitcp/include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/fmi-tcp/lib)
add_subdirectory(strong-coupling-core)
add_subdirectory(FMILibrary-2.0b2)
add_subdirectory(fmi-tcp)
add_subdirectory(fmi-tcp-master)