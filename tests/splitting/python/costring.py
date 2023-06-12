# -*- coding: utf-8 -*-
from pylab import *
from scipy import *
from scipy.sparse import *
from scipy.sparse.linalg  import *
from scipy.integrate  import *
#from scipy.linalg import expm
import cmath

# Functor for solout, for counting steps
class steps_functor:
  def __init__(self):
    self.steps = 0
    self.t = []

  def __call__(self, t, z):
    self.steps += 1
    self.t = concatenate((self.t, [t]))


def intloop3(ts, zs, funs, exchange, state=None, perm=None):
  tprev = ts[0]
  steps = steps_functor()
  tode = [tprev]
  zs2 = [z.copy() for z in zs]
  zode = column_stack([z.transpose() for z in zs])

  for t in ts[1:]:
    # Exchange values
    zbs, stepsinner = exchange(tprev, t, zs2, state) if not exchange is None else (None,0)
    steps.steps += stepsinner

    for i in range(len(zs2)):
      r = ode(funs[i]).set_integrator('dop853',rtol=1e-8,atol=1e-8)
      r.set_solout(steps)
      r.set_solout(steps)
      r.set_initial_value(zs2[i], tprev)
      if not zbs is None and not zbs[i] is None:
        r.set_f_params(zbs[i])
      else:
        r.set_f_params(None)
      while r.successful() and r.t < t:
        r.integrate(t)
      zs2[i] = matrix(r.y)

    tode = concatenate((tode, [t]))
    zode = row_stack([zode, column_stack([z.transpose() for z in zs2])])
    tprev = t

  # Permute zode for sanity
  return tode, matrix(zode[:,perm] if not perm is None else zode), steps.steps

def intloop2(zs, funs, exchange, state, H, tstart, tend, perm):
  # Make sure both tstart and tend become part of ts
  nsteps = int(round((tend-tstart)/H)) + 1
  ts = tstart + array(range(nsteps))*(tend-tstart)/(nsteps-1)
  return intloop3(ts, zs, funs, exchange, state, perm)

def uniquelabels():
  # https://stackoverflow.com/questions/26337493/pyplot-combine-multiple-line-labels-in-legend#26338419
  handles, labels = plt.gca().get_legend_handles_labels()
  i =1
  while i<len(labels):
      if labels[i] in labels[:i]:
          del(labels[i])
          del(handles[i])
      else:
          i +=1
  plt.legend(handles, labels)

