/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER engine2
#define MODEL_GUID "{cbbffdc6-77a2-4783-bdc5-e23a26787c3f}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 17
#define NUMBER_OF_INTEGERS 2
#define NUMBER_OF_BOOLEANS 2
#define NUMBER_OF_STRINGS 1
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

//will be defined in fmuTemplate.h
//needed in generated_fmi2GetX/fmi2SetX for wrapper.c
struct ModelInstance;


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    theta_out; //VR=1
    fmi2Real    omega_out; //VR=2
    fmi2Real    alpha_out; //VR=3
    fmi2Real    tau_out; //VR=4
    fmi2Real    theta_in; //VR=5
    fmi2Real    omega_in; //VR=6
    fmi2Real    tau_in; //VR=7
    fmi2Real    theta0; //VR=8
    fmi2Real    omega0; //VR=9
    fmi2Real    kp; //VR=10
    fmi2Real    tau_max; //VR=11
    fmi2Real    omega_target; //VR=12
    fmi2Real    jinv; //VR=13
    fmi2Real    k1; //VR=14
    fmi2Real    k2; //VR=15
    fmi2Real    k_in; //VR=16
    fmi2Real    d_in; //VR=17
    fmi2Integer filter_length; //VR=98
    fmi2Integer integrator; //VR=1002
    fmi2Boolean octave_output; //VR=1001
    fmi2Boolean integrate_dtheta; //VR=18
    fmi2Char    octave_output_file[1024]; //VR=1003
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.000000, //theta_out
    0.000000, //omega_out
    0.000000, //alpha_out
    0.000000, //tau_out
    0.000000, //theta_in
    0.000000, //omega_in
    0.000000, //tau_in
    0.000000, //theta0
    0.000000, //omega0
    20.000000, //kp
    1350.000000, //tau_max
    38.888889, //omega_target
    0.250000, //jinv
    1.000000, //k1
    0.100000, //k2
    0.000000, //k_in
    0.000000, //d_in
    0, //filter_length
    2, //integrator
    0, //octave_output
    0, //integrate_dtheta
    "engine2.m", //octave_output_file
};


#define VR_THETA_OUT 1
#define VR_OMEGA_OUT 2
#define VR_ALPHA_OUT 3
#define VR_TAU_OUT 4
#define VR_THETA_IN 5
#define VR_OMEGA_IN 6
#define VR_TAU_IN 7
#define VR_THETA0 8
#define VR_OMEGA0 9
#define VR_KP 10
#define VR_TAU_MAX 11
#define VR_OMEGA_TARGET 12
#define VR_JINV 13
#define VR_K1 14
#define VR_K2 15
#define VR_K_IN 16
#define VR_D_IN 17
#define VR_FILTER_LENGTH 98
#define VR_INTEGRATOR 1002
#define VR_OCTAVE_OUTPUT 1001
#define VR_INTEGRATE_DTHETA 18
#define VR_OCTAVE_OUTPUT_FILE 1003

//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: value[i] = md->theta_out; break;
        case 2: value[i] = md->omega_out; break;
        case 3: value[i] = md->alpha_out; break;
        case 4: value[i] = md->tau_out; break;
        case 5: value[i] = md->theta_in; break;
        case 6: value[i] = md->omega_in; break;
        case 7: value[i] = md->tau_in; break;
        case 8: value[i] = md->theta0; break;
        case 9: value[i] = md->omega0; break;
        case 10: value[i] = md->kp; break;
        case 11: value[i] = md->tau_max; break;
        case 12: value[i] = md->omega_target; break;
        case 13: value[i] = md->jinv; break;
        case 14: value[i] = md->k1; break;
        case 15: value[i] = md->k2; break;
        case 16: value[i] = md->k_in; break;
        case 17: value[i] = md->d_in; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: md->theta_out = value[i]; break;
        case 2: md->omega_out = value[i]; break;
        case 3: md->alpha_out = value[i]; break;
        case 4: md->tau_out = value[i]; break;
        case 5: md->theta_in = value[i]; break;
        case 6: md->omega_in = value[i]; break;
        case 7: md->tau_in = value[i]; break;
        case 8: md->theta0 = value[i]; break;
        case 9: md->omega0 = value[i]; break;
        case 10: md->kp = value[i]; break;
        case 11: md->tau_max = value[i]; break;
        case 12: md->omega_target = value[i]; break;
        case 13: md->jinv = value[i]; break;
        case 14: md->k1 = value[i]; break;
        case 15: md->k2 = value[i]; break;
        case 16: md->k_in = value[i]; break;
        case 17: md->d_in = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 98: value[i] = md->filter_length; break;
        case 1002: value[i] = md->integrator; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 98: md->filter_length = value[i]; break;
        case 1002: md->integrator = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1001: value[i] = md->octave_output; break;
        case 18: value[i] = md->integrate_dtheta; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1001: md->octave_output = value[i]; break;
        case 18: md->integrate_dtheta = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetString(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1003: value[i] = md->octave_output_file; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1003: if (strlcpy(md->octave_output_file, value[i], sizeof(md->octave_output_file)) >= sizeof(md->octave_output_file)) { return fmi2Error; } break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
