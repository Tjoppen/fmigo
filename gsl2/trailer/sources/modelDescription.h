/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER trailer
#define MODEL_GUID "{ca64d56e-cc83-44bb-ad78-afbe219a947c}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 36
#define NUMBER_OF_INTEGERS 2
#define NUMBER_OF_BOOLEANS 3
#define NUMBER_OF_STRINGS 1
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

//will be defined in fmuTemplate.h
//needed in generated_fmi2GetX/fmi2SetX for wrapper.c
struct ModelInstance;


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    x0; //VR=1
    fmi2Real    v0; //VR=2
    fmi2Real    mass; //VR=3
    fmi2Real    r_w; //VR=4
    fmi2Real    r_g; //VR=5
    fmi2Real    area; //VR=6
    fmi2Real    rho; //VR=7
    fmi2Real    c_d; //VR=8
    fmi2Real    g; //VR=9
    fmi2Real    c_r_1; //VR=10
    fmi2Real    c_r_2; //VR=11
    fmi2Real    mu; //VR=12
    fmi2Real    k_d; //VR=13
    fmi2Real    gamma_d; //VR=14
    fmi2Real    k_t; //VR=15
    fmi2Real    gamma_t; //VR=16
    fmi2Real    dummy; //VR=19
    fmi2Real    phi_i; //VR=20
    fmi2Real    omega_i; //VR=21
    fmi2Real    tau_d; //VR=22
    fmi2Real    tau_e; //VR=23
    fmi2Real    angle; //VR=24
    fmi2Real    brake; //VR=25
    fmi2Real    x_in; //VR=26
    fmi2Real    v_in; //VR=27
    fmi2Real    f_in; //VR=28
    fmi2Real    x; //VR=29
    fmi2Real    v; //VR=30
    fmi2Real    a; //VR=31
    fmi2Real    phi; //VR=32
    fmi2Real    omega; //VR=33
    fmi2Real    alpha; //VR=34
    fmi2Real    tau_c; //VR=35
    fmi2Real    f_c; //VR=36
    fmi2Real    triangle_amplitude; //VR=37
    fmi2Real    triangle_wavelength; //VR=38
    fmi2Integer filter_length; //VR=98
    fmi2Integer integrator; //VR=1002
    fmi2Boolean integrate_dw; //VR=17
    fmi2Boolean integrate_dx; //VR=18
    fmi2Boolean octave_output; //VR=1001
    fmi2Char    octave_output_file[500]; //VR=1003
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.000000, //x0
    0.000000, //v0
    10000.000000, //mass
    0.500000, //r_w
    1.000000, //r_g
    10.000000, //area
    0.001000, //rho
    0.500000, //c_d
    9.820000, //g
    0.000000, //c_r_1
    0.000000, //c_r_2
    0.000000, //mu
    0.000000, //k_d
    0.000000, //gamma_d
    0.000000, //k_t
    0.000000, //gamma_t
    0.000000, //dummy
    0.000000, //phi_i
    0.000000, //omega_i
    0.000000, //tau_d
    0.000000, //tau_e
    0.000000, //angle
    0.000000, //brake
    0.000000, //x_in
    0.000000, //v_in
    0.000000, //f_in
    0.000000, //x
    0.000000, //v
    0.000000, //a
    0.000000, //phi
    0.000000, //omega
    0.000000, //alpha
    0.000000, //tau_c
    0.000000, //f_c
    0.000000, //triangle_amplitude
    5.000000, //triangle_wavelength
    0, //filter_length
    2, //integrator
    0, //integrate_dw
    0, //integrate_dx
    0, //octave_output
    "trailer.m", //octave_output_file
};


#define VR_X0 1
#define VR_V0 2
#define VR_MASS 3
#define VR_R_W 4
#define VR_R_G 5
#define VR_AREA 6
#define VR_RHO 7
#define VR_C_D 8
#define VR_G 9
#define VR_C_R_1 10
#define VR_C_R_2 11
#define VR_MU 12
#define VR_K_D 13
#define VR_GAMMA_D 14
#define VR_K_T 15
#define VR_GAMMA_T 16
#define VR_DUMMY 19
#define VR_PHI_I 20
#define VR_OMEGA_I 21
#define VR_TAU_D 22
#define VR_TAU_E 23
#define VR_ANGLE 24
#define VR_BRAKE 25
#define VR_X_IN 26
#define VR_V_IN 27
#define VR_F_IN 28
#define VR_X 29
#define VR_V 30
#define VR_A 31
#define VR_PHI 32
#define VR_OMEGA 33
#define VR_ALPHA 34
#define VR_TAU_C 35
#define VR_F_C 36
#define VR_TRIANGLE_AMPLITUDE 37
#define VR_TRIANGLE_WAVELENGTH 38
#define VR_FILTER_LENGTH 98
#define VR_INTEGRATOR 1002
#define VR_INTEGRATE_DW 17
#define VR_INTEGRATE_DX 18
#define VR_OCTAVE_OUTPUT 1001
#define VR_OCTAVE_OUTPUT_FILE 1003

