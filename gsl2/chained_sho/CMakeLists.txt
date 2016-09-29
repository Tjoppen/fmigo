CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(ARCH "linux64" CACHE STRING "Architecture: linux64, linux32, win32...")
SET(TARGET chained_sho)
SET(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../templates/fmi2/fmuTemplate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../templates/fmi2/fmuTemplate_impl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../templates/gsl2/gsl-interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../templates/gsl2/gsl-interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/chained_sho.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/modelDescription.h
)
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sources/modelDescription.h
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../../../fmu-builder/bin/modeldescription2header
    ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml >
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/modelDescription.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml )

ADD_DEFINITIONS(  )
ADD_LIBRARY(${TARGET} SHARED ${SRCS})
INCLUDE_DIRECTORIES(
    ../../../FMILibrary-2.0.1/ThirdParty/FMI/default
    ../../templates/fmi2
    ../../templates/gsl2
)
SET_TARGET_PROPERTIES(${TARGET} PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/binaries/${ARCH}")
SET(LIBS gsl gslcblas)
if (UNIX)
    set(LIBS ${LIBS} m)
endif ()
TARGET_LINK_LIBRARIES( ${TARGET} ${LIBS} )


ADD_EXECUTABLE(${TARGET}_c ${SRCS})
SET_TARGET_PROPERTIES(${TARGET}_c PROPERTIES COMPILE_DEFINITIONS CONSOLE)
TARGET_LINK_LIBRARIES( ${TARGET}_c ${LIBS} )


# There doesn't seem to be a way to repack an FMU in case it is deleted.
# A post-build command is good enough for now.
# Having to have a separate series of commands for Windows is also not optimal
if (WIN32)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo Packing ${TARGET}.fmu
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/fmu/sources
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/fmu/binaries/win32
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${TARGET}.dll ${CMAKE_CURRENT_BINARY_DIR}/fmu/binaries/win32/
        COMMAND ${CMAKE_COMMAND} -E copy ${SRCS} ${CMAKE_CURRENT_BINARY_DIR}/fmu/sources
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml ${CMAKE_CURRENT_BINARY_DIR}/fmu
        # Later: COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_BINARY_DIR}/fmu
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR}/fmu ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.fmu --format=zip modelDescription.xml binaries sources
        MAIN_DEPENDENCY ${TARGET}
    )
else ()
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo Packing ${TARGET}.fmu
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/fmu/sources
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/binaries ${CMAKE_CURRENT_BINARY_DIR}/fmu/binaries
        COMMAND ${CMAKE_COMMAND} -E copy ${SRCS} ${CMAKE_CURRENT_BINARY_DIR}/fmu/sources
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml ${CMAKE_CURRENT_BINARY_DIR}/fmu
        # Later: COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_BINARY_DIR}/fmu
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR}/fmu ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.fmu --format=zip ${CMAKE_CURRENT_BINARY_DIR}/fmu/*
        MAIN_DEPENDENCY ${TARGET}
    )
endif ()
