cmake_minimum_required(VERSION 2.8)

SET(SRCS src/master.c)

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/bin")

ADD_EXECUTABLE(master ${SRCS})

TARGET_LINK_LIBRARIES(master fmilib dl)