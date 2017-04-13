install(FILES
  ssp-launcher.py
  SystemStructureDescription.xsd
  SystemStructureParameterMapping.xsd
  SystemStructureParameterValues.xsd
  FmiGo.xsd
  DESTINATION bin)

add_subdirectory(ssp-test)
add_subdirectory(ssp-work-reports)

add_test(ssp-test         python ssp-launcher.py ${SPRINGCHAINSYSTEM_SSP})
add_test(ssp-work-reports python ssp-launcher.py ${WORKREPORTHOLONOMIC_SSP})

