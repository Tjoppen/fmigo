import sys
from scipy import integrate,interpolate
from math import atan, sqrt, pi
from numpy import array
import csv
import subprocess
#import matplotlib.pyplot as plt
from pylab import *

def interp2( t, x, t1):
    nr = np.shape( x )[  0 ]    # number of rows
    if nr < np.shape( t1 )[0]:
      return x
    nc = 1
    if np.shape( np.shape( x ) )[0] > 1:
      nc = np.shape( x )[  1 ]
    if (nc > 1):
      y = np.zeros( ( np.shape( t1 )[0], nc ) )
      for i in range(0, nc):
        y[:, i] = interpolate.interp1d(t, x[:,i])(t1)
      return  y
    else:
      return interpolate.interp1d(t, x)(t1)

#compute coupling force as it applies to coupled module
def f_coupling(y, w_in, k1, d1):
  phi, w1, dphi1, phi2, w2, dphi2 = y[0:6]
  return k1*dphi1 + d1*(w1 - w_in)

simulate = True

# compute derivatives.  The order of the variables is:
# phi1      0
# w1        1
# dphi1     2
# phi2      3
# w2        4
# dphi2     5
# z_phi1    6
# z_w1      7
# z_dphi1   8
# z_phi2    9
# z_w2     10
# z_dphi2  11
def fun(t, y, w_in, k1, d1, k2, d2, J1, J2):
  phi1, w1, dphi1, phi2, w2, dphi2, = y[0:6]
  dphi = phi1- phi2
  return [
    w1,
    (- k2*dphi - d2*(w1 - w2) - f_coupling(y, w_in, k1, d1))/J1,
    w1 - w_in,
    w2,
    ( k2*dphi + d2*(w1 - w2))/J2,
    0,                          # shaft coupling deactivated
    phi1,
    w1,
    dphi1,
    phi2,
    w2,
    dphi2
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
  #cmdline = ['LD_PRELOAD=/usr/local/lib/valgrind/libmpiwrap-amd64-linux.so', '&&']
  cmdline = ['mpiexec', '-np', str(nfmus+1)]
#             '/usr/local/bin/valgrind', '--tool=callgrind',
  cmdline += ['fmigo-mpi', '-t', str(total_time), '-d', str(comm_step)]
  cmdline += ['-p', '0,octave_output,false']
  cmdline += ['-p', '0,filter_length,%i' % filter_length ]
  cmdline += ['-p', '0,v_in_e,%f' % v_in ]
  cmdline += ['-p', '0,v0_e,0.0' ]
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
  cmdline += ['-p', '0,integrate_dx_e,false'  ]
  cmdline += ['-p', '0,integrate_dx_s,false' ]
  cmdline += ['../../gsl2/clutch3/clutch3.fmu' ]
  
  filename = 'out-%i.csv' % filter_length 

  print(' '.join(cmdline) + " > " + filename, file=sys.stderr)
  return  subprocess.call(cmdline,
                          stdout=open('out-2.csv', 'w')), cmdline

# forcing angular velocity

w_in = 1.0

# Total inertia of the module

J = 1.0

# internal frequency

frequency = 5

# internal coupling spring derived from frequency.  Note that there are two
# modes of motion for the coupled elements: DC and oscillatory.  Simple
# calculations show that if we want to keep total J and assign J/2 to each
# element, the spring and damping constants have to be as below so that the
# oscillations have the desired frequency.

c_internal = (2.0*pi*frequency)**2*J/4

# internal damping constant derived from dimensionless damping

zeta_internal = 0
d_internal =  zeta_internal * J * pi * frequency

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

# simulation configuration

# number of samples per period, i.e., communication step

samples=10.0                    
filter_length=2

# communication step

dt = 1/ samples / frequency

# number of slow periods to integrate

NP = 4

#Now run the FMIGo! simulation.  
if simulate:
  ret, cmd = run_simulations(NP/frequency, 1/samples/frequency,
                             filter_length, frequency, zeta_internal)

ts = []
data = []
couplings = []
# Data layout
# phi1 omega1 alpha1 fc1 phi2 omega2 alpha2 fc2
# 0     1      2      3   4    5      6      7
# load data from fmigo simulation
for row in csv.reader(open('out-2.csv')):
  ts.append(float(row[0]))
  data.append([
    float(row[c]) for c in range( 1, len(row) )
  ])
  

# full simulation data from the FMU.
# Data layout:
# 
#phi1 omega1 phi1 omega1 (dphi1) (dphi2) zphi1 zomega1 zphi1 zomega1 (zdphi1) (zdphi2)
#  0     1    2     3      4        5      6      7      8      9       10       11

alldata = []
tt = []
for row in csv.reader(open('clutch3.m'), delimiter=' '):  
  tt.append(float(row[0]))      # time
  alldata.append([
    float(row[c]) for c in range( 1, len(row) -1 ) 
  ])

# convert to array to make sure we have a sliceable object

alldata = array(alldata)
data    = array(data)
ts      = array(ts)
tt      = array(tt)

# variables themselves

x = alldata[:, arange(0,6)]

# Averaged filtered values since that's not what is output in octave 

z       = vstack(( 0 * alldata[0,arange(6,12)],
                   0.5 * ( alldata[0:-1, arange(6,12)]  
                  +alldata[1:, arange(6,12)]  ) / dt))
# compute the averaged coupling force


# interpolate to sample points

xinterp = interp2(tt, x,  ts)
zinterp = interp2(tt, z,  ts)
# first we need dphi and this will be
dphioct = zinterp[:, -5]
## now work with standard time integration.
## start everything at rest

r = integrate.ode(fun).set_initial_value([0.0]*12, 0)
r.set_f_params(w_in, c_coupling, d_coupling, c_internal, d_internal, J/2, J/2)

# number of samples per *internal* period.  If the filter works, the output
# should be nice that that time scale, even though there are transients
# inside.  Normally, we should sample at the highest frequency. 

## Now, integrate over comm step at a time
tprev = 0
# Use same times as fmigo

ys   = [array(list(r.y[0:6]) + [f_coupling(r.y[0:6], w_in, c_coupling, d_coupling)])]
zs   = [array(list(r.y[6:12]) + [0*f_coupling(r.y[6:12], w_in, c_coupling, d_coupling)])]
zs2  = [zs[0]]
r.set_integrator('lsoda')
for t in ts[1:]:
  # Reset z, and dphi, integrate
  r.set_initial_value([r.y[0], r.y[1], 0.0*r.y[2], r.y[3], r.y[4], 0.0*r.y[5]] + [0.0]*6, tprev)
  r.integrate(t)

  zz = r.y[6:12]/(t-tprev)

  fy = f_coupling(r.y, w_in, c_coupling, d_coupling)
  fz = f_coupling(zz,  w_in, c_coupling, d_coupling)

  # Augment results with coupling force
  ys.append(array(list(r.y[0:6]) + [fy]))
  zs.append(array(list(zz)       + [fz]))
  zs2.append((zs[-1] + zs[-1 if len(zs) == 1 else -2]) / 2)
  tprev = t

# lists if ndarrays aren't sliceable, so convert to array
ys = array(ys)
zs = array(zs)
zs2 = array(zs2)
dphip = zs2[:,-5];



#figure(211)
clf()
title('Velocities')
semilogy(ts, abs(data[:,[1,5]]-zs2[:,[1,4]]), drawstyle='steps')
#legend(['go!', 'ode'])
legend(['w1', 'w2'] )

show()
