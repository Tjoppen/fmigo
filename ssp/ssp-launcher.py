#!/usr/bin/python
from __future__ import print_function
import tempfile
import zipfile
import sys
import os
import os.path
import glob
from lxml import etree
import subprocess
import shutil
import argparse
import psutil
import platform
from zipfile import ZipFile

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

RESOURCE_DIR='resources'
SSD_NAME='SystemStructure.ssd'
CAUSALITY='causality'
MODELDESCRIPTION='modelDescription.xml'

ns = {
    'ssc': 'http://www.pmsf.net/xsd/SystemStructureCommonDraft',
    'ssd': 'http://www.pmsf.net/xsd/SystemStructureDescriptionDraft',
    'ssv': 'http://www.pmsf.net/xsd/SystemStructureParameterValuesDraft',
    'ssm': 'http://www.pmsf.net/xsd/SystemStructureParameterMappingDraft',
    'fmigo':'http://umit.math.umu.se/FmiGo.xsd',
}

# FIXME: these should not be global if the intent is to use this file as a module
fmus = []
systems = []
parameters = {}
shaftconstraints = []

schema_names = {
    'ssd': 'SystemStructureDescription.xsd',
    'ssm': 'SystemStructureParameterMapping.xsd',
    'ssv': 'SystemStructureParameterValues.xsd',
    'fmigo':'FmiGo.xsd',
}
schemas = {}

class SSPException(Exception):
    pass

for type in schema_names:
    # Expect schema to be located next to this script
    try:
        schema_path = os.path.join(os.path.dirname(__file__), schema_names[type])
        schemas[type] = etree.XMLSchema(etree.parse(schema_path))
    except Exception as e:
        eprint(e)
        eprint('Cannot open/parse %s - no %s validation performed' % (schema_path, type))

def parse_and_validate(type, path):
    if type in schemas:
        parser = etree.XMLParser(schema=schemas[type])
    else:
        parser = etree.XMLParser()

    return etree.parse(path, parser)

# Adds (key,value) to given multimap.
# Each (key,value) may appear only once.
def add_multimap_value(multimap, key, value):
    if key in multimap:
        if value in multimap[key]:
            raise SSPException( str((key,value)) + ' already exists in ' + str(multimap))
        multimap[key].add(value)
    else:
        multimap[key] = set([value])

def traverse(startsystem, startkey, target_kind, use_sigdictrefs):
    global fmus, systems
    ttl = len(fmus) + len(systems)
    sys = startsystem
    key = startkey

    #TODO: deal with unit conversion
    factor = 1
    offset = 0
    #eprint('%s.%s (unit=%s)' % (self.get_name(), conn.attrib['name'], str(unit)))

    while True:
        #eprint('key = ' + str(key) + ', sys = ' + sys.name)
        if not key in sys.connections:
            eprint( 'No connection %s in system %s' % (str(key), sys.name))
            return None, None
            # exit(1)

        #eprint('sys.sigdictrefs: ' + str(sys.sigdictrefs))
        key = sys.connections[key]
        if key[0] == '':
            # Connection leads to parent node
            if sys.parent == None:
                # Can't go any further - issue a warning and move on
                eprint ('WARNING: ' + str(startkey) + ' takes data from ' +
                       key[1] + ' of root system, which no output connects to - skipping')
                return None, None

            #eprint('up')
            key = (sys.name, key[1])
            sys = sys.parent
        elif key[0] in sys.fmus:
            fmu = sys.fmus[key[0]]

            #correct kind?
            if fmu.connectors[key[1]]['kind'] == target_kind:
                return fmu, key[1]
            else:
                return None, None
        elif key[0] in sys.subsystems:
            #eprint('down')
            sys = sys.subsystems[key[0]]
            key = ('', key[1])
        elif use_sigdictrefs and key[0] in sys.sigdictrefs and key[1] in sys.sigdictrefs[key[0]][1]:
            #we're pointed to a signal dictionary
            #find the corresponding output
            d = sys.sigdictrefs[key[0]]
            dictionary_name = d[0]
            signaldict = sys.find_signal_dictionary(dictionary_name)
            de = signaldict[key[1]]

            if de[1] == None:
                raise SSPException('SignalDictionary "%s" has no output signal connected to it' % dictionary_name)

            return de[1], de[2]
        else:
            raise SSPException('Key %s of kind %s not found in system %s. fmus = %s, subsystems = %s' %
                (str(key), target_kind, sys.name, sys.fmus, sys.subsystems)
            )

        ttl -= 1
        if ttl <= 0:
            raise SSPException('SSP contains a loop!')

def get_attrib(s, name, default=None):
    if name in s.attrib:
        ret = s.attrib[name]
        del s.attrib[name]
        return ret
    else:
        if default == None:
            raise SSPException('get_attrib(): missing required attribute ' + name)
        return default

# Can remove single node, or list/tuple of nodes
def remove_if_empty(parent, node):
  def remove_if_empty_internal(parent, node):
    if node != None and len(node.attrib) == 0 and len(node) == 0 and (node.text == None or node.text.strip() == ''):
        parent.remove(node)

  #remove if all attributes, subnodes and text have been dealt with
  if isinstance(node, tuple) or isinstance(node, list):
    for n in node:
      remove_if_empty_internal(parent, n)
  else:
    remove_if_empty_internal(parent, node)

def find_elements(s, first, second):
    a = s.find(first,  ns)
    return a, a.findall(second,  ns) if a != None else []

