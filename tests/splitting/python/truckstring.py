# -*- coding: utf-8 -*-
# Based on costring.py
from pylab import *
from scipy import *
from scipy.sparse import *
from scipy.sparse.linalg  import *
from scipy.sparse.csc  import *
from scipy.integrate  import *
#from scipy.linalg import expm
import cmath

import os
import subprocess
import io
import sys
import json

from costring import steps_functor, intloop2, intloop3, uniquelabels

if 'FMUS_PATH' in os.environ:
  CLUTCH2_PATH = os.path.join(os.environ['FMUS_PATH'], 'gsl/clutch/clutch.fmu')
  SPRINGS2_PATH =os.path.join(os.environ['FMUS_PATH'], 'springs2_wrapped_filter.fmu')
else:
  CLUTCH2_PATH = '../../../build/tests/umit-fmus/gsl/clutch/clutch.fmu'
  SPRINGS2_PATH ='../../../build/springs2_wrapped_filter.fmu'

# Approximate truck model with string of springs and masses
# Last mass is much larger than the others
# m = 1 1 1   10000
# k =  1 1 100
k_force_velocity  = 0.25
k_epce  = 3
relaxation = 4.0
compliance = 0.0

# Split masses
splitms= [
  1.0,  1.0,
  1.0,  2.0,
  2.0,  4.0,
  4.0
]

masses = [
  splitms[0],
  splitms[1]+splitms[2],
  splitms[3]+splitms[4],
  splitms[5]+splitms[6]
]
ks     = [1,  2,  4]

M = diag([1.0]*4 + [1.0/m for m in masses])

Aref = M* matrix([
  [0,0,0,0,  1,0,0,0],
  [0,0,0,0,  0,1,0,0],
  [0,0,0,0,  0,0,1,0],
  [0,0,0,0,  0,0,0,1],

  [-ks[0], ks[0]      ,           0,     0, 0, 0, 0, 0],
  [ ks[0],-ks[0]-ks[1], ks[1]      ,     0, 0, 0, 0, 0],
  [     0,       ks[1],-ks[1]-ks[2], ks[2], 0, 0, 0, 0],
  [     0,           0,       ks[2],-ks[2], 0, 0, 0, 0],
], dtype='float')

# Excite the leftmost part of the system
z0 = matrix([[0,0,0,0,1,0,0,0]], dtype='float').transpose()

# slowest mode of interest
w = sort(abs(imag(eig(Aref)[0])))[2]
# fastest mode of interest
W = max(abs(imag(eig(Aref)[0])))
print('w = %g, W = %g' % (w, W))
#print(w)
P = 2*pi/w
NP = 0.5    # Number of periods
tend = P*NP

# force = zb
def fun(t, z, force, i):
  Finternal = matrix([
    [0,0,1,0],
    [0,0,0,1],
    [-ks[i], ks[i],0,0],
    [ ks[i],-ks[i],0,0],
  ], dtype='float')

  zz = matrix(z).transpose()
  ftot = Finternal*zz
  if not force is None:
    ftot += force.astype(ftot.dtype)
  return matrix(diag([1,1] + splitms[(i*2):(i*2+2)]))**-1 * ftot

def fun0(t, z, force):
  return fun(t, z, force, 0)

def fun1(t, z, force):
  return fun(t, z, force, 1)

def fun2(t, z, force):
  return fun(t, z, force, 2)

def fun3(t, z, force):
  Finternal = matrix([
    [0,1],
    [0,0],
  ], dtype='float')

  zz = matrix(z).transpose()
  ftot = Finternal*zz
  if not force is None:
    ftot += force.astype(ftot.dtype)
  return matrix(diag([1, splitms[6]]))**-1 * ftot

# Permutes so we get all seven positions in a row, and all seven velocities after that
perm = [0,1,4,5,8,9,12,2,3,6,7,10,11,13]

