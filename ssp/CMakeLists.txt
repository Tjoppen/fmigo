install(FILES
  ssp-launcher.py
  SystemStructureDescription.xsd
  SystemStructureParameterMapping.xsd
  SystemStructureParameterValues.xsd
  FmiGo.xsd
  DESTINATION bin)

add_subdirectory(ssp-test)
add_subdirectory(ssp-work-reports)

add_test(ssp-test         python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${SPRINGCHAINSYSTEM_SSP})
add_test(ssp-work-reports python ${CMAKE_CURRENT_SOURCE_DIR}/ssp-launcher.py ${WORKREPORTHOLONOMIC_SSP})

