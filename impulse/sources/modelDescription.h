#This file is genereted by modeldescription2header. DO NOT EDIT! 
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER impulse
#define MODEL_GUID "{bea86d20-e0df-47d3-b694-bd366a1d5731}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 6
#define NUMBER_OF_INTEGERS 3
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real theta; //VR=0
    fmi2Real omega; //VR=1
    fmi2Real alpha; //VR=2
    fmi2Real tau; //VR=3
    fmi2Real pulse_amplitude; //VR=7
    fmi2Real dc_offset; //VR=8
    fmi2Integer pulse_type; //VR=4
    fmi2Integer pulse_start; //VR=5
    fmi2Integer pulse_length; //VR=6

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0, //theta
    0, //omega
    0, //alpha
    0.0, //tau
    1.0, //pulse_amplitude
    0.0, //dc_offset
    0, //pulse_type
    0, //pulse_start
    1, //pulse_length

};


#define VR_THETA 0
#define VR_OMEGA 1
#define VR_ALPHA 2
#define VR_TAU 3
#define VR_PULSE_AMPLITUDE 7
#define VR_DC_OFFSET 8
#define VR_PULSE_TYPE 4
#define VR_PULSE_START 5
#define VR_PULSE_LENGTH 6


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->theta; break;
        case 1: value[i] = md->omega; break;
        case 2: value[i] = md->alpha; break;
        case 3: value[i] = md->tau; break;
        case 7: value[i] = md->pulse_amplitude; break;
        case 8: value[i] = md->dc_offset; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->theta = value[i]; break;
        case 1: md->omega = value[i]; break;
        case 2: md->alpha = value[i]; break;
        case 3: md->tau = value[i]; break;
        case 7: md->pulse_amplitude = value[i]; break;
        case 8: md->dc_offset = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 4: value[i] = md->pulse_type; break;
        case 5: value[i] = md->pulse_start; break;
        case 6: value[i] = md->pulse_length; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 4: md->pulse_type = value[i]; break;
        case 5: md->pulse_start = value[i]; break;
        case 6: md->pulse_length = value[i]; break;
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