def kin_exchange(tprev, t2, zs, state):
  global g, gdot, gdotf, S, H, rhs, f

  Hmob      = state[0]
  forces    = state[1]
  holonomic = state[2]

  H = t2 - tprev
  a = 4.0/(H*(1+4*relaxation))
  b = 1.0/(1+4*relaxation)

  # Get volation and constraint velocity
  g = matrix(row_stack([
    zs[0][1] - zs[1][0],
    zs[1][1] - zs[2][0],
    zs[2][1] - zs[3][0],
  ]))

  gdot = matrix(row_stack([
    zs[0][3] - zs[1][2],
    zs[1][3] - zs[2][2],
    zs[2][3] - zs[3][1],
  ]))

  def force_only(tpref, t2, zs, state):
    return state, 0

  # Get future constraint velocity
  #ts, zf, steps = intloop2(zs, (fun0, fun1, fun2, fun3), exchange=None, state=None, H=H, tstart=tprev, tend=tprev+H, perm=perm)
  ts, zf, steps = intloop3([tprev, t2], zs, (fun0, fun1, fun2, fun3), exchange=force_only, state=forces, perm=perm)
  gdotf = (zf[-1, [8,10,12]] - zf[-1, [9,11,13]]).transpose()

  if holonomic:
    # Holonomic
    # Converges at roughly O(h^3.38)
    rhs = -a*g + b*gdot - gdotf
  else:
    # Non-holonomic
    # Converges at roughly O(h^2.34)
    rhs = - gdotf

  S = zeros((3,3))

  if Hmob == 0:
    # No off-diagonals
    S[0,0] = 1/splitms[1] + 1/splitms[2]
    S[1,1] = 1/splitms[3] + 1/splitms[4]
    S[2,2] = 1/splitms[5] + 1/splitms[6]
  else:
    print('Not implemented')
    sys.exit(1)

  # Compute forces
  f = (np.linalg.inv(S)*rhs) / H

  fs = [
    matrix([0,      0,    0, f[0]]).transpose(),
    matrix([0,      0,-f[0], f[1]]).transpose(),
    matrix([0,      0,-f[1], f[2]]).transpose(),
    matrix([0, -f[2]]             ).transpose(),
  ]

  if forces is None:
    state[1] = fs
  else:
    state[1] = [forces[i] + fs[i] for i in range(len(forces))]

  #print('rhs = % .8f, % .8f, % .8f -> df = % .8f, % .8f, % .8f -> f = % .8f, % .8f, % .8f' % (
  #  rhs[0,0], rhs[1,0], rhs[2,0],
  #  f[0,0], f[1,0], f[2,0],
  #  state[1][0][3,0], state[1][1][3,0], state[1][2][3,0]
  #))

  return (state[1], steps)


def force_signals(dt, vbar, z, i, k):
  """ signal evaluation common to epce and fv"""
  z = matrix(z).reshape((7,1))
  x1, x2, v1, v2, dx, zv, zf = (z[i,0] for i in range(7))
  m1 = splitms[2*i+0]
  m2 = splitms[2*i+1]
  mu = m1*m2/(m1+m2)

  # c,d = coupling spring parameters
  if dt > 0:
    # Per Edo Drenth's recommendation
    c = mu*(k*2*math.pi/dt)**2
    d = 2*0.7*math.sqrt(c*mu)
  else:
    c = 0
    d = 0
  fc = c*dx + d*(v2-vbar) if not vbar is None else 0
  return  fc

  
# layout: x1, x2, v1, v2, dx, zv, zf
# where dx is the integral of v2-vbar
# zv is the integral of v1
# and zf is the integral of the force on the x2,v2 side


def fun_signals(t, z, inputs, i, k):
  m1 = splitms[2*i+0]
  m2 = splitms[2*i+1]
  vbar, force, dt = inputs
  z = matrix(z).reshape((7,1))
  x1, x2, v1, v2, dx, zv, zf = (z[i,0] for i in range(7))

  fc = force_signals(dt, vbar, z, i, k)

  return matrix([
    v1,
    v2,
    1.0/m1*(-ks[i]*(x1-x2) + force),
    1.0/m2*(+ks[i]*(x1-x2) - fc),
    v2 - vbar if not vbar is None else 0,
    v1,
    fc,
  ]).transpose()

def fun_fv(t, z, inputs, i):
  return fun_signals(t, z, inputs, i, k=k_force_velocity)

