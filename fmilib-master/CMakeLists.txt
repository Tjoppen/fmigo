cmake_minimum_required(VERSION 2.8)

SET(SRCS
    src/main.c
    src/utils.c
    src/parseargs.c
    src/help.c
    src/jacobi.c
)
SET(HEADERS
    src/main.h
    src/utils.h
    src/parseargs.h
    src/help.h
    src/jacobi.h
)

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/bin")

ADD_EXECUTABLE(fmu-master ${HEADERS} ${SRCS})

TARGET_LINK_LIBRARIES(fmu-master fmilib dl)