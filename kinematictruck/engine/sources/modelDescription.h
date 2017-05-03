/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER engine
#define MODEL_GUID "{a6a01bd9-863d-4a7c-ac09-58b7f438895b}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 11
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 1
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

//will be defined in fmuTemplate.h
//needed in generated_fmi2GetX/fmi2SetX for wrapper.c
struct ModelInstance;


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    theta; //VR=0
    fmi2Real    omega; //VR=1
    fmi2Real    alpha; //VR=2
    fmi2Real    tau; //VR=3
    fmi2Real    jinv; //VR=4
    fmi2Real    d; //VR=5
    fmi2Real    omega_l; //VR=6
    fmi2Real    omega_l0; //VR=7
    fmi2Real    kp; //VR=8
    fmi2Real    tau_max; //VR=9
    fmi2Real    beta; //VR=10

    fmi2Boolean clamp_beta; //VR=0

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.000000, //theta
    0.000000, //omega
    0.000000, //alpha
    0.000000, //tau
    0.250000, //jinv
    1.000000, //d
    0.000000, //omega_l
    38.888889, //omega_l0
    20.000000, //kp
    1350.000000, //tau_max
    0.000000, //beta

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


static fmi2Status generated_fmi2GetReal(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->theta; break;
        case 1: value[i] = md->omega; break;
        case 2: value[i] = md->alpha; break;
        case 3: value[i] = md->tau; break;
        case 4: value[i] = md->jinv; break;
        case 5: value[i] = md->d; break;
        case 6: value[i] = md->omega_l; break;
        case 7: value[i] = md->omega_l0; break;
        case 8: value[i] = md->kp; break;
        case 9: value[i] = md->tau_max; break;
        case 10: value[i] = md->beta; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->theta = value[i]; break;
        case 1: md->omega = value[i]; break;
        case 2: md->alpha = value[i]; break;
        case 3: md->tau = value[i]; break;
        case 4: md->jinv = value[i]; break;
        case 5: md->d = value[i]; break;
        case 6: md->omega_l = value[i]; break;
        case 7: md->omega_l0 = value[i]; break;
        case 8: md->kp = value[i]; break;
        case 9: md->tau_max = value[i]; break;
        case 10: md->beta = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->clamp_beta; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->clamp_beta = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetString(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
