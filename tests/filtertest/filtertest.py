import sys
from scipy import integrate
from math import atan, sqrt, pi
from numpy import array
import csv
import subprocess
import matplotlib.pyplot as plt

simulate=True

#compute coupling force as it applies to coupled module
def f_coupling(y, w_in, k1, d1):
  dphi1, w1, dphi2, w2 = y[0:4]
  return k1*dphi1 + d1*(w1 - w_in)


# compute derivatives.  The order of the variables is:
# dphi1
# w1_dot
# dphi2
# w2_dot
# z1_dot
# z2_dot
def fun(t, y, w_in, k1, d1, k2, d2, J1, J2):
  dphi1, w1, dphi2, w2 = y[0:4]
  return [
    w1 - w_in,
    (- k2*dphi2 - d2*(w1 - w2) - f_coupling(y, w_in, k1, d1))/J1,
    w1 - w2,
    (  k2*dphi2 + d2*(w1 - w2))/J2,
    dphi1,
    w1,
    dphi2,
    w2,
  ]

##
## There's only one mass in the system so its value is 1
## Likewise, there's only one unit of length so the forcing velocity is 1
## zeta for the coupling is best left alone, though magnitude of coupling
## spring should be varied for investigation.  However, it should be at
## least 1000 larger than the internal spring.  That keeps the two
## frequencies in a ratio of 30:1 at least which is enough for the coupling
## dynamics to interfere minimally. 
##
## The coupling spring defines a fundamental unit of time so we set this to
## 1.
##
## Since we are investigating the effect of coupling, we are primarily
## interested in the case where zeta = 0 for the internal spring so we can
## see the effect of the filter more clearly. 
## 
## Total integration time should be kept to about 2/f i.e., 2 periods
##
## That leaves the sampling time which we would like to keep at 1/(10*f) or
## so to capture the dynamics of the system.  The question here is whether
## or not we can do this because of the filter dynamics
##
##
def run_simulations(total_time, comm_step, filter_length,  frequency=1.0,
                    zeta=0.0, k_coupling=1e9, zeta_coupling=0.7,
                    v_in=1.0,
                    integrate_dx=True):
  nfmus = 1
  cmdline = ['mpiexec', '-np', str(nfmus+1), 'fmigo-mpi', '-t',
             str(total_time), '-d', str(comm_step)]
  cmdline += ['-p', '0,filter_length,%i' % filter_length ]
  #cmdline += ['-p', '0,v_in_e,%f' % v_in ]
  cmdline += ['-p', '0,v0_e,1.0' ]
  cmdline += ['-p', '0,gamma_e,0.0' ]
  cmdline += ['-p', '0,gamma_s,0.0' ]
  cmdline += ['-p', '0,k_ec,%f' % k_coupling ]
  cmdline += ['-p', '0,gamma_ec,%f' % ( 2.0 * zeta_coupling  * sqrt( k_coupling ))  ]
  cmdline += ['-p', '0,gear_k,%f' % ( ( 2.0 * pi * frequency )**2 / 4.0  ) ]
  cmdline += ['-p', '0,gear_d,%f' % (zeta * pi * frequency) ]
  cmdline += ['-p', '0,mass_e,0.5' ]
  cmdline += ['-p', '0,mass_s,0.5' ]
  cmdline += ['-p', '0,gear,13' ]
  cmdline += ['-p', '0,is_gearbox,true' ]
  cmdline += ['-p', '0,integrate_dx_e,true'  ]
  cmdline += ['-p', '0,integrate_dx_s,true' ]
  cmdline += ['../../gsl2/clutch2/clutch2.fmu' ]
  
  filename = 'out-%i.csv' % filter_length 

  print(' '.join(cmdline) + " > " + filename, file=sys.stderr)
  return  subprocess.call(cmdline, stdout=open(filename, 'w')), cmdline




