/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER typeconvtest
#define MODEL_GUID "{56b28b4e-2f8c-4805-8925-672da2a60074}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 3
#define NUMBER_OF_INTEGERS 3
#define NUMBER_OF_BOOLEANS 3
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real r_out; //VR=0
    fmi2Real r0; //VR=8
    fmi2Real r_in; //VR=4
    fmi2Integer i_out; //VR=1
    fmi2Integer i_in; //VR=5
    fmi2Integer i0; //VR=9
    fmi2Boolean b_out; //VR=2
    fmi2Boolean b0; //VR=10
    fmi2Boolean b_in; //VR=6

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0, //r_out
    0.0, //r0
    0.0, //r_in
    0, //i_out
    0, //i_in
    0, //i0
    0, //b_out
    0, //b0
    0, //b_in

};


#define VR_R_OUT 0
#define VR_R0 8
#define VR_R_IN 4
#define VR_I_OUT 1
#define VR_I_IN 5
#define VR_I0 9
#define VR_B_OUT 2
#define VR_B0 10
#define VR_B_IN 6


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_R_OUT: value[i] = md->r_out; break;
        case VR_R0: value[i] = md->r0; break;
        case VR_R_IN: value[i] = md->r_in; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->r_out = value[i]; break;
        case 8: md->r0 = value[i]; break;
        case 4: md->r_in = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_I_OUT: value[i] = md->i_out; break;
        case VR_I_IN: value[i] = md->i_in; break;
        case VR_I0: value[i] = md->i0; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: md->i_out = value[i]; break;
        case 5: md->i_in = value[i]; break;
        case 9: md->i0 = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
static fmi2Status generated_fmi2GetBoolean(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_B_OUT: value[i] = md->b_out; break;
        case VR_B0: value[i] = md->b0; break;
        case VR_B_IN: value[i] = md->b_in; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 2: md->b_out = value[i]; break;
        case 10: md->b0 = value[i]; break;
        case 6: md->b_in = value[i]; break;
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
