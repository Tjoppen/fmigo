#include "modelDescription.h"

#define NUMBER_OF_EVENT_INDICATORS 0

#include "fmuTemplate.h"

static void updateStates(modelDescription_t *md){}
static void update_all(modelDescription_t *md){
    md->dx1 = md->v1;
    md->dx2 = md->v2;

    double fi = - md->k_internal * (md->x1 - md->x2) - md->gamma_internal * (md->v1 - md->v2);//out
    md->fc1 = md->k1 * (md->x1 - md->x1_i) + md->gamma1 * (md->v1 - md->v1_i);//out
    md->fc2 = md->k2 * (md->x2 - md->x2_i) + md->gamma2 * (md->v2 - md->v2_i);//out

    md->m1i_o = 1/md->m1;
    md->m2i_o = 2/md->m2;

    md->dv1 = md->m1i_o;
    md->dv2 = md->m2i_o;

    md->dv1 *= (fi - md->fc1 + md->f1);
    md->dv2 *= (-fi- md->fc2 + md->f2);

    md->a1 = md->dv1;
    md->a2 = md->dv2;

    //fprintf(stderr,"%f %f %f %f \n",md->dx0,md->dv0,md->dx1,md->dv1);
}

static fmi2Status getEventIndicator(const modelDescription_t *md, size_t ni, fmi2Real eventIndicators[]){


    return fmi2OK;
}

// used to set the next time event, if any.
static void eventUpdate(ModelInstance *comp, fmi2EventInfo *eventInfo) {
    return;
}



static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
  if (vr == VR_A1) {
    if (wrt == VR_F1 ) {
        *partial = 1.0/comp->s.md.m1;
        return fmi2OK;
    }
    if (wrt == VR_F2 ) {
        *partial = 0;
        return fmi2OK;
    }
  }
  if ( vr == VR_A2){
    if (wrt == VR_F2 ) {
        *partial = 1.0/comp->s.md.m2;
        return fmi2OK;
    }
    if (wrt == VR_F1 ) {
        *partial = 0;
        return fmi2OK;
    }
  }

  return fmi2Error;
}


//gcc -g springs.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