def check_name(name):
    if '.' in name:
        # We might allow this at some point
        raise SSPException('ERROR: FMU/System name "%s" contains a dot, which is not allowed' % name)

# Escapes a variable name or value
def escape(s):
    return s.replace('\\','\\\\').replace(':','\\:').replace(',','\\,')

def parse_parameter_bindings(path, baseprefix, parameterbindings):
    for pb in parameterbindings[1]:
        pb_prefix = get_attrib(pb, 'prefix', '')
        prefix = baseprefix

        # Spec says to use prefix="Foo." to address parameters in subsystems
        # We've split FMU names from variable names, so the case where a prefix would not have a trailing dot is nonsensical
        if len(pb_prefix) > 0:
            # Expect a trailing dot and strip it
            if pb_prefix[-1] != '.':
                raise SSPException('ERROR: ParameterBinding prefix must end in dot (%s -> "%s")' % (baseprefix, pb_prefix) )
            prefix = baseprefix + '.' + pb_prefix[0:-1]

        pvs = pb.find('ssd:ParameterValues', ns)

        x_ssp_parameter_set = 'application/x-ssp-parameter-set'
        t = get_attrib(pb, 'type', x_ssp_parameter_set)
        if t != x_ssp_parameter_set:
            raise SSPException('Expected ' + x_ssp_parameter_set + ', got ' + t)

        if 'source' in pb.attrib:
            if not pvs is None:
                raise SSPException('ParameterBinding must have source or ParameterValues, not both')

            #read from file
            #TODO: print unhandled XML in ParameterSet
            tree = parse_and_validate('ssv', os.path.join(path, get_attrib(pb, 'source')))
            parameterset = tree.getroot()
            inline_ssv = False
        else:
            if pvs is None:
                raise SSPException('ParameterBinding missing both source and ParameterValues')

            # Clarification from MAP-SSP mailing list says that ParameterSet should be put inside ParameterValues
            # Validate while we're at it
            parameterset = pvs.find('ssv:ParameterSet', ns)
            schemas['ssv'].assertValid(parameterset)
            inline_ssv = True

            if parameterset is None:
                raise SSPException('No ParameterSet in ParameterValues')

        ssv_version = get_attrib(parameterset, 'version')
        ssv_name    = get_attrib(parameterset, 'name')
        ps          = find_elements(parameterset, 'ssv:Parameters', 'ssv:Parameter')

        if ssv_version != 'Draft20170606':
            raise SSPException('Expected SSV version Draft20170606, got %s' % ssv_version)

        for pv in ps[1]:
            r = pv.find('ssv:Real', ns)
            i = pv.find('ssv:Integer', ns)
            b = pv.find('ssv:Boolean', ns)
            s = pv.find('ssv:String', ns)
            e = pv.find('ssv:Enumeration', ns)
            name = get_attrib(pv, 'name')
            key = str([prefix, name])
            param = {
            'paramname': name,
            'fmuname':   prefix,
            }

            if r != None:
                if 'unit' in r.attrib:
                    raise SSPException('Not dealing with parameters with units yet')

                param.update({
                    'type': 'r',
                    'value': float(get_attrib(r, 'value')),
                })
            elif i != None:
                param.update({
                    'type': 'i',
                    'value': int(get_attrib(i, 'value')),
                })
            elif b != None:
                param.update({
                    'type': 'b',
                    'value': get_attrib(b, 'value'), #keep booleans as-is
                })
            elif s != None:
                param.update({
                    'type': 's',
                    'value': get_attrib(s, 'value'),
                })
            elif e != None:
                raise SSPException('Enumerations not supported')
            else:
                raise SSPException('Unsupported parameter type: ' + str(pv[0].tag))

            parameters[key] = param
            remove_if_empty(pv, (r, i, b, s))
            remove_if_empty(ps[0], pv)

        remove_if_empty(parameterset, ps[0])
        if inline_ssv:
            remove_if_empty(pvs, parameterset)
            remove_if_empty(pb, pvs)

        #deal with any ssm
        pm = pb.find('ssd:ParameterMapping', ns)
        if pm != None:
            x_ssp_parameter_mapping = 'application/x-ssp-parameter-mapping'
            t = get_attrib(pm, 'type', x_ssp_parameter_mapping)
            if t != x_ssp_parameter_mapping:
                raise SSPException('Expected ' + x_ssp_parameter_mapping + ', got ' + t)

            #TODO: print unhandled XML in ParameterMapping too

            if 'source' in pm.attrib:
                tree = parse_and_validate('ssm', os.path.join(path, get_attrib(pm, 'source')))
                pm2 = tree.getroot()
                inline_ssm = False
            else:
                #TODO: Can't validate inline SSM yet
                #pm2 = pm
                #inline_ssm = True
                raise SSPException("Can't parse inline ParamaterMapping (SSM) yet. See the MAP-SSP mailing list for more information")

            mes = pm2.findall('ssm:MappingEntry', ns)
            for me in mes:
                sourcekey = str([prefix, get_attrib(me, 'source')])
                targetkey = str([prefix, get_attrib(me, 'target')])
                #eprint('target: ' +target)

                p = parameters[sourcekey]
                del parameters[sourcekey]

                lt = me.find('ssm:LinearTransformation', ns)
                if lt != None:
                    factor = float(get_attrib(lt, 'factor'))
                    offset = float(get_attrib(lt, 'offset'))

                    if p['type'] != 'r':
                        raise SSPException('LinearTransformation only supported in Real')

                    p['value'] = p['value'] * factor + offset
                    remove_if_empty(me, lt)
                elif len(me) > 0:
                    raise SSPException("Found MappingEntry with sub-element which isn't LinearTransformation, which is not yet supported")

                parameters[targetkey] = p
                remove_if_empty(pm2, me)

            remove_if_empty(pb, pm)

        remove_if_empty(parameterbindings[0], pb)

