#!/usr/bin/python
from __future__ import print_function
import sys
import uuid
import xml.etree.ElementTree as e
import zipfile
import argparse

# Parse command line arguments
parser = argparse.ArgumentParser(
    description='Generate CMakeLists.txt to build an FMU',
    epilog=""" """
    )
parser.add_argument('-f','--fmu',
                    type=str,
                    help='Path to FMU',
                    default='')
parser.add_argument('-x','--xml',
                    type=str,
                    help='Path to xml',
                    default='')
args = parser.parse_args()

def warning(*objs):
    print("WARNING: ", *objs, file=sys.stderr)

def error(*objs):
    print("ERROR: ", *objs, file=sys.stderr)
    exit(1)

if len(args.fmu):
    fmufile = args.fmu
    warning(fmufile)
    zip=zipfile.ZipFile(fmufile, 'r')
    f=zip.open('modelDescription.xml')
    root = e.fromstring(f.read())
    f.close()
elif len(args.xml):
    xmlfile = args.xml
    # Parse xml file
    tree = e.parse(xmlfile)
    root = tree.getroot()
else:
    warning(sys.args)
    error('%s expect eather: -f myfmu.fmu or -x myxml.xml' %(sys.args[0]))


# Get FMU version
fmiVersion = root.get('fmiVersion')
fmiDescription = root.get('description')
me = root.find('ModelExchange')
if me == None:
    error('wrapper only supports model exchange')
    exit(1)
modelName = root.get('modelName')

print('''<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription
    fmiVersion="%s"
    description="%s"
    modelName="%s"
    guid="%s"
    numberOfEventIndicators="0">

  <CoSimulation
    modelIdentifier="%s"
    canHandleVariableCommunicationStepSize="true"
    canGetAndSetFMUstate="true"
    providesDirectionalDerivative="true"/>

  <LogCategories>
    <Category name="logAll"/>
    <Category name="logError"/>
    <Category name="logFmiCall"/>
    <Category name="logEvent"/>
  </LogCategories>

  <DefaultExperiment startTime="0" stopTime="10" stepSize="0.1"/>
'''%(root.get('fmiVersion'),
     root.get('description'),
     "wrapper_" + modelName,
     uuid.uuid4(),
     "wrapper_"+ modelName,

))
SV = root.find('ModelVariables').findall('ScalarVariable')
print('  <ModelVariables>')
vrs = []
for sv in SV:
    type = 'Real'
    start = sv.find(type)
    if start == None:
        type = 'Boolean'
        start = sv.find(type)
    if start == None:
        type = 'Enum'
        start = sv.find(type)
    if start == None:
        type = 'Integer'
        start = sv.find(type)
    if start == None:
        type = 'String'
        start = sv.find(type)

        error(start)
    if 'derivative' not in start.attrib:
        vrs +=[sv.get('valueReference')]
        print('''    <ScalarVariable%s>'''% (''.join(['\n       '+name+'="' + sv.attrib[name]+'"' for name in sv.attrib])))

        print('      <%s %s/>' %( type,
                           ' '.join([s + '="' + start.attrib[s]+'"' for s in start.attrib])
        ))

        print('''    </ScalarVariable>
''')
vr = 0
while vr in vrs: vr += 1
fmudir = 'sources/' + modelName +'.fmu'
print('''
    <ScalarVariable
        name="fmu"
        valueReference="%d"
        description="Path to the ME fmu to be wrapped"
        causality="parameter">
      <String size="%d" start="%s"/>
</ScalarVariable>'''%(vr,len(fmudir)+1, fmudir))

vr += 1
while vr in vrs: vr += 1

print('''
    <ScalarVariable
        name="directional"
        valueReference="%d"
        description="Expects format vrUnKnown,unKnown,value"
        causality="parameter">
      <String size="20" start=""/>
</ScalarVariable>'''%vr)

outputs = root.find('ModelStructure').find('Outputs')
print('''
  </ModelVariables>
  <ModelStructure>
    <Outputs>
%s
    </Outputs>

    <Derivatives/>
    <DiscreteStates/>
    <InitialUnknowns/>
  </ModelStructure>
</fmiModelDescription>
'''%('\n'.join(['      <Unknown index="'+ out.get('index')+'"/>'for out in outputs.findall('Unknown')]),
))
