SET(HEADERS
    include/fmitcp/fmitcp.pb.h
    include/fmitcp/Client.h
    include/fmitcp/common.h
    include/fmitcp/Logger.h
    include/fmitcp/Server.h
    include/fmitcp/serialize.h
)
SET(SRCS
    src/fmitcp.pb.cc
    src/Client.cpp
    src/common.cpp
    src/Logger.cpp
    src/Server.cpp
    src/serialize.cpp
)

if(USE_LACEWING)
    set(HEADERS ${HEADERS} include/fmitcp/EventPump.h)
    set(SRCS    ${SRCS}    src/EventPump.cpp)
endif()

# Compile proto
IF(WIN32)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/fmitcp/fmitcp.pb.h
        COMMAND ${PROTOBUF_BIN_DIR}/protoc --cpp_out=. fmitcp.proto
        COMMAND cmake -E rename ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/include/fmitcp/fmitcp.pb.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        DEPENDS libprotobuf ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp.proto)
ELSE(WIN32)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/fmitcp/fmitcp.pb.h
        COMMAND protoc --cpp_out=. fmitcp.proto
        COMMAND cmake -E rename ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/include/fmitcp/fmitcp.pb.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/fmitcp.proto)
ENDIF(WIN32)

INCLUDE_DIRECTORIES(include/fmitcp  ${FMIL_INCLUDE_DIR} ${LACEWING_INCLUDE_DIR})
LINK_DIRECTORIES(${FMIL_LIBS_DIR} ${LACEWING_LIBS_DIR})

ADD_LIBRARY(fmitcp ${HEADERS} ${SRCS})
add_dependencies(fmitcp ${CMAKE_CURRENT_SOURCE_DIR}/include/fmitcp/fmitcp.pb.h)

SET_TARGET_PROPERTIES( fmitcp
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../../lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../../lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../../bin"
)

INSTALL(FILES ${HEADERS} DESTINATION include/fmitcp)
INSTALL(TARGETS fmitcp DESTINATION lib)
