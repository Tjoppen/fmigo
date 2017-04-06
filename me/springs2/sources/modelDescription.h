/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER springs
#define MODEL_GUID "{78a384b7-1718-4f46-a8ee-9536df41db41}"
#define FMI_MODELEXCHANGE
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 31
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 5
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real x0; //VR=1
    fmi2Real dx0; //VR=2
    fmi2Real x1; //VR=3
    fmi2Real dx1; //VR=4
    fmi2Real v0; //VR=5
    fmi2Real dv0; //VR=6
    fmi2Real a0; //VR=7
    fmi2Real v1; //VR=8
    fmi2Real dv1; //VR=9
    fmi2Real a1; //VR=10
    fmi2Real k1; //VR=11
    fmi2Real gamma1; //VR=12
    fmi2Real omega1; //VR=13
    fmi2Real phi1; //VR=14
    fmi2Real fc1; //VR=15
    fmi2Real k2; //VR=16
    fmi2Real gamma2; //VR=17
    fmi2Real omega2; //VR=18
    fmi2Real phi2; //VR=19
    fmi2Real fc2; //VR=20
    fmi2Real k_internal; //VR=21
    fmi2Real f1; //VR=22
    fmi2Real f2; //VR=23
    fmi2Real x1; //VR=24
    fmi2Real v1; //VR=25
    fmi2Real x2; //VR=26
    fmi2Real v2; //VR=27
    fmi2Real m1; //VR=28
    fmi2Real m1_i; //VR=29
    fmi2Real m2; //VR=30
    fmi2Real m2_i; //VR=31
    fmi2Boolean dirty;
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //x0
    1.0, //dx0
    0.0, //x1
    3.0, //dx1
    0.0, //v0
    5.0, //dv0
    0.0, //a0
    0.0, //v1
    8.0, //dv1
    9.0, //a1
    0.0, //k1
    0.0, //gamma1
    0.0, //omega1
    0.0, //phi1
    0, //fc1
    0.0, //k2
    0.0, //gamma2
    0.0, //omega2
    0.0, //phi2
    0, //fc2
    0.0, //k_internal
    0.0, //f1
    0.0, //f2
    0.0, //x1
    0.0, //v1
    0.0, //x2
    0.0, //v2
    0.0, //m1
    0.0, //m1_i
    0.0, //m2
    0, //m2_i
    1,
};


#define VR_X0 1
#define VR_DX0 2
#define VR_X1 3
#define VR_DX1 4
#define VR_V0 5
#define VR_DV0 6
#define VR_A0 7
#define VR_V1 8
#define VR_DV1 9
#define VR_A1 10
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
#define VR_F1 22
#define VR_F2 23
#define VR_X1 24
#define VR_V1 25
#define VR_X2 26
#define VR_V2 27
#define VR_M1 28
#define VR_M1_I 29
#define VR_M2 30
#define VR_M2_I 31


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


#define STATES { VR_V1, VR_X0, VR_X1, VR_DV1, VR_V0 }
#define DERIVATIVES { VR_DV1, VR_DX0, VR_DX1, VR_A1, VR_DV0 }


static void update_all(modelDescription_t *md);

static fmi2Status generated_fmi2GetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }
int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0: value[i] = md->x0; break;
        case VR_DX0: value[i] = md->dx0; break;
        case VR_X1: value[i] = md->x1; break;
        case VR_DX1: value[i] = md->dx1; break;
        case VR_V0: value[i] = md->v0; break;
        case VR_DV0: value[i] = md->dv0; break;
        case VR_A0: value[i] = md->a0; break;
        case VR_V1: value[i] = md->v1; break;
        case VR_DV1: value[i] = md->dv1; break;
        case VR_A1: value[i] = md->a1; break;
        case VR_K1: value[i] = md->k1; break;
        case VR_GAMMA1: value[i] = md->gamma1; break;
        case VR_OMEGA1: value[i] = md->omega1; break;
        case VR_PHI1: value[i] = md->phi1; break;
        case VR_FC1: value[i] = md->fc1; break;
        case VR_K2: value[i] = md->k2; break;
        case VR_GAMMA2: value[i] = md->gamma2; break;
        case VR_OMEGA2: value[i] = md->omega2; break;
        case VR_PHI2: value[i] = md->phi2; break;
        case VR_FC2: value[i] = md->fc2; break;
        case VR_K_INTERNAL: value[i] = md->k_internal; break;
        case VR_F1: value[i] = md->f1; break;
        case VR_F2: value[i] = md->f2; break;
        case VR_X1: value[i] = md->x1; break;
        case VR_V1: value[i] = md->v1; break;
        case VR_X2: value[i] = md->x2; break;
        case VR_V2: value[i] = md->v2; break;
        case VR_M1: value[i] = md->m1; break;
        case VR_M1_I: value[i] = md->m1_i; break;
        case VR_M2: value[i] = md->m2; break;
        case VR_M2_I: value[i] = md->m2_i; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    md->dirty = 1;
int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: md->x0 = value[i]; break;
        case 2: md->dx0 = value[i]; break;
        case 3: md->x1 = value[i]; break;
        case 4: md->dx1 = value[i]; break;
        case 5: md->v0 = value[i]; break;
        case 6: md->dv0 = value[i]; break;
        case 7: md->a0 = value[i]; break;
        case 8: md->v1 = value[i]; break;
        case 9: md->dv1 = value[i]; break;
        case 10: md->a1 = value[i]; break;
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
        case 22: md->f1 = value[i]; break;
        case 23: md->f2 = value[i]; break;
        case 24: md->x1 = value[i]; break;
        case 25: md->v1 = value[i]; break;
        case 26: md->x2 = value[i]; break;
        case 27: md->v2 = value[i]; break;
        case 28: md->m1 = value[i]; break;
        case 29: md->m1_i = value[i]; break;
        case 30: md->m2 = value[i]; break;
        case 31: md->m2_i = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
static fmi2Status generated_fmi2GetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }
int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    md->dirty = 1;
int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
static fmi2Status generated_fmi2GetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }
int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    md->dirty = 1;
int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
static fmi2Status generated_fmi2GetString(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }
int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    md->dirty = 1;
int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