class FMU:
    def __init__(self, name, comp, path, system):
        check_name(name)
        self.name = name
        self.path = path
        self.system = system

        self.connectors = {}
        self.physicalconnectors = {}
        self.csvs = []

        connectors = find_elements(comp, 'ssd:Connectors', 'ssd:Connector')
        for conn in connectors[1]:
            name = get_attrib(conn, 'name')
            kind = get_attrib(conn, 'kind')
            unit = None

            if len(conn) > 0:
                if len(conn) > 1:
                    raise SSPException('More then one sub-element of Connector - bailing out')
                unit = get_attrib(conn[0], 'unit', False)
                if unit == False:
                    unit = None
                remove_if_empty(conn, conn[0])

            remove_if_empty(connectors[0], conn)

            self.connectors[name] = {
                'kind': kind,
                'unit': unit,
            }
        remove_if_empty(comp, connectors[0])

        cannotations = find_elements(comp, 'ssd:Annotations', 'ssc:Annotation')
        for cannotation in cannotations[1]:
            type = get_attrib(cannotation, 'type')
            if type == 'se.umu.math.umit.ssp.physicalconnectors':
                if 'fmigo' in schemas:
                    schemas['fmigo'].assertValid(cannotation[0])

                for pc in find_elements(cannotation, 'fmigo:PhysicalConnectors', 'fmigo:PhysicalConnector1D')[1]:
                    name = get_attrib(pc, 'name')
                    pcdict = {
                    'type': '1d',
                    'vars': [get_attrib(pc, ss) for ss in ['stateVariable', 'flowVariable', 'accelerationVariable', 'effortVariable']]
                    }

                    self.physicalconnectors[name] = pcdict
                    remove_if_empty(cannotation[0], pc)
                remove_if_empty(cannotation, cannotation[0])
            elif type == 'se.umu.math.umit.fmigo-master.csvinput':
                if 'fmigo' in schemas:
                    schemas['fmigo'].assertValid(cannotation[0])

                for csv in find_elements(cannotation, 'fmigo:CSVFilenames', 'fmigo:CSVFilename')[1]:
                    self.csvs.append(csv.text)
                    csv.text = None
                    remove_if_empty(cannotation[0], csv)
                remove_if_empty(cannotation, cannotation[0])
            else:
                eprint('WARNING: Found unknown Annotation of type "%s"' % type)
            remove_if_empty(cannotations[0], cannotation)
        remove_if_empty(comp, cannotations[0])

        global fmus
        self.id = len(fmus)
        fmus.append(self)
        #eprint('%i: %s %s, %i connectors' % (self.id, self.name, self.path, len(connectors)))

    def get_name(self):
        return self.system.get_name() + '.' + self.name

    # Returns path relative to given path (d)
    def relpath(self, d):
        return os.path.relpath(self.path, d)

    def connect(self, connectionmultimap):
        '''
        Adds connections for this FMU to other FMUs in the System tree to the given connections multimap
        '''
        for name,conn in self.connectors.items():
            # Look at inputs, work back toward outputs
            if conn['kind'] == 'input':
                fmu, fmu_variable = traverse(self.system, (self.name, name), 'output', True)
                if fmu != None:
                    key = (fmu.id, fmu_variable)
                    value = (self.id, name)
                    add_multimap_value(connectionmultimap, key, value)

class SystemStructure:
    def __init__(self, root):
        units = find_elements(root, 'ssd:Units', 'ssd:Unit')
        self.name = get_attrib(root, 'name')
        self.arguments = []
        check_name(self.name)

        # Get schemaLocation if set. This avoids some residual XML
        self.schemaLocation = get_attrib(root, '{http://www.w3.org/2001/XMLSchema-instance}schemaLocation', '')

        # Units keyed on name. Like {name: {'units': (units,), 'factor': 1, 'offset': 0}}
        self.unitsbyname = {}

        # Units keyed on units. Like {(units,): {name: {'factor': 1, 'offset': 0}}}
        self.unitsbyunits = {}

        for u in units[1]:
            name = get_attrib(u, 'name')
            bu = u.find('ssd:BaseUnit', ns)
            key = tuple([int(get_attrib(bu, n, '0')) for n in ['kg','m','s','A','K','mol','cd','rad']])

            #want factor and offset
            factor = float(get_attrib(bu, 'factor', '1'))
            offset = float(get_attrib(bu, 'offset', '0'))

            #put in the right spots
            self.unitsbyname[name] = {'units': key, 'factor': factor, 'offset': offset}

            if not key in self.unitsbyunits:
                self.unitsbyunits[key] = {}

            self.unitsbyunits[key][name] = {'factor': factor, 'offset': offset}
            remove_if_empty(u, bu)
            remove_if_empty(units[0], u)
        remove_if_empty(root, units[0])

        #eprint('unitsbyname: '+str(self.unitsbyname))
        #eprint('unitsbyunits: '+str(self.unitsbyunits))

        # Return None if no fmigo:MasterArguments
        self.timestep = None
        self.duration = None
        self.executionorder = None

        annotations = find_elements(root, 'ssd:Annotations', 'ssc:Annotation')
        for annotation in annotations[1]:
            type = get_attrib(annotation, 'type')
            if type == 'se.umu.math.umit.fmigo-master.arguments':
                if 'fmigo' in schemas:
                    schemas['fmigo'].assertValid(annotation[0])

                timestep = get_attrib(annotation[0], 'timestep', '')
                if len(timestep) > 0:
                    self.timestep = float(timestep)

                duration = get_attrib(annotation[0], 'duration', '')
                if len(duration) > 0:
                    self.duration = float(duration)

                # We might do this later
                for arg in find_elements(annotation, 'fmigo:MasterArguments', 'fmigo:arg')[1]:
                    self.arguments.append(arg.text)
                    arg.text = None
                    remove_if_empty(annotation[0], arg)

                remove_if_empty(annotation, annotation[0])
            elif type == 'se.umu.math.umit.fmigo-master.executionorder':
                if 'fmigo' in schemas:
                    schemas['fmigo'].assertValid(annotation[0])

                self.executionorder = annotation[0][0]
                annotation[0].remove(self.executionorder)
                remove_if_empty(annotation, annotation[0])
            else:
                eprint('WARNING: Found unknown Annotation of type "%s"' % type)
            remove_if_empty(annotations[0], annotation)
        remove_if_empty(root, annotations[0])

