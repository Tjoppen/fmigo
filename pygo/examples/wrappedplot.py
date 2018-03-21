from python_stuff.h5load import *
from pylab import *
import re
import pickle as pickle 


eps=1e-16

rcParams['text.latex.preamble'] = ["\\usepackage{amsmath}"]
rcParams['text.latex.preview'] =  False
rcParams['text.latex.unicode'] =  False
rcParams['text.usetex'] =  True



def count_steps(data):
    """ Determines how many different communication steps there are """
    d = {}
    for i in data:
        d[i.get_attr("comm_step")] = i.get_attr("comm_step")
    i.get_attr("comm_step")
    return len(d)

f = tb.open_file("chain.h5")


def get_length(s):
     x = re.search("[\d]+$", s)
     # there can be only one or no match here
     n = 0
     if x :
         n = int(x.group(0))
     return n

def get_simulations(N=5):
    ## this will pull all kinematic simulations and sort them in order of length first and communication step second
    kin  = sorted( package_simulations(mlocate_nodes(f.root, "Group", conds = [
        match_value("kinematic", "coupling"),
        match_value(lambda x: get_length(x) == N, "variant"),
    ] ), f, close_file=False),
                   key=lambda y: (get_length(y.get_attr("variant")), float(y.get_attr("comm_step") ) ), reverse=True )

    filtered_parallel  = sorted( package_simulations(mlocate_nodes(f.root, "Group", conds = [
        match_value("signals", "coupling"),
        match_value(re.compile("filtered_.*"), "variant"),
        match_value(re.compile("True"), "parallel"),
        match_value(lambda x: get_length(x) == N, "variant"),
    ] ), f, close_file=False),
                                 key=lambda y: (get_length(y.get_attr("variant")), float(y.get_attr("comm_step") ) ), reverse=True )

    filtered_sequential  = sorted( package_simulations(mlocate_nodes(f.root, "Group", conds = [
        match_value("signals", "coupling"),
        match_value(re.compile("filtered_.*"), "variant"),
        match_value(re.compile("False"), "parallel"),
        match_value(lambda x: get_length(x) == N, "variant"),
    ] ), f, close_file=False),
                                   key=lambda y: (get_length(y.get_attr("variant")), float(y.get_attr("comm_step") ) ), reverse=True )



    plain_parallel  = sorted( package_simulations(mlocate_nodes(f.root, "Group", conds = [
        match_value("signals", "coupling"),
        match_value(re.compile("plain_.*"), "variant"),
        match_value(re.compile("[Tt]rue"), "parallel"),
        match_value(lambda x: get_length(x) == N, "variant"),
    ] ), f, close_file=False),
                              key=lambda y: (get_length(y.get_attr("variant")), float(y.get_attr("comm_step") ) ), reverse=True )

    plain_sequential  = sorted( package_simulations(mlocate_nodes(f.root, "Group", conds = [
        match_value("signals", "coupling"),
        match_value(re.compile("plain_.*"), "variant"),
        match_value(re.compile("False"), "parallel"),
        match_value(lambda x: get_length(x) == N, "variant"),
    ] ), f, close_file=True),
                                key=lambda y: (get_length(y.get_attr("variant")), float(y.get_attr("comm_step") ) ), reverse=True )

    return [kin, filtered_parallel, filtered_sequential, plain_parallel, plain_sequential]


def get_violation(sim):
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

    legend(lgd)
    xlabel("time step [s]")
    ylabel("rms violation")
    title("Convergence plot for N = %d" % get_length(m.get_attr("variant")))
        

N = 3

try:
    f = open("wdata.pickle", "rb")
    all = pickle.load(f)
    f.close()
except:
    all = get_simulations(N=N)
    f = open('wdata.pickle', "wb")
    pickle.dump(all, f)
    f.close() 





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
