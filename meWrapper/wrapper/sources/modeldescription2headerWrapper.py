#!/usr/bin/python
from __future__ import print_function
import sys
import xml.etree.ElementTree as e

def warning(*objs):
    print("WARNING: ", *objs, file=sys.stderr)

def error(*objs):
    print('#error ',*objs)
    print("ERROR: ", *objs, file=sys.stderr)

if len(sys.argv) < 2:
    print('USAGE: '+sys.argv[0] +' modelDescription-filename > header-filename', file=sys.stderr)
    print('Example: '+sys.argv[0]+' modelDescription.xml > header.h', file=sys.stderr)
    exit(1)
print('''/*This file is genereted by modeldescription2header. DO NOT EDIT! */''')

# Parse xml file

tree = e.parse(sys.argv[1])
root = tree.getroot()

# Get FMU version
fmiVersion = root.get('fmiVersion')
ni = root.get('numberOfEventIndicators');

# Get model identifier.
csFmuType = False
meFmuType = False

modelIdentifier = "modelIdentifier"
providesDirectionalDerivative = False
canGetAndSetFMUstate = False

if fmiVersion == "1.0":
    modelIdentifier = root.get('modelIdentifier')
    providesDirectionalDerivative = root.get('providesDirectionalDerivative') == "true"
elif fmiVersion == "2.0":
    me = root.find('ModelExchange')
    cs = root.find('CoSimulation')
    csFmuType = cs != None
    meFmuType = me != None

    if cs != None:
        modelIdentifier = cs.get('modelIdentifier')
        providesDirectionalDerivative = cs.get('providesDirectionalDerivative') == "true"
        canGetAndSetFMUstate = cs.get('canGetAndSetFMUstate') == "true"
    elif me != None:
        modelIdentifier = me.get('modelIdentifier')
        providesDirectionalDerivative = me.get('providesDirectionalDerivative') == "true"
        canGetAndSetFMUstate = me.get('canGetAndSetFMUstate') == "true"
    else:
        warning('FMU is neither ModelExchange or CoSimulation')
        exit(1)


reals = {}
ints  = {}
bools = {}
states = {}
derivatives = {}

SV = root.find('ModelVariables').findall('ScalarVariable')
strs  = {}

for sv in SV:
    name = sv.attrib['name']
    vr = int(sv.attrib['valueReference'])

    if name in reals.values() or name in ints.values() or name in bools.values():
        error(sys.argv[1]+' contains multiple variables named "' + name + '"!')
        exit(1)

    R = sv.find('Real')
    I = sv.find('Integer')
    E = sv.find('Enum')
    B = sv.find('Boolean')
    S = sv.find('String')

    if R != None:
        if vr in reals:
            error(sys.argv[1]+' contains multiple Reals with VR='+str(vr))
            exit(1)
        start = float(R.attrib['start']) if 'start' in R.attrib else 0
        if meFmuType:
            if 'derivative' in R.attrib:
                states[vr] = (SV[int(R.attrib['derivative']) - 1].attrib['name'], start)
                start = float(R.attrib['derivative']) if 'derivative' in R.attrib else 0
                derivatives[vr] = (name, start)
        reals[vr] = (name, start)

    elif I != None or E != None:
        if vr in ints:
            error(sys.argv[1]+' contains multiple Integers/Enums with VR='+str(vr))
            exit(1)
        IE = I if I != None else E
        start = int(IE.attrib['start']) if 'start' in IE.attrib else 0
        ints[vr] = (name, start)
    elif B != None:
        if vr in bools:
            error(sys.argv[1]+' contains multiple Booleans with VR='+str(vr))
            exit(1)
        start = B.attrib['start'] if 'start' in B.attrib else '0'
        bools[vr] = (name, 1 if start == '1' or start == 'true' else 0)
    elif S != None:
        start = S.attrib['start'] if 'start' in S.attrib else ''
        if 'size' in S.attrib:
            size = int(S.attrib['size'])
        else:
            error('String variable "%s" missing size' % name)
            exit(1)
        strs[vr] = (name, start, size)
    else:
        error('Variable "%s" has unknown/unsupported type' % name)
        exit(1)

print('''#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()
#include "commonWrapper/modelExchange.h"

#define MODEL_IDENTIFIER %s
#define MODEL_GUID "%s"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE %i
#define CAN_GET_SET_FMU_STATE %i
#define NUMBER_OF_REALS %i
#define NUMBER_OF_INTEGERS %i
#define NUMBER_OF_BOOLEANS %i
#define NUMBER_OF_STRINGS %i
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0
''' % (
    modelIdentifier,
    root.get('guid'),
    1 if providesDirectionalDerivative else 0,
    1 if canGetAndSetFMUstate else 0,
    len(reals),
    len(ints),
    len(bools),
    len(strs),
))

print('''
#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
%s%s%s%s%s
} modelDescription_t;
''' % (
    ''.join(['    fmi2Real '+value[0]+'; //VR='+str(key)+'\n' for key,value in reals.items()]),
    ''.join(['    fmi2Integer '+value[0]+'; //VR='+str(key)+'\n' for key,value in ints.items()]),
    ''.join(['    fmi2Boolean '+value[0]+'; //VR='+str(key)+'\n' for key,value in bools.items()]),
    ''.join(['    fmi2Char    '+value[0]+'[256]; //VR='+str(key)+'\n' for key,value in strs.items()]),
    '    fmi2Boolean dirty;' if meFmuType else '',
))

print('''
#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
%s%s%s%s%s
};
''' % (
    ''.join(['    '+str(value[1])+', //'+value[0]+'\n' for key,value in reals.items()]),
    ''.join(['    '+str(value[1])+', //'+value[0]+'\n' for key,value in ints.items()]),
    ''.join(['    '+str(value[1])+', //'+value[0]+'\n' for key,value in bools.items()]),
    ''.join(['    "'+value[1]+'", //'+value[0]+'\n' for key,value in strs.items()]),
    '    1,' if meFmuType else '',
))

print('''
%s%s%s%s''' % (
    ''.join(['#define VR_'+value[0].upper()+' '+str(key)+'\n' for key,value in reals.items()]),
    ''.join(['#define VR_'+value[0].upper()+' '+str(key)+'\n' for key,value in ints.items()]),
    ''.join(['#define VR_'+value[0].upper()+' '+str(key)+'\n' for key,value in bools.items()]),
    ''.join(['#define VR_'+value[0].upper()+' '+str(key)+'\n' for key,value in strs.items()]),
))



print('''
//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters
''')


def gen_getters_setters(t, d):
    print('''
static fmi2Status generated_fmi2Get%s(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2%s value[]) {
    fmi2_import_get_%s(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2Set%s(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2%s value[]) {
    if( *getFMU() != NULL)
        fmi2_import_set_%s(*getFMU(),vr,nvr,value);
    return fmi2OK;
}''' % (
        t,t,
        t.lower(),
        t,t,
        t.lower(),
        # A bit convoluted maybe, but it works.
        # This makes sure settings strings larger than the FMU can handle results in an error, not a crash
    ))

gen_getters_setters('Real',    reals)
gen_getters_setters('Integer', ints)
gen_getters_setters('Boolean', bools)
gen_getters_setters('String',  strs)

print('#endif //MODELDESCRIPTION_H')
