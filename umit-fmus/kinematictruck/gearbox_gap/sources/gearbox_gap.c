#include "modelDescription.h"
#include <math.h>

typedef struct {
  int engaged;
} gearbox_state;

#define SIMULATION_TYPE gearbox_state
#define SIMULATION_SET(x) do{}while(0)
#define SIMULATION_GET(x) do{}while(0)

#include "fmuTemplate.h"

#define SIMULATION_EXIT_INIT exitInit
static fmi2Status exitInit(ModelInstance *comp) {
  if (comp->s.md.gap <= 0) {
    fprintf(stderr, "gap <= 0\n");
    return fmi2Error;
  }
  return fmi2OK;
}

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
  return fmi2Error;
}

//don't want to have to deal with name clashes potentially existing min()
static fmi2Real min2(fmi2Real a, fmi2Real b) {
  return a < b ? a : b;
}

static fmi2Real square(fmi2Real a) {
  return a*a;
}

static void update_omegas(state_t *s, fmi2Real Hinv[2][3], fmi2Real h, fmi2Real c, fmi2Real gamma) {
  //[M*v + h*F; -4*c*gamma/h  + gamma * G * v];
  //[M*v + h*F; -4*dx*gamma/h + gamma * G * v];  (with dx=c)
  fmi2Real rhs[3] = {
    s->md.j1*s->md.omega_e + h*s->md.tau_e,
    s->md.j2*s->md.omega_l + h*s->md.tau_l,
    -4*c*gamma/h + gamma*(s->md.omega_e - s->md.alpha*s->md.omega_l),
  };

  s->md.omega_e = Hinv[0][0]*rhs[0] + Hinv[0][1]*rhs[1] + Hinv[0][2]*rhs[2];
  s->md.omega_l = Hinv[1][0]*rhs[0] + Hinv[1][1]*rhs[1] + Hinv[1][2]*rhs[2];
}

static void dxhGv(state_t *s, fmi2Real h) {
  //dx += h * G * v;
  s->md.dphi += h*(s->md.omega_e - s->md.alpha*s->md.omega_l);
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
  //save initial velocities
  fmi2Real omega_e_copy = s->md.omega_e;
  fmi2Real omega_l_copy = s->md.omega_l;
  fmi2Real u =  s->md.gap;
  fmi2Real l = -s->md.gap;
  fmi2Real hh = communicationStepSize/2.0;       //internal step
  fmi2Real tend = currentCommunicationPoint + communicationStepSize;
  fmi2Real ee = 0.0;        //resitution
  //M = diag([m1,m2]);
  //G   = [1, -alpha];
  //H   = [M -G';G, 0*1e-1];
  fmi2Real tau = 2;
  fmi2Real tol = 1e-4;
  //simplify linear algebra
  fmi2Real k = 1.0/s->md.j1 + square(s->md.alpha)/s->md.j2;

  //s->md.alpha = cos(2*M_PI*currentCommunicationPoint / 4) > 0 ? 10 : 1000;

  //compute Hinv = (inv(H))(1:2,:)
  fmi2Real Hinv[2][3] = {
    {1.0 / s->md.j1 - 1.0 / (square(s->md.j1)*k), s->md.alpha / (s->md.j1*s->md.j2*k),                  1.0 / (s->md.j1*k)},
    {Hinv[0][1] /* re-use */,                     1.0 / s->md.j2 - square(s->md.alpha / s->md.j2) / k, -s->md.alpha / (s->md.j2*k)},
  };
  /*fprintf(stderr, "Hinv = [\n %f,%f,%f;\n %f,%f,%f\n];\n",
    Hinv[0][0], Hinv[0][1], Hinv[0][2], 
    Hinv[1][0], Hinv[1][1], Hinv[1][2]
  );*/

  fmi2Real t;
  for (t = currentCommunicationPoint; t < currentCommunicationPoint + communicationStepSize; t += hh) {
    fmi2Real h = min2(currentCommunicationPoint + communicationStepSize - t, hh);
    fmi2Real gamma = 1/(1+4*tau/h);
    /*fprintf(stderr, "t=%f, h=%f, engaged=%i, dphi=%f, %f < %f < %f? dg = v*G' = %f ", t, h, s->simulation.engaged, s->md.dphi,
      l + s->simulation.engaged * tol,
      s->md.dphi,
      u - s->simulation.engaged * tol,
      s->md.omega_e - s->md.alpha*s->md.omega_l
    );*/

    if (l + s->simulation.engaged * tol < s->md.dphi &&
        u - s->simulation.engaged * tol > s->md.dphi) {
      //solve for free case

      //w = v +  M \ (h*F);
      fmi2Real w1 = s->md.omega_e + h*s->md.tau_e/s->md.j1;
      fmi2Real w2 = s->md.omega_l + h*s->md.tau_l/s->md.j2;

      //ddx = dx + h * (w(1)-alpha*w(2));
      fmi2Real ddx = s->md.dphi + h*(w1 - s->md.alpha*w2);

      //check to see if we stepped outside
      if (ddx > u || ddx < l) {
        //yes!  impact the thing
        //z = H \ [M*v; -ee*G*v];
        //v = z(1:2);
        fmi2Real rhs[3] = {
          s->md.j1*s->md.omega_e,
          s->md.j2*s->md.omega_l,
          -ee*(s->md.omega_e - s->md.alpha*s->md.omega_l),
        };
        s->md.omega_e = Hinv[0][0]*rhs[0] + Hinv[0][1]*rhs[1] + Hinv[0][2]*rhs[2];
        s->md.omega_l = Hinv[1][0]*rhs[0] + Hinv[1][1]*rhs[1] + Hinv[1][2]*rhs[2];

        fmi2Real c;
        if (ddx > u) {
          c = u-ddx;
        } else {
          c = l-ddx;
        }

        //fprintf(stderr, "collide c=%f ", c);
        //continue the step
        //z = H \ [M*v + h*F; -4*c*gamma/h  + gamma * G * v];
        //v = z(1:2);
        update_omegas(s, Hinv, h, c, gamma);

        //dx += h * G * v;
        dxhGv(s, h);
        s->simulation.engaged = 1;
      } else {
        //fprintf(stderr, "free ");
        //still free: continue
        s->simulation.engaged = 0;
        //v   = w;
        s->md.omega_e = w1;
        s->md.omega_l = w2;
        //dx += h *  (v(1)-v(2));
        s->md.dphi += h*(w1 - s->md.alpha*w2);
      }
    } else {
      //gear is active
      //z = H \ [M*v + h*F ; -4*dx*gamma/h + gamma *G * v];
      //v = z(1:2);

      //fprintf(stderr, "active ");
      update_omegas(s, Hinv, h, s->md.dphi, gamma);

      //dx += h * G * v;
      dxhGv(s, h);
    }

    //x += h*v;
    s->md.theta_e += h*s->md.omega_e;
    s->md.theta_l += h*s->md.omega_l;
    //fprintf(stderr, "\n");
  }

  //compute numerical acceleration over the entire step
  //this acts like a low-pass filter
  s->md.omegadot_e = (s->md.omega_e - omega_e_copy) / communicationStepSize;
  s->md.omegadot_l = (s->md.omega_l - omega_l_copy) / communicationStepSize;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
