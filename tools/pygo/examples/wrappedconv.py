from wrappedmodels import * 
import refchain
import hashlib

"""
implementation of a spring damper chain with alternating spring damper
constants. 
"""



def doit( N, fname, orders, k=3, factor=2, max_samples= 1000):
    npp = 10                  # safe reference start value
    ## get an initial estimate of the "reasonable" time step
    config  = refchain.make_config_uniform(N, k=0.0, step=0.0)
    step = config["step"]
    print(step)
    for f in range(1,orders+1):
        for kinematic in [True, False]:
            for filtered in [True, False]:
                for parallel in [True, False]:
                    if not (kinematic  and filtered ) and  not (kinematic and not parallel):
                        k0 = float(0) if kinematic else k
                        config = refchain.make_config_uniform(N, k=float(0)) if kinematic else refchain.make_config_uniform(N, step=step, k=k)
                        sim  = chain_mass_springs(filtered, N, config["init"], config["masses"], config["isprings"],
                                                  config["csprings"], kinematic=kinematic)
                        sim.annotations += ("confighash", hashlib.md5(str(config).encode("utf8")).hexdigest()),
                        sim.annotations += ("config", config),
                        sim.annotations += ("npp", npp),
                        try:
                            sim.simulate(config["tend"], step, 
                                         mode="mpi", parallel=parallel, datafile=fname, max_samples = max_samples)
                            refchain.get_pickle_data(config)
                            
                        except:
                            pass

        step /= factor 
        npp *= int(factor)



N = 10
k = 3
orders = 3
doit(N, "chain", orders, factor=2, max_samples= 1000)
