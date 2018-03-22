import math
import numpy as np
from scipy.integrate import ode
import scipy.interpolate as interp
import pylab as py
import scipy
from scipy import sparse
from scipy.linalg import expm
import hashlib

def join_struct_arrays2(arrays):
    newdtype = sum((a.dtype.descr for a in arrays), [])
    newrecarray = np.empty(len(arrays[0]), dtype = newdtype)
    for a in arrays:
        for name in a.dtype.names:
            newrecarray[name] = a[name]
    return newrecarray

def pack_data(t, x, v):
    t.dtype = [("t", "<f8")]
    d = [("t", "<f8")]
    for i in range(1, x.shape[1]+1):
        d += [("x%d" % i, "<f8" ) ]
    for i in range(1, x.shape[1]+1):
        d += [("v%d" % i, "<f8" ) ]
    newrecarray = np.empty(x.shape[0], dtype = d)
    newrecarray["t"] = t
    for i in range(0, x.shape[1]):
        newrecarray["x%d" %(i+1)] = x[:, i]
    for i in range(0, v.shape[1]):
        newrecarray["v%d" %(i+1)] = v[:, i]
    return newrecarray
"""
implementation of a spring damper chain with alternating spring damper
constants. 
"""

def gen_A(n, m, k, d):

    """ generates a sparse matrix which corresponds to the linear 
    system defined by chains of mass springs. 

    n: number of masses
    m: scalar or array of n masses 
    k: scalar or array of n spring constants
    d: scalar or array of n damping constants
    """

    if not hasattr(m, "__len__"):
        m = m * np.ones(n)
    if not hasattr(k, "__len__"):
        k = k * np.ones(n-1)
    if not hasattr(d, "__len__"):
        d = d * np.ones(n-1)
    ## Diagonal for the spring part: sum of upper and lower    
    kd = np.concatenate((k,[0])) +  np.concatenate(([0],k))
    ## pad lower diagonal of spring block
    ku   = np.concatenate(( np.concatenate((np.zeros(1), k)) , np.zeros(1)) )

    ## Diagonal for the damping block: sum of upper and lower diagonals,
    ## and padding. 
    dd = np.concatenate((np.zeros(n), np.concatenate((d,[0])) +  np.concatenate(([0],d)) ))
    dl = np.concatenate((np.zeros(n), d))
    
    Mi = sparse.diags(np.concatenate( (np.ones(n), 1./m)))
    diagonals = [ k, -kd,  ku,  dl, -dd,  dl, np.ones(n)] 
    A = Mi * sparse.diags(diagonals, [-n-1,-n,-n+1,-1,0,1,n]) 
    ## most operations in scipy sparse matrix suite are more efficient in
    ## csc format
    A = A.tocsc()
    return A

def gen_chain_A(masses, isprings):
    m = np.array(masses,dtype=float)
    m = m.reshape(m.shape[0]*m.shape[1])
    ## Fuse the masses from the initial conditions of the
    ## cosimulation
    m = np.concatenate((np.array(m[0]).reshape(1),
                             m[1:-1:2]+m[2:-1:2], np.array(m[-1]).reshape(1)), axis=0)
    ## isprings is a 2 column array
    isprings = np.array(isprings,dtype=float)
    A = gen_A(m.shape[0], m, isprings[:,0], isprings[:,1])
    return A

def get_min_max_period(masses, isprings):
    A = gen_chain_A(masses, isprings).todense()
    lm = np.abs(sparse.linalg.eigs(A,k=1,which='LM', return_eigenvectors=False))[0]
    # ignore the two nearly DC modes
    sm = np.sort(np.abs(sparse.linalg.eigs(A,k=3,which='SM', return_eigenvectors=False)))[-1]
    return 2*math.pi / lm,  2.0*math.pi / sm
    


