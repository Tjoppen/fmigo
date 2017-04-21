/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER bouncingBallWithSpring
#define MODEL_GUID "{260a1a5e-c0e4-4461-9eee-421bfd48a64c}"
#define FMI_MODELEXCHANGE
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 0
#define NUMBER_OF_REALS 9
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 2
#define NUMBER_OF_EVENT_INDICATORS 1


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    h; //VR=1
    fmi2Real    der_h; //VR=2
    fmi2Real    v; //VR=3
    fmi2Real    der_v; //VR=4
    fmi2Real    g; //VR=5
    fmi2Real    e; //VR=6
    fmi2Real    x_in; //VR=10
    fmi2Real    f_out; //VR=20
    fmi2Real    k; //VR=30



    fmi2Boolean dirty;
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    1.000000, //h
    3.000000, //der_h
    0.000000, //v
    5.000000, //der_v
    9.810000, //g
    0.700000, //e
    10.000000, //x_in
    1.000000, //f_out
    0.000000, //k



    1,
};


#define VR_H 1
#define VR_DER_H 2
#define VR_V 3
#define VR_DER_V 4
#define VR_G 5
#define VR_E 6
#define VR_X_IN 10
#define VR_F_OUT 20
#define VR_K 30




//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


#define STATES { VR_H, VR_V }
#define DERIVATIVES { VR_DER_H, VR_DER_V }

static void update_all(modelDescription_t *md);

static fmi2Status generated_fmi2GetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    size_t i;
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: value[i] = md->h; break;
        case 2: value[i] = md->der_h; break;
        case 3: value[i] = md->v; break;
        case 4: value[i] = md->der_v; break;
        case 5: value[i] = md->g; break;
        case 6: value[i] = md->e; break;
        case 10: value[i] = md->x_in; break;
        case 20: value[i] = md->f_out; break;
        case 30: value[i] = md->k; break;
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
        case 1: md->h = value[i]; break;
        case 2: md->der_h = value[i]; break;
        case 3: md->v = value[i]; break;
        case 4: md->der_v = value[i]; break;
        case 5: md->g = value[i]; break;
        case 6: md->e = value[i]; break;
        case 10: md->x_in = value[i]; break;
        case 20: md->f_out = value[i]; break;
        case 30: md->k = value[i]; break;
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
