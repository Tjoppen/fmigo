#!/usr/bin/python
import tempfile
import zipfile
import sys
import os
import os.path
import glob
from lxml import etree
import subprocess
import shutil

if len(sys.argv) < 2:
    #TODO: we probably want to know which of TCP and MPI is wanted
    print('USAGE: %s [--dry-run] ssp-file' % sys.argv[0])
    exit(1)

RESOURCE_DIR='resources'
SSD_NAME='SystemStructure.ssd'
CAUSALITY='causality'
MODELDESCRIPTION='modelDescription.xml'

ns = {
    'ssd': 'http://www.pmsf.net/xsd/SystemStructureDescriptionDraft',
    'ssv': 'http://www.pmsf.net/xsd/SystemStructureParameterValuesDraft',
    'ssm': 'http://www.pmsf.net/xsd/SystemStructureParameterMappingDraft',
    'umit':'http://umit.math.umu.se/UMITSSD',
}
d = tempfile.mkdtemp(prefix='ssp')
print(d)

fmus = []
systems = []
parameters = {}

schema_names = {
    'SSD': 'SystemStructureDescription.xsd',
    'SSM': 'SystemStructureParameterMapping.xsd',
    'SSV': 'SystemStructureParameterValues.xsd',
    'UMIT':'UMITSSD.xsd',
}
schemas = {}

for type in schema_names:
    # Expect schema to be located next to this script
    try:
        schema_path = os.path.join(os.path.dirname(__file__), schema_names[type])
        schemas[type] = etree.XMLSchema(etree.parse(schema_path))
    except Exception as e:
        print(e)
        print('Cannot open/parse %s - no %s validation performed' % (schema_path, type))

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
            print( str((key,value)) + ' already exists in ' + str(multimap))
            exit(1)
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
    #print('%s.%s (unit=%s)' % (self.get_name(), conn.attrib['name'], str(unit)))

    while True:
        #print 'key = ' + str(key) + ', sys = ' + sys.name
        if not key in sys.connections:
            print( 'No connection %s in system %s' % (str(key), sys.name))
            return None, None
            # exit(1)

        #print('sys.sigdictrefs: ' + str(sys.sigdictrefs))
        key = sys.connections[key]
        if key[0] == '':
            # Connection leads to parent node
            if sys.parent == None:
                # Can't go any further - issue a warning and move on
                print ('WARNING: ' + str(startkey) + ' takes data from ' +
                       key[1] + ' of root system, which no output connects to - skipping')
                return None, None

            #print 'up'
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
            #print 'down'
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
                print('SignalDictionary "%s" has no output signal connected to it' % dictionary_name)
                exit(1)

            return de[1], de[2]
        else:
            print('Key %s of kind %s not found in system %s' % (str(key), target_kind, sys.name))
            print (sys.fmus)
            print (sys.subsystems)
            exit(1)

        ttl -= 1
        if ttl <= 0:
            print ('SSP contains a loop!')
            exit(1)

def get_attrib(s, name, default=None):
    if name in s.attrib:
        ret = s.attrib[name]
        del s.attrib[name]
        return ret
    else:
        if default == None:
            print('get_attrib(): missing required attribute ' + name)
            exit(1)
        return default

def remove_if_empty(parent, node):
    #remove if all attributes and subnodes have been dealt with
    if node != None and len(node.attrib) == 0 and len(node) == 0:
        parent.remove(node)

def find_elements(s, first, second):
    a = s.find(first,  ns)
    return a, a.findall(second,  ns) if a != None else []

