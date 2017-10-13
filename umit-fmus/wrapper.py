# Standalone wrapper
import sys
import argparse
import tempfile
import shutil
import os
import subprocess
from wrapper.modeldescription2header import modeldescription2header
from wrapper.xml2wrappedxml import xml2wrappedxml

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Wrap a ModelExchange FMU (.fmu file) into CoSimulation using GNU GSL as an integrator')
  parser.add_argument('sourcefmu', type=str, help='Path to source FMU')
  parser.add_argument('outputfmu', type=str, help='Path to output FMU')
  parser.add_argument('-i','--modelIdentifier', type=str, help='modelIdentifier to use for generated FMU. Default = wrapper_{sourcefmu filename sans extension}')
  args = parser.parse_args()

  modelIdentifier = args.modelIdentifier if args.modelIdentifier != None else 'wrapper_' + os.path.basename(args.sourcefmu).split('.')[0]
  print('modelIdentifier = '+modelIdentifier)

  d = tempfile.mkdtemp(prefix='wrapper_')
  sources = os.path.join(d, 'sources')
  build   = os.path.join(d, 'build')
  os.makedirs(sources)
  os.makedirs(build)

  print(d)

  # Generate modelDescription.xml and modelDescription.h
  md_filename = os.path.join(d, 'modelDescription.xml')
  md = open(md_filename, 'w')
  xml2wrappedxml('', args.sourcefmu, modelIdentifier, file=md)
  md.close()
  mdh_filename = os.path.join(sources, 'modelDescription.h')
  mdh = open(mdh_filename, 'w')
  modeldescription2header(md_filename, True, file=mdh)
  mdh.close()

  shutil.copy('wrapper/sources/wrapper.c', sources)
  shutil.copy('templates/fmi2/strlcpy.h', sources)
  shutil.copy('templates/fmi2/fmuTemplate.h', sources)
  shutil.copy('templates/fmi2/fmuTemplate_impl.h', sources)
  shutil.copytree('wrapper/libwrapper', os.path.join(sources, 'libwrapper'))
  shutil.copytree('../FMILibrary-2.0.1', os.path.join(sources, 'FMILibrary-2.0.1'))
  shutil.copytree('templates/cgsl', os.path.join(sources, 'cgsl'))

  cmake = open(os.path.join(d, 'CMakeLists.txt'), 'w')
  cmake.write('''
cmake_minimum_required(VERSION 2.8)
project(wrapper)

include(CheckIncludeFiles)
check_include_files(fmilib.h HAVE_FMILIB_H)

# Don't bother building FMILib if we have one installed systemwide
# Assume it's a good version. We can't really check if it is >= 2.0.1 unfortunately
if (NOT HAVE_FMILIB_H)
  set(FMILIBRARY_VERSION FMILibrary-2.0.1)
  add_subdirectory(sources/${FMILIBRARY_VERSION})
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/sources/${FMILIBRARY_VERSION})
  include_directories(sources/${FMILIBRARY_VERSION}/src/CAPI/include)
  include_directories(sources/${FMILIBRARY_VERSION}/src/Import/include)
  include_directories(sources/${FMILIBRARY_VERSION}/src/Util/include)
  include_directories(sources/${FMILIBRARY_VERSION}/src/XML/include)
  include_directories(sources/${FMILIBRARY_VERSION}/src/ZIP/include)
  include_directories(sources/${FMILIBRARY_VERSION}/ThirdParty/FMI/default)
endif ()

add_subdirectory(sources/cgsl)
add_subdirectory(sources/libwrapper)

include_directories(
  sources
  sources/cgsl/include
  sources/libwrapper/include
)

set(target %s)
add_library(${target} SHARED sources/wrapper.c)
target_link_libraries(${target}
  fmilib
  cgsl
  wrapperlib
)
if (UNIX)
  target_link_libraries(${target} m)
endif ()

''' % (modelIdentifier, ))
  cmake.close()

  os.chdir(build)
  have_ninja = 'build.ninja' in subprocess.check_output(['cmake','--help']).decode()
  cmake_opts = (['-GNinja'] if have_ninja else []) + [
    '-DFMILIB_BUILD_TESTS=OFF',
    '-DFMILIB_BUILD_SHARED_LIB=OFF',
    '-DFMILIB_INSTALL_SUBLIBS=OFF',
    '-DFMILIB_GENERATE_DOXYGEN_DOC=OFF',
  ]

  if  not subprocess.call(['cmake','..'] + cmake_opts) == 0 or \
      not subprocess.call(['cmake','--build','.']) == 0:
    exit(1)

  #shutil.rmtree(d)
