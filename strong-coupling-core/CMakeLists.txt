CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

SET(SRCS
    src/BallJointConstraint.cpp
    src/Connector.cpp
    src/Constraint.cpp
    src/Equation.cpp
    src/HingeConstraint.cpp
    src/HingeMotorConstraint.cpp
    src/JacobianElement.cpp
    src/LockConstraint.cpp
    src/ShaftConstraint.cpp
    src/Mat3.cpp
    src/Quat.cpp
    src/Slave.cpp
    src/Solver.cpp
    src/Vec3.cpp
)

SET(HEADERS
    include/sc/BallJointConstraint.h
    include/sc/Connector.h
    include/sc/Constraint.h
    include/sc/Equation.h
    include/sc/HingeConstraint.h
    include/sc/HingeMotorConstraint.h
    include/sc/JacobianElement.h
    include/sc/LockConstraint.h
    include/sc/ShaftConstraint.h
    include/sc/Mat3.h
    include/sc/Quat.h
    include/sc/Slave.h
    include/sc/Solver.h
    include/sc/Vec3.h
)

INCLUDE_DIRECTORIES(${UMFPACK_INCLUDE_DIR} include /usr/include/suitesparse)
LINK_DIRECTORIES(${UMFPACK_LIBRARY_DIR})

ADD_LIBRARY(sc STATIC ${HEADERS} ${SRCS})

# OSX
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    TARGET_LINK_LIBRARIES(sc m umfpack amd cblas cholmod colamd suitesparseconfig)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# LINUX
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  ## on Ubuntu, atlas is needed
    #TARGET_LINK_LIBRARIES(sc m umfpack amd blas atlas cholmod colamd suitesparseconfig)
  ## on Arch, this is enough
    TARGET_LINK_LIBRARIES(sc m umfpack amd blas cholmod colamd suitesparseconfig)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if (WIN32)
    # No libm on Windows
    target_link_libraries(sc libumfpack  libamd  libblas libcholmod  libcolamd  libccolamd  libcamd  metis  suitesparseconfig)
endif ()

#TODO: Fix test build on Windows
IF(NOT WIN32)
    ADD_SUBDIRECTORY(test)
ENDIF(NOT WIN32)

INSTALL(TARGETS sc
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
INSTALL(FILES ${HEADERS} DESTINATION include/sc)