def parse_parameter_bindings(path, baseprefix, parameterbindings):
    for pb in parameterbindings[1]:
        prefix = baseprefix + get_attrib(pb, 'prefix', '')

        #print('prefix: '+prefix)
        pvs = (pb, pb.findall('ssd:ParameterValues', ns))

        x_ssp_parameter_set = 'application/x-ssp-parameter-set'
        t = get_attrib(pb, 'type', x_ssp_parameter_set)
        if t != x_ssp_parameter_set:
            print('Expected ' + x_ssp_parameter_set + ', got ' + t)
            exit(1)

        if 'source' in pb.attrib:
            if len(pvs[1]) != 0:
                print('ParameterBindings must have source or ParameterValues, not both')
                exit(1)

            #read from file
            #TODO: print unhandled XML in ParameterSet
            tree = parse_and_validate('SSV', os.path.join(path, get_attrib(pb, 'source')))
            pvs = find_elements(tree.getroot(), 'ssv:Parameters', 'ssv:Parameter')
            #print('Parsed %i params' % len(pvs))
        else:
            if len(pvs[1]) == 0:
                print('ParameterBindings missing both source and ParameterValues')
                exit(1)

            #don't know whether it's ParameterValues -> Parameters -> Parameter or just ParameterValues -> Parameter
            #the spec doesn't help
            print('Parsing ParameterValues is TODO, for lack of examples or complete schema as of 2016-11-11')
            exit(1)

        for pv in pvs[1]:
            r = pv.find('ssv:Real', ns)
            i = pv.find('ssv:Integer', ns)
            b = pv.find('ssv:Boolean', ns)
            s = pv.find('ssv:String', ns)
            e = pv.find('ssv:Enumeration', ns)
            name = prefix+pv.attrib['name']

            if r != None:
                if 'unit' in r.attrib:
                    print('Not dealing with parameters with units yet')
                    exit(1)

                parameters[name] = {
                    'type': 'r',
                    'value': float(r.attrib['value']),
                }
            elif i != None:
                parameters[name] = {
                    'type': 'i',
                    'value': int(i.attrib['value']),
                }
            elif b != None:
                parameters[name] = {
                    'type': 'b',
                    'value': b.attrib['value'], #keep booleans as-is
                }
            elif s != None:
                parameters[name] = {
                    'type': 's',
                    'value': s.attrib['value'],
                }
            elif e != None:
                print('Enumerations not supported')
                exit(1)
            else:
                print('Unsupported parameter type: ' + str(pv[0].tag))
                exit(1)

            remove_if_empty(pvs[0], pv)

        #deal with any ssm
        pm = pb.find('ssd:ParameterMapping', ns)
        if pm != None:
            if not 'source' in pm.attrib:
                print('"source" not set in ParameterMapping. In-line PMs not supported yet')
                exit(1)

            x_ssp_parameter_mapping = 'application/x-ssp-parameter-mapping'
            t = get_attrib(pm, 'type', x_ssp_parameter_mapping)
            if t != x_ssp_parameter_mapping:
                print('Expected ' + x_ssp_parameter_mapping + ', got ' + t)
                exit(1)

            #TODO: print unhandled XML in ParameterMapping too
            tree = parse_and_validate('SSM', os.path.join(path, get_attrib(pm, 'source')))
            mes = tree.getroot().findall('ssm:MappingEntry', ns)

            for me in mes:
                source = prefix+me.attrib['source']
                target = prefix+me.attrib['target']

                p = parameters[source]
                del parameters[source]

                lt = me.find('ssm:LinearTransformation', ns)
                if lt != None:
                    factor = float(lt.attrib['factor'])
                    offset = float(lt.attrib['offset'])

                    if p['type'] != 'r':
                        print('LinearTransformation only supported in Real')
                        exit(1)

                    p['value'] = p['value'] * factor + offset
                elif len(me) > 0:
                    print("Found MappingEntry with sub-element which isn't LinearTransformation, which is not yet supported")
                    exit(1)

                parameters[target] = p

            remove_if_empty(pb, pm)

        remove_if_empty(parameterbindings[0], pb)

