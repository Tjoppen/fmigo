#include "modelDescription.h"

//#define SIMULATION_TYPE cgsl_simulation
//#define SIMULATION_INIT springs_init
//#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

static void updateStates(modelDescription_t *md){}
static void update_all(modelDescription_t *md){
}

static fmi2Status getEventIndicator(const modelDescription_t *md, fmi2Real eventIndicators[]){


    return fmi2OK;
}

// used to set the next time event, if any.
static void eventUpdate(ModelInstance *comp, fmi2EventInfo *eventInfo) {
    return;
}
int fixed(double t, const double x[], double dxdt[], void * params){

  return 0;//GSL_SUCCESS;
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