class System:
    '''
    A System is a tree of Systems and FMUs
    In each System there are Connections between
    '''

    @classmethod
    def fromfile(cls, d, filename, parent=None, residual_is_error=False):
        path = os.path.join(d, filename)

        #parse XML, delete all attribs and element we know how to deal with
        #print residual, indicating things we don't yet support
        tree = parse_and_validate('ssd', path)

        # Remove comments, which would otherwise result in residual XML
        for c in tree.xpath('//comment()'):
            c.getparent().remove(c)

        if 'version' in tree.getroot().attrib:
            version = get_attrib(tree.getroot(), 'version')
        else:
            version = 'Draft20150721'
            eprint('WARNING: version not set in root, assuming ' + self.version)

        structure = SystemStructure(tree.getroot())

        sys = tree.getroot().findall('ssd:System', ns)
        if len(sys) != 1:
            raise SSPException( 'Must have exactly one System')
        s = sys[0]
        ret = cls(d, s, version, parent, structure)
        remove_if_empty(tree.getroot(), s)

        if len(tree.getroot()) > 0 or len(tree.getroot().attrib) > 0:
            s = etree.tostring(tree.getroot())
            err = 'Residual XML: \n'+s.decode('utf-8')
            if residual_is_error:
                raise SSPException(err)
            else:
                eprint('WARNING: '+err)

        return ret

    @classmethod
    def fromxml(cls, d, s, version, parent):
        return cls(d, s, version, parent, parent.structure)

    def __init__(self, d, s, version, parent, structure, residual_is_error=False):
        global systems
        systems.append(self)

        self.d = d
        self.version = version
        self.name = get_attrib(s, 'name')
        check_name(self.name)
        self.description = get_attrib(s, 'description', '')
        self.parent = parent
        self.structure = structure
        self.inputs  = {}
        self.outputs = {}

        #'causality' changed to 'kind' on 2015-10-21
        self.kindKey = 'kind' if self.version != 'Draft20151021' else 'causality'

        #spec allows these to not exist
        connectors  = find_elements(s, 'ssd:Connectors',            'ssd:Connector')
        connections = find_elements(s, 'ssd:Connections',           'ssd:Connection')
        components  = find_elements(s, 'ssd:Elements',              'ssd:Component')
        subsystems  = find_elements(s, 'ssd:Elements',              'ssd:System')
        signaldicts = find_elements(s, 'ssd:SignalDictionaries',    'ssd:SignalDictionary')
        sigdictrefs = find_elements(s, 'ssd:Elements',              'ssd:SignalDictionaryReference')
        params      = find_elements(s, 'ssd:ParameterBindings',     'ssd:ParameterBinding')
        annotations = find_elements(s, 'ssd:Annotations',           'ssc:Annotation')
        elements    = s.find('ssd:Elements', ns)

        for conn in connectors[1]:
            kind = get_attrib(conn, self.kindKey)
            name = get_attrib(conn, 'name')

            #TODO: self.inputs and self.output aren't actually taken into account in traverse()
            if kind == 'input':
                self.inputs[name] = conn
            elif kind == 'output':
                self.outputs[name] = conn
            elif kind in ['parameter', 'calculatedParameter', 'inout']:
                eprint('WARNING: Unimplemented connector kind: ' + kind)
            else:
                raise SSPException('Unknown connector kind: ' + kind)

            if len(conn) > 0:
                eprint("WARNING: Connection %s in system %s has type information, which isn't handled yet" % (name, self.name))
                #remove_if_empty(conn, conn[0])
            remove_if_empty(connectors[0], conn)
        remove_if_empty(s, connectors[0])

        # Bi-directional
        self.connections = {}
        for conn in connections[1]:
            a = (get_attrib(conn, 'startElement', ''), get_attrib(conn, 'startConnector'))
            b = (get_attrib(conn, 'endElement', ''),   get_attrib(conn, 'endConnector'))
            self.connections[a] = b
            self.connections[b] = a
            remove_if_empty(connections[0], conn)
        remove_if_empty(s, connections[0])

        #eprint(self.connections)

        self.subsystems = {}
        self.fmus     = {}
        for comp in components[1]:
            #eprint(comp)
            t    = get_attrib(comp, 'type')
            name = get_attrib(comp, 'name')
            check_name(name)
            #eprint(t)

            if t == 'application/x-ssp-package':
                d2 = os.path.join(d, os.path.splitext(get_attrib(comp, 'source'))[0])
                child = System.fromfile(d2, SSD_NAME, self, residual_is_error=residual_is_error)
                #eprint('Added subsystem ' + child.name)
                self.subsystems[name] = child
            elif t == 'application/x-fmu-sharedlibrary':
                source = os.path.join(d, get_attrib(comp, 'source'))
                self.fmus[name] = FMU(
                    name,
                    comp,
                    source,
                    self,
                )
            else:
                raise SSPException('unknown type: ' + t)

            #parse parameters after subsystems so their values get overriden properly
            cparams = find_elements(comp, 'ssd:ParameterBindings', 'ssd:ParameterBinding')
            parse_parameter_bindings(self.d, self.get_name() + '.' + name , cparams)
            remove_if_empty(comp, cparams[0])
            remove_if_empty(components[0], comp)

        self.signaldicts = {}
        for sigdict in signaldicts[1]:
            des = {}
            for de in sigdict.findall('ssd:DictionaryEntry', ns):
                #NOTE: de[0].tag is the type of the entry, but we don't really need it
                #the second and third entry in the tuple are the FMU and connector on which the signal source is available
                des[get_attrib(de, 'name')] = (get_attrib(de[0], 'unit'), None, '')
                remove_if_empty(de, de[0])
                remove_if_empty(sigdict, de)
            self.signaldicts[get_attrib(sigdict, 'name')] = des
            remove_if_empty(signaldicts[0], sigdict)
        remove_if_empty(s, signaldicts[0])
        #eprint('SignalDictionaries: ' + str(self.signaldicts))

        self.sigdictrefs = {}
        for sdr in sigdictrefs[1]:
            drs = {}
            conns = find_elements(sdr, 'ssd:Connectors', 'ssd:Connector')
            for conn in conns[1]:
                if get_attrib(conn, 'kind') != 'inout':
                    raise SSPException('Only inout connectors supports in SignalDictionaryReferences for now')
                drs[get_attrib(conn, 'name')] = get_attrib(conn[0], 'unit')
                remove_if_empty(conn, conn[0])
                remove_if_empty(conns[0], conn)
            self.sigdictrefs[get_attrib(sdr, 'name')] = (get_attrib(sdr, 'dictionary'), drs)
            remove_if_empty(sdr, conns[0])
            remove_if_empty(sigdictrefs[0], sdr)
        #eprint('SignalDictionaryReference: ' + str(self.sigdictrefs))

        for annotation in annotations[1]:
            type = get_attrib(annotation, 'type')
            if type == 'se.umu.math.umit.ssp.kinematicconstraints':
                if 'fmigo' in schemas:
                    schemas['fmigo'].assertValid(annotation[0])

                for shaft in find_elements(annotation, 'fmigo:KinematicConstraints', 'fmigo:ShaftConstraint')[1]:
                    shaftconstraint = {
                    'holonomic': get_attrib(shaft, 'holonomic') == 'true'
                    }

                    for i in [1,2]:
                        shaftconstraint['element%i' % i]   = self.get_name() + '.' + get_attrib(shaft, 'element%i' % i)
                        shaftconstraint['connector%i' % i] = get_attrib(shaft, 'connector%i' % i)

                    shaftconstraints.append(shaftconstraint)
                    remove_if_empty(annotation[0], shaft)
                remove_if_empty(annotation, annotation[0])
            else:
                eprint('WARNING: Found unknown Annotation of type "%s"' % type)
            remove_if_empty(annotations[0], annotation)
        remove_if_empty(s, annotations[0])

        for subsystem in subsystems[1]:
            ss = System.fromxml(d, subsystem, self.version, self)
            self.subsystems[ss.name] = ss
            remove_if_empty(subsystems[0], subsystem)

        #NOTE: Component, System and SignalDictionaryReference are all subnodes of Elements
        #Only delete it once
        remove_if_empty(s, elements)

        #parse parameters after subsystems so their values get overriden properly
        parse_parameter_bindings(self.d, self.get_name() , params)
        remove_if_empty(s, params[0])

    def find_signal_dictionary(self, dictionary_name):
        sys = self
        while True:
            if dictionary_name in sys.signaldicts:
                #eprint('found the actual dict in system ' + sys.name)
                return sys.signaldicts[dictionary_name]

            sys = sys.parent

            if sys == None:
                #we're assuming that dictionaries exist somewhere straight up in the hierarchy, never in a sibling/child system
                #this may change if we find a counterexample in the wild
                raise SSPException('Failed to find SignalDictionary "%s" starting at System "%s"' % (dictionary_name, self.name))

    def resolve_dictionary_inputs(self):
        #for each signal dictionary, find where it is referenced and if that reference is connected to by an output
        #this because multiple inputs can connect to a dictionary, but only one output can be connected
        for name,(dictionary_name,drs) in self.sigdictrefs.items():
            #eprint('name: ' + name)
            #eprint('dict: ' + dictionary)
            #eprint('drs: ' + str(drs))

            for key,unit in drs.items():
                #eprint('traversing from ' + str((name, key)))
                fmu, fmu_variable = traverse(self, (name, key), 'output', False)

                if fmu != None:
                    #found it!
                    #now find the actual signal dictionary so everyone can find the output
                    #eprint(str((dictionary_name, key)) + ' <-- ' + str((fmu.id,fmu_variable)))

                    signaldict = self.find_signal_dictionary(dictionary_name)
                    de = signaldict[key]

                    if de[1] != None:
                        raise SSPException('Dictionary %s has more than one output connected to %s' % (dictionary_name, key))

                    #put the FMU and variable name in the dict
                    signaldict[key] = (de[0], fmu, fmu_variable)

        for name,subsystem in self.subsystems.items():
            subsystem.resolve_dictionary_inputs()

    def get_name(self):
        return self.parent.get_name() + '.' + self.name if self.parent != None else self.name

