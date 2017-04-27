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
  add_subdirectory(ssp-kinematic-example)

  add_test(ssp-test         python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${SPRINGCHAINSYSTEM_SSP})
  add_test(ssp-work-reports python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${WORKREPORTHOLONOMIC_SSP})
  add_test(ssp-kinematic-example python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${KINEMATICEXAMPLE_SSP})
  add_test(ssp-test-unzipped         python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${SPRINGCHAINSYSTEM_DIR}/SystemStructure.ssd)
  add_test(ssp-work-reports-unzipped python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${WORKREPORTHOLONOMIC_DIR}/SystemStructure.ssd)
  add_test(ssp-kinematic-example-unzipped python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${KINEMATICEXAMPLE_DIR}/SystemStructure.ssd)
endif ()
