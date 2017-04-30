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

## return a command line with the parameters of one of the gearboxes
def parameters(fmu_id, k_coupling, zeta_coupling, frequency, zeta,  w_in=0, filter_length=2, integrator=2, octave='false'):
  cmdline  = ['-p', '%i,octave_output,%s' % (fmu_id, octave)]
  cmdline += ['-p', '%i,octave_output_file,clutch3-%i-%i.m' % (fmu_id,fmu_id, filter_length) ]
  cmdline += ['-p', '%i,integrator,%i' % (fmu_id, integrator) ]
  cmdline += ['-p', '%i,filter_length,%i' % (fmu_id, filter_length) ]
#  cmdline += ['-p', '%i,v_in_e,%f' % (fmu_id, w_in) ]
  cmdline += ['-p', '%i,v0_e,0.0' % fmu_id ]
  cmdline += ['-p', '%i,x0_s,0.0' % fmu_id ]
  cmdline += ['-p', '%i,v0_s,0.0' % fmu_id ]
  cmdline += ['-p', '%i,k_ec,0.0' % fmu_id ]
  cmdline += ['-p', '%i,gamma_e,0' % fmu_id]
  cmdline += ['-p', '%i,gamma_s,0' % fmu_id ]
  cmdline += ['-p', '%i,k_ec,%f' % (fmu_id, k_coupling) ]
  cmdline += ['-p', '%i,gamma_ec,%f' % (fmu_id, ( 2.0 * zeta_coupling  * sqrt( k_coupling )))  ]
  cmdline += ['-p', '%i,gear_k,%f' % (fmu_id, ( ( 2.0 * pi * frequency )**2 / 4.0  )) ]
  cmdline += ['-p', '%i,gear_d,%f' % (fmu_id, (zeta * pi * frequency)) ]
  cmdline += ['-p', '%i,mass_e,0.5' % fmu_id]
  cmdline += ['-p', '%i,mass_s,0.5' % fmu_id]
  cmdline += ['-p', '%i,gear,13' % fmu_id]
  cmdline += ['-p', '%i,is_gearbox,true' % fmu_id]
  cmdline += ['-p', '%i,integrate_dx_e,true'  % fmu_id]
  cmdline += ['-p', '%i,integrate_dx_s,true' % fmu_id]
  return cmdline

def connect(fmu1, fmu2):
  cmdline  = ['-c', '%i,x_s,%i,x_in_e' % (fmu1, fmu2)]
  cmdline += ['-c', '%i,v_s,%i,v_in_e' % (fmu1, fmu2)]
  cmdline += ['-c', '%i,force_e,%i,force_in_s' % (fmu2, fmu1)]
  return cmdline
  
fmus = [ '../../gsl2/clutch3/clutch3.fmu' ]
fmus = ['../../gsl2/clutch3/clutch3.fmu', '../../impulse/impulse.fmu'  ]


def run_simulations(total_time, comm_step, filter_length,  frequency=1.0,
                    zeta=0.0, k_coupling=1e9, zeta_coupling=0.7,
                    w_in=1.0,
                    integrate_dx=True):

  nfmus = len(fmus)
  octave='true'
  integrator=5
  #cmdline = ['LD_PRELOAD=/usr/local/lib/valgrind/libmpiwrap-amd64-linux.so', '&&']
  cmdline = ['mpiexec', '-np', str(nfmus+1)]
  cmdline += ['fmigo-mpi', '-t', str(total_time), '-d', str(comm_step)]
  #             '/usr/local/bin/valgrind', '--tool=callgrind',
  
  cmdline += parameters(0, k_coupling, zeta_coupling, frequency, zeta,
                        w_in=w_in, filter_length=filter_length, integrator=integrator, octave=octave)

  #parameters for the impulse
  cmdline += ['-p', '1,pulse_type,4:1,pulse_start,1:1,pulse_length,500000:1,pulse_amplitude,1000']
# coupling impulse and fmu
  cmdline += ['-c', '1,omega,0,v_in_e' ] 

  cmdline += fmus

  filename = 'out-%i.csv' % filter_length 

  print(' '.join(cmdline) + " > " + filename, file=sys.stderr)
  return  subprocess.call(cmdline,
                          stdout=open('out-%i.csv' % filter_length, 'w')), cmdline

# forcing angular velocity

w_in = 1.0

# Total inertia of the module

J = 1.0

# internal frequency

frequency = 1

# internal coupling spring derived from frequency.  Note that there are two
# modes of motion for the coupled elements: DC and oscillatory.  Simple
# calculations show that if we want to keep total J and assign J/2 to each
# element, the spring and damping constants have to be as below so that the
# oscillations have the desired frequency.

k_internal = (2.0*pi*frequency)**2*J/4

# internal damping constant derived from dimensionless damping

zeta_internal = 0
d_internal =  zeta_internal * J * pi * frequency

# Coupling spring

k_coupling = 1e9

