/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER springs
#define MODEL_GUID "{78a384b7-1718-4f46-a8ee-9536df41db41}"

#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 13
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    x_in; //VR=1
    fmi2Real    x0; //VR=2
    fmi2Real    dx0; //VR=3
    fmi2Real    x1; //VR=4
    fmi2Real    dx1; //VR=5
    fmi2Real    v0; //VR=6
    fmi2Real    dv0; //VR=7
    fmi2Real    v1; //VR=8
    fmi2Real    dv1; //VR=9
    fmi2Real    k1; //VR=10
    fmi2Real    k2; //VR=11
    fmi2Real    f_in; //VR=12
    fmi2Real    f_out; //VR=13



} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    -1.000000, //x_in
    -1.000000, //x0
    0.000000, //dx0
    1.000000, //x1
    0.000000, //dx1
    0.000000, //v0
    0.000000, //dv0
    0.000000, //v1
    0.000000, //dv1
    1.000000, //k1
    0.000000, //k2
    0.000000, //f_in
    0.000000, //f_out



};


#define VR_X_IN 1
#define VR_X0 2
#define VR_DX0 3
#define VR_X1 4
#define VR_DX1 5
#define VR_V0 6
#define VR_DV0 7
#define VR_V1 8
#define VR_DV1 9
#define VR_K1 10
#define VR_K2 11
#define VR_F_IN 12
#define VR_F_OUT 13




//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: value[i] = md->x_in; break;
        case 2: value[i] = md->x0; break;
        case 3: value[i] = md->dx0; break;
        case 4: value[i] = md->x1; break;
        case 5: value[i] = md->dx1; break;
        case 6: value[i] = md->v0; break;
        case 7: value[i] = md->dv0; break;
        case 8: value[i] = md->v1; break;
        case 9: value[i] = md->dv1; break;
        case 10: value[i] = md->k1; break;
        case 11: value[i] = md->k2; break;
        case 12: value[i] = md->f_in; break;
        case 13: value[i] = md->f_out; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: md->x_in = value[i]; break;
        case 2: md->x0 = value[i]; break;
        case 3: md->dx0 = value[i]; break;
        case 4: md->x1 = value[i]; break;
        case 5: md->dx1 = value[i]; break;
        case 6: md->v0 = value[i]; break;
        case 7: md->dv0 = value[i]; break;
        case 8: md->v1 = value[i]; break;
        case 9: md->dv1 = value[i]; break;
        case 10: md->k1 = value[i]; break;
        case 11: md->k2 = value[i]; break;
        case 12: md->f_in = value[i]; break;
        case 13: md->f_out = value[i]; break;
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