# forcing angular velocity
w_in = 1.0
# Inertia tensor
J = 1.0
# internal frequency
f = 1
# internal coupling spring derived from frequency.  Note that there are two
# modes of motion for the coupled elements: DC and oscillatory.  Simple
# calculations show that if we want to keep total J and assign J/2 to each
# element, the spring and damping constants have to be as below so that the
# oscillations have the desired frequency.
c_internal = (2.0*pi*f)**2*J/4
# internal damping constant derived from dimensionless damping
zeta_internal = 0.7
d_internal =  zeta_internal * J * pi * f
# Coupling spring
c_coupling = 1e9
# damping: as above.  Note that the mass is explicitly set to one here
# meaning that the true non dimensional damping will be different
# The correct mass here should be
# ( 1/J1 + 1/J)**-1  where J1 is the coupled module. 
# so this should be
# d_coupling = 2 * zeta_coupling * sqrt(c_coupling/ ( 1/J1 + 1/J))

zeta_coupling = 0.7
d_coupling = 2.0 * zeta_coupling * sqrt(c_coupling)

r = integrate.ode(fun).set_initial_value([0.0]*8, 0)
r.set_f_params(w_in, c_coupling, d_coupling, c_internal, d_internal, J/2, J/2)

# number of samples per *internal* period.  If the filter works, the output
# should be nice that that time scale, even though there are transients
# inside.  Normally, we should sample at the highest frequency. 

samples=100.0
filter_length=2

#Now run the FMIGo! simulation.  Integrate over two full periods of the
#slow oscillator
ret, cmd = run_simulations(2/f, 1/samples/f, filter_length, f, 0, 0, 0, 0, False)

ts = []
data = []
alldata = []

# load data from fmigo simulation
for row in csv.reader(open('out-2.csv')):
  ts.append(float(row[0]))
  data.append([
    float(row[c]) for c in [0,2,6,4]
  ])
  alldata.append( row[1:-1])

data    = array(data)
alldata = array(alldata)

ys   = [array(list(r.y[0:4]) + [f_coupling(r.y[0:4], w_in, c_coupling, d_coupling)])]
zs   = [array(list(r.y[4:8]) + [f_coupling(r.y[4:8], w_in, c_coupling, d_coupling)])]
zs2  = [zs[0]]

tprev = 0
# Use same times as fmigo
for t in ts[1:]:
  # Reset z, integrate
  r.set_initial_value([r.y[0], r.y[1], r.y[2], r.y[3]] + [0.0]*4, tprev)
  r.integrate(t)

  zz = r.y[4:8]/(t-tprev)

  fy = f_coupling(r.y, w_in, c_coupling, d_coupling)
  fz = f_coupling(zz,  w_in, c_coupling, d_coupling)

  # Augment results with coupling force
  ys.append(array(list(r.y[0:4]) + [fy]))
  zs.append(array(list(zz)       + [fz]))
  zs2.append((zs[-1] + zs[-1 if len(zs) == 1 else -2]) / 2)
  tprev = t

# lists if ndarrays aren't sliceable, so convert to array
ys = array(ys)
zs = array(zs)
zs2 = array(zs2)

#plot(ts, ys, ts, zs, ts, zs2)

#args = ['mpiexec', 'fmigo-mpi', '-np', '2', 
#s = subprocess.check_output(args)
plt.subplot(221)
plt.title('Velocities')
plt.plot(ts, data[:,[1,2]])
#plt.plot(ts, ys[:,[1,3]])
#plt.plot(ts, zs[:,[1,3]])
plt.plot(ts, zs2[:,[1,3]])

plt.subplot(222)
plt.title('Velocity diffs')
#plt.semilogy(ts, abs(data[:,[1,2]] - ys[:,[1,3]]))
plt.semilogy(ts, abs(data[:,[1,2]] - zs2[:,[1,3]]))

plt.subplot(223)
plt.title('Coupling torques')
plt.plot(ts, data[:,3])
#plt.plot(ts, ys[:,4])
plt.plot(ts, zs2[:,4])
plt.legend(['reference','fmu'])

plt.subplot(224)
plt.title('Coupling torque diff')
#plt.semilogy(ts, abs(data[:,3] - ys[:,4]))
plt.semilogy(ts, abs(data[:,3] - zs2[:,4]))

plt.show()

