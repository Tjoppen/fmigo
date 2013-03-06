cmake_minimum_required(VERSION 2.8)
SET(SRCS src/master.c)
ADD_EXECUTABLE(build/master ${SRCS})
TARGET_LINK_LIBRARIES(build/master fmilib dl)