SET(HEADERS
    include/fmitcp/fmitcp.pb.h
    include/fmitcp/Client.h
    include/fmitcp/common.h
    include/fmitcp/Logger.h
    include/fmitcp/Server.h
    include/fmitcp/EventPump.h
)
SET(SRCS
    src/fmitcp.pb.cc
    src/Client.cpp
    src/common.cpp
    src/Logger.cpp
    src/Server.cpp
    src/EventPump.cpp
)

# Compile proto
IF(WIN32)
  EXECUTE_PROCESS(COMMAND ${PROTOBUF_BIN_DIR}/protoc --cpp_out=. fmitcp.proto
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
ELSE(WIN32)
  EXECUTE_PROCESS(COMMAND protoc --cpp_out=. fmitcp.proto
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
ENDIF(WIN32)

FILE(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp.pb.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/fmitcp/fmitcp.pb.h)

INCLUDE_DIRECTORIES(include/fmitcp  ${FMIL_INCLUDE_DIR} ${LACEWING_INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIR})
LINK_DIRECTORIES(${FMIL_LIBS_DIR} ${LACEWING_LIBS_DIR} ${PROTOBUF_LIBRARY})

ADD_LIBRARY(fmitcp ${HEADERS} ${SRCS})

SET_TARGET_PROPERTIES( fmitcp
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../../lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../../lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../../bin"
)

INSTALL(FILES ${HEADERS} DESTINATION include/fmitcp)
INSTALL(TARGETS fmitcp DESTINATION lib)
