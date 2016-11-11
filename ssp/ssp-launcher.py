#!/usr/bin/python
import tempfile
import zipfile
import sys
import os
import os.path
import glob
import xml.etree.ElementTree as ET
import subprocess
import shutil

if len(sys.argv) < 2:
    #TODO: we probably want to know which of TCP and MPI is wanted
    print('USAGE: %s ssp-file' % sys.argv[0])
    exit(1)

RESOURCE_DIR='resources'
SSD_NAME='SystemStructure.ssd'
CAUSALITY='causality'
MODELDESCRIPTION='modelDescription.xml'

ns = {'ssd': 'http://www.pmsf.net/xsd/SystemStructureDescriptionDraft'}
d = tempfile.mkdtemp(prefix='ssp')
print(d)

fmus = []
systems = []

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
            if fmu.connectors[key[1]].attrib[sys.kindKey] == target_kind:
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
                print('SignalDictionary "%" has no output signal connected to it' % dictionary_name)
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

def find_elements(s, first, second):
    return s.find(first,  ns).findall(second,  ns) if s.find(first,  ns)  != None else []

class FMU:
    def __init__(self, name, path, connectors, system):
        self.name = name
        self.path = path
        self.connectors = {}

        for conn in connectors:
            self.connectors[conn.attrib['name']] = conn

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
            if conn.attrib[self.system.kindKey] == 'input':
                fmu, fmu_variable = traverse(self.system, (self.name, name), 'output', True)
                if fmu != None:
                    unit = conn[0].attrib['unit'] if len(conn) > 0 else None
                    key = (fmu.id, fmu_variable)
                    value = (self.id, name)
                    add_multimap_value(connectionmultimap, key, value)

class SystemStructure:
    def __init__(self, root):
        units  = root.find('ssd:Units',  ns).findall('ssd:Unit',  ns) if root.find('ssd:Units',  ns)  != None else []

        # Units keyed on name. Like {name: {'units': (units,), 'factor': 1, 'offset': 0}}
        self.unitsbyname = {}

        # Units keyed on units. Like {(units,): {name: {'factor': 1, 'offset': 0}}}
        self.unitsbyunits = {}

        for u in units:
            name = u.attrib['name']
            bu = u.find('ssd:BaseUnit', ns)
            key = tuple([int(bu.attrib[n]) if n in bu.attrib else 0 for n in ['kg','m','s','A','K','mol','cd','rad']])

            #want factor and offset
            factor = float(bu.attrib['factor']) if 'factor' in bu.attrib else 1
            offset = float(bu.attrib['offset']) if 'offset' in bu.attrib else 0

            #put in the right spots
            self.unitsbyname[name] = {'units': key, 'factor': factor, 'offset': offset}

            if not key in self.unitsbyunits:
                self.unitsbyunits[key] = {}

            self.unitsbyunits[key][name] = {'factor': factor, 'offset': offset}

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
        #print 'parse_ssd: ' + path
        tree = ET.parse(path)
        #print tree
        #print tree.getroot()
        
        #tree.register_namespace('ssd', 'http://www.pmsf.net/xsd/SystemStructureDescriptionDraft')
        if 'version' in tree.getroot().attrib:
            version = tree.getroot().attrib['version']
        else:
            version = 'Draft20150721'
            printf('WARNING: version not set in root, assuming ' + self.version)

        structure = SystemStructure(tree.getroot())

        sys = tree.getroot().findall('ssd:System', ns)
        if len(sys) != 1:
            print( 'Must have exactly one System')
            exit(1)
        s = sys[0]
        return cls(s, version, parent, structure)

    @classmethod
    def fromxml(cls, s, version, parent):
        return cls(s, version, parent, parent.structure)

    def __init__(self, s, version, parent, structure):
        global systems
        systems.append(self)

        self.version = version
        self.name = s.attrib['name']
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

        for conn in connectors:
            if conn.attrib[self.kindKey] == 'input':
                self.inputs[conn.attrib['name']] = conn
            elif conn.attrib[self.kindKey] == 'output':
                self.outputs[conn.attrib['name']] = conn
            elif conn.attrib[self.kindKey] in ['parameter', 'calculatedParameter', 'inout']:
                print('WARNING: Unimplemented connector kind: ' + conn.attrib[self.kindKey])
            else:
                print('Unknown connector kind: ' + conn.attrib[self.kindKey])
                exit(1)

        # Bi-directional
        self.connections = {}
        for conn in connections:
            a = (conn.attrib['startElement'] if 'startElement' in conn.attrib else '',
                 conn.attrib['startConnector'])
            b = (conn.attrib['endElement']   if 'endElement'   in conn.attrib else '',
                 conn.attrib['endConnector'])
            self.connections[a] = b
            self.connections[b] = a

        #print self.connections

        self.subsystems = {}
        self.fmus     = {}
        for comp in components:
            #print comp
            t = comp.attrib['type']
            #print t

            if t == 'application/x-ssp-package':
                d2 = os.path.join(d, os.path.splitext(comp.attrib['source'])[0])
                child = System.fromfile(d2, SSD_NAME, self)
                #print 'Added subsystem ' + child.name
                self.subsystems[comp.attrib['name']] = child
            elif t == 'application/x-fmu-sharedlibrary':
                pass #print 
                self.fmus[comp.attrib['name']] = FMU(
                    comp.attrib['name'],
                    os.path.join(d, comp.attrib['source']),
                    comp.find('ssd:Connectors', ns).findall('ssd:Connector', ns),
                    self,
                )
            else:
                print('unknown type: ' + t)
                exit(1)

        self.signaldicts = {}
        for sigdict in signaldicts:
            des = {}
            for de in sigdict.findall('ssd:DictionaryEntry', ns):
                #NOTE: de[0].tag is the type of the entry, but we don't really need it
                #the second and third entry in the tuple are the FMU and connector on which the signal source is available
                des[de.attrib['name']] = (de[0].attrib['unit'], None, '')
            self.signaldicts[sigdict.attrib['name']] = des
        #print('SignalDictionaries: ' + str(self.signaldicts))

        self.sigdictrefs = {}
        for sdr in sigdictrefs:
            drs = {}
            conns = sdr.find('ssd:Connectors', ns).findall('ssd:Connector') if sdr.find('ssd:Connectors', ns) != None else []
            for conn in conns:
                if conn.attrib['kind'] != 'inout':
                    print('Only inout connectors supports in SignalDictionaryReferences for now')
                    exit(1)
                drs[conn.attrib['name']] = conn[0].attrib['unit']
            self.sigdictrefs[sdr.attrib['name']] = (sdr.attrib['dictionary'], drs)
        #print('SignalDictionaryReference: ' + str(self.sigdictrefs))

        for subsystem in subsystems:
            ss = System.fromxml(subsystem, self.version, self)
            self.subsystems[subsystem.attrib['name']] = ss

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
        return self.parent.get_name() + '::' + self.name if self.parent != None else self.name

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