class spring():

    """Just a single spring to establish the main features needed: data
    collection and basic setup"""


    def __init__(self, z0=[1,0]):
        self.n = int(len(z0)/2)
        self.r= ode(self).set_integrator('dop853',rtol=1e-8,atol=1e-8,
                                         nsteps=100000)
        self.r.set_initial_value(z0, 0)
        self.z0 = z0
        self.A = sparse.csc_matrix([[0,1],[-1,0]])
        self.name = "dummy mass spring"
        self.slow = False
        

    def __call__(self, t, z):
        x = z[0:self.n]
        v = z[self.n::]
        zdot = 0 * z
        zdot[0:self.n] = v
        zdot[self.n::] = -x
        return zdot
    def sample(self, t):
        ## delete duplicated data point at t=0
        if self.t[0] == self.t[1] :
            self.t = np.delete(self.t, 0)
            self.x = np.delete(self.x, (0), axis=0)
            self.v = np.delete(self.v, (0), axis=0)
        self.xs = interp.interp1d(self.t, self.x, kind='cubic', axis=0)(t)
        self.vs = interp.interp1d(self.t, self.v, kind='cubic', axis=0)(t)
        return self


    class collector():
        def __init__(self, outer, z0, t0 = 0):
            self.outer = outer
            self.outer.n = int(len(z0)/2)
            n = self.outer.n
            
            if self.outer.slow:
                self.outer.x = z0[0:n].reshape((1,n))
                self.outer.v = z0[n::].reshape((1,n))
                self.outer.t = np.array([t0])
            else:
                self.outer.x = [] 
                self.outer.v = []
                self.outer.t = []

                self.outer.x = [z0[0:n]]
                self.outer.v = [z0[n::]]
                self.outer.t = [t0]

            
        def __call__(self, t, z):
            
            n = self.outer.n
            if t > self.outer.t0:
                if self.outer.slow :
                    self.outer.x = np.concatenate((self.outer.x, z[0:n].reshape((1,n))), axis=0)
                    self.outer.v = np.concatenate((self.outer.v, z[n::].reshape((1,n))), axis=0)
                    self.outer.t = np.append(self.outer.t, t)
                else:
                    self.outer.x += [z[0:n]]
                    self.outer.v += [z[n::]]
                    self.outer.t += [t]

        def repack(self):
            l = lambda w : np.array(w).reshape((len(w), w[0].shape[0]))
            self.outer.x  = l(self.outer.x)
            self.outer.v  = l(self.outer.v)
            self.outer.t = np.array(self.outer.t)

                    
    def doit(self, tend, h=0.0, z0=None, t0=0):
        """ Here we consider the case where new initial conditions are
        given """
        self.t0 = t0
        if z0 is not None :
            self.n = int(len(z0)/2)
            self.r.set_initial_value(z0, self.t0)
            self.z0 = z0
            
        ## avoid potential memory leak in case we are solving a new problem
        try:
            del(self.data)
        except:
            pass

        self.data = self.collector(self, self.z0)

        if h == 0.0:
            self.r.set_solout(self.data)
            self.r.integrate(tend)
        else:
            n = math.ceil( tend / h ) 
            m = expm(np.multiply(self.A, h))
            self.expa = m
            z = self.z0
            t = 0
            for i in range(n):
                z  = m * z
                t += h
                self.data(t, z)
        if not self.slow:
            self.data.repack()
                