class FMU:
    def __init__(self, name, path, connectors, system):
        self.name = name
        self.path = path
        self.connectors = {}

        for conn in connectors[1]:
            name = get_attrib(conn, 'name')
            kind = get_attrib(conn, 'kind')
            unit = None

            if len(conn) > 0:
                if len(conn) > 1:
                    print('More then one sub-element of Connector - bailing out')
                    exit(1)
                unit = get_attrib(conn[0], 'unit', False)
                if unit == False:
                    unit = None
                remove_if_empty(conn, conn[0])

            remove_if_empty(connectors[0], conn)

            self.connectors[name] = {
                'kind': kind,
                'unit': unit,
            }

        self.system = system

        global fmus
        self.id = len(fmus)
        fmus.append(self)
        #print '%i: %s %s, %i connectors' % (self.id, self.name, self.path, len(connectors))

    def get_name(self):
        return self.system.get_name() + '.' + self.name

    def connect(self, connectionmultimap):
        '''
        Adds connections for this FMU to other FMUs in the System tree to the given connections multimap
        '''
        for name,conn in self.connectors.iteritems():
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

        #not sure what to use schemaLocation for, or if we should even require it
        # self.schemaLocation = get_attrib(root, '{http://www.w3.org/2001/XMLSchema-instance}schemaLocation')

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

        #print('unitsbyname: '+str(self.unitsbyname))
        #print('unitsbyunits: '+str(self.unitsbyunits))

