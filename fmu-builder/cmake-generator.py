#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys, tempfile, os, subprocess, zipfile, argparse, shutil, re
import xml.etree.ElementTree as e
from os.path import join as pjoin

# Parse command line arguments
parser = argparse.ArgumentParser(
    description='Generate CMakeLists.txt to build an FMU',
    epilog="""It is assumed that the current directory contains an XML file called "modelDescription.xml" and a directory "sources" containing .c and .h files.
Built by Tomas Härdin at UMIT Research Lab 2016."""
    )
parser.add_argument('-l','--link',
                    type=str,
                    help='Libraries to link with, separated with commas.',
                    default='')
parser.add_argument('-d','--definitions',
                    type=str,
                    help='Adds flags to the compiler. Separate with commas and remove the -. Example: -d DFLAG,DKEY=VALUE,msse3',
                    default='')
parser.add_argument('-i','--includedir',
                    type=str,
                    help='Include directories. Separate with commas.',
                    default='')
parser.add_argument('-t','--templatedir',
                    action='append',
                    help='Directories to pull template code from, to avoid copy-pasting it all over the place. May also be a filename')
parser.add_argument('-m','--md2hdr',
                    type=str,
                    help='Location of modeldescription2header')
parser.add_argument('-c','--console',
                    action='store_true',
                    default=False,
                    help='Generate CONSOLE target')
parser.add_argument('-x','--srcxml',
                    default='',
                    help='Source XML for this FMUs modelDescription.xml, for wrapping. Mutually exclusive with -f')
parser.add_argument('-f','--srcfmu',
                    default='',
                    help='Source FMU for this FMUs modelDescription.xml, for wrapping. Mutually exclusive with -x')
parser.add_argument('-X','--xml2wrappedxml',
                    type=str,
                    help='Location of xml2wrappedxml.py. Requires -f or -x',
                    default='')
parser.add_argument('-p','--prefix',
                    type=str,
                    help='Wrapped FMU modelIdentifier prefix. -p option to xml2wrappedxml',
                    default='wrapper_')

args = parser.parse_args()
link_libraries = args.link.split(",")
definitions = " -".join(args.definitions.split(","))
if len(definitions):
    definitions = "-"+definitions
include_directories = args.includedir.split(',')

# For logging errors and messages
def log(m):
    print( "%s: %s" % (sys.argv[0],m) )

if len(args.srcfmu) > 0 and len(args.srcxml) > 0:
    log('Must specify -x or -f, not both')
    exit(1)

# Parse xml file
tree = e.parse('modelDescription.xml')
root = tree.getroot()

# Get FMU version
fmiVersion = root.get('fmiVersion')

# Get FMU type (co-simulation or model exchange)
fmuType = "cs"

# Get model identifier.
modelIdentifier = "modelIdentifier"
if fmiVersion == "1.0":
    modelIdentifier = root.get('modelIdentifier')
elif fmiVersion == "2.0":
    me = root.find('ModelExchange')
    cs = root.find('CoSimulation')
    if cs != None:
        modelIdentifier = cs.get('modelIdentifier')
    elif me != None:
        modelIdentifier = me.get('modelIdentifier')
    else:
        log('FMU is neither ModelExchange or CoSimulation')
        exit(1)

path = "CMakeLists.txt"

def dopath(s):
    return os.path.relpath(s).replace('\\','/')

