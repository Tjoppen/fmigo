CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(RIGID_SRCS
    RigidBody.cpp
    rigid.cpp
)
SET(RIGID_HEADERS
    RigidBody.h
)

INCLUDE_DIRECTORIES(${UMFPACK_INCLUDE_DIR})
LINK_DIRECTORIES(   ${UMFPACK_LIBRARY_DIR})

OPTION(USE_OSG "Enables OSG viewing of the rigid demo")
IF(USE_OSG)
ADD_DEFINITIONS(-DSC_USE_OSG)
ENDIF()

ADD_EXECUTABLE(rigid       ${RIGID_HEADERS} ${RIGID_SRCS})

IF(USE_OSG)
    SET(OSGLIBS osg osgViewer osgGA osgUtil)
ELSE()
    SET(OSGLIBS )
ENDIF()

IF(WIN32)
    TARGET_LINK_LIBRARIES(rigid      sc umfpack amd cblas suitesparseconfig ${OSGLIBS})
ELSE()
    #TARGET_LINK_LIBRARIES(rigid      sc m umfpack amd cblas ${OSGLIBS})
    TARGET_LINK_LIBRARIES(rigid      sc m umfpack amd ${OSGLIBS})
ENDIF()

