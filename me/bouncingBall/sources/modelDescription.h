/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER bouncingBall
#define MODEL_GUID "{8c4e810f-3df3-4a00-8276-176fa3c9f003}"
#define FMI_MODELEXCHANGE
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 0
#define NUMBER_OF_REALS 6
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 2
#define NUMBER_OF_EVENT_INDICATORS 1


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    h; //VR=0
    fmi2Real    der_h; //VR=1
    fmi2Real    v; //VR=2
    fmi2Real    der_v; //VR=3
    fmi2Real    g; //VR=4
    fmi2Real    e; //VR=5



    fmi2Boolean dirty;
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    1.000000, //h
    1.000000, //der_h
    0.000000, //v
    3.000000, //der_v
    9.810000, //g
    0.700000, //e



    1,
};


#define VR_H 0
#define VR_DER_H 1
#define VR_V 2
#define VR_DER_V 3
#define VR_G 4
#define VR_E 5




//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


#define STATES { VR_H, VR_V }
#define DERIVATIVES { VR_DER_H, VR_DER_V }

static void update_all(modelDescription_t *md);

static fmi2Status generated_fmi2GetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }
int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->h; break;
        case 1: value[i] = md->der_h; break;
        case 2: value[i] = md->v; break;
        case 3: value[i] = md->der_v; break;
        case 4: value[i] = md->g; break;
        case 5: value[i] = md->e; break;
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
        case 0: md->h = value[i]; break;
        case 1: md->der_h = value[i]; break;
        case 2: md->v = value[i]; break;
        case 3: md->der_v = value[i]; break;
        case 4: md->g = value[i]; break;
        case 5: md->e = value[i]; break;
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
