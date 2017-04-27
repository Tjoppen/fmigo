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
  add_subdirectory(ssp-test)
  add_subdirectory(ssp-work-reports)

  # Run each test over both MPI and TCP
  foreach (p FALSE TRUE)
    if (p)
      set(suffix "-tcp")
      set(option "-p")
    else ()
      set(suffix "-mpi")
      set(option "")
    endif()

    add_test(ssp-test${suffix}                  python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${option} -- ${SPRINGCHAINSYSTEM_SSP})
    add_test(ssp-work-reports${suffix}          python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${option} -- ${WORKREPORTHOLONOMIC_SSP})
    add_test(ssp-test-unzipped${suffix}         python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${option} -- ${SPRINGCHAINSYSTEM_DIR}/SystemStructure.ssd)
    add_test(ssp-work-reports-unzipped${suffix} python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${option} -- ${WORKREPORTHOLONOMIC_DIR}/SystemStructure.ssd)
  endforeach ()
endif ()
