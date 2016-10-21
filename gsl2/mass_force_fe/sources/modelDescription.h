#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER mass_force_fe
#define MODEL_GUID "{c1714c5d-20c2-470c-ad9b-16e26aeae2ce}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 11
#define NUMBER_OF_INTEGERS 1
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real vin; //VR=0
    fmi2Real force_c; //VR=1
    fmi2Real x; //VR=2
    fmi2Real v; //VR=3
    fmi2Real damping; //VR=4
    fmi2Real mass; //VR=5
    fmi2Real coupling_spring; //VR=6
    fmi2Real coupling_damping; //VR=7
    fmi2Real force_out1; //VR=8
    fmi2Real force_out2; //VR=9
    fmi2Real dx; //VR=10
    fmi2Integer filter_length; //VR=98

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    1.0, //vin
    0.0, //force_c
    0.0, //x
    0.0, //v
    0.0, //damping
    1.0, //mass
    1000.0, //coupling_spring
    10.0, //coupling_damping
    0.0, //force_out1
    0.0, //force_out2
    0.0, //dx
    0, //filter_length

};


#define VR_VIN 0
#define VR_FORCE_C 1
#define VR_X 2
#define VR_V 3
#define VR_DAMPING 4
#define VR_MASS 5
#define VR_COUPLING_SPRING 6
#define VR_COUPLING_DAMPING 7
#define VR_FORCE_OUT1 8
#define VR_FORCE_OUT2 9
#define VR_DX 10
#define VR_FILTER_LENGTH 98


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->vin; break;
        case 1: value[i] = md->force_c; break;
        case 2: value[i] = md->x; break;
        case 3: value[i] = md->v; break;
        case 4: value[i] = md->damping; break;
        case 5: value[i] = md->mass; break;
        case 6: value[i] = md->coupling_spring; break;
        case 7: value[i] = md->coupling_damping; break;
        case 8: value[i] = md->force_out1; break;
        case 9: value[i] = md->force_out2; break;
        case 10: value[i] = md->dx; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->vin = value[i]; break;
        case 1: md->force_c = value[i]; break;
        case 2: md->x = value[i]; break;
        case 3: md->v = value[i]; break;
        case 4: md->damping = value[i]; break;
        case 5: md->mass = value[i]; break;
        case 6: md->coupling_spring = value[i]; break;
        case 7: md->coupling_damping = value[i]; break;
        case 8: md->force_out1 = value[i]; break;
        case 9: md->force_out2 = value[i]; break;
        case 10: md->dx = value[i]; break;
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
