#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER springs
#define MODEL_GUID "{94c48023-cb4a-4ea1-9e70-ee6556d3ba0c}"
#define FMI_MODELEXCHANGE
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 8
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 1
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real x0; //VR=0
    fmi2Real v0; //VR=1
    fmi2Real x1; //VR=2
    fmi2Real v1; //VR=3
    fmi2Real k1; //VR=4
    fmi2Real k2; //VR=5
    fmi2Real x_in; //VR=6
    fmi2Real x_out; //VR=7
    fmi2Real dx0;

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    -1.0, //x0
    0.0, //v0
    1.0, //x1
    0.0, //v1
    0.0, //k1
    1.0, //k2
    0.0, //x_in
    0, //x_out
    0, //dx0

};


#define VR_X0 0
#define VR_V0 1
#define VR_X1 2
#define VR_V1 3
#define VR_K1 4
#define VR_K2 5
#define VR_X_IN 6
#define VR_X_OUT 7


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0: value[i] = md->x0; break;
        case VR_V0: value[i] = md->v0; break;
        case VR_X1: value[i] = md->x1; break;
        case VR_V1: value[i] = md->v1; break;
        case VR_K1: value[i] = md->k1; break;
        case VR_K2: value[i] = md->k2; break;
        case VR_X_IN: value[i] = md->x_in; break;
        case VR_X_OUT: value[i] = md->x_out; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0: md->x0 = value[i]; break;
        case VR_V0: md->v0 = value[i]; break;
        case VR_X1: md->x1 = value[i]; break;
        case VR_V1: md->v1 = value[i]; break;
        case VR_K1: md->k1 = value[i]; break;
        case VR_K2: md->k2 = value[i]; break;
        case VR_X_IN: md->x_in = value[i]; break;
        case VR_X_OUT: md->x_out = value[i]; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

#define STATES { VR_X0 }


static fmi2Status generated_fmi2GetContinuousStates(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0: value[i] = md->x0; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetContinuousStates(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0: md->x0 = value[i]; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}


static void updateStates(modelDescription_t *md);

static fmi2Status generated_fmi2GetDerivatives(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    updateStates(md);
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0: md->dx0 = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
