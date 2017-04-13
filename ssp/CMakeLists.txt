install(FILES
  ssp-launcher.py
  SystemStructureDescription.xsd
  SystemStructureParameterMapping.xsd
  SystemStructureParameterValues.xsd
  UMITSSD.xsd
  DESTINATION bin)

add_subdirectory(ssp-test)
add_subdirectory(ssp-work-reports)
