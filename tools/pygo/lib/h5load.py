import numpy as np
import re
import sys
sys.path.insert(0, "../lib")
from fmisims import patch_string
import tables as tb
from matplotlib.mlab import find
from numpy.lib.recfunctions import append_fields

### utilities to load a data file produced by this framework

class value_range:
    def __init__(self, low=0, up=0):
        self.low = low
        self.up  = up
    def __contains__(self, v):
        above = float(v) >= float(self.low)
        below = float(v)<= float(self.up)
        return above and below 

### utilities to load a data file produced by this framework

class match_value:
    """
    This provides a functor which helps collecting data from the file by
    matching certain attribute values or just the node's name with a
    regexp. 
    
    The condition given as argument can be a number of things such as a
    list, a dictionary, a tuple, a range (see above), a regexp, or a
    functor if the previous list is not extensive enough.

    If the `name'  argument to the __call__ method is empty, then we try to
    match a condition on the node variable. 

    TODO: at this time, only a regexp condition on the name is supported. 

    """
    def __init__(self, cond = None, attr_name="", reverse=False ):
        self.condition = cond
        self.type  = type(cond)
        self.fun_type = type(lambda x: x)
        self.re_type = type(re.compile(""))
        self.reverse = reverse
        self.name = attr_name
    def nil(self):
        pass
    def __call__(self, node):
        """ 
        """
        attrs = node._v_attrs
        found = False
        v = None
        if hasattr(attrs, self.name):
            v = attrs[self.name]
            if self.condition:
                if hasattr(self.type, "__contains__") and self.type!= type(""):
                    found = v in self.condition 
                elif hasattr(self.condition, "__call__"):
                    found = self.condition(v)
                elif type(self.condition) == self.re_type:
                    found = True if re.match(self.condition, v) else False
                elif type(self.condition) == self.fun_type:
                    found = self.condition(v) 
                else:           # scalar case
                    found = self.condition == v
            else:
                found = True
        elif self.name == "":
            if ( re.match(self.condition, node._v_name) ):
                 found = True
        if  found  == None :
            found = False
        return found^self.reverse
    
def locate_nodes(g, ntype, attr_name, cond=None, reverse=False):
    """ 
    Traverse under g and find nodes which match a given condition.
    """
    match = match_value(cond, attr_name, reverse)
    out = {}
    for n in g._f_walknodes(classname=ntype):
        if match(n):
            out[n._v_pathname]  = n    
    return out

def cmp_to_key(mycmp):
    'Convert a cmp= function into a key= function'
    class K:
        def __init__(self, obj, *args):
            self.obj = obj
        def __lt__(self, other):
            return mycmp(self.obj, other.obj) < 0
        def __gt__(self, other):
            return mycmp(self.obj, other.obj) > 0
        def __eq__(self, other):
            return mycmp(self.obj, other.obj) == 0
        def __le__(self, other):
            return mycmp(self.obj, other.obj) <= 0
        def __ge__(self, other):
            return mycmp(self.obj, other.obj) >= 0
        def __ne__(self, other):
            return mycmp(self.obj, other.obj) != 0
    return K

def compare_step(sim1, sim2):
    if sim1._v_attrs["comm_step"] < sim2._v_attrs["comm_step"]:
        return -1
    elif sim1._v_attrs["comm_step"] > sim2._v_attrs["comm_step"]:
        return 1
    else:
        return 0


def mlocate_nodes(g, ntype, conds=None):
    """ 
    Traverse under g and find nodes which match a list of given conditions.
    The argument `conds' is an array of match_value functors. 
    """
    out = []
    for n in g._f_walknodes(classname=ntype):
        # there are better ways to do this: only add if all conditions are
        # met. 
        keep = True
        for i in conds:
            keep &= i(n)
            if not keep:
                break
        if keep:    
            out   += [n]
    return out 

class meta_data():
    """ 
    Attributes for a simulation or a node.  This is to have m[name] instead
    of g._v_attrs[name]
    """
    def __init__(self, g):
        self.attrs = {} 
        for a in g._v_attrs._f_list():
            self.attrs[a] = g._v_attrs[a]
    def __getitem__(self, a):
        try: 
            return self.attrs[a]
        except:
            return None
    def __setitem__(self, k,v):
            self.attrs[k] = v
 