def unzip_ssp(dest_dir, ssp_filename):
    #eprint('unzip_ssp: ' + dest_dir + ' ' + ssp_filename)
    with zipfile.ZipFile(ssp_filename) as z:
        z.extractall(dest_dir)
        # Extract sub-SSPs
        for f in glob.glob(os.path.join(dest_dir, RESOURCE_DIR, '*.ssp')):
            d = os.path.join(dest_dir, RESOURCE_DIR, os.path.splitext(f)[0])
            os.mkdir(d)
            unzip_ssp(d, f)

        # Extract modelDescription.xml from FMUs
        for f in glob.glob(os.path.join(dest_dir, RESOURCE_DIR, '*.fmu')):
            d = os.path.join(dest_dir, RESOURCE_DIR, os.path.splitext(f)[0])
            os.mkdir(d)
            with zipfile.ZipFile(f) as z:
                z.extract(MODELDESCRIPTION, d)


def parse_ssp(ssp_path, cleanup_zip = True, residual_is_error = False):

    global fmus, system, parameters, shaftconstraints, SSD_NAME, d
    fmus = []
    systems = []
    parameters = {}
    shaftconstraints = []

    file_ext = os.path.splitext(ssp_path)[1].lower()

    # Allow custom named ssd files
    if file_ext == '.ssd':
       SSD_NAME = os.path.basename(ssp_path)

    # Check if we run master directly from an SSD XML file instead of an SSP zip archive
    if os.path.basename(ssp_path) == SSD_NAME:
        d = os.path.dirname(ssp_path)
        unzipped_ssp = False
    else:
        d = tempfile.mkdtemp(prefix='ssp')
        unzip_ssp(d, ssp_path)
        unzipped_ssp = True

    root_system = System.fromfile(d, SSD_NAME, residual_is_error=residual_is_error)

    root_system.resolve_dictionary_inputs()

    # Figure out connections, parse modelDescriptions
    connectionmultimap = {} # Multimap of outputs to inputs
    mds = []
    fmumap = {}  #maps fmu names to IDs
    for fmu in fmus:
        fmumap[fmu.get_name()] = fmu.id
        fmu.connect(connectionmultimap)

        with zipfile.ZipFile(fmu.path) as z:
            md_data = z.read('modelDescription.xml')

        # Parse modelDescription, turn variable list into map
        # tree = etree.parse(os.path.join(os.path.splitext(fmu.path)[0], MODELDESCRIPTION))
        # root = tree.getroot()
        # eprint(md_data)
        root = etree.XML(md_data)

        svs = {}
        for sv in root.find('ModelVariables').findall('ScalarVariable'):
            name = sv.attrib['name']
            if name in svs:
                raise SSPException(fmu.path + ' contains multiple variables named "' + name + '"!')

            causality = ''

            if not CAUSALITY in sv.attrib:
                if 'variability' in sv.attrib and sv.attrib['variability'] == 'parameter':
                    #this happens with SampleSystemSubSystemDictionary.ssp
                    eprint(('WARNING: Found variable %s without causality and variability="parameter". ' % name)+
                          'This violates the spec. Treating as a parameter')
                    causality = 'parameter'
                else:
                    eprint('WARNING: FMU %s has variable %s without causality - ignoring' % (fmu.get_name(), name))
                    continue
            else:
                causality = sv.attrib[CAUSALITY]

            t = ''
            if sv.find('Real')      != None: t = 'r'
            elif sv.find('Integer') != None: t = 'i'
            elif sv.find('Boolean') != None: t = 'b'
            elif sv.find('Enum')    != None: t = 'e'
            elif sv.find('String')  != None: t = 's'
            else:
                raise SSPException(fmu.path + ' variable "' + name + '" has unknown type')

            svs[name] = {
                'vr': int(sv.attrib['valueReference']),
                CAUSALITY: causality,
                'type': t,
            }
        mds.append(svs)

    flatparams = []
    for value in parameters.values():
        fmuname   = value['fmuname']
        paramname = value['paramname']
        if fmuname in fmumap:
            fmu = fmus[fmumap[fmuname]]
            if paramname in mds[fmu.id]:
                p = mds[fmu.id][paramname]
                if p[CAUSALITY] == 'input' or p[CAUSALITY] == 'parameter':
                    flatparams.extend([
                        '-p','%s,%i,%i,%s' % (
                            value['type'],
                            fmu.id,
                            p['vr'],
                            # Escape backslashes and colons
                            escape(str(value['value']))
                        )
                    ])
                else:
                    eprint('WARNING: FMU %s, tried to set variable %s which is neither an input nor a parameter' % (fmuname, paramname))
            else:
                eprint('WARNING: FMU %s has no variable called %s' % (fmuname, paramname))
        else:
            eprint('WARNING: No FMU called %s for parameter %s' % (fmuname, paramname))

    #eprint(connections)
    #eprint(mds)

    # Build command line
    flatconns = []
    for key in connectionmultimap.keys():
        fr = key
        to1 = connectionmultimap[key]
        for to in to1:
            #eprint(str((fr,to)) + ' vs ' + str(mds[fr[0]]))
            f = mds[fr[0]]
            fv = f[fr[1]]
            t = mds[to[0]]
            tv = t[to[1]]

            connstr = '%s,%i,%i,%s,%i,%i' % (fv['type'], fr[0], fv['vr'], tv['type'], to[0], tv['vr'])
            flatconns.extend(['-c', connstr])

    holonomic = None
    kinematicconns = []
    for shaft in shaftconstraints:
        ids = [fmumap[shaft['element%i' % i]] for i in [1,2]]
        conn = ['shaft'] + [str(id) for id in ids]

        for i in [1,2]:
            fmu     = fmus[ids[i-1]]
            fmuname = fmu.get_name()
            pcs     = fmu.physicalconnectors
            name    = shaft['connector%i' % i]

            if not name in pcs:
                raise SSPException('ERROR: Shaft constraint refers to physical connector %s in %s, which does not exist' % (name, fmuname))

            pc = pcs[name]

            if pc['type'] != '1d':
                raise SSPException('ERROR: Physical connector %s in %s of type %s, not 1d' % (name, fmuname, pc['type']))

            eprint("shaft['holonomic'] = " + str(shaft['holonomic']))
            if holonomic is None:
                holonomic = shaft['holonomic']
            elif shaft['holonomic'] != holonomic:
                raise SSPException('ERROR: ShaftConstraints cannot mix-and-match holonomic true/false')

            conn += [escape(key) for key in pc['vars']]

        kinematicconns.extend(['-C', ','.join(conn)])

    executionorder = []
    if not root_system.structure.executionorder is None:
        # Set for keeping track of which FMUs we've seen in the ExecutionOrder
        fmuids = set()

        def traverse(p, tag):
            # p -> s -> p ...
            sp = 's' if tag == 'p' else 'p'
            for child in p:
                if child.tag == '{%s}%s' % (ns['fmigo'], sp):
                    traverse(child, sp)
                elif child.tag == '{%s}c' % ns['fmigo']:
                    # <c> Component. Resolve to int, change to <f>
                    fmuid = fmumap[child.text.strip()]
                    if fmuid in fmuids:
                        raise SSPException('%s is specified in ExecutionOrder more than once' % child.text)
                    fmuids.add(fmuid)
                    child.text = '%i' % fmuid
                    child.tag = '{%s}f' % ns['fmigo']
                else:
                    raise SSPException('Execution order has unknown tag <%s> inside <%s>' % (child.tag, tag))

        # root_system.structure.executionorder is <p>
        traverse(root_system.structure.executionorder, 'p');

        if len(fmuids) != len(fmumap):
            raise SSPException('You must specify every FMU if ExecutionOrder is used')

        executionorder = ['-G', etree.tostring(root_system.structure.executionorder).decode()]

    # -N only if holonomic=false
    holonomic_arg = ['-N'] if holonomic is False else []

    csvs = []
    for fmu in fmus:
        for csv in fmu.csvs:
            csvs.extend(['-V','%i,%s' % (fmu.id, escape(csv))])

    if unzipped_ssp and cleanup_zip:
        shutil.rmtree(d)

    return {
    'flatconns':        flatconns,
    'flatparams':       flatparams,
    'kinematicconns':   kinematicconns,
    'csvs':             csvs,
    'unzipped_ssp':     unzipped_ssp,
    'temp_dir':         d,
    'timestep':         root_system.structure.timestep, # None if no fmigo:MasterArguments
    'duration':         root_system.structure.duration, # None if no fmigo:MasterArguments
    'masterarguments':  root_system.structure.arguments + holonomic_arg,
    'executionorder':   executionorder,
    }

    #return flatconns, flatparams, kinematicconns, csvs, unzipped_ssp, d, \
    #        root_system.structure.timestep, root_system.structure.duration, root_system.structure.arguments

