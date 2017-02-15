#include "modelDescription.h"
#include "gsl-interface.h"

#define NUMBER_OF_EVENT_INDICATORS 0

#include "fmuTemplate.h"

static void updateStates(modelDescription_t *md){}
static void update_all(modelDescription_t *md){
    //std::cout << "hello " << sdt::endl;
    md->dx0 = md->v0;
    md->dv0 = -md->k1 * (md->x0 - md->x1) - md->f_in;
    md->dx1 = md->v1;
    md->f_out = -md->k2 * (md->x1 - md->x_in);
    md->dv1 = -md->k1 * (md->x1 - md->x0) -md->k2 * (md->x1 - md->x_in);
    //fprintf(stderr,"%f %f %f %f \n",md->dx0,md->dv0,md->dx1,md->dv1);
}

static fmi2Status getEventIndicator(const modelDescription_t *md, size_t ni, fmi2Real eventIndicators[]){


    return fmi2OK;
}

// used to set the next time event, if any.
static void eventUpdate(ModelInstance *comp, fmi2EventInfo *eventInfo) {
    return;
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