class attributes():
    """ 
    This is to get around 
    """
    def __init__(self, g):
        self.attrs = g._v_attrs
    def __getitem__(self, a):
        try: 
            return self.attrs[a]
        except:
            return None       

class sim_data():
    def __init__(self, sim, fmus, metadata):
        self.sim = sim
        self.fmus = fmus
        self.metadata = metadata
        pos = re.compile("eq[\d]+_g")
        l = [ x for x in list(self.sim.dtype.names) if pos.fullmatch(x)]
        self.nc = len( l ) 
        self.pos = sim[l]
        vel = re.compile("eq[\d]+_gv")
        l = [ x for x in list(self.sim.dtype.names) if vel.fullmatch(x)]
        self.vel = sim[l]
        self.t = self.sim["t"]

    def get_sim(self):
        return self.sim
    def get_time(self):
        return self.sim["t"]
    def time(self):
        return self.sim["t"]
    def __getitem__(self, f):
        return None if not f in self.fmus else self.fmus[f]
    def __len__(self):
        """ return the number of FMUs in the simulation """
        return len(self.fmus) -1
    def __contains__(self, f):
        return f in self.fmus
    def get_metadata(self):
        return self.metadata
    def has_attr(self, name):
        return True if name in self.metadata else False
    def get_attr(self, name):
        return self.metadata[name]
    def set_attr(self, name, v):
        self.metadata[name] = v
    def get_n_constraints(self):
        return  nc

    def get_rms(self, x):
        return np.sqrt( np.power(x.view((x.dtype[0], len(x.dtype.names))), 2).mean(axis=1) ).view()

    def get_violation(self, x, how = "rms"):
        """ Compute constraint violations for positions or velocities.  
        x = "pos"  or "vel"
        Three choices are given: 
        how == n: a single column is returned
        how = "rms" then we get the rms violation
        how = "global"  then we return the mean of the rms violations
        """

        if type(how) == type(0) and how < self.nc:
            return self.sim["eq%d_g%s" % (how, "" if x == "pos" else "v" ) ] 
        elif self.nc>0 and how == "rms" :
            """ this computes the RMS of the columns """
            return self.get_rms( self.pos  if x == "pos" else self.vel ).view()
        elif self.nc>0 and how == "global":
            return np.mean(self.get_rms( self.pos  if x == "pos" else self.vel )).view()
        else:
            return None
        
    def get_x_violation(self, how="rms"):
        return self.get_violation("pos", how=how)
    def get_v_violation(self, how="rms"):
        return self.get_violation("vel", how=how)
    
                 
class simulation_data():
    """ Here, all the numpy arrays are kept together for each fmu and each
    set of global data.  Metadata about the specific simulation is also
    available."""

    def __init__(self, sims, md):
        self.sims = sims
        
        self.metadata = md
    ## pass  `which' as a tuple giving the simulation, fmu, and variable
    ## ids
    def items(self):
        return list(self.sims.keys())
    def __getitem__(self, which ):
        return self.sims[ which[0] ][1][which[1]][which[2]]
    ## This will get the global and metadata for the simulation 
    def get_time(self, s):
        return self.sims[s][0]
    def time(self, s):
        return self.sims[s][0]
    def get_sim(self, s):
        return self.sims[s][0], self.metadata[s]
    ## This extracts the complete table for a given FMU
    def get_fmu(self, s, f):
        try:
            return self.sims[s][1][f]
        except:
            return None
    def get_names(self):
        return self.sims.keys()
    def get_variant(self, s):
        return self.metadata[s].attrs["variant"]
    def get_date(self, s):
        return self.metadata[s].attrs["data"]


