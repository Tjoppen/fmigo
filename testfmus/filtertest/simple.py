import sys
from scipy import ,interpolate
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


simulate = True
fmus = [ '../../gsl2/clutch2/clutch2.fmu' ]
fmus = [ '../../gsl2/clutch2/clutch2.fmu',
    '../../impulse/impulse.fmu'  ]



## return a command line with the parameters of one of the gearboxes
def parameters(fmu_id, k_coupling, zeta_coupling, frequency, zeta,  w_in=0, integrator=2, octave='false'):
  cmdline  = ['-p', '%i,octave_output,%s' % (fmu_id, octave)]
  cmdline += ['-p', '%i,octave_output_file,clutch2-%i-%i.dat' % (fmu_id,fmu_id) ]
  cmdline += ['-p', '%i,integrator,%i' % (fmu_id, integrator) ]
#  cmdline += ['-p', '%i,v_in_e,%f' % (fmu_id, w_in) ]
  cmdline += ['-p', '%i,v0_e,0.0' % fmu_id ]
  cmdline += ['-p', '%i,x0_s,0.0' % fmu_id ]
  cmdline += ['-p', '%i,v0_s,0.0' % fmu_id ]
  cmdline += ['-p', '%i,k_ec,0.0' % fmu_id ]
  cmdline += ['-p', '%i,gamma_e,0' % fmu_id]
  cmdline += ['-p', '%i,gamma_s,0' % fmu_id ]
  cmdline += ['-p', '%i,k_ec,%f' % (fmu_id, k_coupling) ]
  cmdline += ['-p', '%i,gamma_ec,%f' % (fmu_id, ( 2.0 * zeta_coupling  * sqrt( k_coupling )))  ]
  cmdline += ['-p', '%i,gear_k,%f' % (fmu_id, ( ( 2.0 * pi * frequency )**2 / 2.0  )) ]
  cmdline += ['-p', '%i,gear_d,%f' % (fmu_id, (zeta * pi * frequency)) ]
  cmdline += ['-p', '%i,mass_e,0.5' % fmu_id]
  cmdline += ['-p', '%i,mass_s,0.5' % fmu_id]
  cmdline += ['-p', '%i,gear,13' % fmu_id]
  cmdline += ['-p', '%i,is_gearbox,true' % fmu_id]
  cmdline += ['-p', '%i,integrate_dx_e,true'  % fmu_id]
  cmdline += ['-p', '%i,integrate_dx_s,false' % fmu_id]
  cmdline += ['-p', '%i,reset_dx_e,true'  % fmu_id]
  cmdline += ['-p', '%i,reset_dx_s,false' % fmu_id]
  return cmdline
  
def run_simulations(total_time, comm_step, frequency=1.0,
                    zeta=0.0, k_coupling=1e9, zeta_coupling=0.7,
                    w_in=1.0,
                    integrate_dx=True):

  nfmus = len(fmus)
  octave='true'
  integrator=4
  cmdline = ['mpiexec', '-np', str(nfmus+1)]
  cmdline += ['fmigo-mpi', '-t', str(total_time), '-d', str(comm_step)]
  cmdline += parameters(0, k_coupling, zeta_coupling, frequency, zeta,
                        w_in=w_in,  integrator=integrator, octave=octave)

  #parameters for the impulse
  cmdline += ['-p', '1,pulse_type,4:1,pulse_start,0:1,pulse_length,500000000000000:1,pulse_amplitude,1']
# coupling impulse and fmu
  cmdline += ['-c', '1,omega,0,v_in_e' ] 
  #cmdline += ['-c', '1,theta,0,x_in_e' ] 

  cmdline += fmus

  filename = 'out.csv' 

  print(' '.join(cmdline) + " > " + filename, file=sys.stderr)
  return  subprocess.call(cmdline,
                          stdout=open('out.csv','w')), cmdline

# forcing angular velocity
w_in = 1.0
# Total inertia of the module
J = 1.0
# internal frequency
frequency = 1
k_internal = (2.0*pi*frequency)**2*J/2
# internal damping constant derived from dimensionless damping
zeta_internal = 0*0.7 
d_internal =  zeta_internal * J * pi * frequency
# Coupling spring
k_coupling = 1e7
# damping: as above.  
zeta_coupling = 0.7
d_coupling = 2.0 * zeta_coupling * sqrt(k_coupling)

# simulation configuration

# number of samples per period, i.e., communication step
samples=10.0                    
# communication step
dt = 1/ samples / frequency
# number of slow periods to integrate
NP = 1

#Now run the FMIGo! simulation.  
if simulate:

  ret, cmd = run_simulations(NP/frequency, 1/samples/frequency,
                             2, frequency=frequency, zeta=zeta_internal,k_coupling=k_coupling,
                             zeta_coupling=zeta_coupling, w_in=w_in, integrate_dx=True)
  ret, cmd = run_simulations(NP/frequency, 1/samples/frequency,
                             0, frequency=frequency, zeta=zeta_internal,k_coupling=k_coupling,
                             zeta_coupling=zeta_coupling, w_in=w_in, integrate_dx=True)

def get_simdata():
    t = []
    data = []
    couplings = []
    # Data layout: (for clutch 2, alpha2 is number of steps between comm steps)
    # phi  omega  alpha  phi1 omega1  alpha1 fc1    phi2 omega2 alpha2 fc2
    # 0     1      2      3    4       5      6      7    8      9      10
    # load data from fmigo simulation
    for row in csv.reader(open('out-i.csv')):
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

ts, xsim, vsim, fcsim, steps = get_simdata()
