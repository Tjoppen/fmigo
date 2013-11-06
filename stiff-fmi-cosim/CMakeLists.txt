CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(TARGET main)
SET(SRCS src/main.c src/model.c src/fmi.c )
SET(HEADERS src/fmi.h src/model.h src/fmiFunctions.h )

ADD_EXECUTABLE(${TARGET} ${SRCS} ${HEADERS})

TARGET_LINK_LIBRARIES( ${TARGET} m )