# Utility to read ndarray from a table add alias columns
# Concatenation of structured array is from:
# 
# https://stackoverflow.com/questions/5355744/numpy-joining-structured-arrays
def read_apply_aliases(node):
    """ this will read a structured numpy array and, if applicable,
    duplicate columns under aliases """
    
    names = []
    x = node.read()
    try:
        aliases =  node._f_getattr("aliases")
        newdtype =  []
        for n in x.dtype.names:
            newdtype += [(n,'<f8')]
        for k,v in names.items():
            newdtype += [(patch_string(k),'<f8')]
        
        newrecarray = np.empty(len(x), dtype = newdtype)
        for name in array.dtype.names:
            newrecarray[name] = x[name]
        for k,v in names.items():
            newrecarray[patch_string(k)] = array[patch_string(v)]
        x = newrecarray
    except:
        pass
    return x

def package_simulations(simulations, dfile, close_file=True):
    """ 
    This pulls the data from simulations and extract the numpy
    arrays
    """
    data = []
    sims={}
    sdata={}
    for s in simulations:
        m = s._v_name
        sdata[m] = meta_data(s)
        fmus= {}
        for i in s._f_walknodes("Leaf"):
            fmus[i.name] = read_apply_aliases(i)
            # store ndarray's generated from the table
        sims[m] = [s.simulation.read(), fmus ]       
        del(fmus["simulation"])
        data += [ sim_data(s.simulation.read(), fmus, meta_data(s)) ] 
    # we have to close the hdf5 file here since the numpy arrays still
    # point to it and so the dtype.names  are conflicting.
    if close_file:
        dfile.close()
    return  data

def get_simulations_data( filename,  which=0, close_file=True):
    """
    Fetch simulation groups from the `which' argument.
    which == 0:  all of them 
    type(which) == list:  extract a number of simulation
    type(which) = int:  extract just one 
    
    If the values in the list are negative, we count from the end.
    """

    f5 = tb.open_file(filename, "r")

    n = len(f5.list_nodes("/"))
    r = []
    # extract only relevant simulations
    if which == 0:
        r = np.array(list(range(1,n+1)))
    elif type(which) == int:
        r = np.array([which])
    else: 
        r = np.array(which)

    r[ find(r<0)] += n+1
    snodes  = []
    for i in r:
        m = "simulation%04d" % i
        snodes += [ f5.root._f_get_child(m) ]

    if close_file:    
        f5.close()

    return snodes

'''
for k in data.items():
    py.figure(fig)
    fig += 1
    py.clf()
    # get the time
    t = data.get_time(k)
    md = data.metadata[k]
    gw = data[(k, "gearbox", "w_output_shaft")]
    tdw = data[(k, "truck", "w_drive_shaft")]
    ax = data.get_fmu(k, "axle")
    if ax is None: 
        py.plot(t, gw-tdw)
        lgd = ["gear - truck diff"]
    else:
        pw = ax["w_prop_shaft"]
        adw = ax["w_drive_shaft"]
        py.plot(t, pw-gw, t, adw-tdw)
        lgd = ["gear-axle in", "axle out - truck in"]
        
    py.title("%s %s $H=$%s%s gear out truck in" % (md["variant"], md["coupling_type"],
                                                   md["comm_step"], md["time_unit"]))
    py.legend(lgd)
py.show()
'''


''''
## some notes:
## file object:
f = tb.open_file("foo", "r") 
## list of nodes at root
f.list_nodes("/")  
## here we ge a group by name and for us, that's one full simulation
s  = f.root._f_get_child(n)
#The attributes here are obtained from 
s._v_attrs["name"]
## to get a list of the attributes on a *table*
t = s._f_get_child("something")
t._v_attrs
## to get a specific attribute
t._v_attrs["something"]
# a list of names of *user* defined attributes
t._v_attrs._f_list()
# a list of *all* attribures 
t._v_attrs._v_attrnames

### TODO:
# merge two files
# Metadata:
#    - name  (truck)
#    - variant    ( some short name that can be used as variable)
#    - description (long form, multiline)
#    - os
#    - date
# 
# Find function:
#    - find a node with a given name
#    - find a node with a given attribute
#    - find all nodes which do not have a given attribute  (do this with
#       set differences?

Narratives: 

Plot the same variable for all variants of a given model for a given step

Plot same variable from one module for diferent steps, do this for all
variants.  

Plot errors for different steps

Looks like for most of this, working directly on the h5 file and tables is
a good idea.  

'''
