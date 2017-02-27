#This file is genereted by modeldescription2header. DO NOT EDIT! 
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER clutch_ef
#define MODEL_GUID "{95857e9e-77b7-4d00-aef0-0836cffe26d0}"
#define FMI_COSIMULATION

#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 14
#define NUMBER_OF_INTEGERS 2
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real xi0; //VR=0
    fmi2Real vi0; //VR=1
    fmi2Real xo0; //VR=2
    fmi2Real vo0; //VR=3
    fmi2Real mass1; //VR=4
    fmi2Real gamma1; //VR=5
    fmi2Real mass2; //VR=6
    fmi2Real gamma2; //VR=7
    fmi2Real clutch_damping; //VR=8
    fmi2Real force_in1; //VR=9
    fmi2Real force_in2; //VR=10
    fmi2Real v1; //VR=11
    fmi2Real v2; //VR=12
    fmi2Real on_off; //VR=13
    fmi2Integer filter_length; //VR=98
    fmi2Integer integrator_type; //VR=99

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //xi0
    0.0, //vi0
    0.0, //xo0
    0.0, //vo0
    1.0, //mass1
    1.0, //gamma1
    1.0, //mass2
    1.0, //gamma2
    100.0, //clutch_damping
    0.0, //force_in1
    0.0, //force_in2
    0, //v1
    0, //v2
    1.0, //on_off
    0, //filter_length
    6, //integrator_type

};


#define VR_XI0 0
#define VR_VI0 1
#define VR_XO0 2
#define VR_VO0 3
#define VR_MASS1 4
#define VR_GAMMA1 5
#define VR_MASS2 6
#define VR_GAMMA2 7
#define VR_CLUTCH_DAMPING 8
#define VR_FORCE_IN1 9
#define VR_FORCE_IN2 10
#define VR_V1 11
#define VR_V2 12
#define VR_ON_OFF 13
#define VR_FILTER_LENGTH 98
#define VR_INTEGRATOR_TYPE 99


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters



static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_XI0: value[i] = md->xi0; break;
        case VR_VI0: value[i] = md->vi0; break;
        case VR_XO0: value[i] = md->xo0; break;
        case VR_VO0: value[i] = md->vo0; break;
        case VR_MASS1: value[i] = md->mass1; break;
        case VR_GAMMA1: value[i] = md->gamma1; break;
        case VR_MASS2: value[i] = md->mass2; break;
        case VR_GAMMA2: value[i] = md->gamma2; break;
        case VR_CLUTCH_DAMPING: value[i] = md->clutch_damping; break;
        case VR_FORCE_IN1: value[i] = md->force_in1; break;
        case VR_FORCE_IN2: value[i] = md->force_in2; break;
        case VR_V1: value[i] = md->v1; break;
        case VR_V2: value[i] = md->v2; break;
        case VR_ON_OFF: value[i] = md->on_off; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_XI0: md->xi0 = value[i]; break;
        case VR_VI0: md->vi0 = value[i]; break;
        case VR_XO0: md->xo0 = value[i]; break;
        case VR_VO0: md->vo0 = value[i]; break;
        case VR_MASS1: md->mass1 = value[i]; break;
        case VR_GAMMA1: md->gamma1 = value[i]; break;
        case VR_MASS2: md->mass2 = value[i]; break;
        case VR_GAMMA2: md->gamma2 = value[i]; break;
        case VR_CLUTCH_DAMPING: md->clutch_damping = value[i]; break;
        case VR_FORCE_IN1: md->force_in1 = value[i]; break;
        case VR_FORCE_IN2: md->force_in2 = value[i]; break;
        case VR_V1: md->v1 = value[i]; break;
        case VR_V2: md->v2 = value[i]; break;
        case VR_ON_OFF: md->on_off = value[i]; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_FILTER_LENGTH: value[i] = md->filter_length; break;
        case VR_INTEGRATOR_TYPE: value[i] = md->integrator_type; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_FILTER_LENGTH: md->filter_length = value[i]; break;
        case VR_INTEGRATOR_TYPE: md->integrator_type = value[i]; break;

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
