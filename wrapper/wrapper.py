# Standalone wrapper
import sys
import argparse
import tempfile
import shutil
import os
import subprocess
import zipfile
from modeldescription2header import modeldescription2header
from xml2wrappedxml import xml2wrappedxml

if __name__ == '__main__':
  # Figure out where this script is located
  # We may be called from someplace other than umit-fmus, and
  # need a proper base path for the stuff we need to pull in.
  scriptpath = os.path.split(sys.argv[0])[0]
  cwd = os.getcwd()
  wrapper_path = os.path.join(cwd, scriptpath)

  parser = argparse.ArgumentParser(description='''
  Wrap a ModelExchange FMU (.fmu file) into CoSimulation using GNU GSL as an integrator.
  Compilation will be significantly faster if FMILib and ninja are installed systemwide.
  ''')
  parser.add_argument(
    'sourcefmu',
    type=str,
    help='Path to source FMU'
  )
  parser.add_argument(
    'outputfmu',
    type=str,
    help='Path to output FMU'
  )
  parser.add_argument(
    '-i',
    dest='modelIdentifier',
    type=str,
    help='Model identifier to use for generated FMU. Default = outputfmu filename sans extension ("output" for output.fmu)'
  )
  parser.add_argument(
    '-t',
    dest='build_type',
    default='Release',
    type=str,
    help='Build type (default: Release)'
  )
  parser.add_argument(
    '-d',
    dest='directional',
    action='append',
    help='''Contents of resources/directional.txt. Can be used in two ways:
    either one -d per line (python wrapper.py -d "0 1 2" -d "3 4 5" in.fmu out.fmu)
    or a single -d if newlines in the given string are appropriately escaped.
    In other words all -d options are joined by newlines and the resulting string
    is written to resources/directional.txt in the output FMU.
    '''
  )
  parser.add_argument(
    '-f',
    dest='filter',
    action='store_true',
    help='''Enable EPCE filter''',
  )
  args = parser.parse_args()

  modelIdentifier = args.modelIdentifier if args.modelIdentifier != None else os.path.basename(args.outputfmu).split('.')[0]

  d = tempfile.mkdtemp(prefix='wrapper_')
  sources   = os.path.join(d, 'sources')
  build     = os.path.join(d, 'build')
  os.makedirs(sources)
  os.makedirs(build)

  # Generate modelDescription.xml and modelDescription.h
  md_filename = os.path.join(d, 'modelDescription.xml')
  md = open(md_filename, 'w')
  xml2wrappedxml('', args.sourcefmu, modelIdentifier, file=md)
  md.close()
  mdh_filename = os.path.join(sources, 'modelDescription.h')
  mdh = open(mdh_filename, 'w')
  modeldescription2header(md_filename, True, file=mdh)
  mdh.close()

  # FmuBase.cmake contains lots of useful utilities..
  shutil.copy(os.path.join(wrapper_path, '../FmuBase.cmake'), sources)
  shutil.copytree(os.path.join(wrapper_path), os.path.join(sources, 'wrapper'))
  shutil.copytree(os.path.join(wrapper_path, '../umit-fmus/templates'), os.path.join(sources, 'umit-fmus/templates'))
  shutil.copytree(os.path.join(wrapper_path, '../3rdparty/FMILibrary-2.0.1'), os.path.join(sources, 'FMILibrary-2.0.1'))
  shutil.copytree(os.path.join(wrapper_path, '../umit-fmus/wingsl'), os.path.join(sources, 'wingsl'))

  cmake = open(os.path.join(d, 'CMakeLists.txt'), 'w')
  cmake.write('''
cmake_minimum_required(VERSION 2.8)
add_subdirectory(sources)
''')
  cmake.close()

  cmake2 = open(os.path.join(sources, 'CMakeLists.txt'), 'w')
  cmake2.write('''
cmake_minimum_required(VERSION 2.8)
project(wrapper)

include(CheckIncludeFiles)
check_include_files(fmilib.h HAVE_FMILIB_H)
option(WRAPPER_USE_FILTER "Use EPCE filter on outputs?" OFF)

if (WRAPPER_USE_FILTER)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWRAPPER_USE_FILTER")
endif ()

# Don't bother building FMILib if we have one installed systemwide
# Assume it's a good version. We can't really check if it is >= 2.0.1 unfortunately
if (NOT HAVE_FMILIB_H)
  if(CMAKE_VERSION VERSION_GREATER "3.3")
      #suppress warning about libexpat.a
      cmake_policy(SET CMP0058 OLD)
  endif()

  set(FMILIBRARY_VERSION FMILibrary-2.0.1)
  add_subdirectory(${FMILIBRARY_VERSION})
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/${FMILIBRARY_VERSION})
  include_directories(${FMILIBRARY_VERSION}/src/CAPI/include)
  include_directories(${FMILIBRARY_VERSION}/src/Import/include)
  include_directories(${FMILIBRARY_VERSION}/src/Util/include)
  include_directories(${FMILIBRARY_VERSION}/src/XML/include)
  include_directories(${FMILIBRARY_VERSION}/src/ZIP/include)
  include_directories(${FMILIBRARY_VERSION}/ThirdParty/FMI/default)
endif ()

include(FmuBase.cmake)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  cgsl/include
  libwrapper/include
)

wrap_existing_fmu2("%s" "%s" "${CMAKE_CURRENT_BINARY_DIR}")
''' % (modelIdentifier, os.path.join(cwd, args.sourcefmu).replace('\\','\\\\')))
  cmake2.close();

  os.chdir(build)

  # Use ninja if it's installed
  try:
    subprocess.check_output(['ninja','--version'])
    have_ninja = True
  except:
    have_ninja = False

  cmake_opts = (['-GNinja'] if have_ninja else []) + [
    '-DFMILIB_BUILD_TESTS=OFF',
    '-DFMILIB_BUILD_SHARED_LIB=OFF',
    '-DFMILIB_INSTALL_SUBLIBS=OFF',
    '-DFMILIB_GENERATE_DOXYGEN_DOC=OFF',
    '-DWRAPPER_USE_FILTER=' + ('ON' if args.filter else 'OFF'),
    '-DCMAKE_BUILD_TYPE=%s' % args.build_type,
    '--no-warn-unused-cli', # Don't warn about FMILIB_* being unused in case of using system FMILib
  ]

  if  not subprocess.call(['cmake','..'] + cmake_opts) == 0 or \
      not subprocess.call(['cmake','--build','.','--config',args.build_type]) == 0:
    exit(1)

  os.chdir(cwd)
  builtfmu = os.path.join(build, 'sources/%s.fmu' % modelIdentifier)

  # Add directional.txt to zip
  if not args.directional is None:
    s = '\n'.join(args.directional)
    if s[-1] != '\n':
      s = s + '\n'

    with zipfile.ZipFile(builtfmu, 'a') as z:
      z.writestr('resources/directional.txt', s)

  shutil.move(builtfmu, args.outputfmu)
  shutil.rmtree(d)