def fun_epce(t, z, inputs, i):
  return fun_signals(t, z, inputs, i, k=k_epce)

def fun0_epce(t, z, inputs):
  return fun_epce(t, z, inputs, 0)
def fun1_epce(t, z, inputs):
  return fun_epce(t, z, inputs, 1)
def fun2_epce(t, z, inputs):
  return fun_epce(t, z, inputs, 2)

def fun0_fv(t, z, inputs):
  return fun_fv(t, z, inputs, 0)
def fun1_fv(t, z, inputs):
  return fun_fv(t, z, inputs, 1)
def fun2_fv(t, z, inputs):
  return fun_fv(t, z, inputs, 2)
# x,v,zv

def fun3_signals(t, z, force):
  z = matrix(z).reshape((3,1))
  x, v, zv = (z[i,0] for i in range(3))
  asd = [
    v,
    force / splitms[6] if not force is None else 0.0,
    v,
  ]
  return matrix(asd).transpose()

class epce_state:
  def __init__(self):
    self.zlog = [None]*4
    self.zbar = [0,0,0]
  def add_zbs(self, zbs):
    for i in range(4):
      self.zlog[i] = column_stack([self.zlog[i], matrix(zbs[i])]) if not self.zlog[i] is None else matrix(zbs[i])
      if self.zlog[i].shape[1] > 2:
        self.zlog[i] = self.zlog[i][:,1:]
  def get_filtered(self):
    if self.zlog[0] is None:
      return None
    else:
      return [mean(self.zlog[i], 1) for i in range(4)]

class fv_state:
  def __init__(self):
    self.zbar = [0,0,0]


def epce_exchange(tprev, t2, zs, state):
  # Input to fun0..2 is vbar,force,dt
  # Input to fun3 is just force
  dt = t2 - tprev

  if tprev > 0.0:
    state.add_zbs([
      zs[0][5:7,0] / dt,
      zs[1][5:7,0] / dt,
      zs[2][5:7,0] / dt,
      zs[3][2:3,0] / dt,
    ])

  # Reset integrals
  zs[0][4:7,0] *= 0
  zs[1][4:7,0] *= 0
  zs[2][4:7,0] *= 0
  zs[3][2:3,0] *= 0

  filt = state.get_filtered()

  if filt is None:
    inputs = [(None, 0, dt), (None, 0, dt), (None, 0, dt), 0]
  else:
    inputs = [
      (filt[1][0,0], 0, dt),
      (filt[2][0,0], filt[0][1,0], dt),
      (filt[3][0,0], filt[1][1,0], dt),
      filt[2][1,0],
    ]

  return (inputs, 0)


def fv_exchange(tprev, t2, zs, state):
  # Input to fun0..2 is vbar,force,dt
  # Input to fun3 is just force
  dt = t2 - tprev


  fc1 = force_signals(dt, state.zbar[0], zs[0],  0, k_force_velocity)
  fc2 = force_signals(dt, state.zbar[1], zs[1], 1, k_force_velocity)
  fc3 = force_signals(dt, state.zbar[2], zs[2],  2, k_force_velocity)

  # Reset integrals
  zs[0][4:7,0] *= 0
  zs[1][4:7,0] *= 0
  zs[2][4:7,0] *= 0
  zs[3][2:3,0] *= 0 
 
  state.zbar = [
    zs[1][0,0],
    zs[2][0,0],
    zs[3][0,0]]
  inputs = [
    (zs[1][0,0], 0, dt),
    (zs[2][0,0], fc1, dt),
    (zs[3][0,0], fc2, dt),
    fc3,
  ]

  return (inputs, 0)
test_mode = sys.argv[1] == '--test' if len(sys.argv) > 1 else False
presentation = False

if not test_mode:
  from mpl_toolkits.mplot3d import Axes3D
  import matplotlib.pyplot as plt
  from matplotlib import cm
  from matplotlib.ticker import LinearLocator, FormatStrFormatter

results = {}

octaves = 6
if test_mode:
  npps = [40]
elif presentation:
  npps = [int(10*2**i) for i in range(4)]
else: #convergence plots
  npps = [int(8*2**i) for i in range(octaves)]

