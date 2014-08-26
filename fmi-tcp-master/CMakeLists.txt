CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Protobuf
INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(Threads REQUIRED)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

INCLUDE_DIRECTORIES(${FMIL_INCLUDE_DIR} ${STRONG_COUPLING_INCLUDE_DIR} ${FMITCP_INCLUDE_DIR} ${LACEWING_INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIR} include)
LINK_DIRECTORIES   (${FMIL_LIBS_DIR}    ${STRONG_COUPLING_LIBS_DIR}    ${FMITCP_LIBS_DIR}    ${LACEWING_LIBS_DIR}    ${PROTOBUF_LIBRARY})

SET(MASTER_SRCS
    src/master/Connection.cpp
    src/master/StrongConnector.cpp
    src/master/main.cpp
    src/master/Master.cpp
    src/master/FMIClient.cpp
    src/master/WeakConnection.cpp
    src/common/url_parser.c
    src/common/common.cpp
    src/master/BaseMaster.cpp
    src/master/parseargs.cpp
    src/master/StrongMaster.cpp
)
SET(MASTER_HEADERS
    include/master/Connection.h
    include/master/Master.h
    include/master/FMIClient.h
    include/master/StrongConnector.h
    include/master/WeakConnection.h
    include/common/common.h
    include/common/url_parser.h
)

SET(TEST_SRCS
    test/main.cpp
    src/master/Connection.cpp
    src/master/Master.cpp
    src/master/FMIClient.cpp
    src/master/StrongConnector.cpp
    src/master/WeakConnection.cpp
    src/common/url_parser.c
    src/common/common.cpp
)
SET(TEST_HEADERS
    include/master/Connection.h
    include/master/Master.h
    include/master/FMIClient.h
    include/master/StrongConnector.h
    include/master/WeakConnection.h
    include/common/common.h
    include/common/url_parser.h
)

SET(SLAVE_SRCS
  src/slave/main.cpp
)
SET(SLAVE_HEADERS
)

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/bin")

ADD_EXECUTABLE(fmi-tcp-master ${MASTER_HEADERS} ${MASTER_SRCS})
ADD_EXECUTABLE(fmi-tcp-slave  ${SLAVE_HEADERS}  ${SLAVE_SRCS})
ADD_EXECUTABLE(test   ${TEST_HEADERS}   ${TEST_SRCS})

IF(WIN32)
    SET(WINLIBS
        fmitcp
        fmilib
        shlwapi
        lacewing
        ws2_32
        mswsock
        crypt32
        secur32
        mpr
        sc m umfpack amd cblas suitesparseconfig # For strong coupling
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        protobuf
        ${CMAKE_THREAD_LIBS_INIT})
    TARGET_LINK_LIBRARIES(fmi-tcp-master ${WINLIBS})
    TARGET_LINK_LIBRARIES(fmi-tcp-slave  ${WINLIBS})
    TARGET_LINK_LIBRARIES(test   ${WINLIBS})
ELSE(WIN32)
    SET(LINUXLIBS
        fmitcp
        fmilib
        lacewing
        dl
        sc m umfpack amd blas atlas cholmod colamd suitesparseconfig # For strong coupling
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${PROTOBUF_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT})
    TARGET_LINK_LIBRARIES(fmi-tcp-master ${LINUXLIBS})
    TARGET_LINK_LIBRARIES(fmi-tcp-slave  ${LINUXLIBS})
    TARGET_LINK_LIBRARIES(test   ${LINUXLIBS})
ENDIF(WIN32)

INSTALL(TARGETS fmi-tcp-master fmi-tcp-slave DESTINATION bin)

SET(FMU_SRCS
  fmu/fmu_interface.c
)
SET(FMU_HEADERS
  fmu/include/fmiFunctions.h
  fmu/include/fmiFunctionTypes.h
  fmu/include/fmiTypesPlatform.h
  fmu/include/fmu_interface.h
)

# create the fmu.dll
SET(CMAKE_SHARED_LIBRARY_PREFIX "")
ADD_LIBRARY(vanilla SHARED ${FMU_HEADERS} ${FMU_SRCS})

SET_TARGET_PROPERTIES(vanilla
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin"
)

# remove the exisitng fmu and directories
EXECUTE_PROCESS(COMMAND rm -r -f "${CMAKE_CURRENT_LIST_DIR}/bin/binaries")
EXECUTE_PROCESS(COMMAND rm -f "${CMAKE_CURRENT_LIST_DIR}/bin/vanilla.fmu")

# create the needed directories
EXECUTE_PROCESS(COMMAND mkdir -p "${CMAKE_CURRENT_LIST_DIR}/bin/binaries/win32")

# copy the fmu.dll to binaries & modelDescription.xml to bin
EXECUTE_PROCESS(COMMAND cp "${CMAKE_CURRENT_LIST_DIR}/bin/vanilla.dll" "${CMAKE_CURRENT_LIST_DIR}/bin/binaries/win32")
EXECUTE_PROCESS(COMMAND cp "${CMAKE_CURRENT_LIST_DIR}/fmu/modelDescription.xml" "${CMAKE_CURRENT_LIST_DIR}/bin")

# make a zip file
EXECUTE_PROCESS(COMMAND zip -r "vanilla.fmu" "binaries" "modelDescription.xml"
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
