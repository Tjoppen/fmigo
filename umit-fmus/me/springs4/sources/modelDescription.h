/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER springs4
#define MODEL_GUID "{78a384b7-1718-4f46-a8ee-9536df41db61}"
#define FMI_MODELEXCHANGE
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 25
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 4
#define NUMBER_OF_EVENT_INDICATORS 0

//will be defined in fmuTemplate.h
//needed in generated_fmi2GetX/fmi2SetX for wrapper.c
struct ModelInstance;


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    zero; //VR=0
    fmi2Real    x1; //VR=1
    fmi2Real    v1; //VR=2
    fmi2Real    a1; //VR=3
    fmi2Real    fc1; //VR=4
    fmi2Real    minv1; //VR=5
    fmi2Real    x2; //VR=6
    fmi2Real    v2; //VR=7
    fmi2Real    a2; //VR=8
    fmi2Real    fc2; //VR=9
    fmi2Real    minv2; //VR=10
    fmi2Real    k1; //VR=11
    fmi2Real    gamma1; //VR=12
    fmi2Real    k2; //VR=13
    fmi2Real    gamma2; //VR=14
    fmi2Real    k_internal; //VR=15
    fmi2Real    gamma_internal; //VR=16
    fmi2Real    f1; //VR=17
    fmi2Real    f2; //VR=18
    fmi2Real    x1_i; //VR=19
    fmi2Real    v1_i; //VR=20
    fmi2Real    x2_i; //VR=21
    fmi2Real    v2_i; //VR=22
    fmi2Real    m1; //VR=23
    fmi2Real    m2; //VR=24



    fmi2Boolean dirty;
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.000000, //zero
    0.000000, //x1
    1.000000, //v1
    2.000000, //a1
    0.000000, //fc1
    0.000000, //minv1
    0.000000, //x2
    6.000000, //v2
    7.000000, //a2
    0.000000, //fc2
    0.000000, //minv2
    0.000000, //k1
    0.000000, //gamma1
    0.000000, //k2
    0.000000, //gamma2
    0.000000, //k_internal
    0.000000, //gamma_internal
    0.000000, //f1
    0.000000, //f2
    0.000000, //x1_i
    0.000000, //v1_i
    0.000000, //x2_i
    0.000000, //v2_i
    1.000000, //m1
    1.000000, //m2



    1,
};


#define VR_ZERO 0
#define VR_X1 1
#define VR_V1 2
#define VR_A1 3
#define VR_FC1 4
#define VR_MINV1 5
#define VR_X2 6
#define VR_V2 7
#define VR_A2 8
#define VR_FC2 9
#define VR_MINV2 10
#define VR_K1 11
#define VR_GAMMA1 12
#define VR_K2 13
#define VR_GAMMA2 14
#define VR_K_INTERNAL 15
#define VR_GAMMA_INTERNAL 16
#define VR_F1 17
#define VR_F2 18
#define VR_X1_I 19
#define VR_V1_I 20
#define VR_X2_I 21
#define VR_V2_I 22
#define VR_M1 23
#define VR_M2 24




//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


#define STATES { VR_V2, VR_X1, VR_V1, VR_X2 }
#define DERIVATIVES { VR_A2, VR_V1, VR_A1, VR_V2 }

static void update_all(modelDescription_t *md);

static fmi2Status generated_fmi2GetReal(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    size_t i;
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->zero; break;
        case 1: value[i] = md->x1; break;
        case 2: value[i] = md->v1; break;
        case 3: value[i] = md->a1; break;
        case 4: value[i] = md->fc1; break;
        case 5: value[i] = md->minv1; break;
        case 6: value[i] = md->x2; break;
        case 7: value[i] = md->v2; break;
        case 8: value[i] = md->a2; break;
        case 9: value[i] = md->fc2; break;
        case 10: value[i] = md->minv2; break;
        case 11: value[i] = md->k1; break;
        case 12: value[i] = md->gamma1; break;
        case 13: value[i] = md->k2; break;
        case 14: value[i] = md->gamma2; break;
        case 15: value[i] = md->k_internal; break;
        case 16: value[i] = md->gamma_internal; break;
        case 17: value[i] = md->f1; break;
        case 18: value[i] = md->f2; break;
        case 19: value[i] = md->x1_i; break;
        case 20: value[i] = md->v1_i; break;
        case 21: value[i] = md->x2_i; break;
        case 22: value[i] = md->v2_i; break;
        case 23: value[i] = md->m1; break;
        case 24: value[i] = md->m2; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    size_t i;
    md->dirty = 1;


    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->zero = value[i]; break;
        case 1: md->x1 = value[i]; break;
        case 2: md->v1 = value[i]; break;
        case 3: md->a1 = value[i]; break;
        case 4: md->fc1 = value[i]; break;
        case 5: md->minv1 = value[i]; break;
        case 6: md->x2 = value[i]; break;
        case 7: md->v2 = value[i]; break;
        case 8: md->a2 = value[i]; break;
        case 9: md->fc2 = value[i]; break;
        case 10: md->minv2 = value[i]; break;
        case 11: md->k1 = value[i]; break;
        case 12: md->gamma1 = value[i]; break;
        case 13: md->k2 = value[i]; break;
        case 14: md->gamma2 = value[i]; break;
        case 15: md->k_internal = value[i]; break;
        case 16: md->gamma_internal = value[i]; break;
        case 17: md->f1 = value[i]; break;
        case 18: md->f2 = value[i]; break;
        case 19: md->x1_i = value[i]; break;
        case 20: md->v1_i = value[i]; break;
        case 21: md->x2_i = value[i]; break;
        case 22: md->v2_i = value[i]; break;
        case 23: md->m1 = value[i]; break;
        case 24: md->m2 = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    size_t i;
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    size_t i;
    md->dirty = 1;


    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    size_t i;
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    size_t i;
    md->dirty = 1;


    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetString(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    size_t i;
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    size_t i;
    md->dirty = 1;


    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