//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: value[i] = md->x0; break;
        case 2: value[i] = md->v0; break;
        case 3: value[i] = md->mass; break;
        case 4: value[i] = md->r_w; break;
        case 5: value[i] = md->r_g; break;
        case 6: value[i] = md->area; break;
        case 7: value[i] = md->rho; break;
        case 8: value[i] = md->c_d; break;
        case 9: value[i] = md->g; break;
        case 10: value[i] = md->c_r_1; break;
        case 11: value[i] = md->c_r_2; break;
        case 12: value[i] = md->mu; break;
        case 13: value[i] = md->k_d; break;
        case 14: value[i] = md->gamma_d; break;
        case 15: value[i] = md->k_t; break;
        case 16: value[i] = md->gamma_t; break;
        case 19: value[i] = md->dummy; break;
        case 20: value[i] = md->phi_i; break;
        case 21: value[i] = md->omega_i; break;
        case 22: value[i] = md->tau_d; break;
        case 23: value[i] = md->tau_e; break;
        case 24: value[i] = md->angle; break;
        case 25: value[i] = md->brake; break;
        case 26: value[i] = md->x_in; break;
        case 27: value[i] = md->v_in; break;
        case 28: value[i] = md->f_in; break;
        case 29: value[i] = md->x; break;
        case 30: value[i] = md->v; break;
        case 31: value[i] = md->a; break;
        case 32: value[i] = md->phi; break;
        case 33: value[i] = md->omega; break;
        case 34: value[i] = md->alpha; break;
        case 35: value[i] = md->tau_c; break;
        case 36: value[i] = md->f_c; break;
        case 37: value[i] = md->triangle_amplitude; break;
        case 38: value[i] = md->triangle_wavelength; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: md->x0 = value[i]; break;
        case 2: md->v0 = value[i]; break;
        case 3: md->mass = value[i]; break;
        case 4: md->r_w = value[i]; break;
        case 5: md->r_g = value[i]; break;
        case 6: md->area = value[i]; break;
        case 7: md->rho = value[i]; break;
        case 8: md->c_d = value[i]; break;
        case 9: md->g = value[i]; break;
        case 10: md->c_r_1 = value[i]; break;
        case 11: md->c_r_2 = value[i]; break;
        case 12: md->mu = value[i]; break;
        case 13: md->k_d = value[i]; break;
        case 14: md->gamma_d = value[i]; break;
        case 15: md->k_t = value[i]; break;
        case 16: md->gamma_t = value[i]; break;
        case 19: md->dummy = value[i]; break;
        case 20: md->phi_i = value[i]; break;
        case 21: md->omega_i = value[i]; break;
        case 22: md->tau_d = value[i]; break;
        case 23: md->tau_e = value[i]; break;
        case 24: md->angle = value[i]; break;
        case 25: md->brake = value[i]; break;
        case 26: md->x_in = value[i]; break;
        case 27: md->v_in = value[i]; break;
        case 28: md->f_in = value[i]; break;
        case 29: md->x = value[i]; break;
        case 30: md->v = value[i]; break;
        case 31: md->a = value[i]; break;
        case 32: md->phi = value[i]; break;
        case 33: md->omega = value[i]; break;
        case 34: md->alpha = value[i]; break;
        case 35: md->tau_c = value[i]; break;
        case 36: md->f_c = value[i]; break;
        case 37: md->triangle_amplitude = value[i]; break;
        case 38: md->triangle_wavelength = value[i]; break;
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
        case 17: value[i] = md->integrate_dw; break;
        case 18: value[i] = md->integrate_dx; break;
        case 1001: value[i] = md->octave_output; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 17: md->integrate_dw = value[i]; break;
        case 18: md->integrate_dx = value[i]; break;
        case 1001: md->octave_output = value[i]; break;
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