class split_string(spring):
    """ This the mathematical equivalent of co-simulation case: every other
    mass is connected via a very stiff spring-damper.
    """

    def __init__(self, step, init, masses, isprings, k=3):
        
        self.name = "Chain of split mass springs"
        self.step = step
        self.csprings = self.setup(step, masses, k=k)
        self.isprings = np.array(isprings, dtype=float)
        self.N = 2*self.isprings.shape[0]
        N = self.N
        self.m =  np.array(masses, dtype=float).reshape(N)
        self.im = 1.  / self.m
        z0 = np.array(init, dtype=float).reshape(2*N)
        self.z0 = z0
        self.z0[0:N]  = z0[0::2] 
        self.z0[N::]  = z0[1::2] 
        super().__init__(z0 = self.z0)
        self.K = np.zeros(N-1)
        self.D = np.zeros(N-1)
        self.K[0::2]    = self.isprings[:, 0]
        self.K[1:-1:2]  = self.csprings[:, 0]

        self.D[0::2]    = self.isprings[:, 1]
        self.D[1:-1:2]  = self.csprings[:, 1]

        self.A  = gen_A(self.m.shape[0], self.m, self.K, self.D)

    def setup(self, step, masses, k=3):
        N = len(masses)
        csprings = [[0,0]]*(N-1)
        for i in range(0,N-1):
            mu = 1./ ( 1./ masses[i][1] + 1./masses[i+1][0]) 
            c = mu*(k*2*math.pi/step)**2
            d = 2*0.7*math.sqrt(c*mu)
            csprings[i] = [c,d]
        return np.array(csprings, dtype=float)

    def errors(self):
        self.dx = np.diff(self.x, axis=1)[:,1:-1:2]
        self.dv = np.diff(self.v, axis=1)[:,1:-1:2]

    def sample(self, t):
        try:
            a = self.dx.shape[0]
        except:
            self.errors()

        self.errors()
        self.dxs =  interp.interp1d(self.t, self.dx, kind='cubic', axis=0)(t)
        self.dvs =  interp.interp1d(self.t, self.dv, kind='cubic', axis=0)(t)
        super().sample(t)
        return  self

    def __call__(self, t, z):
        return self.A * z
    
 


class spring_chain(spring):
    def __init__(self, init, masses, isprings):
        """ This is the original chain of spring-dampers *before* split
            into two mass-spring modules 
        """
        self.name = "Chain of mass springs"

        ## tacit assumption that the initial conditions are consisitent: we
        ## pick the information of every other mass only
        i0 = np.array(init, dtype=float)
        x = np.concatenate((np.array(i0[0][0]).reshape(1), i0[:,2]))
        v = np.concatenate((np.array(i0[0][1]).reshape(1), i0[:,3]))
        self.z0 = np.concatenate((x,v),axis=0)
        super().__init__(self.z0)
        
        m = np.array(masses,dtype=float)
        m = m.reshape(m.shape[0]*m.shape[1])
        ## Fuse the masses from the initial conditions of the
        ## cosimulation
        self.m = np.concatenate((np.array(m[0]).reshape(1),
                                 m[1:-1:2]+m[2:-1:2], np.array(m[-1]).reshape(1)), axis=0)
        self.im = 1./self.m
        self.n = self.im.shape[0]
        self.A  = gen_chain_A(masses, isprings)

         
    def doit(self, tend, h=0, z0=None, t0 = 0):
        if z0 is not None :
            if type(init)==type(list) or type(init) == type( self.isprings) and len(z0.shape) != 1:
                ### this is probably redundant: see __init__
                x = np.concatenate((np.array(z0[0][0]).reshape(1), z0[:,2]))
                v = np.concatenate((np.array(z0[0][1]).reshape(1), z0[:,3]))
                z0 = np.concatenate((x,v),axis=0)
            self.n = int(len(z0)/2)
            self.r.set_initial_value(z0, 0)
            self.z0 = z0
            
        self.t0 = t0
        super().doit(tend, t0=self.t0)

    

            

    def __call__(self, t, z):
        return self.A * z
if __name__ == "__main__":
    N = 3
    init = [[1,0,0,0]] + [[0,0,0,0]]*(N-1)
    masses = [[1,2]]*N
    isprings = [[1,0]]*N
    k = 3.0
    configref = {"N" : N, "init" : init, "masses" : masses, "isprings" : isprings, "k" : k }
    init = np.array(init)
    md5hashref = hashlib.md5(str(configref).encode("utf8"))
    
    
    eps = 1e-16            # machine precision
    min_p, max_p = get_min_max_period(masses, isprings)
    tend = 2*max_p
    
    NPP = 10
    step = min_p / NPP
    if True:
        s1 = spring_chain(init,masses, isprings)
        s0 = split_string(step, init,masses, isprings, k=k)
        s0.doit(tend, h=step)
        t0 = np.linspace(10, s0.t[-1], 200)
        s0.sample(t0)
        py.figure(1)
        py.clf()
        py.plot(t0, s0.xs)
        py.figure(2)
        py.clf()
        try:
            py.semilogy(t0, abs(s0.dxs)+eps)
            py.title("Log of coupling errors for split chain")
        except:
            pass
        py.show()
