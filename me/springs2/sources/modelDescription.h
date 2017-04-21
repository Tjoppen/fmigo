/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER springs2
#define MODEL_GUID "{78a384b7-1718-4f46-a8ee-9536df41db41}"
#define FMI_MODELEXCHANGE
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 32
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 4
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    x1; //VR=1
    fmi2Real    dx1; //VR=2
    fmi2Real    x2; //VR=3
    fmi2Real    dx2; //VR=4
    fmi2Real    v1; //VR=5
    fmi2Real    dv1; //VR=6
    fmi2Real    a1; //VR=7
    fmi2Real    v2; //VR=8
    fmi2Real    dv2; //VR=9
    fmi2Real    a2; //VR=10
    fmi2Real    k1; //VR=11
    fmi2Real    gamma1; //VR=12
    fmi2Real    omega1; //VR=13
    fmi2Real    phi1; //VR=14
    fmi2Real    fc1; //VR=15
    fmi2Real    k2; //VR=16
    fmi2Real    gamma2; //VR=17
    fmi2Real    omega2; //VR=18
    fmi2Real    phi2; //VR=19
    fmi2Real    fc2; //VR=20
    fmi2Real    k_internal; //VR=21
    fmi2Real    gamma_internal; //VR=22
    fmi2Real    f1; //VR=23
    fmi2Real    f2; //VR=24
    fmi2Real    x1_i; //VR=25
    fmi2Real    v1_i; //VR=26
    fmi2Real    x2_i; //VR=27
    fmi2Real    v2_i; //VR=28
    fmi2Real    m1; //VR=29
    fmi2Real    m1i_o; //VR=30
    fmi2Real    m2; //VR=31
    fmi2Real    m2i_o; //VR=32



    fmi2Boolean dirty;
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.000000, //x1
    1.000000, //dx1
    0.000000, //x2
    3.000000, //dx2
    0.000000, //v1
    5.000000, //dv1
    0.000000, //a1
    0.000000, //v2
    8.000000, //dv2
    0.000000, //a2
    0.000000, //k1
    0.000000, //gamma1
    0.000000, //omega1
    0.000000, //phi1
    0.000000, //fc1
    0.000000, //k2
    0.000000, //gamma2
    0.000000, //omega2
    0.000000, //phi2
    0.000000, //fc2
    0.000000, //k_internal
    0.000000, //gamma_internal
    0.000000, //f1
    0.000000, //f2
    0.000000, //x1_i
    0.000000, //v1_i
    0.000000, //x2_i
    0.000000, //v2_i
    1.000000, //m1
    0.000000, //m1i_o
    1.000000, //m2
    0.000000, //m2i_o



    1,
};


#define VR_X1 1
#define VR_DX1 2
#define VR_X2 3
#define VR_DX2 4
#define VR_V1 5
#define VR_DV1 6
#define VR_A1 7
#define VR_V2 8
#define VR_DV2 9
#define VR_A2 10
#define VR_K1 11
#define VR_GAMMA1 12
#define VR_OMEGA1 13
#define VR_PHI1 14
#define VR_FC1 15
#define VR_K2 16
#define VR_GAMMA2 17
#define VR_OMEGA2 18
#define VR_PHI2 19
#define VR_FC2 20
#define VR_K_INTERNAL 21
#define VR_GAMMA_INTERNAL 22
#define VR_F1 23
#define VR_F2 24
#define VR_X1_I 25
#define VR_V1_I 26
#define VR_X2_I 27
#define VR_V2_I 28
#define VR_M1 29
#define VR_M1I_O 30
#define VR_M2 31
#define VR_M2I_O 32




//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


#define STATES { VR_V2, VR_X1, VR_X2, VR_V1 }
#define DERIVATIVES { VR_DV2, VR_DX1, VR_DX2, VR_DV1 }

static void update_all(modelDescription_t *md);

static fmi2Status generated_fmi2GetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    size_t i;
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: value[i] = md->x1; break;
        case 2: value[i] = md->dx1; break;
        case 3: value[i] = md->x2; break;
        case 4: value[i] = md->dx2; break;
        case 5: value[i] = md->v1; break;
        case 6: value[i] = md->dv1; break;
        case 7: value[i] = md->a1; break;
        case 8: value[i] = md->v2; break;
        case 9: value[i] = md->dv2; break;
        case 10: value[i] = md->a2; break;
        case 11: value[i] = md->k1; break;
        case 12: value[i] = md->gamma1; break;
        case 13: value[i] = md->omega1; break;
        case 14: value[i] = md->phi1; break;
        case 15: value[i] = md->fc1; break;
        case 16: value[i] = md->k2; break;
        case 17: value[i] = md->gamma2; break;
        case 18: value[i] = md->omega2; break;
        case 19: value[i] = md->phi2; break;
        case 20: value[i] = md->fc2; break;
        case 21: value[i] = md->k_internal; break;
        case 22: value[i] = md->gamma_internal; break;
        case 23: value[i] = md->f1; break;
        case 24: value[i] = md->f2; break;
        case 25: value[i] = md->x1_i; break;
        case 26: value[i] = md->v1_i; break;
        case 27: value[i] = md->x2_i; break;
        case 28: value[i] = md->v2_i; break;
        case 29: value[i] = md->m1; break;
        case 30: value[i] = md->m1i_o; break;
        case 31: value[i] = md->m2; break;
        case 32: value[i] = md->m2i_o; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    size_t i;
    md->dirty = 1;


    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: md->x1 = value[i]; break;
        case 2: md->dx1 = value[i]; break;
        case 3: md->x2 = value[i]; break;
        case 4: md->dx2 = value[i]; break;
        case 5: md->v1 = value[i]; break;
        case 6: md->dv1 = value[i]; break;
        case 7: md->a1 = value[i]; break;
        case 8: md->v2 = value[i]; break;
        case 9: md->dv2 = value[i]; break;
        case 10: md->a2 = value[i]; break;
        case 11: md->k1 = value[i]; break;
        case 12: md->gamma1 = value[i]; break;
        case 13: md->omega1 = value[i]; break;
        case 14: md->phi1 = value[i]; break;
        case 15: md->fc1 = value[i]; break;
        case 16: md->k2 = value[i]; break;
        case 17: md->gamma2 = value[i]; break;
        case 18: md->omega2 = value[i]; break;
        case 19: md->phi2 = value[i]; break;
        case 20: md->fc2 = value[i]; break;
        case 21: md->k_internal = value[i]; break;
        case 22: md->gamma_internal = value[i]; break;
        case 23: md->f1 = value[i]; break;
        case 24: md->f2 = value[i]; break;
        case 25: md->x1_i = value[i]; break;
        case 26: md->v1_i = value[i]; break;
        case 27: md->x2_i = value[i]; break;
        case 28: md->v2_i = value[i]; break;
        case 29: md->m1 = value[i]; break;
        case 30: md->m1i_o = value[i]; break;
        case 31: md->m2 = value[i]; break;
        case 32: md->m2i_o = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
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

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    size_t i;
    md->dirty = 1;


    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
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

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    size_t i;
    md->dirty = 1;


    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetString(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
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

static fmi2Status generated_fmi2SetString(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
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
