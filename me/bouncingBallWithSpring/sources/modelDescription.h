#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER bouncingBallWithSpring
#define MODEL_GUID "{260a1a5e-c0e4-4461-9eee-421bfd48a64c}"

#define FMI_MODELEXCHANGE
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 0
#define NUMBER_OF_REALS 9
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 2
#define NUMBER_OF_EVENT_INDICATORS 1


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real h; //VR=1
    fmi2Real der_h; //VR=2
    fmi2Real v; //VR=3
    fmi2Real der_v; //VR=4
    fmi2Real g; //VR=5
    fmi2Real e; //VR=6
    fmi2Real x_in; //VR=10
    fmi2Real f_out; //VR=20
    fmi2Real k; //VR=30
    fmi2Boolean dirty;
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    1.0, //h
    3.0, //der_h
    0.0, //v
    5.0, //der_v
    9.81, //g
    0.7, //e
    10.0, //x_in
    1.0, //f_out
    0.0, //k
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
    if (md->dirty){
        update_all(md);
        md->dirty = 0;
    }
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_H: value[i] = md->h; break;
        case VR_DER_H: value[i] = md->der_h; break;
        case VR_V: value[i] = md->v; break;
        case VR_DER_V: value[i] = md->der_v; break;
        case VR_G: value[i] = md->g; break;
        case VR_E: value[i] = md->e; break;
        case VR_X_IN: value[i] = md->x_in; break;
        case VR_F_OUT: value[i] = md->f_out; break;
        case VR_K: value[i] = md->k; break;

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
        case VR_H: md->h = value[i]; break;
        case VR_DER_H: md->der_h = value[i]; break;
        case VR_V: md->v = value[i]; break;
        case VR_DER_V: md->der_v = value[i]; break;
        case VR_G: md->g = value[i]; break;
        case VR_E: md->e = value[i]; break;
        case VR_X_IN: md->x_in = value[i]; break;
        case VR_F_OUT: md->f_out = value[i]; break;
        case VR_K: md->k = value[i]; break;

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
#endif //MODELDESCRIPTION_H