# damping: as above.  Note that the mass is explicitly set to one here
# meaning that the true non dimensional damping will be different
# The correct mass here should be
# ( 1/J1 + 1/J)**-1  where J1 is the coupled module. 
# so this should be
# d_coupling = 2 * zeta_coupling * sqrt(k_coupling/ ( 1/J1 + 1/J))

zeta_coupling = 0.7
d_coupling = 2.0 * zeta_coupling * sqrt(k_coupling)

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
                             2, frequency=frequency, zeta=zeta_internal,k_coupling=k_coupling,
                             zeta_coupling=zeta_coupling, w_in=w_in, integrate_dx=True)
  ret, cmd = run_simulations(NP/frequency, 1/samples/frequency,
                             0, frequency=frequency, zeta=zeta_internal,k_coupling=k_coupling,
                             zeta_coupling=zeta_coupling, w_in=w_in, integrate_dx=True)

def get_simdata(filter_length):
    t = []
    data = []
    couplings = []
    # Data layout: (for clutch 2, alpha2 is number of steps between comm steps)
    # phi  omega  alpha  phi1 omega1  alpha1 fc1    phi2 omega2 alpha2 fc2
    # 0     1      2      3    4       5      6      7    8      9      10
    # load data from fmigo simulation
    for row in csv.reader(open('out-%i.csv' % filter_length)):
        t.append(float(row[0]))
        data.append([ float(row[c]) for c in range( 1, len(row) ) ])
  
    data =  array(data)
    #data = data[:, 3:]              # discard pulse data
    t =  array(t)
    x = data[:,[0,4]]
    v = data[:,[1,5]]
    f = data[:,[3,7]]
    s = data[:,[6]]
    return t, x, v, f, s


def get_alldata(ts, filter_length, fmuid=1):
    # full simulation data from the FMU written by cgsl
    # Data layout:
    # 
    #phi1 omega1 phi1 omega1 (dphi1) (dphi2) zphi1 zomega1 zphi1 zomega1 (zdphi1) (zdphi2)
    #  0     1    2     3      4        5      6      7      8      9       10
    # 11

    alldata = []
    tt = []
    filename = 'clutch3-%i-%i.m' % (fmuid, filter_length)
    for row in csv.reader(open(filename), delimiter=' '):  
        tt.append(float(row[0]))      # time
        alldata.append([
            float(row[c]) for c in range( 1, len(row) ) 
        ])

        # convert to array to make sure we have a sliceable object
        
    alldata = array(alldata)
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
    return tt, x, z, xinterp, zinterp, dphioct

ts, xsim, vsim, fcsim, steps = get_simdata(filter_length)
tt, x, z, xinterp, zinterp, dphioct = get_alldata(ts, filter_length)

ts0, xsim0, vsim0, fcsim0, steps0 = get_simdata(0)
tt0, x0, z0, xinterp0, zinterp0, dphioct0 = get_alldata(ts, 0)
    
## now work with standard time integration.
## start everything at rest

r = integrate.ode(fun).set_initial_value([0.0]*12, 0)
r.set_f_params(w_in, k_coupling, d_coupling, k_internal, d_internal, J/2, J/2)

# number of samples per *internal* period.  If the filter works, the output
# should be nice that that time scale, even though there are transients
# inside.  Normally, we should sample at the highest frequency. 

## Now, integrate over comm step at a time
tprev = 0
# Use same times as fmigo

ys   = [array(list(r.y[0:6]) + [f_coupling(r.y[0:6], w_in, k_coupling, d_coupling)])]
zs   = [array(list(r.y[6:12]) + [0*f_coupling(r.y[6:12], w_in, k_coupling, d_coupling)])]
zs2  = [zs[0]]
r.set_integrator('lsoda')
for t in ts[1:]:
  # Reset z, and dphi, integrate
  r.set_initial_value([r.y[0], r.y[1], 0.0*r.y[2], r.y[3], r.y[4], 0.0*r.y[5]] + [0.0]*6, tprev)
  r.integrate(t)

  zz = r.y[6:12]/(t-tprev)

  fy = f_coupling(r.y, w_in, k_coupling, d_coupling)
  fz = f_coupling(zz,  w_in, k_coupling, d_coupling)

  # Augment results with coupling force
  ys.append(array(list(r.y[0:6]) + [fy]))
  zs.append(array(list(zz)       + [fz]))
  zs2.append((zs[-1] + zs[-1 if len(zs) == 1 else -2]) / 2)
  tprev = t

# lists if ndarrays aren't sliceable, so convert to array
ys = array(ys)
zs = array(zs2)
dphip = zs[:,-5];



figure(1)
clf()
subplot(311)
plot(ts, xsim, ts0, xsim0)
legend(['xe2', 'xs2', 'xe0', 'xs0'])
subplot(312)
plot(ts, vsim,ts0, vsim0)
legend(['ve2', 'vs2', 've0', 'vs0'])
subplot(313)
plot(ts, fcsim,ts0, fcsim0)
legend(['fe2', 'fs2', 'fe0', 'fs0'])
show()
