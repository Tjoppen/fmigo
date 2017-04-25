# USAGE: make_copy_command(var dest [filenames...])
# Sets ${var} to a command that copies filenames to dest
macro (make_copy_command var dest)
  if (CMAKE_VERSION VERSION_LESS "3.5.0")
    set(${var} cp ${ARGN} ${dest})
  else ()
    set(${var} ${CMAKE_COMMAND} -E copy ${ARGN} ${dest})
  endif ()
endmacro ()

function (add_copy_command TARGET BUILD_STEP dest)
  if (CMAKE_VERSION VERSION_LESS "3.5.0")
    add_custom_command(TARGET ${TARGET} ${BUILD_STEP} COMMAND cp ${ARGN} ${dest})
  else ()
    add_custom_command(TARGET ${TARGET} ${BUILD_STEP} COMMAND ${CMAKE_COMMAND} -E copy ${ARGN} ${dest})
  endif ()
endfunction ()


# USAGE: make_copy_directory_command(var dest [dirnames...])
# Sets ${var} to a command that copies filenames to dest, recursively
macro (make_copy_directory_command var dest)
  if (CMAKE_VERSION VERSION_LESS "3.5.0")
    set(${var} cp -r ${ARGN} ${dest})
  else ()
    set(${var} ${CMAKE_COMMAND} -E copy_directory ${ARGN} ${dest})
  endif ()
endmacro ()


# USAGE: make_zip_command(var zipfile [filenames...])
# Sets ${var} to a command that zips filenames into zipfile
macro (make_zip_command var zipfile)
  if (CMAKE_VERSION VERSION_LESS "3.5.0")
    set(${var} zip -r ${zipfile} ${ARGN})
  else ()
    set(${var} ${CMAKE_COMMAND} -E tar cf ${zipfile} --format=zip ${ARGN})
  endif ()
endmacro ()

function (make_xml2wrappedxml_command xml2wrappedxml x_or_f filename)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/${xml2wrappedxml}
         ${x_or_f} ${CMAKE_CURRENT_SOURCE_DIR}/${filename} > ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source_xml} )
endfunction ()

# Make modeldescription2header call, with optional arguments (such as -w)
function (make_md2hdr_command md2hdr)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sources/modelDescription.h
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/${md2hdr} ${ARGN}
      ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml >
      ${CMAKE_CURRENT_SOURCE_DIR}/sources/modelDescription.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml )
  # Might not need this, not sure
  # https://cmake.org/Wiki/CMake_FAQ#How_can_I_add_a_dependency_to_a_source_file_which_is_generated_in_a_subdirectory.3F
  add_custom_target(${TARGET}_modelDescription_h DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sources/modelDescription.h)
  add_dependencies(${TARGET} ${TARGET}_modelDescription_h)
endfunction ()

# There doesn't seem to be a way to repack an FMU in case it is deleted.
# A post-build command is good enough for now.
# Having to have a separate series of commands for Windows is also not optimal
function (make_fmu_pack_command)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo Packing ${TARGET}.fmu
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/fmu/sources
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/fmu/resources
    )

  if (WIN32)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/fmu/binaries/win32
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${TARGET}.dll ${CMAKE_CURRENT_BINARY_DIR}/fmu/binaries/win32/
    )
  else ()
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/binaries ${CMAKE_CURRENT_BINARY_DIR}/fmu/binaries
    )
  endif ()

  add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND ${COPY_COMMAND}
      COMMAND ${COPY_FMU_COMMAND}
      COMMAND ${COPY_RESOURCES_COMMAND}
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml ${CMAKE_CURRENT_BINARY_DIR}/fmu
      # Later: COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_BINARY_DIR}/fmu
      COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR}/fmu ${ZIP_COMMAND}
  )
endfunction ()

# Example: copy_recursive(MyTarget PRE_BUILD srcdir destdir)
function(copy_recursive TARGET BUILD_STEP distant dst)
    file(GLOB_RECURSE DistantFiles
        RELATIVE ${distant}
        ${distant}/*)

    add_custom_command(TARGET ${TARGET} ${BUILD_STEP}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dst}
        )

    foreach(Filename ${DistantFiles})
      string(LENGTH ${Filename} stringLen)

      string(FIND ${Filename} / slash REVERSE)
      set(interdir)
      if( ${slash} GREATER -1)
          MATH(EXPR slash "${slash}+1")
          string(SUBSTRING ${Filename} 0 ${slash} interdir)
          string(SUBSTRING ${Filename} ${slash} ${stringLen} Filename)
          if( ${interdir} STRGREATER "")
              add_custom_command(TARGET ${TARGET} ${BUILD_STEP}
                  COMMAND ${CMAKE_COMMAND} -E make_directory ${dst}${interdir}
                  )
          endif()
      endif()
      add_copy_command(${TARGET} ${BUILD_STEP} ${dst} "${distant}/${Filename}")

    endforeach(Filename)
endfunction()

if (WIN32)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/wingsl/lib)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/wingsl/include)
    set(CMAKE_SHARED_LINKER_FLAGS "/SAFESEH:NO")
    set(CMAKE_EXE_LINKER_FLAGS "/SAFESEH:NO")
endif ()

if (UNIX)
    # Treat warnings as errors, especially implicit-function-declaration
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif ()

# Don't add cgsl twice
if (NOT TARGET cgsl)
    add_subdirectory(templates/cgsl)
endif ()

add_subdirectory(meWrapper/libwrapper)