hs = []
# Mean square errors
holonomic_rms_errs = []
nonholonomic_rms_errs = []
holonomic_fmigo_rms_errs = []
nonholonomic_fmigo_rms_errs = []
epce_rms_errs = []
fv_rms_errs = []

data = {
  "holonomic_rms_errs":  holonomic_rms_errs ,
  "nonholonomic_rms_errs" :  nonholonomic_rms_errs ,
  "holonomic_fmigo_rms_errs" :  holonomic_fmigo_rms_errs ,
  "nonholonomic_fmigo_rms_errs" :  nonholonomic_fmigo_rms_errs ,
  "epce_rms_errs" :  epce_rms_errs , 
  "fv_rms_errs" :  fv_rms_errs , 
  "npps" : [], 
  "hs" : hs}

cmdline_start = ['mpiexec']

for npp in npps:
 data["npps"].append(npp)
 results[npp] = {}
 h = 2*pi/W / npp
 hs.append(h)
 Aphi = expm(h*Aref)
 for holonomic in [True, False]:
  # Run fmigo for the last one of them
  cmdline = cmdline_start + [
    '-np', '1', 'fmigo-mpi','-t',str(tend),'-d',str(h)
  ]

  if not holonomic:
    cmdline += ['-N']
    
  rtol = str(1e-10)
  atol = str(1e-10)
  integrator = 4
  cmdline += [
    '-p', '0,integrator,%i:0,reltol,%s:0,abstol,%s:0,x0_e,%g:0,v0_e,%g:0,x0_s,%g:0,v0_s,%g:0,mass_e,%g:0,gamma_e,0:0,mass_s,%g:0,gamma_s,0:0,is_gearbox,true:0,gear_k,%g:0,gear_d,0:0,gear,13:0,octave_output_file,""'
      % (integrator,rtol, atol, z0[0], z0[4], z0[1], z0[5], splitms[0], splitms[1], ks[0]),
    '-p', '1,integrator,%i:1,reltol,%s:1,abstol,%s:1,x0_e,%g:1,v0_e,%g:1,x0_s,%g:1,v0_s,%g:1,mass_e,%g:1,gamma_e,0:1,mass_s,%g:1,gamma_s,0:1,is_gearbox,true:1,gear_k,%g:1,gear_d,0:1,gear,13:1,octave_output_file,""'
      % (integrator,rtol, atol, z0[1], z0[5], z0[2], z0[6], splitms[2], splitms[3], ks[1]),
    '-p', '2,integrator,%i:2,reltol,%s:2,abstol,%s:2,x0_e,%g:2,v0_e,%g:2,x0_s,%g:2,v0_s,%g:2,mass_e,%g:2,gamma_e,0:2,mass_s,%g:2,gamma_s,0:2,is_gearbox,true:2,gear_k,%g:2,gear_d,0:2,gear,13:2,octave_output_file,""'
      % (integrator,rtol, atol, z0[2], z0[6], z0[3], z0[7], splitms[4], splitms[5], ks[2]),
    '-p', '3,integrator,%i:3,reltol,%s:3,abstol,%s:3,x0_e,%g:3,v0_e,%g:3,mass_e,%g:3,gamma_e,0:3,is_gearbox,true:3,gear_k,0:3,gear_d,0:3,gear,0:3,octave_output_file,""'
      % (integrator,rtol, atol, z0[3], z0[7], splitms[6]),
    '-C', 'shaft,0,1,x_s,v_s,a_s,force_in_s,x_e,v_e,a_e,force_in_e',
    '-C', 'shaft,1,2,x_s,v_s,a_s,force_in_s,x_e,v_e,a_e,force_in_e',
    '-C', 'shaft,2,3,x_s,v_s,a_s,force_in_s,x_e,v_e,a_e,force_in_e',
    ':', '-np', '4', 'fmigo-mpi', CLUTCH2_PATH
  ]
  #print(' '.join(cmdline))
  print('holonomic = ' + str(holonomic) + ', npp = ' + str(npp) + ': fmigo')
  s = subprocess.check_output(cmdline)
  d = genfromtxt(io.BytesIO(s), delimiter=',')
  if holonomic:
    d_h = d
  else:
    d_n = d

  tref = d[:,0]

  zref = row_stack([( matrix_power(Aphi,i) * z0).transpose() for i in range(len(tref))])

  print('holonomic = ' + str(holonomic) + ', npp = ' + str(npp) + ': simulation')
  ts, zs, steps = intloop3(
    tref,
    [
      z0[[0,1,4,5]],
      z0[[1,2,5,6]],
      z0[[2,3,6,7]],
      z0[[3,7]],
    ],
    (fun0, fun1, fun2, fun3),
    kin_exchange,

    state  = [0, None, holonomic],   # Hmob, force accumulators, holonomic
    perm   = perm
  )
  results[npp]['reference'] = (tref, zref)
  results[npp]['holonomic' if holonomic else 'nonholonomic'] = (ts, zs)

  # Compare both sides of each constraint with the reference
  # Hence the duplicated indices in the list, like 1,1, 2,2, and so on
  zref2 = zref[:,[0,1,1,2,2,3,3,4,5,5,6,6,7,7]]

  # Differences as vector
  dkin = sqrt(mean(asarray(zref2 - zs).reshape(-1)**2))
  # Kinematic violations for the split system:
  #  | (1)  (2a) | - |   (2b)  (3a)  | - | (3b)  (4a) |  - | (4b) |
  
  dkx  = sqrt( mean( asarray(zs[:, arange(1,7,2)] - zs[:, arange(2,7,2)]).reshape(-1)**2 ))
  dkv  = sqrt( mean( asarray(zs[:, arange(8,14,2)] - zs[:, arange(9,14,2)]).reshape(-1)**2 ))

  if holonomic:
    holonomic_rms_errs.append([dkx,dkv, dkin])
  else:
    nonholonomic_rms_errs.append([dkx,dkv,dkin])

  zkingo  = d[:,[1,5,9,13,17,21,25, 2,6,10,14,18,22,26]]
  dkin = sqrt(mean(asarray(zref2 - zkingo).reshape(-1)**2))

  dkx  = sqrt( mean( asarray(zkingo[:, arange(1,7,2)] -  zkingo[:, arange(2,7,2)]).reshape(-1)**2 ))
  dkv  = sqrt( mean( asarray(zkingo[:, arange(8,14,2)] - zkingo[:, arange(9,14,2)]).reshape(-1)**2 ))

  if holonomic:
    holonomic_fmigo_rms_errs.append([dkx, dkv,dkin])
  else:
    nonholonomic_fmigo_rms_errs.append([dkx, dkv,dkin])

  # Sum of absolute differences for the test mode
  err = sum(abs(zref2 - zs), 1)
  err2 = sum(abs(zref2 - d[:,[1,5,9,13,17,21,25, 2,6,10,14,18,22,26]]), 1)

  max1 = max(err)[0,0]
  max2 = max(err2)[0,0]

  if test_mode:
    if max1 > 0.11 or max2 > 0.1:
      print('truckstring failed. max1 = %f, max2 = %f' % (max1, max2))
      print('kinematic solver likely broke')
      sys.exit(1)
  elif presentation:
    plt.figure(1 if holonomic else 3)
    plt.semilogy(ts, err,  'x-', label='npp = %i, steps = %i' % (npp, steps))
    plt.figure(2 if holonomic else 4)
    plt.semilogy(ts, err2, 'x-', label='npp = %i' % (npp,))

 # need zse for test mode and non-test mode
 print('npp = ' + str(npp) + ': epce')
 tse, zse, stepse = intloop3(
   tref,
   [
     row_stack([z0[[0,1,4,5]], zeros((3,1))]),
     row_stack([z0[[1,2,5,6]], zeros((3,1))]),
     row_stack([z0[[2,3,6,7]], zeros((3,1))]),
     row_stack([z0[[3,7]],     zeros((1,1))]),
   ],
   (fun0_epce, fun1_epce, fun2_epce, fun3_signals),
   epce_exchange,
   state=epce_state(),
   perm = [0,1,4+3,5+3,8+6,9+6,12+9,    2,3,6+3,7+3,10+6,11+6,13+9]
  )

 if not test_mode:
  results[npp]['epce'] = (tse, zse)
  # Differences as vector
  depce = sqrt(mean(asarray(zref2 - zse).reshape(-1)**2))

  dkx  = sqrt( mean( asarray(zse[:, arange(1,7,2)] -  zse[:, arange(2,7,2)]).reshape(-1)**2 ))
  dkv  = sqrt( mean( asarray(zse[:, arange(8,14,2)] - zse[:, arange(9,14,2)]).reshape(-1)**2 ))

  
  epce_rms_errs.append([dkx, dkv, depce])
  if presentation:
    erre = sum(abs(zref2 - zse), 1)
    plt.figure(5)
    plt.semilogy(tse, erre, 'x-', label='npp = %i, steps = %i' % (npp, stepse))

  print('npp = ' + str(npp) + ': fv')
  ## overwrite the data from the epce experiment: no problem, thrown away
  ## anyway
  tsfv, zsfv, stepsfv = intloop3(
    tref,
    [
      row_stack([z0[[0,1,4,5]], zeros((3,1))]),
      row_stack([z0[[1,2,5,6]], zeros((3,1))]),
      row_stack([z0[[2,3,6,7]], zeros((3,1))]),
      row_stack([z0[[3,7]],     zeros((1,1))]),
    ],
    (fun0_fv, fun1_fv, fun2_fv, fun3_signals), fv_exchange,
    state=fv_state(),
    perm = [0,1,4+3,5+3,8+6,9+6,12+9,    2,3,6+3,7+3,10+6,11+6,13+9]
  )
  # Differences as vector
  dfv = sqrt(mean(asarray(zref2 - zsfv).reshape(-1)**2))
  dkx  = sqrt( mean( asarray(zse[:, arange(1,7,2)] -  zsfv[:, arange(2,7,2)]).reshape(-1)**2 ))
  dkv  = sqrt( mean( asarray(zse[:, arange(8,14,2)] - zsfv[:, arange(9,14,2)]).reshape(-1)**2 ))
  fv_rms_errs.append([dkx, dkv,dfv])

  # incrementally store to file.
  f = open('data-new.json', "w")
  print("storing to json")
  json.dump(data, f)
  f.close() 

 if test_mode:
  cmdline = cmdline_start + [
    '-np', '1', 'fmigo-mpi','-t',str(tend),'-d',str(h)
  ]

  #TODO: rtol and atol for wrapper
  #overestimate spring, should be fine
  print('**** RUNNING EPCE ME SPRINGS2 THINGY ****')
  cs = []
  ds = []
  k = k_epce
  for i in range(3):
    m1 = splitms[2*i+0]
    m2 = splitms[2*i+1]
    mu = m1*m2/(m1+m2)

    # Per Edo Drenth's recommendation
    c = mu*(k*2*math.pi/h)**2
    d = 2*0.7*math.sqrt(c*mu)
    cs.append(c)
    ds.append(d)

  rtol = str(1e-10)
  atol = str(1e-10)
  integrator = 4
  cmdline += [
    #EPCE via wrapped ME springs2 FMU
    '-p', '0,integrator,%i:0,x1,%g:0,v1,%g:0,x2,%g:0,v2,%g:0,m1,%g:0,m2,%g:0,k_internal,%g:0,gamma_internal,0:0,k1,0:0,gamma1,0:0,k2,%g:0,gamma2,%g'
      % (integrator, z0[0], z0[4], z0[1], z0[5], splitms[0], splitms[1], ks[0], cs[0], ds[0]),
    '-p', '1,integrator,%i:1,x1,%g:1,v1,%g:1,x2,%g:1,v2,%g:1,m1,%g:1,m2,%g:1,k_internal,%g:1,gamma_internal,0:1,k1,0:1,gamma1,0:1,k2,%g:1,gamma2,%g'
      % (integrator, z0[1], z0[5], z0[2], z0[6], splitms[2], splitms[3], ks[1], cs[1], ds[1]),
    '-p', '2,integrator,%i:2,x1,%g:2,v1,%g:2,x2,%g:2,v2,%g:2,m1,%g:2,m2,%g:2,k_internal,%g:2,gamma_internal,0:2,k1,0:2,gamma1,0:2,k2,%g:2,gamma2,%g'
      % (integrator, z0[2], z0[6], z0[3], z0[7], splitms[4], splitms[5], ks[2], cs[2], ds[2]),
    '-p', '3,integrator,%i:3,x1,%g:3,v1,%g:3,m1,%g'
      % (integrator, z0[3], z0[7], splitms[6]),
    '-c', '0,fc2,1,f1:1,v1,0,v2_i',
    '-c', '1,fc2,2,f1:2,v1,1,v2_i',
    '-c', '2,fc2,3,f1:3,v1,2,v2_i',
    ':', '-np', '4', 'fmigo-mpi', SPRINGS2_PATH
  ]
  #print(' '.join(cmdline))
  print('holonomic = ' + str(holonomic) + ', npp = ' + str(npp) + ': fmigo')
  s = subprocess.check_output(cmdline)
  d_epce = genfromtxt(io.BytesIO(s), delimiter=',')
  tepcego = d_epce[:,0]
  zepcego = d_epce[:,[1,6,11,16,21,26,31,  2,7,12,17,22,27,32]]
  maxdiff = np.max(abs(zepcego - zse))
  if maxdiff > 0.1:
    print('truckstring failed. zepcego maxdiff = %f' % maxdiff)
    print('wrapper likely broke')
    sys.exit(1)

