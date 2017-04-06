from scipy import integrate
from math import atan, sqrt

def fun(t, y, w_in, k1, d1, k2, d2, J1, J2):
  dphi1, w1, dphi2, w2 = y
  return [
    w_in - w1,
    (k1*dphi1 + d1*(w_in - w1) - k2*dphi2 - d2*(w1 - w2))/J1,
    w1 - w2,
    (k2*dphi2 + d2*(w1   - w2))/J2,
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

r = integrate.ode(fun).set_initial_value([0,0,0,0], 0).set_f_params(w_in, c, d, c_internal, d_internal, J/2, J/2)
t1 = 0.1
dt = 0.001

solt = [0]
sol = [r.y]
print('%f %s' % (r.t, str(r.y)))
while r.successful() and r.t < t1:
  r.integrate(r.t + dt)
  print('%f %s' % (r.t, str(r.y)))
  solt.append(r.t)
  sol.append(r.y)

