"""

This provides wrappers for FMUs so they are easy to assemble in a short
script, as well as support writing data in HDF5 format using pytable, as
well as a library of FMUs ready to be assembled.

The structure of the file starts with the FMU utilities, goes on to define
modules and simulations, and ends with a list of FMUs. 

The end result is a simulation that is easy to specify in python, and self
described data stored in HDF5 files which can be read with either python or
matlab.  This does not work with octave however. 
"""

import tables as tb
import re
import os
import platform
import numpy as np
from collections import OrderedDict
import datetime as dtime        
import time                     # this might be redundant
import sys
import subprocess
import timeit
import shutil as shutil
import tempfile as tempfile
from collections import Counter 



def patch_string(s):
    special = "[=~!@\$%\^&\.\(\)-\+\\\|\]\}\[\{';:/\?<>]"
    return re.sub(special, "_", s)


       
def platform_path():
    if 'linux' in sys.platform:
        return 'dymola_models/Linux/'
    elif 'win' in sys.platform:
        return 'dymola_models/Windows/'
    else:
        print('Unsupported platform ' + sys.platform)
        sys.exit(1)

class module:
    """ Base class for FMUs Everything and the kitchen sink. 
    This contains enough info to hide all the complexity of constructing
    the command line for the execution environment. 

    Also, there is information here that goes into the HDF5 datafile. 
    That information is not mandatory but useful.   No effort is made at
    this time to parse modelDescription.xml files to gather UUID and other
    good info. 
    """ 
    def __init__(self, filename, name, connectors=None, inputs=None,
                 outputs=None, parameters=None, host="tcp://localhost",
                 prefixcmd=[],
                 debug_level=0, port = None, description="",
                 aliases={}, attributes={}
    ):
        self.id = None          # for fmigo's command line 
        self.name=name
        self.description=description
        self.filename = filename   # the fmu itself
        self.connectors=connectors #kinematic couplings: variable lists
        self.inputs  = inputs      # input ports
        self.outputs = outputs     # outs
        if hasattr(self, "parameters") and parameters:
            self.parameters.update(parameters)
        else:
            self.parameters=parameters

        self.id = None    # unique id assigned from simulation
        self.port = port  # will be assigned by simulation if on local host
        self.host = host  # which computer this runs on
        self.prefixcmd = prefixcmd # special extra commands like optirun or
                                   # xterm etc.
        self.debug_level=debug_level 
        self.attributes = attributes
        self.aliases = aliases
        

    def make_aliases(self):
        """ this is to make identical names for corresponding variables"""
        if hasattr(self, "inputs") and self.inputs:
            self.aliases.update(self.inputs)
        if hasattr(self, "outputs") and self.outputs and type(self.outputs)==type({}):
            self.aliases.update(self.outputs)
        if hasattr(self, "connectors") and self.connectors:
            for k,v in self.connectors.items():
                self.aliases["phi_"+k] = v[0]
                self.aliases["w_"  +k] = v[1]
                self.aliases["a_"  +k] = v[2]
                self.aliases["tau_"+k] = v[3]

        # now construct the reverse map
        self.raliases = {}
        for k,v in self.aliases.items():
            self.raliases[v] = k
        
    # construct command line for parameters
    def make_pars(self):
        s=""
        if self.parameters:
            for k in self.parameters:
                s +=":"+str(self.id)+","+k+","+self.parameters[k]
            return ['-p', s[1:]]
        else:
            return []
    
    def uri(self, mode="tcp"):
        if mode == "tcp":
            return self.host+":"+str(self.port)
        elif mode=="mpi":
            return self.filename
        else:
            print("Only mpi and tcp modes are available but %s was requested." %mode)
            sys.exit(-1)
 