class System:
    '''
    A System is a tree of Systems and FMUs
    In each System there are Connections between 
    '''

    @classmethod
    def fromfile(cls, d, filename, parent=None):
        path = os.path.join(d, filename)

        #parse XML, delete all attribs and element we know how to deal with
        #print residual, indicating things we don't yet support
        tree = parse_and_validate('SSD', path)

        # Remove comments, which would otherwise result in residual XML
        for c in tree.xpath('//comment()'):
            c.getparent().remove(c)

        if 'version' in tree.getroot().attrib:
            version = get_attrib(tree.getroot(), 'version')
        else:
            version = 'Draft20150721'
            print('WARNING: version not set in root, assuming ' + self.version)

        structure = SystemStructure(tree.getroot())

        sys = tree.getroot().findall('ssd:System', ns)
        if len(sys) != 1:
            print( 'Must have exactly one System')
            exit(1)
        s = sys[0]
        ret = cls(d, s, version, parent, structure)
        remove_if_empty(tree.getroot(), s)

        if len(tree.getroot()) > 0 or len(tree.getroot().attrib) > 0:
            print('WARNING: Residual XML: '+etree.tostring(tree.getroot()))

        return ret

    @classmethod
    def fromxml(cls, d, s, version, parent):
        return cls(d, s, version, parent, parent.structure)

    def __init__(self, d, s, version, parent, structure):
        global systems
        systems.append(self)

        self.d = d
        self.version = version
        self.name = get_attrib(s, 'name')
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
        annotations = find_elements(s, 'ssd:Annotations',           'ssd:Annotation')
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
                print('WARNING: Unimplemented connector kind: ' + kind)
            else:
                print('Unknown connector kind: ' + kind)
                exit(1)

            if len(conn) > 0:
                print("WARNING: Connection %s in system %s has type information, which isn't handled yet" % (name, self.name))
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

        #print self.connections

        self.subsystems = {}
        self.fmus     = {}
        for comp in components[1]:
            #print comp
            t    = get_attrib(comp, 'type')
            name = get_attrib(comp, 'name')
            #print t

            if t == 'application/x-ssp-package':
                d2 = os.path.join(d, os.path.splitext(get_attrib(comp, 'source'))[0])
                child = System.fromfile(d2, SSD_NAME, self)
                #print 'Added subsystem ' + child.name
                self.subsystems[name] = child
            elif t == 'application/x-fmu-sharedlibrary':
                source = os.path.join(d, get_attrib(comp, 'source'))
                connectors = find_elements(comp, 'ssd:Connectors', 'ssd:Connector')
                self.fmus[name] = FMU(
                    name,
                    source,
                    connectors,
                    self,
                )
                remove_if_empty(comp, connectors[0])
            else:
                print('unknown type: ' + t)
                exit(1)

            #parse parameters after subsystems so their values get overriden properly
            cparams = find_elements(comp, 'ssd:ParameterBindings', 'ssd:ParameterBinding')
            parse_parameter_bindings(self.d, self.get_name() + '.' + name + '.', cparams)
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
        #print('SignalDictionaries: ' + str(self.signaldicts))

        self.sigdictrefs = {}
        for sdr in sigdictrefs[1]:
            drs = {}
            conns = find_elements(sdr, 'ssd:Connectors', 'ssd:Connector')
            for conn in conns[1]:
                if get_attrib(conn, 'kind') != 'inout':
                    print('Only inout connectors supports in SignalDictionaryReferences for now')
                    exit(1)
                drs[get_attrib(conn, 'name')] = get_attrib(conn[0], 'unit')
                remove_if_empty(conn, conn[0])
                remove_if_empty(conns[0], conn)
            self.sigdictrefs[get_attrib(sdr, 'name')] = (get_attrib(sdr, 'dictionary'), drs)
            remove_if_empty(sdr, conns[0])
            remove_if_empty(sigdictrefs[0], sdr)
        #print('SignalDictionaryReference: ' + str(self.sigdictrefs))

        for annotation in annotations[1]:
            type = get_attrib(annotation, 'type')
            if type == 'se.umu.math.umit.ssp.kinematicconstraints':
                if 'UMIT' in schemas:
                    schemas['UMIT'].assertValid(annotation[0])

                for shaft in find_elements(annotation, 'umit:KinematicConstraints', 'umit:ShaftConstraint')[1]:
                    get_attrib(shaft, 'element1')
                    get_attrib(shaft, 'element2')
                    get_attrib(shaft, 'angle1', '')
                    get_attrib(shaft, 'angle2', '')
                    get_attrib(shaft, 'angularVelocity1')
                    get_attrib(shaft, 'angularVelocity2')
                    get_attrib(shaft, 'angularAcceleration1')
                    get_attrib(shaft, 'angularAcceleration2')
                    get_attrib(shaft, 'torque1')
                    get_attrib(shaft, 'torque2')
                    remove_if_empty(annotation[0], shaft)

                remove_if_empty(annotation, annotation[0])
            else:
                print('WARNING: Found unknown Annotation of type "%s"' % type)
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
        parse_parameter_bindings(self.d, self.get_name() + '.', params)
        remove_if_empty(s, params[0])

    def find_signal_dictionary(self, dictionary_name):
        sys = self
        while True:
            if dictionary_name in sys.signaldicts:
                #print('found the actual dict in system ' + sys.name)
                return sys.signaldicts[dictionary_name]

            sys = sys.parent

            if sys == None:
                #we're assuming that dictionaries exist somewhere straight up in the hierarchy, never in a sibling/child system
                #this may change if we find a counterexample in the wild
                print('Failed to find SignalDictionary "%s" starting at System "%s"' % (dictionary_name, self.name))
                exit(1)

    def resolve_dictionary_inputs(self):
        #for each signal dictionary, find where it is referenced and if that reference is connected to by an output
        #this because multiple inputs can connect to a dictionary, but only one output can be connected
        for name,(dictionary_name,drs) in self.sigdictrefs.iteritems():
            #print('name: ' + name)
            #print('dict: ' + dictionary)
            #print('drs: ' + str(drs))

            for key,unit in drs.iteritems():
                #print('traversing from ' + str((name, key)))
                fmu, fmu_variable = traverse(self, (name, key), 'output', False)

                if fmu != None:
                    #found it!
                    #now find the actual signal dictionary so everyone can find the output
                    #print(str((dictionary_name, key)) + ' <-- ' + str((fmu.id,fmu_variable)))

                    signaldict = self.find_signal_dictionary(dictionary_name)
                    de = signaldict[key]

                    if de[1] != None:
                        print('Dictionary %s has more than one output connected to %s' % (dictionary_name, key))
                        exit(1)

                    #put the FMU and variable name in the dict
                    signaldict[key] = (de[0], fmu, fmu_variable)

        for name,subsystem in self.subsystems.iteritems():
            subsystem.resolve_dictionary_inputs()

    def get_name(self):
        return self.parent.get_name() + '.' + self.name if self.parent != None else self.name

def unzip_ssp(dest_dir, ssp_filename):
    #print 'unzip_ssp: ' + dest_dir + ' ' + ssp_filename
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


