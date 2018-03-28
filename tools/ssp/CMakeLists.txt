# USAGE: add_ssp(directory target-name fmu [fmu ...])
#
# fmus should be given with "naked" names to which suitable suffixes can be added,
# such as "clutch" -> "clutch_fmu" and "clutch_fmu_target". This is required
# for DEPENDS to work correctly.
#
function (add_ssp dir sspname)
    set(ZIPDIR ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${sspname})
    set(ZIPNAME ${ZIPDIR}.ssp)
    set(${sspname}_dir ${ZIPDIR}  CACHE INTERNAL "" FORCE)
    set(${sspname}_ssp ${ZIPNAME} CACHE INTERNAL "" FORCE)
    set(${sspname}_ssd ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/SystemStructure.ssd CACHE INTERNAL "" FORCE)

    file(GLOB extra_resources ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/resources/*)
    set(FMUS "")
    set(FMU_TARGETS "")
    foreach (fmu ${ARGN})
        set(FMUS        ${FMUS}         ${${fmu}_fmu})
        set(FMU_TARGETS ${FMU_TARGETS}  ${fmu}_fmu_target)
    endforeach ()

    if (CMAKE_VERSION VERSION_LESS "3.5.0")
        set(ZIP_COMMAND            zip -r ${ZIPNAME} .)
        set(COPY_SSD_COMMAND       cp ${${sspname}_ssd} ${ZIPDIR})
        set(COPY_RESOURCES_COMMAND cp ${FMUS} ${extra_resources} ${ZIPDIR}/resources)
    else ()
        set(COPY_SSD_COMMAND       ${CMAKE_COMMAND} -E copy ${${sspname}_ssd} ${ZIPDIR})
        set(COPY_RESOURCES_COMMAND ${CMAKE_COMMAND} -E copy ${FMUS} ${extra_resources} ${ZIPDIR}/resources)
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
        DEPENDS ${FMUS} ${FMU_TARGETS} ${${sspname}_ssd} ${extra_resources}
    )
    add_custom_target(${sspname} ALL DEPENDS ${ZIPNAME})

    # Run each test over both MPI and TCP
    foreach (p FALSE TRUE)
      if (p)
        set(suffix "tcp")
        set(option "-e" "-p")
      else ()
        set(suffix "mpi")
        set(option "-e")
      endif()

      function(test_ssp PYTHON PYTHONNAME)
        add_test(ssp-${sspname}-${suffix}-${PYTHONNAME}           ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${option} -- ${${sspname}_ssp}                      -d 0.1 -t 0.1 )
        add_test(ssp-unzipped-${sspname}-${suffix}-${PYTHONNAME}  ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${option} -- ${${sspname}_dir}/SystemStructure.ssd  -d 0.1 -t 0.1 )
      endfunction()

      if (PYTHON2)
        test_ssp(${PYTHON2} python2)
      endif ()

      if (PYTHON3)
        test_ssp(${PYTHON3} python3)
      endif ()
    endforeach ()

endfunction ()


install(FILES
  ssp-launcher.py
  SystemStructureCommon.xsd
  SystemStructureDescription.xsd
  SystemStructureParameterMapping.xsd
  SystemStructureParameterValues.xsd
  FmiGo.xsd
  DESTINATION bin)

if (BUILD_FMUS)
  # We don't have very many FMUs in our SSPs, so we can list each
  # one without having to use a generator to parse SystemStructure.ssd
  add_ssp(ssp-test                        SpringChainSystem         clutch)
  add_ssp(ssp-work-reports                WorkReportHolonomic       engine      gearbox2    body)
  add_ssp(examples/kinematic              KinematicExample          clutch)
  add_ssp(examples/kinematic-nonholonomic KinematicNHExample        clutch)
  add_ssp(examples/master-args            MasterArguments           clutch)
  add_ssp(examples/parameters-inline      ParametersInline          clutch)
  add_ssp(examples/parameters-external    ParametersExternal        clutch)
  add_ssp(examples/strange_variable_names StrangeVarNames           strange_variable_names)
  add_ssp(examples/execution-order        ExecutionOrder            clutch)
endif ()