if presentation and not test_mode:
  for f in [1,2,3,4,5]:
    plt.figure(f)
    plt.legend(loc='lower right')
    plt.axis((0,ts[-1],1e-5,100))
    plt.title(
      [
        'Error, simulation vs reference (non-holonomic)',
        'Error, fmigo vs reference (non-holonomic)',
        'Error, simulation vs reference (holonomic)',
        'Error, fmigo vs reference (holonomic)',
        'Error, epce vs reference',
      ][f-1]
    )

  plt.figure(6)
  plt.plot(ts, zs[:,7:14], 'r-', label='simulation')
  plt.plot(d_h[:,0], d_h[:,[2,6,  10,14,  18,22,  26,30]], 'b-', label='fmigo holo')
  plt.plot(d_n[:,0], d_n[:,[2,6,  10,14,  18,22,  26,30]], 'g-', label='fmigo nonholo')
  plt.plot(tref, zref[:,4:8], 'k--', label='reference')
  plt.plot(tse, zse[:,7:14], 'g--', label='epce')
  uniquelabels()

  #plt.show()
else: # test_mode
  print('truckstring test OK')

# For presentation
if presentation and not test_mode:
    plt.figure(7)
    subplot(211);plot(tref,zref[:,0:4],'k-');title('Reference'); ylabel('position'); subplot(212); plot(tref,zref[:,4:8],'k-'); ylabel('velocity'); xlabel('time');
    savefig('../presentation/reference.png')

    plt.figure(8)
    subplot(211);
    nppk = 20
    plot(results[nppk]['epce'][0],results[nppk]['nonholonomic'][1][:,0:7],'r-',label='kinematic'); plot(tref,zref[:,0:4],'k--',label='reference');
    title('Kinematic, non-holonomic, %i steps per period' % nppk); ylabel('position'); uniquelabels();
    subplot(212);
    plot(results[nppk]['epce'][0],results[nppk]['nonholonomic'][1][:,7:14],'r-',label='kinematic'); plot(tref,zref[:,4:8],'k--',label='reference');
    ylabel('velocity'); xlabel('time'); uniquelabels()
    savefig('../presentation/nonholonomic.png')

    plt.figure(9)
    subplot(211);
    nppe = 80
    plot(results[nppe]['epce'][0],results[nppe]['epce'][1][:,0:7],'b-',label='EPCE'); plot(tref,zref[:,0:4],'k--',label='reference');
    title('EPCE, %i steps per period' % nppe); ylabel('position'); uniquelabels();
    subplot(212);
    plot(results[nppe]['epce'][0],results[nppe]['epce'][1][:,7:14],'b-',label='EPCE'); plot(tref,zref[:,4:8],'k--',label='reference');
    ylabel('velocity'); xlabel('time'); uniquelabels()
    savefig('../presentation/epce.png')

    show()
