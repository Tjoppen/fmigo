#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER gearbox
#define MODEL_GUID "{16176ce9-5941-49b2-9f50-b6870dd10c46}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 8
#define NUMBER_OF_INTEGERS 1
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real theta_e; //VR=0
    fmi2Real omega_e; //VR=1
    fmi2Real tau_e; //VR=2
    fmi2Real d1; //VR=3
    fmi2Real theta_l; //VR=4
    fmi2Real omega_l; //VR=5
    fmi2Real tau_l; //VR=6
    fmi2Real d2; //VR=7
    fmi2Integer gear; //VR=8

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //theta_e
    0.0, //omega_e
    0, //tau_e
    1.0, //d1
    0, //theta_l
    0, //omega_l
    0.0, //tau_l
    1.0, //d2
    1, //gear

};


#define VR_THETA_E 0
#define VR_OMEGA_E 1
#define VR_TAU_E 2
#define VR_D1 3
#define VR_THETA_L 4
#define VR_OMEGA_L 5
#define VR_TAU_L 6
#define VR_D2 7
#define VR_GEAR 8


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_THETA_E: value[i] = md->theta_e; break;
        case VR_OMEGA_E: value[i] = md->omega_e; break;
        case VR_TAU_E: value[i] = md->tau_e; break;
        case VR_D1: value[i] = md->d1; break;
        case VR_THETA_L: value[i] = md->theta_l; break;
        case VR_OMEGA_L: value[i] = md->omega_l; break;
        case VR_TAU_L: value[i] = md->tau_l; break;
        case VR_D2: value[i] = md->d2; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_THETA_E: md->theta_e = value[i]; break;
        case VR_OMEGA_E: md->omega_e = value[i]; break;
        case VR_TAU_E: md->tau_e = value[i]; break;
        case VR_D1: md->d1 = value[i]; break;
        case VR_THETA_L: md->theta_l = value[i]; break;
        case VR_OMEGA_L: md->omega_l = value[i]; break;
        case VR_TAU_L: md->tau_l = value[i]; break;
        case VR_D2: md->d2 = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_GEAR: value[i] = md->gear; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_GEAR: md->gear = value[i]; break;
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
