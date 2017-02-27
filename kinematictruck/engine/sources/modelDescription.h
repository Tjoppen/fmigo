#This file is genereted by modeldescription2header. DO NOT EDIT! 
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER engine
#define MODEL_GUID "{a6a01bd9-863d-4a7c-ac09-58b7f438895b}"
#define FMI_COSIMULATION

#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 11
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 1
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real theta; //VR=0
    fmi2Real omega; //VR=1
    fmi2Real alpha; //VR=2
    fmi2Real tau; //VR=3
    fmi2Real jinv; //VR=4
    fmi2Real d; //VR=5
    fmi2Real omega_l; //VR=6
    fmi2Real omega_l0; //VR=7
    fmi2Real kp; //VR=8
    fmi2Real tau_max; //VR=9
    fmi2Real beta; //VR=10
    fmi2Boolean clamp_beta; //VR=0

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //theta
    0.0, //omega
    0.0, //alpha
    0.0, //tau
    0.25, //jinv
    1.0, //d
    0.0, //omega_l
    38.8888888889, //omega_l0
    20.0, //kp
    1350.0, //tau_max
    0.0, //beta
    1, //clamp_beta

};


#define VR_THETA 0
#define VR_OMEGA 1
#define VR_ALPHA 2
#define VR_TAU 3
#define VR_JINV 4
#define VR_D 5
#define VR_OMEGA_L 6
#define VR_OMEGA_L0 7
#define VR_KP 8
#define VR_TAU_MAX 9
#define VR_BETA 10
#define VR_CLAMP_BETA 0


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters



static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_THETA: value[i] = md->theta; break;
        case VR_OMEGA: value[i] = md->omega; break;
        case VR_ALPHA: value[i] = md->alpha; break;
        case VR_TAU: value[i] = md->tau; break;
        case VR_JINV: value[i] = md->jinv; break;
        case VR_D: value[i] = md->d; break;
        case VR_OMEGA_L: value[i] = md->omega_l; break;
        case VR_OMEGA_L0: value[i] = md->omega_l0; break;
        case VR_KP: value[i] = md->kp; break;
        case VR_TAU_MAX: value[i] = md->tau_max; break;
        case VR_BETA: value[i] = md->beta; break;

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
        case VR_ALPHA: md->alpha = value[i]; break;
        case VR_TAU: md->tau = value[i]; break;
        case VR_JINV: md->jinv = value[i]; break;
        case VR_D: md->d = value[i]; break;
        case VR_OMEGA_L: md->omega_l = value[i]; break;
        case VR_OMEGA_L0: md->omega_l0 = value[i]; break;
        case VR_KP: md->kp = value[i]; break;
        case VR_TAU_MAX: md->tau_max = value[i]; break;
        case VR_BETA: md->beta = value[i]; break;

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
        case VR_CLAMP_BETA: value[i] = md->clamp_beta; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_CLAMP_BETA: md->clamp_beta = value[i]; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
