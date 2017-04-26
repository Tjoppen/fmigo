# USAGE: add_ssp(directory target-name fmu [fmu ...])
#
# fmus should be given with "naked" names to which suitable suffixes can be added,
# such as "clutch2" -> "clutch2_fmu" and "clutch2_fmu_target". This is required
# for DEPENDS to work correctly.
#
function (add_ssp dir sspname)
    set(ZIPDIR ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${sspname})
    set(ZIPNAME ${ZIPDIR}.ssp)
    set(${sspname}_dir ${ZIPDIR}  CACHE INTERNAL "" FORCE)
    set(${sspname}_ssp ${ZIPNAME} CACHE INTERNAL "" FORCE)

    set(FMUS "")
    set(FMU_TARGETS "")
    foreach (fmu ${ARGN})
        set(FMUS        ${FMUS}         ${${fmu}_fmu})
        set(FMU_TARGETS ${FMU_TARGETS}  ${fmu}_fmu_target)
    endforeach ()

    if (CMAKE_VERSION VERSION_LESS "3.5.0")
        set(ZIP_COMMAND            zip -r ${ZIPNAME} .)
        set(COPY_SSD_COMMAND       cp ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/SystemStructure.ssd ${ZIPDIR})
        set(COPY_RESOURCES_COMMAND cp ${FMUS} ${ZIPDIR}/resources)
    else ()
        set(COPY_SSD_COMMAND       ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/SystemStructure.ssd ${ZIPDIR})
        set(COPY_RESOURCES_COMMAND ${CMAKE_COMMAND} -E copy ${FMUS} ${ZIPDIR}/resources)
        set(ZIP_COMMAND            ${CMAKE_COMMAND} -E tar cf ${ZIPNAME} --format=zip .)
    endif ()

    add_custom_command(OUTPUT ${ZIPNAME}
        COMMAND ${CMAKE_COMMAND} -E echo Packing ${sspname}.ssp
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ZIPDIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ZIPDIR}/resources
        COMMAND ${COPY_SSD_COMMAND}
        COMMAND ${COPY_RESOURCES_COMMAND}
        COMMAND ${CMAKE_COMMAND} -E remove -f ${ZIPNAME}
        COMMAND ${CMAKE_COMMAND} -E chdir ${ZIPDIR} ${ZIP_COMMAND}
        DEPENDS ${FMUS} ${FMU_TARGETS}
    )
    add_custom_target(${sspname} ALL DEPENDS ${ZIPNAME})
endfunction ()


install(FILES
  ssp-launcher.py
  SystemStructureDescription.xsd
  SystemStructureParameterMapping.xsd
  SystemStructureParameterValues.xsd
  FmiGo.xsd
  DESTINATION bin)

# I'm not sure why these don't build with GPL disabled, but this
# seems to be the case with 64-bit builds on MSVC12 at least
if (USE_GPL AND BUILD_FMUS)
  # We don't have very many FMUs in our SSPs, so we can list each
  # one without having to use a generator to parse SystemStructure.ssd
  add_ssp(ssp-test              SpringChainSystem       clutch2)
  add_ssp(ssp-work-reports      WorkReportHolonomic     engine      gearbox2    body)

  add_test(ssp-test         python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${SpringChainSystem_ssp})
  add_test(ssp-work-reports python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${WorkReportHolonomic_ssp})
  add_test(ssp-test-unzipped         python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${SpringChainSystem_dir}/SystemStructure.ssd)
  add_test(ssp-work-reports-unzipped python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${WorkReportHolonomic_dir}/SystemStructure.ssd)
endif ()