with open(path,'w') as f:

    # Find all .c and .h files in the current FMU folder
    cwd = os.getcwd()
    source_files = []
    dirs = [pjoin(cwd,"sources")]
    dirs.extend([pjoin(cwd,td) for td in args.templatedir])
    for dir in dirs:
        if os.path.isfile(dir):
            source_files.append(dir)
        else:
            source_files.extend([ pjoin(dir,s)
                for s in os.listdir(dir)
                if os.path.isfile(pjoin(dir,s))
                and os.path.splitext(s)[1] in ['.c','.cpp','.h','.hpp']
            ])

    # Check if there were any files
    if not len(source_files):
        log("No C files found in 'sources/'. Exiting...")
        exit(1)

    # Create a CMakeLists.txt
    for d in dirs:
        # Behave like a predictable set()
        d2 = d if os.path.isdir(d) else os.path.dirname(d)
        if not d2 in include_directories:
          include_directories.append(d2)

    source_files        = ["${CMAKE_CURRENT_SOURCE_DIR}/"+dopath(s) for s in source_files]
    source_files.sort()
    include_directories = [dopath(i) for i in include_directories]
    include_directories.sort()
    files = "SET(SRCS\n    " + "\n    ".join(source_files) + "\n)"
    md2hdr = dopath(args.md2hdr)

    # libm needs special handling since it doesn't exist/isn't needed on Windows
    libmextra = ''
    if 'm' in link_libraries:
        link_libraries.remove('m')
        libmextra = "\nif (UNIX)\n    set(LIBS ${LIBS} m)\nendif ()"

    if len(args.xml2wrappedxml) > 0:
        md2hdr += " -w "
        dependentFMU = modelIdentifier.split('_')
        dependentFMU.pop(0)
        dependentFMU = '_'.join(dependentFMU)

        if len(args.srcfmu) > 0:
            x_or_f = '-f'
            srcxmlorfmu = dopath(args.srcfmu)
        elif len(args.srcxml) > 0:
            x_or_f = '-x'
            srcxmlorfmu = dopath(args.srcxml)
        else:
            log('-X requires -f or -x')
            exit(1)

        wrappedstuff = """
make_xml2wrappedxml_command(%(xml2wrappedxml)s %(x_or_f)s %(srcxmlorfmu)s %(prefix)s)
make_copy_command(COPY_FMU_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fmu/resources ${%(modelIdentifierUpper)s_FMU})
""" % {
        'xml2wrappedxml':       dopath(args.xml2wrappedxml),
        'x_or_f':               x_or_f,
        'srcxmlorfmu':          srcxmlorfmu,
        'modelIdentifierUpper': dependentFMU.upper(),
        'prefix':               args.prefix,
        }
    else:
        wrappedstuff = ''

    resourcesdir = pjoin(cwd,"resources")
    if os.path.isdir(resourcesdir):
        resources = """
        copy_recursive(${TARGET} PRE_BUILD
          ${CMAKE_CURRENT_SOURCE_DIR}/%(resourcedir)s
          ${CMAKE_CURRENT_BINARY_DIR}/fmu/resources/)""" % {
            'resourcedir': dopath(resourcesdir),
        }
    else:
        resources = ''
    console = """ADD_EXECUTABLE(${TARGET}_c ${SRCS})
SET_TARGET_PROPERTIES(${TARGET}_c PROPERTIES COMPILE_DEFINITIONS CONSOLE)
TARGET_LINK_LIBRARIES( ${TARGET}_c ${LIBS} )
"""
    cmakestuff = """#This file is generated by cmake-generator, DO NOT EDIT

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(ARCH "linux64" CACHE STRING "Architecture: linux64, linux32, win32...")
SET(TARGET %(modelIdentifier)s)
SET(%(modelIdentifierUpper)s_FMU ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}.fmu CACHE INTERNAL "" FORCE)
%(files)s
ADD_DEFINITIONS( %(definitions)s )
ADD_LIBRARY(${TARGET} SHARED ${SRCS})
INCLUDE_DIRECTORIES(
    %(include_directories)s
)
SET_TARGET_PROPERTIES(${TARGET} PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/binaries/${ARCH}")
SET(LIBS %(link_libraries)s)%(libmextra)s
TARGET_LINK_LIBRARIES( ${TARGET} ${LIBS} )

%(console)s

# See umit-fmus/FmuBase.CMake for what these macros actually do
make_md2hdr_command(%(md2hdr)s)
%(wrappedstuff)s
%(resources)s
make_zip_command (ZIP_COMMAND  ${%(modelIdentifierUpper)s_FMU} modelDescription.xml binaries sources resources)
make_copy_command(COPY_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fmu/sources ${SRCS})
make_fmu_pack_command()
""" % {
    'modelIdentifier':      modelIdentifier,
    'modelIdentifierUpper': modelIdentifier.upper(),
    'files':                files,
    'md2hdr':               md2hdr,
    'definitions':          definitions,
    'include_directories':  "\n    ".join(include_directories),
    'link_libraries':       " ".join(link_libraries),
    'libmextra':            libmextra,
    'console':              console if args.console else "",
    'wrappedstuff':         wrappedstuff,
    'resources':            resources,
    }

    f.write( cmakestuff )
    f.close()
    print("Created %s" % (path))
    # Done!
