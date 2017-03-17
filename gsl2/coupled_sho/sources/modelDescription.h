/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER coupled_sho
#define MODEL_GUID "{e447a3ab-8844-4206-a525-ec659bee2e37}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 14
#define NUMBER_OF_INTEGERS 2
#define NUMBER_OF_BOOLEANS 1
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    mu; //VR=0
    fmi2Real    omega_c; //VR=1
    fmi2Real    zeta_c; //VR=2
    fmi2Real    x0; //VR=3
    fmi2Real    zeta_i; //VR=4
    fmi2Real    v0; //VR=5
    fmi2Real    force; //VR=6
    fmi2Real    xstart; //VR=7
    fmi2Real    vstart; //VR=8
    fmi2Real    dxstart; //VR=9
    fmi2Real    force_c; //VR=10
    fmi2Real    x; //VR=11
    fmi2Real    v; //VR=12
    fmi2Real    omega_i; //VR=95
    fmi2Integer iterations; //VR=96
    fmi2Integer filter_length; //VR=98
    fmi2Boolean dump_data; //VR=97

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    1.000000, //mu
    1.000000, //omega_c
    1.000000, //zeta_c
    1.000000, //x0
    1.000000, //zeta_i
    0.000000, //v0
    0.000000, //force
    1.000000, //xstart
    1.000000, //vstart
    1.000000, //dxstart
    0.000000, //force_c
    0.000000, //x
    0.000000, //v
    1.000000, //omega_i
    0, //iterations
    0, //filter_length
    0, //dump_data

};


#define VR_MU 0
#define VR_OMEGA_C 1
#define VR_ZETA_C 2
#define VR_X0 3
#define VR_ZETA_I 4
#define VR_V0 5
#define VR_FORCE 6
#define VR_XSTART 7
#define VR_VSTART 8
#define VR_DXSTART 9
#define VR_FORCE_C 10
#define VR_X 11
#define VR_V 12
#define VR_OMEGA_I 95
#define VR_ITERATIONS 96
#define VR_FILTER_LENGTH 98
#define VR_DUMP_DATA 97


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->mu; break;
        case 1: value[i] = md->omega_c; break;
        case 2: value[i] = md->zeta_c; break;
        case 3: value[i] = md->x0; break;
        case 4: value[i] = md->zeta_i; break;
        case 5: value[i] = md->v0; break;
        case 6: value[i] = md->force; break;
        case 7: value[i] = md->xstart; break;
        case 8: value[i] = md->vstart; break;
        case 9: value[i] = md->dxstart; break;
        case 10: value[i] = md->force_c; break;
        case 11: value[i] = md->x; break;
        case 12: value[i] = md->v; break;
        case 95: value[i] = md->omega_i; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->mu = value[i]; break;
        case 1: md->omega_c = value[i]; break;
        case 2: md->zeta_c = value[i]; break;
        case 3: md->x0 = value[i]; break;
        case 4: md->zeta_i = value[i]; break;
        case 5: md->v0 = value[i]; break;
        case 6: md->force = value[i]; break;
        case 7: md->xstart = value[i]; break;
        case 8: md->vstart = value[i]; break;
        case 9: md->dxstart = value[i]; break;
        case 10: md->force_c = value[i]; break;
        case 11: md->x = value[i]; break;
        case 12: md->v = value[i]; break;
        case 95: md->omega_i = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 96: value[i] = md->iterations; break;
        case 98: value[i] = md->filter_length; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 96: md->iterations = value[i]; break;
        case 98: md->filter_length = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 97: value[i] = md->dump_data; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 97: md->dump_data = value[i]; break;
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
