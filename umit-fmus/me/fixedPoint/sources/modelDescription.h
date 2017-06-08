/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER fixedPoint
#define MODEL_GUID "{fa4a04df-f38d-4b16-b5c2-75c5526fbfb5}"
#define FMI_MODELEXCHANGE
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 0
#define NUMBER_OF_REALS 1
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

//will be defined in fmuTemplate.h
//needed in generated_fmi2GetX/fmi2SetX for wrapper.c
struct ModelInstance;


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    x0; //VR=1



    fmi2Boolean dirty;
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    -1.000000, //x0



    1,
};


#define VR_X0 1




//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


#define STATES {  }
#define DERIVATIVES {  }

static void update_all(modelDescription_t *md);

static fmi2Status generated_fmi2GetReal(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    size_t i;
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: value[i] = md->x0; break;
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
        case 1: md->x0 = value[i]; break;
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