def get_fmu_platforms(fmu_path):
    zip = ZipFile(fmu_path)
    platforms = []
    for fileName in zip.namelist():
        if fileName.startswith('binaries/'):
            platforms.append(fileName.split('/')[1])
        elif fileName.startswith('binaries\\'):
            platforms.append(fileName.split('\\')[1])
    return platforms

def get_fmigo_binary(executable, install_env_name):
    fmigo_install_dir = os.environ.get(install_env_name)

    if sys.platform == "win32":
        executable += '.exe'

    if fmigo_install_dir is None:
        raise Exception("FmiGo master environment variable {} not specified!".format(install_env_name))

    if not os.path.exists(fmigo_install_dir):
        raise Exception("{}: {} does not exist".format(install_env_name, str(umit_install_dir)))

    binary_full_path = os.path.join(fmigo_install_dir, 'bin', executable)

    if not os.path.exists(binary_full_path):
        raise Exception("Unable to locate FmiGo binary {}".format(str(binary_full_path)))

    return binary_full_path

def get_fmu_server(fmu_path, executable):

    platforms = get_fmu_platforms(fmu_path)

    plaform_prefix_table = {
        'win32':  'win',
        'darwin': 'darwin',
        'linux':  'linux',
        'linux2': 'linux'
    }

    plaform_prefix = plaform_prefix_table[sys.platform]

    if not ('FMIGO_64_INSTALL_DIR' in os.environ and 'FMIGO_32_INSTALL_DIR' in os.environ):
        # Make sure FMU has same architecture as fmigo-server
        bits, linkage = platform.architecture(executable)
        if (plaform_prefix + '64') in platforms and bits != '64bit':
            raise Exception('Trying to start 64bit FMU using 32bit FmiGo. Please specify FMIGO_64_INSTALL_DIR and FMIGO_32_INSTALL_DIR environment variables')
        elif (plaform_prefix + '32') in platforms and bits != '32bit':
            raise Exception('Trying to start 32bit FMU using 64bit FmiGo. Please specify FMIGO_64_INSTALL_DIR and FMIGO_32_INSTALL_DIR environment variables')
        return executable


    if (plaform_prefix + '64') in platforms:
        fmigo_server = get_fmigo_binary(executable, 'FMIGO_64_INSTALL_DIR')
    else:
        fmigo_server = get_fmigo_binary(executable, 'FMIGO_32_INSTALL_DIR')

    return fmigo_server


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='%s: Launch an SSP with either MPI (default) or TCP (see -p)' %sys.argv[0],
        )
    parser.add_argument('-d','--dry-run',
                        help='Run without starting the simulation',
                        action='store_true')
    parser.add_argument('-p','--ports', metavar="PORT",
                        help='''
                        If set, run over TCP.
                        If zero PORTs are given (ssp-launcher.py -p -- example.ssp) then the program will try to automatically look for available TCP ports to use.
                        If specifc ports are required then the program should be given as many PORTs are there are FMUs (ssp-launcher.py -p 1024 1024 -- twofmus.ssp).
                        Giving any other number of ports is considered an error.
                        If not set, use MPI (default).
                        Use another argument or -- to separate from SSP name
                        ''',
                        default=None,
                        nargs='*',
                        type=int)
    parser.add_argument('-e','--residual-is-error',
                        help='Consider residual/unparsed XML to be an error',
                        action='store_true')
    parser.add_argument('ssp', metavar='ssp-filename',
                        help='SSP file to be launched')

    parser.add_argument('args',
                        metavar='...',
                        help='Remaining positional arguments, passed to fmigo-master',
                        nargs=argparse.REMAINDER)

    parse = parser.parse_args()

    try:
        ssp_dict = parse_ssp(parse.ssp, False, parse.residual_is_error)
    except Exception as e:
        print('Exception during SSP parsing: {}'.format(e))
        raise e

    d = ssp_dict['temp_dir']

    duration = ssp_dict['duration'] if not ssp_dict['duration'] is None else 10
    timestep = ssp_dict['timestep'] if not ssp_dict['timestep'] is None else 0.01

    # If we are working with TCP, create tcp://localhost:port for all given local hosts
    # If no ports are given then try to find some free TCP ports
    if parse.ports != None:
        ports = []
        tcpIPport = []

        #list all tcp ports that are not available
        tcpportsinuse = []
        for con in psutil.net_connections():
            tcpportsinuse.append(con.laddr[1])

        if len(parse.ports) == 0:
            # Start at 1024
            port = 1024
            while port < 65536 and len(ports) < len(fmus):
                if not port in tcpportsinuse:
                    ports.append(port)
                port += 1

            eprint('Automatically picked TCP ports ' + ', '.join([str(port) for port in ports]))

            if port >= 65536:
                eprint('Not enough available TCP ports!')
                exit(1)
        else:
            if len(fmus) > len(parse.ports):
                eprint('Error: Not given one port for each FMU, expected %d' %len(fmus))
                exit(1)
            elif len(fmus) < len(parse.ports):
                eprint('Error: Given too many ports, expected %d' %len(fmus))
                exit(1)

            ports = parse.ports
            for i in range(len(fmus)):
                if parse.ports[i] in tcpportsinuse:
                    eprint('%s: port %d already in use' %(sys.argv[0], parse.ports[i]))
                    exit(1)

        for port in ports:
            tcpIPport.append("tcp://localhost:" + str(port))

        # Everything looks OK; start servers
        for i in range(len(fmus)):
            fmigo_server = get_fmu_server(fmus[i].path, 'fmigo-server')
            subprocess.Popen([fmigo_server,'-p', str(ports[i]), fmus[i].relpath(d)], cwd=d)

        #read connections and parameters from stdin, since they can be quite many
        #stdin because we want to avoid leaving useless files on the filesystem
        args   = ['fmigo-master']
        append = tcpIPport
    else:
        #read connections and parameters from stdin, since they can be quite many
        #stdin because we want to avoid leaving useless files on the filesystem
        args   = ['mpiexec','-n', '1','fmigo-mpi']
        append = []

        for fmu in fmus:
            fmigo_mpi = get_fmu_server(fmu.path, 'fmigo-mpi')
            append += [':','-n', '1', '-wdir', d, fmigo_mpi]
            append += [fmu.relpath(d)]

    args += ['-t',str(duration),'-d',str(timestep)] + ['-a','-']
    args += append

    pipelist = \
      ssp_dict['flatconns'] +\
      ssp_dict['flatparams'] +\
      ssp_dict['kinematicconns'] +\
      ssp_dict['csvs'] +\
      ssp_dict['masterarguments'] +\
      ssp_dict['executionorder'] +\
      parse.args
    pipeinput = " ".join(['"%s"' % s.replace('"','\\"') for s in pipelist])
    eprint("(cd %s && %s <<< '%s')" % (d, " ".join(args), pipeinput))

    if parse.dry_run:
        ret = 0
    else:
        #pipe arguments to master, leave stdout and stderr alone
        p = subprocess.Popen(args, stdin=subprocess.PIPE)
        p.communicate(input=pipeinput.encode('utf-8'))
        ret = p.returncode  #ret can be None

    if ret == 0:
        if ssp_dict['unzipped_ssp']:
            shutil.rmtree(d)
    else:
        eprint('An error occured (returncode = ' + str(ret) + '). Check ' + d)

    exit(ret)
