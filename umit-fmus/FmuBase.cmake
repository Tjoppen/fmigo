# Copies one or more files in ARGN to dest
# CMake < 3.5 is not able to copy more than one file at a time. Hence this function
function (copy_multiple TARGET BUILD_STEP dest)
  foreach (src ${ARGN})
    add_custom_command(TARGET ${TARGET} ${BUILD_STEP} COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dest})
  endforeach ()
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
      copy_multiple(${TARGET} ${BUILD_STEP} ${dst} "${distant}/${Filename}")

    endforeach(Filename)
endfunction()


function (add_fmu_internal srcdir dstdir xmldir target extra_srcs libs extra_includes console md2hdr_option xmldeps extra_resources)
  set(${target}_fmu     ${dstdir}/${target}.fmu CACHE INTERNAL "" FORCE)
  set(${target}_srcdir  ${srcdir}               CACHE INTERNAL "" FORCE)
  set(${target}_dstdir  ${dstdir}               CACHE INTERNAL "" FORCE)
  set(${target}_xmldir  ${xmldir}               CACHE INTERNAL "" FORCE)
  set(${target}_packdir ${CMAKE_CURRENT_BINARY_DIR}/${target}/fmu        CACHE INTERNAL "" FORCE)

  file(GLOB fmu_sources ${${target}_srcdir}/sources/*)
  set(binaries_dir ${CMAKE_CURRENT_BINARY_DIR}/${target}/binaries)

  set(srcs
    templates/fmi2/fmuTemplate.h
    templates/fmi2/fmuTemplate_impl.h
    templates/fmi2/strlcpy.h
    ${${target}_dstdir}/sources/modelDescription.h
    ${fmu_sources}
    ${extra_srcs}
  )

  set(includes
    templates/fmi2
    ${${target}_srcdir}/sources
    ${${target}_dstdir}/sources
    ${extra_includes}
  )

  add_library(${target} SHARED
    ${srcs}
  )

  add_custom_command(
    OUTPUT ${${target}_dstdir}/sources/modelDescription.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${${target}_dstdir}/sources
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/modeldescription2header.py ${md2hdr_option}
      ${${target}_xmldir}/modelDescription.xml >
      ${${target}_dstdir}/sources/modelDescription.h
    DEPENDS ${${target}_xmldir}/modelDescription.xml ${xmldeps} ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/modeldescription2header.py)
  add_custom_target(${target}_md DEPENDS ${${target}_dstdir}/sources/modelDescription.h)
  add_dependencies(${target} ${target}_md)

  set_source_files_properties(${${target}_dstdir}/sources/modelDescription.h PROPERTIES GENERATED TRUE) # see further down
  target_include_directories(${target} PUBLIC ${includes})
  set_target_properties(${target} PROPERTIES PREFIX "")

  if(APPLE)
    set(arch_dir "darwin64")
  else()
    set(arch_dir "linux64")
  endif()
  set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${binaries_dir}/${arch_dir}")
  target_link_libraries(${target} ${libs})
  if (UNIX)
    target_link_libraries(${target} m)
  endif ()

  if (${console})
    add_executable(${target}_c ${srcs})
    target_include_directories(${target}_c PUBLIC ${includes})
    set_target_properties(${target}_c PROPERTIES COMPILE_DEFINITIONS CONSOLE)
    target_link_libraries( ${target}_c ${libs} )
    if (UNIX)
      target_link_libraries(${target}_c m)
    endif ()
  endif ()

  set(ZIP_ARGS modelDescription.xml binaries sources resources)
  if (CMAKE_VERSION VERSION_LESS "3.5.0")
    set(ZIP_COMMAND zip -r ${${target}_fmu} ${ZIP_ARGS})
  else ()
    set(ZIP_COMMAND ${CMAKE_COMMAND} -E tar cf ${${target}_fmu} --format=zip ${ZIP_ARGS})
  endif ()

  add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${${target}_packdir}/sources
        COMMAND ${CMAKE_COMMAND} -E make_directory ${${target}_packdir}/resources
    )

  if (WIN32)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${${target}_packdir}/binaries/win32
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${target}.dll ${${target}_packdir}/binaries/win32/
    )
  else ()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${binaries_dir} ${${target}_packdir}/binaries
    )
  endif ()

  if (IS_DIRECTORY ${${target}_srcdir}/resources)
    copy_recursive(${target} POST_BUILD ${${target}_srcdir}/resources ${${target}_packdir}/resources/)
  endif ()

  if (NOT "${extra_resources}" STREQUAL "")
    copy_multiple(${target} POST_BUILD ${${target}_packdir}/resources ${extra_resources})
  endif ()

  add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${${target}_xmldir}/modelDescription.xml ${${target}_packdir}
  )

  add_custom_command(OUTPUT ${${target}_fmu}
      COMMAND ${CMAKE_COMMAND} -E echo Packing ${target}.fmu
      COMMAND ${CMAKE_COMMAND} -E remove -f ${${target}_fmu}
      COMMAND ${CMAKE_COMMAND} -E chdir ${${target}_packdir} ${ZIP_COMMAND}
      DEPENDS ${target}
  )
  add_custom_target(${target}_fmu_target ALL DEPENDS ${${target}_fmu})
endfunction ()


function (add_fmu dir target libs)
  add_fmu_internal("${CMAKE_CURRENT_SOURCE_DIR}/${dir}" "${CMAKE_CURRENT_BINARY_DIR}/${dir}" "${CMAKE_CURRENT_SOURCE_DIR}/${dir}" "${target}" "" "${libs}" "" FALSE "" "" "")
endfunction ()


function (add_wrapped dir sourcetarget)
  set(target "wrapper_${sourcetarget}")
  set(srcxml "${${sourcetarget}_xmldir}/modelDescription.xml")
  set(dstxml ${CMAKE_CURRENT_BINARY_DIR}/${dir}/modelDescription.xml)

  add_custom_command(
    OUTPUT ${dstxml}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${dir}
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/xml2wrappedxml.py
         -x ${srcxml} -f "${${sourcetarget}_fmu}" -i ${target} > ${dstxml}
    DEPENDS ${srcxml} ${sourcetarget} ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/xml2wrappedxml.py)
  add_custom_target(${target}_xml DEPENDS ${dstxml})

  add_fmu_internal("${CMAKE_CURRENT_SOURCE_DIR}/${dir}" "${CMAKE_CURRENT_BINARY_DIR}/${dir}" "${CMAKE_CURRENT_BINARY_DIR}/${dir}" "${target}" "wrapper/sources/wrapper.c" "cgsl;fmilib" "wrapper/sources" FALSE "-w" "${target}_xml" "${${sourcetarget}_fmu}")
  add_dependencies(${target} ${sourcetarget}_fmu_target)
endfunction ()


function (add_wrapped_fmu dir sourcetarget sourcefmu)
  set(target wrapperfmu_${sourcetarget})
  set(dstxml ${CMAKE_CURRENT_BINARY_DIR}/${dir}/modelDescription.xml)

  add_custom_command(
    OUTPUT ${dstxml}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${dir}
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/xml2wrappedxml.py
         -f ${sourcefmu} -i ${target} > ${dstxml}
    DEPENDS ${sourcefmu} ${sourcetarget}_fmu_target ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/xml2wrappedxml.py)
  add_custom_target(${target}_xml DEPENDS ${dstxml})

  add_fmu_internal("${CMAKE_CURRENT_BINARY_DIR}/${dir}" "${CMAKE_CURRENT_BINARY_DIR}/${dir}" "${CMAKE_CURRENT_BINARY_DIR}/${dir}" "${target}" "wrapper/sources/wrapper.c" "cgsl;fmilib" "wrapper/sources" FALSE "-w" "${target}_xml" "${${sourcetarget}_fmu}")
  add_dependencies(${target} ${sourcetarget}_fmu_target)
endfunction ()

# wrap_existing_fmu
# Wraps an existing FMU that does not have a target that generates it. Arguments:
#
# modelIdentifier: modelIdentifier of the wrapped FMU. Becomes the base name of outer FMU, its modelIdentifier = wrapperfmu_${modelIdentifier}
#       sourcefmu: Full input FMU filename, for instance "${CMAKE_CURRENT_SOURCE_DIR}/me/foo/foo.fmu"
#             dir: Output directory. Currently inside ${CMAKE_CURRENT_SOURCE_DIR}.
#                  Currently required, will be removed in the future, placing all FMUs in ${CMAKE_CURRENT_BINARY_DIR}
#
# Example usage:
#
#  wrap_existing_fmu(springs    ${CMAKE_CURRENT_SOURCE_DIR}/me/springs/springs.fmu  meWrapperFmu/springs)
#
function (wrap_existing_fmu2 target sourcefmu dir)
  set(dstxml ${dir}/modelDescription.xml)

  add_custom_command(
    OUTPUT ${dstxml}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/xml2wrappedxml.py
         -f ${sourcefmu} -i ${target} > ${dstxml}
    DEPENDS ${sourcefmu} ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/xml2wrappedxml.py)
  add_custom_target(${target}_xml DEPENDS ${dstxml})

  #function (add_fmu_internal dir target extra_srcs libs extra_includes console md2hdr_option xmldeps extra_resources)
  add_fmu_internal("${dir}" "${dir}" "${dir}" "${target}" "wrapper/sources/wrapper.c" "cgsl;fmilib" "wrapper/sources" FALSE "-w" "${target}_xml" "${sourcefmu}")
endfunction ()

function (wrap_existing_fmu modelIdentifier sourcefmu dir)
  wrap_existing_fmu2("wrapperfmu_${modelIdentifier}" "${sourcefmu}" "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
endfunction ()


if (WIN32)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/wingsl/lib)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/wingsl/include)
    set(CMAKE_SHARED_LINKER_FLAGS "/SAFESEH:NO")
    set(CMAKE_EXE_LINKER_FLAGS "/SAFESEH:NO")

    # Disable warnings about fopen() on Windows
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

if (UNIX)
    # Treat warnings as errors, especially implicit-function-declaration
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif ()

# Don't add cgsl twice
if (NOT TARGET cgsl)
    add_subdirectory(templates/cgsl)
endif ()
