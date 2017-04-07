from scipy import integrate
from math import atan, sqrt
from numpy import array
import csv
import subprocess
from StringIO import StringIO
import matplotlib.pyplot as plt

def f_coupling(y, w_in, k1, d1):
  dphi1, w1, dphi2, w2 = y[0:4]
  return k1*dphi1 + d1*(w1 - w_in)

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

w_in = 1.0
J = 1.0
f = 500.0

w0 = f*8*atan(1)
c_internal = w0**2*J/2
d_internal = 1.4*sqrt(J/2 * c_internal)

# Coupling
c_coupling = 1e9
d_coupling = 1.4*sqrt(c_coupling)

r = integrate.ode(fun).set_initial_value([0.0]*8, 0)
#r.set_integrator('vode', nsteps=1000)
r.set_f_params(w_in, c_coupling, d_coupling, c_internal, d_internal, J/2, J/2)

ts = []
data = []
for row in csv.reader(open('out-2.csv')):
  ts.append(float(row[0]))
  data.append([
  float(row[c]) for c in [0,2,6,4]
  ])
data = array(data)

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

# lists if ndarrays aren't slizeable, so convert to array
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

