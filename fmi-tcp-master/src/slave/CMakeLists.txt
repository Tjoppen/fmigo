cmake_minimum_required(VERSION 2.8)

# Protobuf
INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(Threads REQUIRED)

INCLUDE_DIRECTORIES(../../lib ../ ${FMIL_INCLUDE_DIR} ${LACEWING_INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIR} include ../fmitcp/include)
LINK_DIRECTORIES(${FMIL_LIBS_DIR} ${LACEWING_LIBS_DIR} ${PROTOBUF_LIBRARY} ${CMAKE_BINARY_DIR}/../lib)

SET(SLAVE_SRCS
  src/main.cpp
)
SET(SLAVE_HEADERS
)

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/../../bin")

ADD_EXECUTABLE(slave ${SLAVE_HEADERS}   ${SLAVE_SRCS})

IF(WIN32)
  TARGET_LINK_LIBRARIES(slave
    fmitcp
    fmilib
    shlwapi
    lacewing
    ws2_32
    mswsock
    crypt32
    secur32
    mpr
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    protobuf
    ${CMAKE_THREAD_LIBS_INIT}
)
ELSE(WIN32)
  TARGET_LINK_LIBRARIES(slave
    fmitcp
    fmilib
    dl
    lacewing
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${PROTOBUF_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)
ENDIF(WIN32)
