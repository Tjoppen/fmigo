from wrappedmodels import * 
import refchain
import hashlib

"""
implementation of a spring damper chain with alternating spring damper
constants. 
"""
config = {}
N = 10
init = [[1,0,0,0]] + [[0,0,0,0]]*(N-1)
masses = [[1,2]]*N
isprings = [[1,0]]*N
k = 3.0
config = {"N" : N, "init" : init, "masses" : masses, "isprings" : isprings, "k" : k }
mu = 1/ ( 1/ masses[0][0] + 1/masses[0][1]) 
min_p, max_p = refchain.get_min_max_period(masses, isprings)
min_p
max_p
tend  = 3*max_p

## We want to prune the following case:
## kinematic and not parallel and filtered (makes no sense)
##
orders = 9
factor = 5.0
npp = [10]
step = min_p / float(npp[0])

for f in range(1,orders+1):
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
                    config["csprings"] = csprings
                    sim  = chain_mass_springs(filtered, N, init, masses, isprings, csprings, kinematic=kinematic)
                    sim.annotations += ("steps_per_period", npp[-1]),
                    sim.annotations += ("config", config),
                    sim.annotations += ("confighash", hashlib.md5(str(config).encode("utf8")).hexdigest()),
                    try:
                        sim.simulate(tend, step, mode="mpi", parallel=parallel, datafile="chain")
                    except:
                        pass
                    
    step /= factor 
    npp += [npp[-1]*int(factor)]

                         