def parse_ssp(ssp_path, cleanup_zip = True):

    global fmus, system, parameters, SSD_NAME, d
    fmus = []
    systems = []
    parameters = {}

    file_ext = os.path.splitext(ssp_path)[1].lower()

    # Allow custom named ssd files
    if file_ext == '.ssd':
       SSD_NAME = os.path.basename(ssp_path) 

    # Check if we run master directly from an SSD XML file instead of an SSP zip archive
    if os.path.basename(ssp_path) == SSD_NAME:
        d = os.path.dirname(ssp_path)
        unzipped_ssp = False
    else:
        unzip_ssp(d, ssp_path)
        unzipped_ssp = True

    root = System.fromfile(d, SSD_NAME)

    root.resolve_dictionary_inputs()

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
        # print(md_data)
        root = etree.XML(md_data)

        svs = {}
        for sv in root.find('ModelVariables').findall('ScalarVariable'):
            name = sv.attrib['name']
            if name in svs:
                print(fmu.path + ' contains multiple variables named "' + name + '"!')
                exit(1)

            causality = ''

            if not CAUSALITY in sv.attrib:
                if 'variability' in sv.attrib and sv.attrib['variability'] == 'parameter':
                    #this happens with SampleSystemSubSystemDictionary.ssp
                    print(('WARNING: Found variable %s without causality and variability="parameter". ' % name)+
                          'This violates the spec. Treating as a parameter')
                    causality = 'parameter'
                else:
                    print('WARNING: FMU %s has variable %s without causality - ignoring' % (fmu.get_name(), name))
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
                print(fmu.path + ' variable "' + name + '" has unknown type')
                exit(1)

            svs[name] = {
                'vr': int(sv.attrib['valueReference']),
                CAUSALITY: causality,
                'type': t,
            }
        mds.append(svs)

    flatparams = []
    for key,value in parameters.iteritems():
        parts = key.split('.')
        fmuname = '.'.join(parts[0:-1])
        paramname = parts[-1]
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
                            str(value['value']).replace('\\','\\\\').replace(':','\\:').replace(',','\\,')
                        )
                    ])
                else:
                    print('WARNING: FMU %s, tried to set variable %s which is neither an input nor a parameter' % (fmuname, paramname))
            else:
                print('WARNING: FMU %s has no variable called %s' % (fmuname, paramname))
        else:
            print('WARNING: No FMU called %s for parameter %s' % (fmuname, paramname))

    #print connections
    #print mds

    # Build command line
    flatconns = []
    for key in connectionmultimap.keys():
        fr = key
        to1 = connectionmultimap[key]  
        for to in to1:
            #print str((fr,to)) + ' vs ' + str(mds[fr[0]])
            f = mds[fr[0]]
            fv = f[fr[1]]
            t = mds[to[0]]
            tv = t[to[1]]

            connstr = '%s,%i,%i,%s,%i,%i' % (fv['type'], fr[0], fv['vr'], tv['type'], to[0], tv['vr'])
            flatconns.extend(['-c', connstr])

    if unzipped_ssp and cleanup_zip:
        shutil.rmtree(d)

    return flatconns, flatparams, unzipped_ssp, d

if __name__ == '__main__':

    dry_run = sys.argv[1] == '--dry-run'

    if dry_run and len(sys.argv) < 3:
        print('ERROR: missing ssp-name')
        exit(1)

    flatconns, flatparams, unzipped_ssp, d = parse_ssp(sys.argv[-1], False)

    #read connections and parameters from stdin, since they can be quite many
    #stdin because we want to avoid leaving useless files on the filesystem
    args = ['mpiexec','-np',str(len(fmus)+1),'fmigo-mpi','-t','9.9','-d','0.1','-a','-'] + [fmu.path for fmu in fmus]
    print(" ".join(args) + " <<< " + '"' + " ".join(flatconns+flatparams) + '"')

    if dry_run:
        ret = 0
    else:
        #pipe arguments to master, leave stdout and stderr alone
        p = subprocess.Popen(args, stdin=subprocess.PIPE)
        p.communicate(input=" ".join(flatconns).encode('utf-8'))
        ret = p.returncode  #ret can be None

    if ret == 0:
        if unzipped_ssp:
            shutil.rmtree(d)
    else:
        print('An error occured (returncode = ' + str(ret) + '). Check ' + d)

    exit(ret)