unzip_ssp(d, sys.argv[1])
root = System.fromfile(d, SSD_NAME)

root.resolve_dictionary_inputs()

# Figure out connections, parse modelDescriptions
connectionmultimap = {} # Multimap of outputs to inputs
mds = []
for fmu in fmus:
    fmu.connect(connectionmultimap)

    # Parse modelDescription, turn variable list into map
    tree = ET.parse(os.path.join(os.path.splitext(fmu.path)[0], MODELDESCRIPTION))

    svs = {}
    for sv in tree.getroot().find('ModelVariables').findall('ScalarVariable'):
        if sv.attrib['name'] in svs:
            print(fmu.path + ' contains multiple variables named "' + sc.attrib['name'] + '"!')
            exit(1)

        if not CAUSALITY in sv.attrib:
            # Not an input or output. Probably a parameter
            continue

        t = ''
        if sv.find('Real')      != None: t = 'r'
        elif sv.find('Integer') != None: t = 'i'
        elif sv.find('Boolean') != None: t = 'b'
        elif sv.find('Enum')    != None: t = 'e'
        elif sv.find('String')  != None: t = 's'
        else:
            print(fmu.path + ' variable "' + sv.attrib['name'] + '" has unknown type')
            exit(1)

        svs[sv.attrib['name']] = {
            'vr': int(sv.attrib['valueReference']),
            CAUSALITY: sv.attrib[CAUSALITY],
            'type': t,
        }
    mds.append(svs)

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

servers = []
for fmu in fmus:
    servers.extend([':','-np','1','fmi-mpi-server',fmu.path])

#read connections and parameters from stdin, since they can be quite many
#stdin because we want to avoid leaving useless files on the filesystem
args = ['mpiexec','-np','1','fmi-mpi-master','-t','9.9','-d','0.1','-a','-'] + servers
print(" ".join(args) + " <<< " + '"' + " ".join(flatconns) + '"')

#pipe arguments to master, leave stdout and stderr alone
p = subprocess.Popen(args, stdin=subprocess.PIPE)
p.communicate(input=" ".join(flatconns).encode('utf-8'))
ret = p.returncode  #ret can be None

if ret == 0:
    shutil.rmtree(d)
else:
    print('An error occured (returncode = ' + str(ret) + '). Check ' + d)

exit(ret)
