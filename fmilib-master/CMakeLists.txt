cmake_minimum_required(VERSION 2.8)

SET(SRCS
    src/main.c
    src/utils.c
    src/parseargs.c
    src/help.c
    src/jacobi.c
    src/gs.c
    src/simulate.c
    src/log.c
)
SET(HEADERS
    src/main.h
    src/utils.h
    src/parseargs.h
    src/help.h
    src/jacobi.h
    src/gs.h
    src/simulate.h
    src/log.h
)

set(MASTER_NAME fmu-master)

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/bin")

ADD_EXECUTABLE(${MASTER_NAME} ${HEADERS} ${SRCS})

TARGET_LINK_LIBRARIES(${MASTER_NAME} fmilib dl rt)

SET(IN_MANPAGE ${CMAKE_CURRENT_LIST_DIR}/doc/${MASTER_NAME}.1)
SET(OUT_MANPAGE ${CMAKE_CURRENT_BINARY_DIR}/${MASTER_NAME}.1.gz)
ADD_CUSTOM_TARGET(manpage COMMAND gzip -kc9 ${IN_MANPAGE} > ${OUT_MANPAGE} DEPENDS ${IN_MANPAGE} SOURCES ${IN_MANPAGE})
ADD_DEPENDENCIES(${MASTER_NAME} manpage)

INSTALL (TARGETS ${MASTER_NAME} DESTINATION bin)
INSTALL (FILES ${OUT_MANPAGE} DESTINATION man/man1)