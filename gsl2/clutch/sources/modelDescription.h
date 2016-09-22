#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER clutch
#define MODEL_GUID "{5ef23f51-b93d-46d4-aad9-c45a9cc44121}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define NUMBER_OF_REALS 10
#define NUMBER_OF_INTEGERS 1
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real x0; //VR=0
    fmi2Real v0; //VR=1
    fmi2Real dx0; //VR=2
    fmi2Real mass; //VR=3
    fmi2Real gamma; //VR=4
    fmi2Real clutch_damping; //VR=5
    fmi2Real v_in; //VR=6
    fmi2Real force_in; //VR=7
    fmi2Real v; //VR=8
    fmi2Real force_clutch; //VR=9
    fmi2Integer filter_length; //VR=98

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //x0
    0.0, //v0
    0.0, //dx0
    1.0, //mass
    1.0, //gamma
    1.0, //clutch_damping
    0.0, //v_in
    0.0, //force_in
    0, //v
    0, //force_clutch
    0, //filter_length

};


#define VR_X0 0
#define VR_V0 1
#define VR_DX0 2
#define VR_MASS 3
#define VR_GAMMA 4
#define VR_CLUTCH_DAMPING 5
#define VR_V_IN 6
#define VR_FORCE_IN 7
#define VR_V 8
#define VR_FORCE_CLUTCH 9
#define VR_FILTER_LENGTH 98


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->x0; break;
        case 1: value[i] = md->v0; break;
        case 2: value[i] = md->dx0; break;
        case 3: value[i] = md->mass; break;
        case 4: value[i] = md->gamma; break;
        case 5: value[i] = md->clutch_damping; break;
        case 6: value[i] = md->v_in; break;
        case 7: value[i] = md->force_in; break;
        case 8: value[i] = md->v; break;
        case 9: value[i] = md->force_clutch; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->x0 = value[i]; break;
        case 1: md->v0 = value[i]; break;
        case 2: md->dx0 = value[i]; break;
        case 3: md->mass = value[i]; break;
        case 4: md->gamma = value[i]; break;
        case 5: md->clutch_damping = value[i]; break;
        case 6: md->v_in = value[i]; break;
        case 7: md->force_in = value[i]; break;
        case 8: md->v = value[i]; break;
        case 9: md->force_clutch = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
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
#endif //MODELDESCRIPTION_H