class simulation:
    """ Here is all the runtime support for both TCP and MPI.  Modules are
    initialized and then sorted for the connections can be delivered to the
    master directly from the dictionary of connections"""
    def __init__(self,   fmus,
                 couplings=(),
                 signals=(),
                 cleanup=None,
                 annotations=(),
                 START_PORT=3000,
                 extra_args=[],
                 debug_level=0 ,
                 name="",
                 variant="",
                 description="some simulation"
    ):
        self.fmus = self.remove_name_clashes(fmus)
        self.couplings = couplings # maybe this doesn't need to be stored
        self.signals   = signals   # same
        self.cleanup = cleanup
        self.cmd_pre    = []
        self.cmd        = []
        self.cmd_post   = []
        self.cmd_pars = []
        self.cmd_couplings = []
        self.cmd_signals     = []
        self.ports = []
        self.processes = []     # for when we use TCP
        self.port = START_PORT
        self.uris = None
        self.extra_args = extra_args
        self.debug_level = debug_level
        self.is_kinematic = False
        self.wall_time = 0
        self.name = name
        self.description = description
        self.annotations = annotations
        self.variant = variant

    ##  Assign an ordering to the FMUs from 0
    ##  This id is relevant only to fmigo-{master|mpi}
    ##  We also take care of the parameters here --- could be done when
    ##  launching simulation
    def init_fmus(self, comm_step):
        id = 0
        self.cmd_pars = []
        for k,f in self.fmus.items():
            f.id = id
            if hasattr(f, "setup"):
                f.setup(comm_step)
            id  += 1
            s=""
            if f.parameters:
                for k in f.parameters:
                    s +=":"+str(f.id)+","+k+","+f.parameters[k]
                self.cmd_pars += ['-p', s[1:]]
            f.make_aliases()
        return 

    # assuming that no work has been done so far, one might have simply a
    # dictionary of pairs and connector tags which then generates the
    # command line bits.  This really belongs in this class. 
    def init_couplings(self):
        c = []
        self.is_kinematic = True if self.couplings else False
        for i in self.couplings:
            c += ["-C", "shaft,%d,%d,%s,%s" %
                  (self.fmus[i[0]].id, self.fmus[i[2]].id, 
                   ",".join(self.fmus[i[0]].connectors[i[1]]),
                   ",".join(self.fmus[i[2]].connectors[i[3]]))]
        self.cmd_couplings = c

    def init_signals(self):
        c = []
        if self.signals:
            for i in self.signals:
                c+= ["-c", "%d,%s,%d,%s" % (self.fmus[i[0]].id,
                                            self.fmus[i[0]].outputs[i[1]], 
                                            self.fmus[i[2]].id,
                                            self.fmus[i[2]].inputs[i[3]])]
            self.cmd_signals = c
    # find the runtime resources dependencies  and paths, which would
    # include the AGX paths and environment variables. 
    def find_external_dependencies(self):
        pass
        
    # Construct command line for fmigo-server processes, start these,
    # report both process ID and URI
    def launch_tcp_servers(self):
        self.uris = []
        self.processes = []
        for n,i in self.fmus.items():
            cmd = []
            if i.prefixcmd:
                cmd = i.prefixcmd
         
            cmd += ["fmigo-server"]
            if i.debug_level:
                cmd += ['-l', str(i.debug_level)]
            cmd += ["--port", str(self.port), i.filename]
            # TODO: this needs work since a given FMU might work on a different
            # host but that's beyond the current scope
            self.uris = self.uris +['tcp://localhost:%d' %self.port]
            self.ports = self.ports + [self.port]
            self.port += 1
            self.processes = self.processes + [subprocess.Popen(cmd)]
        return  cmd


    ## Given an already assembled command line for connections and parameters,
    ## run the simulation and a cleanup command as needed.
    ## There's a possibility to override the filename here
    def simulate(self,tend, dt, datafile, mode="tcp", holonomic=True,
                 parallel=True, max_samples=None, convertonly=False,
                 extra_args = None):
        
        if extra_args:
            self.extra_args  = extra_args
        self.datafile = datafile
        self.tend = tend
        self.dt = dt
        self.mode = mode
        self.annotations += ("mode", mode),
        if datafile:
            self.datafile = datafile
        self.init_fmus( dt)
        self.init_couplings()
        self.init_signals()
        self.parallel = parallel
        self.mode = mode
        processes = []
        opts = ["-t", str(self.tend), "-d", str(self.dt), '-H']  

        if max_samples:
            opts += ['-S', str(max_samples)]

        if self.is_kinematic:
            self.parallel = True
            self.annotations += ("coupling", "kinematic"),
            self.annotations += ("coupling_type", "%s" % "holonomic" if holonomic  else "nonholonomic"),
            if not holonomic:
                opts += ["-N"]
        else:
            self.annotations += ("coupling", "signals"),

        self.annotations += ("parallel", str(self.parallel)),
        opts += ["-m", "%s" % ("jacobi" if self.parallel else "gs")]
        opts += self.extra_args 
        
                
        if self.datafile:
            opts += ['-o', self.datafile + ".csv" ]

        opts += self.cmd_pars + self.cmd_couplings + self.cmd_signals
        if self.debug_level:
            opts += ['-l', str(self.debug_level)]

        if self.mode == "tcp":
            cmd =   ['fmigo-master']+ opts
            if not convertonly:
                self.launch_tcp_servers()
                cmd = cmd + self.uris
            if not convertonly:
                time.sleep(1)
        elif self.mode == "mpi":
            cmd = ['mpiexec', '-oversubscribe', '-np', str(1),'fmigo-mpi'] + opts
            for n,i in self.fmus.items():
                cmd += [":", "-np", str(1)]
                if i.prefixcmd:
                    cmd += i.prefixcmd

                cmd += ["fmigo-mpi"] 
                if i.debug_level:
                    cmd += ['l', str(i.debug_level)]
                cmd += [i.filename]
        else:    
            print("Only mpi and tcp modes are available but %s was requested." %mode)
            sys.exit(-1)

        print(' '.join(cmd))
        
        ts = dtime.datetime.now()
        if not convertonly:
            proc = subprocess.Popen(cmd)
        self.wall_time = str(dtime.datetime.now() -ts)

        

        self.annotations += ("wall_time", self.wall_time),
        self.annotations += ("fail",
                             "true" if convertonly or proc.wait() != 0 else "success"),


        csv = self.datafile + ".csv" 
        self.fix_csv(csv)
        
        self.pack_FMU_CSV_H5()
        # raise a signal if things went wrong
        if not convertonly and proc.wait() != 0:
            sys.exit(1)
        return cmd

    def fix_csv(self, csv):
        ## truncate file if there are bad lines
        infile = open(csv, "r")
        infile.seek(0)
        l = infile.readline()
        N = len(re.findall(",", l))
        infile.close()

        if N > 0:
            outfile = tempfile.NamedTemporaryFile()
            infile = open(csv, "r")
            infile.seek(0)
            for l in infile:
                if N == len(re.findall(",", l)):
                    outfile.write(l.encode())
                else:
                    break
                # close and reopen to overwrite
            outfile.seek(0)
            infile.close()
            infile = open(csv, "wb")
            shutil.copyfileobj(outfile, infile)
            outfile.close()
            infile.close()


    #
    #  This provides the operations necessary to take a csv file containing
    #  data from several different modules and repack them in a well structured
    #  hdf5 file  with annotation.
    #
    #  It is also possible to append the full simulation to an existing HDF5
    #  file. 
    #
    #  Because there are many operations involved in converting csv files to
    #  decent HDF5 ones, we make a class and split the various tricks into
    #  methods which can be tested individually.
    #

    def pack_FMU_CSV_H5(self):
        """ 
        List of FMU names, csv file to read from h5 file to write to,
        communication step, a tuple of duples with (name, value) for
        attributes, and additional description string

        The HDF5 files have a structure as follows: 

        / 
        ./simulation0001 ./simulation0002 ... ./simulationXXXX

        Numbering is done automatically for each new simulation dumped in a file. 

        Simulation groups have the structure
        ./simulationXXXX
        ./global_data ./fmu1 ./fmu2 .../fmuN

        and the group for each fmu group is the name of the FMU as given in the
        module definitions.   The global_data group contains a pytable with data
        collected by the master which does not belong to any FMU.

        Each fmu group then contains a pytable. 

        The simulation groups also contain the following attributes: 
        -- date
        -- start   (TODO: this should be as accurate as possible)
        -- stop
        -- comm_step
        -- N      (number of steps)
        -- kinematic   (value is no, holonomic, nonholonomic)
        -- OS on which simulation ran
        -- mode  (mpi or tcp/ip)
        -- parallel  (yes or no, depending on whether we used Jacobi or sequential)

        """

        ## short hands
        csv = self.datafile+".csv"
        hdf = self.datafile+".h5"
        H = self.dt
        fmus = list(self.fmus.items())
        

        ### Utility functions follow

        ## Replace special characters in `names'  in csv files otherwise they
        ## get deleted. 
        def patch_string(s):
            special = "[=~!@\$%\^&\.\(\)-\+\\\|\]\}\[\{';:/\?<>]"
            return re.sub(special, "_", s)
        ## Replace special characters in `names'  in csv files otherwise they
        ## get deleted. 
        def patch_names(file):
            f = open(file, "r+")
            line = f.readline()
            n = len(line)
            line = patch_string(line)
            if len(line) <= n:      # here we can do this in place. 
                f.seek(0) 
                f.write(line)
                f.close()       
            else:                   # here we have to move all the data
                tf =  tempfile.NamedTemporaryFile()
                tf.write(line)
                for l in f:
                    fs.write(l)
                f.close()
                shutil.move(tf.name, file)

        def set_attr(g, name, attr):
            g._f_setattr(patch_string(name), attr)
        # Works with a numpy structured array and replace non-descript names with
        # sensible ones so that 
        # `fmu_XXX_something'  `something'
        # This is all put under a group called fmu_XXX in the end
        # Here, x contains all the names corresponding to one given FMU, i.e.,
        # fmu1_x, fmu1_y, fmu1_z,, ... , fmu1_....
        def rename(x):
            names = ()
            for i in x.dtype.names:
                v = re.search('fmu([^_]+)_*',i)
                if v:  
                    i = re.sub("fmu[^_]+_", "", i)
                names = names + (i,)
            x.dtype.names = names


        # remove bad characters in the names in the data file.
        patch_names(csv)
        # read
        data =  np.genfromtxt(csv, delimiter=",", names=True) 
        # split the columns according to fmu0_*, fmu1_*   etc
        # and the rest
        # get all keys starting with fmu[[:digits:]]+_  and replace with
        # fmu_[[:digits:]]
        # sorted makes  the list [fmu1, fmu2, fmu3 ...  ]
        keys = sorted(list(OrderedDict.fromkeys(
            re.sub("(fmu[^_]+)_[^ ]*", "\g<1>",
                   " ".join(data.dtype.names)).split())))

        # global simulation data: whatever does not start with fmu
        global_cols =  sorted(re.sub("fmu[^ ]+", "", " ".join(keys)).split())
        # modules is what's left over
        fmu_cols =  sorted(list(set(keys) - set(global_cols)))

        # Time to get an output file

        dfile = tb.open_file(hdf, mode = "a")
        ## TODO: something smart to organize data 


        n = len(dfile.list_nodes("/")) +1
        g = dfile.create_group("/", 'simulation%04d' % n , 'Simulation')       

        set_attr(g,"date", dtime.datetime.now().strftime("%Y-%m-%d-%H:%M:%S"))
        time_units = [["s", 1], ["ms", 1e3],
                      ["mus", 1e6]]
        time_scale = 2
        set_attr(g,"comm_step", str(self.dt*time_units[time_scale][1]))
        set_attr(g,"time_unit", str(time_units[time_scale][0]))
        set_attr(g,"time_factor", str(time_units[time_scale][1]))
        set_attr(g,"tend", str(self.tend))
        set_attr(g, "os", platform.uname().system)
        set_attr(g, "machine", platform.uname().node)
        set_attr(g, "release", platform.uname().release)
        
        if self.name:
            set_attr(g, "name", self.name)
        if self.variant:
            set_attr(g, "variant", self.variant)
        for i in self.annotations:
            set_attr(g, i[0], i[1])
        if self.description:
            set_attr(g, "description", self.description)

        data_map = {}
        # store all columns fmuX_*  in a separate group, name the group
        # according to a dictionary entry in fmus
        for i in fmu_cols:
            # get the FMU id
            ix = int(re.sub("fmu([^ ]*)", "\g<1>", i))
            rx = re.compile("%s_[^ ]*" % i)
            # get the list of columns
            c = re.findall(rx, " ".join(data.dtype.names))
            # put the corresponding table in a dictionary
            data_map[fmus[ix][0]] = data[c]
            # now time for suitable names
            rename(data_map[fmus[ix][0]])
            # Here's where we insert the data
            # TODO: the group should have additional attributes from the tuple
            # of duple.  
            table = dfile.create_table(g, fmus[ix][0], data_map[fmus[ix][0]], "FMU data")  
            set_attr(table, "name", fmus[ix][1].name)
            ## attributes are 
            if  hasattr(fmus[ix][1], "attributes"):
                for k,v in fmus[ix][1].attributes.items():
                    set_attr(table, k, v)
            if  hasattr(fmus[ix][1], "parameters") and fmus[ix][1].parameters:
                for k,v in fmus[ix][1].parameters.items():
                    set_attr(table, k, v)
            if  hasattr(fmus[ix][1], "description"):
                set_attr(table, "description", fmus[ix][1].description)
            if hasattr(fmus[ix][1], "aliases"):
                set_attr(table, "aliases", fmus[ix][1].aliases)
            if hasattr(fmus[ix][1], "raliases"):
                set_attr(table, "raliases", fmus[ix][1].raliases)



        data_map["stepper"] = data[global_cols]    
        
        table = dfile.create_table(g, "simulation", data_map["stepper"], "Stepper data")  


        dfile.close()
        return dfile

    ## It is possible for a given FMU to appear more than once in a simulation so this
    ## function will rename as needed. 
    def remove_name_clashes(self, fmus):
        """
        fmus is a dictionary containing objects which are assumed to have a
        "name" attribute
        
        """
        #1) collect names from the dictionary into a list
        names = []
        for k,v in fmus.items():
            names += [v.name]
            # Counter creates a dictionary with the names as key and incidence
            # count as value.
        counts = Counter(names) 
    
        # Here we take care of elements with incidence count higher than 1
        # for each of thense, names.index(s)  will return the index of the
        # first element with that name.  Since that name is being modified, we
        # consume all duplicates and append an integer xxx to it. 
        for s,num in counts.items():
            if num > 1:
                for suffix in range(1, num+1):
                    names[names.index(s)] += "%03d" %  suffix
            
        # Now replace the names in the original dictionary,
        # consuming the elements in names as we go along.  The sequence of the
        # dictionary traversal is the same as before. 
        for k,v in fmus.items():
            v.name = names.pop(0)
            
        return fmus

       

