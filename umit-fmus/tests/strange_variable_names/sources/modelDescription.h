/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER strange_variable_names
#define MODEL_GUID "{f15cb318-7f08-4e71-a21b-b709c8559d1e}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 3
#define NUMBER_OF_INTEGERS 3
#define NUMBER_OF_BOOLEANS 3
#define NUMBER_OF_STRINGS 3
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

//will be defined in fmuTemplate.h
//needed in generated_fmi2GetX/fmi2SetX for wrapper.c
struct ModelInstance;


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    dot_out; //VR=0
    fmi2Real    dot_in; //VR=4
    fmi2Real    dot_0; //VR=8
    fmi2Integer colon_out; //VR=1
    fmi2Integer colon_in; //VR=5
    fmi2Integer colon_0; //VR=9
    fmi2Boolean der_out_; //VR=2
    fmi2Boolean der_in_; //VR=6
    fmi2Boolean der_0_; //VR=10
    fmi2Char    space_out[1024]; //VR=3
    fmi2Char    space_in[1024]; //VR=7
    fmi2Char    space_0[1024]; //VR=11
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.000000, //dot_out
    0.000000, //dot_in
    0.000000, //dot_0
    0, //colon_out
    0, //colon_in
    0, //colon_0
    0, //der_out_
    0, //der_in_
    0, //der_0_
    "", //space_out
    "", //space_in
    "", //space_0
};


#define VR_DOT_OUT 0
#define VR_DOT_IN 4
#define VR_DOT_0 8
#define VR_COLON_OUT 1
#define VR_COLON_IN 5
#define VR_COLON_0 9
#define VR_DER_OUT_ 2
#define VR_DER_IN_ 6
#define VR_DER_0_ 10
#define VR_SPACE_OUT 3
#define VR_SPACE_IN 7
#define VR_SPACE_0 11

//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->dot_out; break;
        case 4: value[i] = md->dot_in; break;
        case 8: value[i] = md->dot_0; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->dot_out = value[i]; break;
        case 4: md->dot_in = value[i]; break;
        case 8: md->dot_0 = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: value[i] = md->colon_out; break;
        case 5: value[i] = md->colon_in; break;
        case 9: value[i] = md->colon_0; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: md->colon_out = value[i]; break;
        case 5: md->colon_in = value[i]; break;
        case 9: md->colon_0 = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 2: value[i] = md->der_out_; break;
        case 6: value[i] = md->der_in_; break;
        case 10: value[i] = md->der_0_; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 2: md->der_out_ = value[i]; break;
        case 6: md->der_in_ = value[i]; break;
        case 10: md->der_0_ = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetString(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 3: value[i] = md->space_out; break;
        case 7: value[i] = md->space_in; break;
        case 11: value[i] = md->space_0; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 3: if (strlcpy(md->space_out, value[i], sizeof(md->space_out)) >= sizeof(md->space_out)) { return fmi2Error; } break;
        case 7: if (strlcpy(md->space_in, value[i], sizeof(md->space_in)) >= sizeof(md->space_in)) { return fmi2Error; } break;
        case 11: if (strlcpy(md->space_0, value[i], sizeof(md->space_0)) >= sizeof(md->space_0)) { return fmi2Error; } break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
