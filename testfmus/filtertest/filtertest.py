from scipy import integrate
from math import atan, sqrt
from numpy import array

def fun(t, y, w_in, k1, d1, k2, d2, J1, J2):
  dphi1, w1, dphi2, w2 = y[0:4]
  return [
    w_in - w1,
    (k1*dphi1 + d1*(w_in - w1) - k2*dphi2 - d2*(w1 - w2))/J1,
    w1 - w2,
    (k2*dphi2 + d2*(w1   - w2))/J2,
    dphi1,
    w1,
    dphi2,
    w2,
  ]

w_in = 10.0
J = 1.0
f = 50.0

w0 = f*8*atan(1)
c_internal = w0**2*J/2
d_internal = 1.4*sqrt(J/2 * c_internal)

# Coupling
c = 1e5
d = 1.4*sqrt(c)

r = integrate.ode(fun).set_initial_value([0.0]*8, 0).set_f_params(w_in, c, d, c_internal, d_internal, J/2, J/2)
t1 = 0.1
dt = 0.03

ts   = [0]
ys   = [r.y[0:4]]
zs   = [r.y[4:8]]
zs2  = [r.y[4:8]]

while r.successful() and r.t < t1:
  r.set_initial_value([r.y[0], r.y[1], r.y[2], r.y[3]] + [0.0]*4, r.t)
  r.integrate(r.t + dt)

  ts.append(r.t)
  ys.append(r.y[0:4])
  zs.append(r.y[4:8]/dt)
  zs2.append((zs[-1] + zs[-1 if len(zs) == 1 else -2]) / 2)

# lists if ndarrays aren't slizeable, so convert to array
ys = array(ys)
zs = array(zs)
zs2 = array(zs2)

#plot(ts, ys, ts, zs, ts, zs2)
