import sys
sys.path.insert(0, "../lib")
from h5load import *
from pylab import *
import re
import pickle as pickle 
import numpy as np



## this to get proper labels on plots
rcParams['text.latex.preamble'] = ["\\usepackage{amsmath}"]
rcParams['text.latex.preview'] =  False
rcParams['text.latex.unicode'] =  False
rcParams['text.usetex'] =  True




def get_length(s):
    """ Determine the length of the string from it's name which is
    prepended as: 
    "foobar%d" % N
    """
    x = re.search("[\d]+$", s)
    # there can be only one or no match here
    n = 0
    if x :
        n = int(x.group(0))
    return n

class match_config():
    """Provide a functor to see if two configurations are equivalent for a
    given number of fields.  This is used when traversing the H5 file to
    select simulations to be analyzed.

    In the constructor, `c' is the reference config and `f' is a *list* of
    fields to match.

    There is an issue: there is no == operator for ndarrays at least, but
    other objects might also have the same problem.

    """
    
    def __init__(self, c, f):
        self.config = c
        self.fields = f         # a list

    def update(self, f):
        self.fields += list(f)

    def __call__(self, v):
        gotit = True
        for f in self.fields:
            if type(self.config[f]) == np.ndarray:
                gotit &= ( self.config[f] == v[f] ).all()
            else:
                gotit &= (self.config[f] == v[f])
        return gotit

def get_simulations(fname, config ):
    """ Here we pull all simulations which have the right number of
    elements and sort them according to time step.  This is split into
    categories: 
    -- kinematic
    -- filtered and parallel
    -- filtered and sequential
    -- not filtered and parallel
    -- not filtered and sequential
    We also make sure that we have a matching configuration in terms of
    initial conditions, spring constants, and 'k' factor.  
    """
    f = tb.open_file(fname)
    sims = {}
    simsref = {}
    # do kinematic separately to avoid crazy logic
    mc = match_config(config, ["N", "masses", "isprings"])
    cc = [ match_value(mc, "config")  ]
    sims["kinematic"] = sorted(
                    package_simulations( mlocate_nodes(f.root, "Group", conds = cc), f, close_file=False), 
                    key=lambda y: float(y.get_attr("comm_step")  ), reverse=True )
    simsref["kinematic"] = 
    # need the `k' parameter for the other ones
    mc.update("k")

    for i in [ "filtered_", "plain_"]:
        for k in [True, False]:  # parallel or not
            # assemble the filter line
           choose = cc + [match_value("signals", "coupling"), 
                          match_value(re.compile(str(k)), "parallel"),
                          match_value(re.compile("%s.*"%i, re.I), "variant")]
           key = i + ("parallel" if k else "sequential")
           sims[key] = sorted(
               package_simulations( mlocate_nodes(f.root, "Group", conds = choose), f, close_file=False), 
               key=lambda y: float(y.get_attr("comm_step")  ), reverse=True )
    f.close()
    return sims


def get_violation(sim):
    """ This has a lot of hard coded stuff and should be fixed. 
    What it does here is provide a time sequence of 
    """
    n = len(sim)
    dx = 0.
    dv = 0.
    for i in range(n-1):
       dx += (sim["m%d" %i]["phi_out"]-sim["m%d" % (i+1)]["phi_in"])**2
       dv += (sim["m%d" %i]["w_out"]-sim["m%d" % (i+1)]["w_in"])**2
    return sqrt(dx)/(n-1), sqrt(dv)/(n-1)

def get_rms_violation(sim):
    x,v = get_violation(sim)
    return norm(x), norm(v)


def rms_error(pos, f, l):
    lgd = []
    figure(f)
    clf()
    for i in  l: 
        for m in i:
            x,v = get_violation(m)
            x = x if pos == "x" else v
            semilogy(m.get_time(), x)
            s = re.match("[^_]+", m.get_attr("variant")).group(0)  + " " 
            s += "parallel" if m.get_attr("parallel") == "True" else "serial"
            s += " H = " + m.get_attr("comm_step") + "ms"
            lgd += [ s ] 

            legend(lgd)
            title("Time evolution of RMS %s error for N=%d" % ("position" if pos == "x" else "velocity", get_length(m.get_attr("variant"))))


def convergence(f, l):
    eps=1e-16                   # needed for loglog plots
    lgd = []
    figure(f)
    clf()
    for i in  l: 
        if i:
            dt = []
            dx = []
            dv = []
            for m in i:
                H = float(m.get_attr("comm_step")) / float(m.get_attr("time_factor"))
                dt += [ H ]
                x,v = get_rms_violation(m)
                dx += [x]
                dv += [v]
                print("H  = %g  x = %f  v = %f" % (H, x, v))

            loglog(dt, dx)
            loglog(dt, dv)
            s = re.match("[^_]+", m.get_attr("variant")).group(0)  + " " 
            s += "parallel" if m.get_attr("parallel") == "True" else "serial"
            s += " H = " + m.get_attr("comm_step") + "ms"

            lgd += [ "dx, " + s + ": %1.1f" % (polyfit(log10(dt), log10(dx), 1)[0]) ] 
            lgd += [ "dv, " + s + ": %1.1f" % (polyfit(log10(dt), log10(dv), 1)[0]) ] 

    if lgd:
        legend(lgd)
        xlabel("time step [s]")
        ylabel("rms violation")
        title("Convergence plot for N = %d" % get_length(m.get_attr("variant")))
    else:
        print("oops!  nothing to plot!")
        




### A few things to do: 
### 
### First, check that 'all'  is defined and has the right type: if not,
### read from that file.
### If 'all'  is defined, check that it contains sims for the right 'N' 
### If there's nothing in the hdf5 for N, then rerun the simulations for
### that.
### 
### 

'''
try:
    f = open("wdata.pickle", "rb")
    all = pickle.load(f)
    f.close()
    if not all or get_length(all[0][0].get_attr("variant") ) != N :
        throw
except:
    all = get_simulations(N=N)
    f = open('wdata.pickle', "wb")
    pickle.dump(all, f)
    f.close() 
'''


'''

all = [all[-2], all[-1]]
if all:
    f = 1
    rms_error("x", f, all)
    f+=1
    rms_error("v", f, all)
    f+=1
    convergence(f, all)


f += 1
figure(f)
vanilla = all[-1][-1]
clf()
lgd = []
for k,v in vanilla.fmus.items():
    plot(vanilla.t, v["phi_in"], vanilla.t, v["phi_out"])
    lgd += [ "%s: x1" % k, "%s: x1" % k]

legend(lgd)
show()
'''
