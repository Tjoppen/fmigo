import sys
sys.path.insert(0, "../lib")
from fmisims import *
import math
umit="../../release/umit-fmus/"
pfmu  = umit + "meWrapper/springs2/wrapper_springs2.fmu"
ffmu = umit + "meWrapperFiltered/springs2/wrapper_filtered_springs2.fmu"
cfmu = umit + "gsl2/clutch2/clutch2.fmu"

class clutch(module):
    """ Modules with two point masses hooked by a spring.  This was first
    designed to model a clutch by using a special spring, and can represent
    a gear as well.  But considering this as just point masses is probably
    a good idea.  Only a fraction of the parameters are specified here. """
    def __init__(self, parameters={
            "x0_e" : "0",
            "v0_e" : "0",
            "dx0_e": "0",
            "k_ec" : "0",
            "gamma_ec" : "0",
            "integrate_dx_e" : "0",
            "mass_e" : "1",
            "gamma_e" : "1",
            "reset_dx_e" : "true",
            "x0_s" : "0",
            "v0_s" : "0",
            "dx0_s": "0",
            "k_sc" : "0",
            "gamma_sc" : "0",
            "integrate_dx_s" : "0",
            "mass_s" : "1",
            "gamma_s" : "1",
            "reset_dx_s" : "true",
            "is_gearbox" : "true",
            "gear_k" : "1",
            "gear_d" : "0",
            "gear" : "13",
            "integrator" : "2",  #2 corresponds to rk45f
            } ):
        pass

class two_bodies(module):
    """ Two masses and springs
    """
    def __init__(self, filtered, parameters={
                        "x1" : "1", "v1" : "1", "k1" : "0", "gamma1" : "0",
                        "x2" : "1", "v2" : "1", "k2" : "0", "gamma2": "0",
                        "k_internal": "10", "gamma_internal" : "0",
                        "m1" : "1", "m2" : "1"},
                        ):
        

        module.__init__(self, 
            filename= ffmu if filtered else pfmu,
                    name="two_masses",
                    connectors={"in" : ["x1", "v1", "a1", "f1"],
                                "out": ["x2","v2","a2","f2"]},
                    ## this will create aliases in the HDF5 files.  One can
                    ## specify the connections either using the aliased
                    ## names or the original names in the
                    ## modelDescription.xml file.  This applies to both
                    ## inputs and outputs. 
                    inputs = {
                            "in_x1": "x1_i", "in_v1": "v1_i", "in_f1": "f1",
                            "in_x2": "x2_i", "in_v2": "v2_i", "in_f2": "f2"
                    },
                    outputs = {
                            "zero": "zero",
                        "out_x1": "x1", "out_v1": "v1", "out_a1": "a1",
                        "out_f1": "fc1", "minv1":  "minv1",
                        "out_x2": "x2", "out_v2": "v2", "out_a2": "a2",
                        "out_f2": "fc2", "minv2":  "minv2"
                    } ,
                    parameters = parameters, 
                    description="two masses with a spring-damper", 
        )
#########  END MODULE LIBRARY ################
 
### Now define simulations 

def make_parameters(filtered, N, init, masses, isprings, csprings, kinematic=False):
    """ 
    Do this as a function so we can reuse this mess for both force-velocity
    and kinematic couplings. Both types of couplings are returned.
    """
    fmus = {}
    couplings =() 
    signals  = ()

    for i in range(N):
        parameters = {
            "x1" : str(init[i][0]),
            "v1" : str(init[i][1]),
            "x2" : str(init[i][2]),
            "v2" : str(init[i][3]),
            "k1" : str(csprings[i][0]),
            "gamma1" : str(csprings[i][1]),
            "k2" : str(csprings[i][2]),
            "gamma2" : str(csprings[i][3]),
            "k_internal"     : str(isprings[i][0]),
            "gamma_internal" : str(isprings[i][1]),
            "m1" : str(masses[i][0]),
            "m2" : str(masses[i][1]),
            "integrator" : str(4)
            }
        
        if kinematic:
            parameters["k1"]     = str(0)
            parameters["gamma1"] = str(0)
            parameters["k2"]     = str(0)
            parameters["gamma2"] = str(0)

        
        fmus["m%d" % i]  = two_bodies(filtered, parameters=parameters)

    if kinematic:
        for i in range(N-1):
            # one can either use a 4-tuple using aliased signals: 
            #couplings += ("m%d" %i, "out", "m%d" %(i+1), "in"),
            # or directly use the named value references or simply value
            # references in a 10-tuple
            couplings+= ("m%d" %i, "x1", "v1", "a1", "f1",
                                "m%d" %(i+1), "x2","v2","a2","f2"),
    else:
        for i in range(N-1):
            # this is hard wired to go left to right,
            #  output velocity, input forces
            signals += ("m%d" %i, "v2", "m%d" %(i+1), "v1_i"),
            signals += ("m%d" %(i+1), "fc1", "m%d" %(i), "in_f2"),

    return fmus, couplings, signals


class chain_mass_springs(simulation):
    """
    A chain, again.

    Constructor arguments: 
    N            number of elements
    init         4D sequence of length N for [x1,v1,x2,v2] as initial conditions
    masses       2D sequence of length N for mass 1 and mass 2
    isprings     2D sequence of length N with spring and damping constants
    csprings     4D sequence of length N with coupling spring dampes: [k1, d1, k2, d2]

    
    """
    def __init__(self, filtered, N, init, masses, isprings, csprings,
                 kinematic = False, annotations=()):

        fmus, couplings, signals = make_parameters(filtered, N, init,
                                                   masses, isprings,
                                                   csprings,
                                                   kinematic=kinematic)
        
        simulation.__init__(self, fmus,
                            name = "me_wrapped_spring_damper",
                            variant ="%s_wrapped_me_chain_%d" %("kinematic" if kinematic  else "filtered" if filtered else "plain" , N),
                            couplings=couplings,
                            signals=signals,
                            annotations=annotations,
                            description= """wrapped %s spring-dampers in
                            chain of %d elements with %s couplings"""
                            % ( "filtered" if filtered else "", N, "kinematic" if kinematic else "signals")
        )

## try this thing
N = 3
init = [[1,0,0,0]] + [[0,0,0,0]]*(N-1)
masses = [[1,2]]*N
isprings = [[1,0]]*N
mu = 1/ ( 1/ masses[0][0] + 1/masses[0][1]) 
k = 3
tend = 10
##
## We want to prune the following case:
## kinematic and not parallel and filtered (makes no sense)
##
for step in [1e-2]:   #[1e-1, 1e-2, 1e-3, 1e-4]:
    for kinematic in [True, False]:
        for filtered in [True, False]:
            for parallel in [True, False]:
                cd = not (kinematic  and filtered ) and  not (kinematic and not parallel) 
                if cd:
                    if not kinematic:
                        c = mu*(k*2*math.pi/step)**2
                        d = 2*0.7*math.sqrt(c*mu)
                        csprings = [[c,d,0,0]]*N
                        csprings[0] = [0,0,0,0]
                    else:
                        csprings = [[0,0,0,0]]*N
                    sim  = chain_mass_springs(filtered, N, init, masses, isprings, csprings, kinematic=kinematic)
                    sim.simulate(tend, step, mode="mpi", parallel=parallel, datafile="chain")

                         