if __name__ == '__main__':
  from mpl_toolkits.mplot3d import Axes3D
  import matplotlib.pyplot as plt
  from matplotlib import cm
  from matplotlib.ticker import LinearLocator, FormatStrFormatter


  # Three masses, m={1,2,1}, springs with k=1, d=0
  # Coupling spring parameters:
  k = 10000
  d = 0.1*sqrt(k) # must be somewhat below 2*sqrt(k)

  relaxation = 4
  compliance = 0

  # Mass ratio of elements, especially important for Edo Drenth's method
  edo_mu = 1.0
  #mr = 2.0 / (edo_mu + 1)
  #ml = 2.0 - mr
  #print('%f + %f = %f' % (ml, mr, ml+mr))

  masses = [1.0, edo_mu, edo_mu**2, edo_mu**3]

  M = diag([1.0]*4 + [1.0/m for m in masses])
  Mleft = diag([1.0]*2 + [1.0/m for m in masses[0:2]])
  Mright = diag([1.0]*2 + [1.0/m for m in masses[2:4]])


  Aref = M* matrix([
    [0,0,0,0,  1,0,0,0],
    [0,0,0,0,  0,1,0,0],
    [0,0,0,0,  0,0,1,0],
    [0,0,0,0,  0,0,0,1],

    [-1, 1,   0,   0, 0, 0, 0,0],
    [ 1,-k-1, k,   0, 0,-d, d,0],
    [ 0, k,  -k-1, 1, 0, d,-d,0],
    [ 0, 0,   1,  -1, 0, 0, 0,0],
  ], dtype='float')

  # slowest mode of interest
  w = sort(abs(imag(eig(Aref)[0])))[2]
  #print(w)
  P = 2*pi/w
  NP = 2     # Number of periods
  h = P / 20

  def intloop(zleft, zright, lfun, rfun, exchange, state = None, H = h, tstart = 0, tend = NP*P):
    return intloop2([zleft, zright], (lfun, rfun), exchange, state, H, tstart, tend, [0,1,4,5,2,3,6,7])

  # Excite the leftmost part of the system
  z0 = matrix([[1,0,0,0,0,0,0,0]], dtype='float').transpose()

  Aphi = expm(h*Aref)
  tref = arange(0,NP*P,h)
  zref = row_stack([( matrix_power(Aphi,i) * z0).transpose() for i in range(len(tref))])

  # Force computation matrices
  Finternal = matrix([
    [0,0,1,0],
    [0,0,0,1],
    [-1,1,0,0],
    [1,-1,0,0],
  ], dtype='float')
  Cleft = matrix([
    [0,0,0,0],
    [0,0,0,0],
    [0,0,0,0],
    [0,-k,0,-d],
  ], dtype='float')
  Cright = matrix([
    [0,0,0,0],
    [0,0,0,0],
    [-k,0,-d,0],
    [0,0,0,0],
  ], dtype='float')

  # Position-position coupling
  def pospos(z0, H=h):
    def lfun(t, z, zb):
      zz = matrix(z).transpose()
      return Mleft*(Finternal*zz + Cleft*(zz-zb))

    def rfun(t, z, zb):
      zz = matrix(z).transpose()
      return Mright*(Finternal*zz + Cright*(zz-zb))

    def exchange(t, t2, zlr, state):
      # Left to right
      zb_ltr = row_stack([zlr[0][[1,3]]]+[0]*2)

      # Right to left
      zb_rtl = row_stack([0]*2+[zlr[1][[0,2]]])

      return (zb_rtl, zb_ltr), 0

    zleft = z0[[0,1,4,5]]
    zright = z0[[2,3,6,7]]

    return intloop(zleft, zright, lfun, rfun, exchange, None, H)


  # Extrapolated position-position coupling
  def extrap(z0, H=h):
    def lfun(t, z, zb):
      zz = matrix(z).transpose()
      t0 = zb[4]
      # Extrapolate dphi
      dphi = z[2] - zb[2] - (t-t0)*zb[3]
      dphi2 = [0,0,dphi,0]
      return Mleft*(Finternal*zz + Cleft*matrix(dphi2).transpose())

    def rfun(t, z, zb):
      zz = matrix(z).transpose()
      t0 = zb[4]
      # Extrapolate dphi
      dphi = z[0] - zb[0] - (t-t0)*zb[1]
      dphi2 = [dphi,0,0,0]
      return Mright*(Finternal*zz + Cright*matrix(dphi2).transpose())

    def exchange(t, t2, zlr, state):
      # Put time as fifth parameter to function in both

      # Left to right
      zb_ltr = row_stack([zlr[0][[1,3]]]+[0]*2+[t])

      # Right to left
      zb_rtl = row_stack([0]*2+[zlr[1][[0,2]]]+[t])

      return (zb_rtl, zb_ltr), 0

    zleft = z0[[0,1,4,5]]
    zright = z0[[2,3,6,7]]

    return intloop(zleft, zright, lfun, rfun, exchange, None, H)


  # Just adds force to the system
  # force = zb
  def fun_force_l(t, z, force):
    zz = matrix(z).transpose()
    ftot = Finternal*zz
    if not force is None:
      ftot += force
    return Mleft*ftot

  def fun_force_r(t, z, force):
    zz = matrix(z).transpose()
    ftot = Finternal*zz
    if not force is None:
      ftot += force
    return Mright*ftot

  def kin_exchange(t, t2, zlr, He):
    Hmobility, extrapolate_gdotf = He
    H = t2 - t

    a = 4.0/(H*(1+4*relaxation))
    b = 1.0/(1+4*relaxation)

    # Get volation and constraint velocity
    g     = zlr[0][1] - zlr[1][0]
    gdot  = zlr[0][3] - zlr[1][2]
    if extrapolate_gdotf:
      # Extrapolate gdotf
      al = fun_force_l(t, zlr[0].transpose(), None)
      ar = fun_force_r(t, zlr[1].transpose(), None)
      gdotf = gdot + (al[3] - ar[2]) * H
      steps = 0
    else:
      # Get future velocities
      # exchange function = None (else we'd get infinite recursion!)
      ts, zs, steps = intloop(zlr[0], zlr[1], fun_force_l, fun_force_r, None, None, H, t, t+H)
      gdotf = zs[-1, 5] - zs[-1, 6]

    g = float(reshape(g, ()))
    gdot = float(reshape(gdot, ()))

    # Holonomic
    rhs = -a*g + b*gdot - gdotf
    # Non-holonomic
    #rhs = - gdotf

    # Get mobilities, compute system matrix and force
    if Hmobility > 0:
      # Input unit force, step for Hmobility, compute acceleration difference
      fz = matrix([0,0,0,0]).transpose()
      fl = matrix([0,0,0,1]).transpose()
      fr = matrix([0,0,1,0]).transpose()

      # Give unit forces to each system
      def exchange_force(t, t2, zlr, flfr):
        return flfr, 0

      ts, zs0, steps0 = intloop(zlr[0], zlr[1], fun_force_l, fun_force_r, exchange_force, (fz,fz), Hmobility, t, t+Hmobility)
      ts, zs1, steps1 = intloop(zlr[0], zlr[1], fun_force_l, fun_force_r, exchange_force, (fl,fr), Hmobility, t, t+Hmobility)

      # Accumulate steps
      steps += steps0 + steps1

      # Get accelerations
      zdot0l = fun_force_l(ts[-1], zs0[-1, [0,1,4,5]], fz)
      zdot1l = fun_force_l(ts[-1], zs1[-1, [0,1,4,5]], fl)
      zdot0r = fun_force_r(ts[-1], zs0[-1, [2,3,6,7]], fz)
      zdot1r = fun_force_r(ts[-1], zs1[-1, [2,3,6,7]], fr)

      # m^-1 = da/fd = (a1 - a0) / (f1 - f0) = a1 - a0  (df = 1)
      m1 = zdot1l[3] - zdot0l[3]
      m2 = zdot1r[2] - zdot0r[2]
      #print('m1 {}, m2 {}'.format(m1,m2))
    else:
      m1 = 1.0 / masses[1]
      m2 = 1.0 / masses[2]

    S = m1 + m2
    f = (rhs / S) / H

    zb_ltr = matrix([0,0,-f,0]).transpose()
    zb_rtl = matrix([0,0, 0,f]).transpose()
    return (zb_rtl, zb_ltr), steps

  # Hmobility = timestep used to compute mobility (0 -> use analytical value)
  def kinematic(z0, H=h, Hmobility=0, extrapolate_gdotf=False):
    zleft = z0[[0,1,4,5]]
    zright = z0[[2,3,6,7]]
    return intloop(zleft, zright, fun_force_l, fun_force_r, kin_exchange, (Hmobility, extrapolate_gdotf), H)

  # Maximum internal frequency
  wmax = sort(abs(imag(eig(Mleft*(Finternal+Cleft))[0])))[-1]

  # Constant h*tau, communicating in the middle of each impact
  # Frequency of coupling spring is wmax, communicate at the 45, 90 or 180 degree point
  degrees = 180
  h2 = 2*pi/wmax*degrees/360.0

  # Timesteps to use for numeric mobilities
  Hmob0 = h
  Hmob1 = P*0.5
  Hmob2 = P*1.5

  # Simulate co-simulation
  print(' 1/11 pospos');                        tpospos, zpospos, pospos_steps = pospos(z0)
  print(' 2/11 extrap');                        textrap, zextrap, extrap_steps = extrap(z0)
  print(' 3/11 pospos h2');                     thtau,   zhtau,   htau_steps = pospos(z0, h2)
  print(' 4/11 extrap h2');                     thtaue,  zhtaue,  htaue_steps = extrap(z0, h2)
  print(' 5/11 kinematic');                     tkin, zkin, kin_steps = kinematic(z0)
  print(' 6/11 kinematic, numeric0');           tkinn0, zkinn0, kinn0_steps = kinematic(z0, h, Hmob0)
  print(' 7/11 kinematic, numeric1');           tkinn1, zkinn1, kinn1_steps = kinematic(z0, h, Hmob1)
  print(' 8/11 kinematic, numeric2');           tkinn2, zkinn2, kinn2_steps = kinematic(z0, h, Hmob2)
  print(' 9/11 kinematic, extrap fvel');        tkine, zkine, kine_steps = kinematic(z0, h, 0, True)
  print('10/11 kinematic (h/2)');               tkin2, zkin2, kin2_steps = kinematic(z0, h/2)
  print('11/11 kinematic, extrap fvel (h/2)');  tkine2, zkine2, kine2_steps = kinematic(z0, h/2, 0, True)

  #tfvel, zfvel, fvel_steps = fvel(z0)
  #tintfvel, zintfvel, intfvel_steps = intfvel(z0)
  #tedo, zedo, edo_steps = edo(z0)

  ax = [0, tref[-1], -3, 3]

  plt.figure(1, figsize=(16, 12), dpi=80)
  plt.subplot(221)
  plt.plot(tpospos, zpospos[:,0:4])
  plt.plot(tref, zref[:,0:4], 'k--')
  plt.legend(['a','b','c','d','reference'])
  plt.title('Position-position (%i steps)' % pospos_steps)
  plt.axis(ax)

  plt.subplot(222)
  plt.plot(textrap, zextrap[:,0:4])
  plt.plot(tref, zref[:,0:4], 'k--')
  plt.legend(['a','b','c','d','reference'])
  plt.title('Extrapolation (%i steps)' % extrap_steps)
  plt.axis(ax)

  plt.subplot(223)
  plt.plot(thtau, zhtau[:,0:4])
  plt.plot(tref, zref[:,0:4], 'k--')
  plt.legend(['a','b','c','d','reference'])
  plt.title('h*tau, pos-pos (%i steps)' % htau_steps)
  plt.axis(ax)

  plt.subplot(224)
  plt.plot(thtaue, zhtaue[:,0:4])
  plt.plot(tref, zref[:,0:4], 'k--')
  plt.legend(['a','b','c','d','reference'])
  plt.title('h*taum, extrap (%i steps)' % htaue_steps)
  plt.axis(ax)

  # Position-position and extrapolation
  plt.savefig('pospos_extrap.svg')

  #####

  plt.figure(2, figsize=(16, 12), dpi=80)

  plt.subplot(221)
  plt.plot(tkin, zkin[:,0:4])
  plt.plot(tref, zref[:,0:4], 'k--')
  plt.legend(['a','b','c','d','reference'])
  plt.title('Kinematic, analytic (%i steps)' % kin_steps)
  plt.axis(ax)

  plt.subplot(222)
  plt.plot(tkinn0, zkinn0[:,0:4])
  plt.plot(tref, zref[:,0:4], 'k--')
  plt.legend(['a','b','c','d','reference'])
  plt.title(u'Kinematic, numeric (%i steps, h_mob = h)' % kinn0_steps)
  plt.axis(ax)

  plt.subplot(223)
  plt.plot(tkinn1, zkinn1[:,0:4])
  plt.plot(tref, zref[:,0:4], 'k--')
  plt.legend(['a','b','c','d','reference'])
  plt.title(u'Kinematic, numeric (%i steps, h_mob = %.1f°)' % (kinn1_steps, 360 * Hmob1 / P))
  plt.axis(ax)

  plt.subplot(224)
  plt.plot(tkinn2, zkinn2[:,0:4])
  plt.plot(tref, zref[:,0:4], 'k--')
  plt.legend(['a','b','c','d','reference'])
  plt.title(u'Kinematic, numeric (%i steps, h_mob = %.1f°)' % (kinn2_steps, 360 * Hmob2 / P))
  plt.axis(ax)

  plt.savefig('kinematic.svg')


  plt.figure(3, figsize=(16, 12), dpi=80)

  plt.subplot(211)
  plt.plot(tkin,  zkin[:,0:4],  'r', label='actual gdotf (%i steps)' % kin_steps)
  plt.plot(tkine, zkine[:,0:4], 'g', label='extrapolated gdotf (%i steps)' % kine_steps)
  plt.plot(tref, zref[:,0:4], 'k--', label='reference')
  uniquelabels()
  plt.title('Kinematic, actual future velocity vs. extrapolated from acceleration (analytic mobility)')
  plt.axis(ax)

  plt.subplot(212)
  plt.plot(tkin2,  zkin2[:,0:4],  'r', label='actual gdotf (%i steps)' % kin2_steps)
  plt.plot(tkine2, zkine2[:,0:4], 'g', label='extrapolated gdotf (%i steps)' % kine2_steps)
  plt.plot(tref, zref[:,0:4], 'k--', label='reference')
  uniquelabels()
  plt.title('Kinematic, actual future velocity vs. extrapolated from acceleration (analytic mobility)')
  plt.axis(ax)

  plt.savefig('extrap_gdotf.svg')

  plt.show()
