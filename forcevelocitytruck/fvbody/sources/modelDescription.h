#This file is genereted by modeldescriptin2header. DO NOT EDIT! 
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER fvbody
#define MODEL_GUID "{cd4d8666-fd68-4809-b9bb-5b62a55cab84}"
#define FMI_COSIMULATION

#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 7
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real theta; //VR=0
    fmi2Real omega; //VR=1
    fmi2Real tau; //VR=2
    fmi2Real jinv; //VR=3
    fmi2Real kc; //VR=4
    fmi2Real dc; //VR=5
    fmi2Real d; //VR=6

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //theta
    0.0, //omega
    0, //tau
    0.0001, //jinv
    5000.0, //kc
    250.0, //dc
    10.0, //d

};


#define VR_THETA 0
#define VR_OMEGA 1
#define VR_TAU 2
#define VR_JINV 3
#define VR_KC 4
#define VR_DC 5
#define VR_D 6


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters



static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_THETA: value[i] = md->theta; break;
        case VR_OMEGA: value[i] = md->omega; break;
        case VR_TAU: value[i] = md->tau; break;
        case VR_JINV: value[i] = md->jinv; break;
        case VR_KC: value[i] = md->kc; break;
        case VR_DC: value[i] = md->dc; break;
        case VR_D: value[i] = md->d; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_THETA: md->theta = value[i]; break;
        case VR_OMEGA: md->omega = value[i]; break;
        case VR_TAU: md->tau = value[i]; break;
        case VR_JINV: md->jinv = value[i]; break;
        case VR_KC: md->kc = value[i]; break;
        case VR_DC: md->dc = value[i]; break;
        case VR_D: md->d = value[i]; break;

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
#endif //MODELDESCRIPTION_H
