#This file is genereted by modeldescriptin2header. DO NOT EDIT! 
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER gearbox2
#define MODEL_GUID "{9b727233-c36a-4ede-a3e9-ec1ab2cee17b}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 13
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real theta_e; //VR=0
    fmi2Real omega_e; //VR=1
    fmi2Real omegadot_e; //VR=2
    fmi2Real tau_e; //VR=3
    fmi2Real j1; //VR=4
    fmi2Real d1; //VR=5
    fmi2Real theta_l; //VR=6
    fmi2Real omega_l; //VR=7
    fmi2Real omegadot_l; //VR=8
    fmi2Real tau_l; //VR=9
    fmi2Real alpha; //VR=10
    fmi2Real j2; //VR=11
    fmi2Real d2; //VR=12


} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //theta_e
    0.0, //omega_e
    0.0, //omegadot_e
    0.0, //tau_e
    1.0, //j1
    1.0, //d1
    0.0, //theta_l
    0.0, //omega_l
    0.0, //omegadot_l
    0.0, //tau_l
    4.3, //alpha
    1.0, //j2
    1.0, //d2


};


#define VR_THETA_E 0
#define VR_OMEGA_E 1
#define VR_OMEGADOT_E 2
#define VR_TAU_E 3
#define VR_J1 4
#define VR_D1 5
#define VR_THETA_L 6
#define VR_OMEGA_L 7
#define VR_OMEGADOT_L 8
#define VR_TAU_L 9
#define VR_ALPHA 10
#define VR_J2 11
#define VR_D2 12



//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->theta_e; break;
        case 1: value[i] = md->omega_e; break;
        case 2: value[i] = md->omegadot_e; break;
        case 3: value[i] = md->tau_e; break;
        case 4: value[i] = md->j1; break;
        case 5: value[i] = md->d1; break;
        case 6: value[i] = md->theta_l; break;
        case 7: value[i] = md->omega_l; break;
        case 8: value[i] = md->omegadot_l; break;
        case 9: value[i] = md->tau_l; break;
        case 10: value[i] = md->alpha; break;
        case 11: value[i] = md->j2; break;
        case 12: value[i] = md->d2; break;
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
        case 2: md->omegadot_e = value[i]; break;
        case 3: md->tau_e = value[i]; break;
        case 4: md->j1 = value[i]; break;
        case 5: md->d1 = value[i]; break;
        case 6: md->theta_l = value[i]; break;
        case 7: md->omega_l = value[i]; break;
        case 8: md->omegadot_l = value[i]; break;
        case 9: md->tau_l = value[i]; break;
        case 10: md->alpha = value[i]; break;
        case 11: md->j2 = value[i]; break;
        case 12: md->d2 = value[i]; break;
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
