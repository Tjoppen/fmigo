CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
# We have multiple FMUs with the same name, but in different directories
cmake_policy(SET CMP0002 OLD)
SET(ARCH "linux64" CACHE STRING "Architecture: linux64, linux32, win32...")
SET(TARGET typeconvtest)
SET(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/typeconvtest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/modelDescription.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../templates/fmi2/fmuTemplate_impl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../templates/fmi2/fmuTemplate.h
)
ADD_DEFINITIONS(  )
ADD_LIBRARY(${TARGET} SHARED ${SRCS} ${HEADERS})
INCLUDE_DIRECTORIES(
    ../../FMILibrary-2.0.1/ThirdParty/FMI/default
    ../templates/fmi2
)
SET_TARGET_PROPERTIES(${TARGET} PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/binaries/${ARCH}")
TARGET_LINK_LIBRARIES( ${TARGET}  )

# There doesn't seem to be a way to repack an FMU in case it is deleted.
# A post-build command is good enough for now.
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo Packing ${TARGET}.fmu
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/fmu
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/fmu/sources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/binaries ${CMAKE_CURRENT_BINARY_DIR}/fmu/binaries
    COMMAND ${CMAKE_COMMAND} -E copy ${SRCS} ${CMAKE_CURRENT_BINARY_DIR}/fmu/sources
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml ${CMAKE_CURRENT_BINARY_DIR}/fmu
    # Later: COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_BINARY_DIR}/fmu
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR}/fmu ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.fmu --format=zip ${CMAKE_CURRENT_BINARY_DIR}/fmu/*
    MAIN_DEPENDENCY ${TARGET}
)
