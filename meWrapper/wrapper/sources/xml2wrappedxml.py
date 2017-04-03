#!/usr/bin/python
from __future__ import print_function
import sys
import uuid
import xml.etree.ElementTree as e

def warning(*objs):
    print("WARNING: ", *objs, file=sys.stderr)

def error(*objs):
    print("ERROR: ", *objs, file=sys.stderr)

if len(sys.argv) < 2:
    print('USAGE: '+sys.argv[0] +' modelDescription-filename > header-filename', file=sys.stderr)
    print('Example: '+sys.argv[0]+' modelDescription.xml > header.h', file=sys.stderr)
    exit(1)

# Parse xml file
tree = e.parse(sys.argv[1])
root = tree.getroot()

# Get FMU version
fmiVersion = root.get('fmiVersion')
fmiDescription = root.get('description')
me = root.find('ModelExchange')
if me == None:
    error('wrapper only supports model exchange')
    exit(1)

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
    providesDirectionalDerivative="false"/>

  <LogCategories>
    <Category name="logAll"/>
    <Category name="logError"/>
    <Category name="logFmiCall"/>
    <Category name="logEvent"/>
  </LogCategories>

  <DefaultExperiment startTime="0" stopTime="10" stepSize="0.1"/>
'''%(root.get('fmiVersion'),
     root.get('description'),
     "wrapper_"+ root.get('modelName'),
     uuid.uuid4(),
     "wrapper_"+ root.get('modelName'),

))
SV = root.find('ModelVariables').findall('ScalarVariable')
print('  <ModelVariables>')
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
        print('''    <ScalarVariable%s>'''% (''.join(['\n       '+name+'="' + sv.attrib[name]+'"' for name in sv.attrib])))

        vor = [s + '="' + start.attrib[s]+'"' for s in start.attrib]
        print('      <%s %s/>' %( type,
                           ' '.join([s + '="' + start.attrib[s]+'"' for s in start.attrib])
        ))

        print('''    </ScalarVariable>
''')

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
