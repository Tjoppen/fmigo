#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER exp
#define MODEL_GUID "{fc5bb3ba-e1d3-4ab8-b3da-2acd5fbaf068}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define NUMBER_OF_REALS 3
#define NUMBER_OF_INTEGERS 1
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real x0; //VR=0
    fmi2Real x; //VR=1
    fmi2Real logx; //VR=2
    fmi2Integer filter_length; //VR=98

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    1.0, //x0
    0, //x
    0, //logx
    0, //filter_length

};


#define VR_X0 0
#define VR_X 1
#define VR_LOGX 2
#define VR_FILTER_LENGTH 98


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->x0; break;
        case 1: value[i] = md->x; break;
        case 2: value[i] = md->logx; break;
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
        case 1: md->x = value[i]; break;
        case 2: md->logx = value[i]; break;
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
