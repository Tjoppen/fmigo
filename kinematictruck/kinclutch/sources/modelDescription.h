/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER kinclutch
#define MODEL_GUID "{5f71ee8b-047f-4780-a809-cca8f9efe480}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 10
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real theta_e; //VR=0
    fmi2Real omega_e; //VR=1
    fmi2Real alpha_e; //VR=2
    fmi2Real tau_e; //VR=3
    fmi2Real j_e; //VR=4
    fmi2Real theta_l; //VR=5
    fmi2Real omega_l; //VR=6
    fmi2Real alpha_l; //VR=7
    fmi2Real tau_l; //VR=8
    fmi2Real j_l; //VR=9

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //theta_e
    0.0, //omega_e
    0.0, //alpha_e
    0.0, //tau_e
    1.0, //j_e
    0.0, //theta_l
    0.0, //omega_l
    0.0, //alpha_l
    0.0, //tau_l
    1.0, //j_l

};


#define VR_THETA_E 0
#define VR_OMEGA_E 1
#define VR_ALPHA_E 2
#define VR_TAU_E 3
#define VR_J_E 4
#define VR_THETA_L 5
#define VR_OMEGA_L 6
#define VR_ALPHA_L 7
#define VR_TAU_L 8
#define VR_J_L 9


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_THETA_E: value[i] = md->theta_e; break;
        case VR_OMEGA_E: value[i] = md->omega_e; break;
        case VR_ALPHA_E: value[i] = md->alpha_e; break;
        case VR_TAU_E: value[i] = md->tau_e; break;
        case VR_J_E: value[i] = md->j_e; break;
        case VR_THETA_L: value[i] = md->theta_l; break;
        case VR_OMEGA_L: value[i] = md->omega_l; break;
        case VR_ALPHA_L: value[i] = md->alpha_l; break;
        case VR_TAU_L: value[i] = md->tau_l; break;
        case VR_J_L: value[i] = md->j_l; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->theta_e = value[i]; break;
        case 1: md->omega_e = value[i]; break;
        case 2: md->alpha_e = value[i]; break;
        case 3: md->tau_e = value[i]; break;
        case 4: md->j_e = value[i]; break;
        case 5: md->theta_l = value[i]; break;
        case 6: md->omega_l = value[i]; break;
        case 7: md->alpha_l = value[i]; break;
        case 8: md->tau_l = value[i]; break;
        case 9: md->j_l = value[i]; break;
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
static fmi2Status generated_fmi2GetString(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
