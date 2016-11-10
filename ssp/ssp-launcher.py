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

class FMU:
    def __init__(self, name, path, connectors, system):
        self.name = name
        self.path = path
        self.connectors = connectors
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
        for conn in self.connectors:
            # Look at inputs, work back toward outputs
            if conn.attrib[self.system.kindKey] == 'input':
                global fmus, systems
                ttl = len(fmus) + len(systems)
                sys = self.system
                key = (self.name, conn.attrib['name'])
                #print self.get_name() + '.' + conn.attrib['name']

                while True:
                    #print 'key = ' + str(key) + ', sys = ' + sys.name
                    if not key in sys.connections:
                        print( 'No connection %s in system %s' % (str(key), self.system.name))
                        break
                        # exit(1)

                    key = sys.connections[key]
                    if key[0] == '':
                        # Connection leads to parent node
                        if sys.parent == None:
                            # Can't go any further - issue a warning and move on
                            print ('WARNING: ' + self.get_name() + '.' + conn.attrib['name'] + ' takes data from ' +
                                   key[1] + ' of root system, which no output connects to - skipping')
                            break

                        #print 'up'
                        key = (sys.name, key[1])
                        sys = sys.parent
                    elif key[0] in sys.fmus:
                        fmu = sys.fmus[key[0]]
                        value = (self.id, conn.attrib['name'])
                        key = (fmu.id, key[1])
                        add_multimap_value(connectionmultimap, key, value)
                        break
                    elif key[0] in sys.subsystems:
                        #print 'down'
                        sys = sys.subsystems[key[0]]
                        key = ('', key[1])
                    else:
                        print('Key %s not found in system %s' % (str(key), sys.name))
                        print (sys.fmus)
                        print (sys.subsystems)
                        exit(1)

                    ttl -= 1
                    if ttl <= 0:
                        print ('SSP contains a loop!')
                        exit(1)

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
        connectors  = s.find('ssd:Connectors',  ns).findall('ssd:Connector',  ns) if s.find('ssd:Connectors',  ns)  != None else []
        connections = s.find('ssd:Connections', ns).findall('ssd:Connection', ns) if s.find('ssd:Connections',  ns) != None else []
        components  = s.find('ssd:Elements',    ns).findall('ssd:Component',  ns) if s.find('ssd:Elements',  ns)    != None else []
        subsystems  = s.find('ssd:Elements',    ns).findall('ssd:System',     ns) if s.find('ssd:Elements',  ns)    != None else []

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

        for subsystem in subsystems:
            ss = System.fromxml(subsystem, self.version, self)
            self.subsystems[subsystem.attrib['name']] = ss

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
