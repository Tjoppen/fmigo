#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER trailer
#define MODEL_GUID "{ca64d56e-cc83-44bb-ad78-afbe219a947c}"
#define FMI_COSIMULATION

#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 36
#define NUMBER_OF_INTEGERS 1
#define NUMBER_OF_BOOLEANS 2
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real x0; //VR=1
    fmi2Real v0; //VR=2
    fmi2Real mass; //VR=3
    fmi2Real r_w; //VR=4
    fmi2Real r_g; //VR=5
    fmi2Real area; //VR=6
    fmi2Real rho; //VR=7
    fmi2Real c_d; //VR=8
    fmi2Real g; //VR=9
    fmi2Real c_r_1; //VR=10
    fmi2Real c_r_2; //VR=11
    fmi2Real mu; //VR=12
    fmi2Real k_d; //VR=13
    fmi2Real gamma_d; //VR=14
    fmi2Real k_t; //VR=15
    fmi2Real gamma_t; //VR=16
    fmi2Real integrator; //VR=19
    fmi2Real phi_i; //VR=20
    fmi2Real omega_i; //VR=21
    fmi2Real tau_d; //VR=22
    fmi2Real tau_e; //VR=23
    fmi2Real angle; //VR=24
    fmi2Real brake; //VR=25
    fmi2Real x_in; //VR=26
    fmi2Real v_in; //VR=27
    fmi2Real f_in; //VR=28
    fmi2Real x; //VR=29
    fmi2Real v; //VR=30
    fmi2Real a; //VR=31
    fmi2Real phi; //VR=32
    fmi2Real omega; //VR=33
    fmi2Real alpha; //VR=34
    fmi2Real tau_c; //VR=35
    fmi2Real f_c; //VR=36
    fmi2Real triangle_amplitude; //VR=37
    fmi2Real triangle_wavelength; //VR=38
    fmi2Integer filter_length; //VR=98
    fmi2Boolean integrate_dw; //VR=17
    fmi2Boolean integrate_dx; //VR=18

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //x0
    0.0, //v0
    10000.0, //mass
    0.5, //r_w
    1.0, //r_g
    10.0, //area
    0.001, //rho
    0.5, //c_d
    9.82, //g
    0.0, //c_r_1
    0.0, //c_r_2
    0.0, //mu
    0.0, //k_d
    0.0, //gamma_d
    0.0, //k_t
    0.0, //gamma_t
    0.0, //integrator
    0.0, //phi_i
    0.0, //omega_i
    0.0, //tau_d
    0.0, //tau_e
    0.0, //angle
    0.0, //brake
    0.0, //x_in
    0.0, //v_in
    0.0, //f_in
    0, //x
    0, //v
    0, //a
    0, //phi
    0, //omega
    0, //alpha
    0, //tau_c
    0, //f_c
    0.0, //triangle_amplitude
    5.0, //triangle_wavelength
    0, //filter_length
    0, //integrate_dw
    0, //integrate_dx

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
#define VR_INTEGRATOR 19
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
#define VR_INTEGRATE_DW 17
#define VR_INTEGRATE_DX 18


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters



static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0: value[i] = md->x0; break;
        case VR_V0: value[i] = md->v0; break;
        case VR_MASS: value[i] = md->mass; break;
        case VR_R_W: value[i] = md->r_w; break;
        case VR_R_G: value[i] = md->r_g; break;
        case VR_AREA: value[i] = md->area; break;
        case VR_RHO: value[i] = md->rho; break;
        case VR_C_D: value[i] = md->c_d; break;
        case VR_G: value[i] = md->g; break;
        case VR_C_R_1: value[i] = md->c_r_1; break;
        case VR_C_R_2: value[i] = md->c_r_2; break;
        case VR_MU: value[i] = md->mu; break;
        case VR_K_D: value[i] = md->k_d; break;
        case VR_GAMMA_D: value[i] = md->gamma_d; break;
        case VR_K_T: value[i] = md->k_t; break;
        case VR_GAMMA_T: value[i] = md->gamma_t; break;
        case VR_INTEGRATOR: value[i] = md->integrator; break;
        case VR_PHI_I: value[i] = md->phi_i; break;
        case VR_OMEGA_I: value[i] = md->omega_i; break;
        case VR_TAU_D: value[i] = md->tau_d; break;
        case VR_TAU_E: value[i] = md->tau_e; break;
        case VR_ANGLE: value[i] = md->angle; break;
        case VR_BRAKE: value[i] = md->brake; break;
        case VR_X_IN: value[i] = md->x_in; break;
        case VR_V_IN: value[i] = md->v_in; break;
        case VR_F_IN: value[i] = md->f_in; break;
        case VR_X: value[i] = md->x; break;
        case VR_V: value[i] = md->v; break;
        case VR_A: value[i] = md->a; break;
        case VR_PHI: value[i] = md->phi; break;
        case VR_OMEGA: value[i] = md->omega; break;
        case VR_ALPHA: value[i] = md->alpha; break;
        case VR_TAU_C: value[i] = md->tau_c; break;
        case VR_F_C: value[i] = md->f_c; break;
        case VR_TRIANGLE_AMPLITUDE: value[i] = md->triangle_amplitude; break;
        case VR_TRIANGLE_WAVELENGTH: value[i] = md->triangle_wavelength; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0: md->x0 = value[i]; break;
        case VR_V0: md->v0 = value[i]; break;
        case VR_MASS: md->mass = value[i]; break;
        case VR_R_W: md->r_w = value[i]; break;
        case VR_R_G: md->r_g = value[i]; break;
        case VR_AREA: md->area = value[i]; break;
        case VR_RHO: md->rho = value[i]; break;
        case VR_C_D: md->c_d = value[i]; break;
        case VR_G: md->g = value[i]; break;
        case VR_C_R_1: md->c_r_1 = value[i]; break;
        case VR_C_R_2: md->c_r_2 = value[i]; break;
        case VR_MU: md->mu = value[i]; break;
        case VR_K_D: md->k_d = value[i]; break;
        case VR_GAMMA_D: md->gamma_d = value[i]; break;
        case VR_K_T: md->k_t = value[i]; break;
        case VR_GAMMA_T: md->gamma_t = value[i]; break;
        case VR_INTEGRATOR: md->integrator = value[i]; break;
        case VR_PHI_I: md->phi_i = value[i]; break;
        case VR_OMEGA_I: md->omega_i = value[i]; break;
        case VR_TAU_D: md->tau_d = value[i]; break;
        case VR_TAU_E: md->tau_e = value[i]; break;
        case VR_ANGLE: md->angle = value[i]; break;
        case VR_BRAKE: md->brake = value[i]; break;
        case VR_X_IN: md->x_in = value[i]; break;
        case VR_V_IN: md->v_in = value[i]; break;
        case VR_F_IN: md->f_in = value[i]; break;
        case VR_X: md->x = value[i]; break;
        case VR_V: md->v = value[i]; break;
        case VR_A: md->a = value[i]; break;
        case VR_PHI: md->phi = value[i]; break;
        case VR_OMEGA: md->omega = value[i]; break;
        case VR_ALPHA: md->alpha = value[i]; break;
        case VR_TAU_C: md->tau_c = value[i]; break;
        case VR_F_C: md->f_c = value[i]; break;
        case VR_TRIANGLE_AMPLITUDE: md->triangle_amplitude = value[i]; break;
        case VR_TRIANGLE_WAVELENGTH: md->triangle_wavelength = value[i]; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_FILTER_LENGTH: value[i] = md->filter_length; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_FILTER_LENGTH: md->filter_length = value[i]; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_INTEGRATE_DW: value[i] = md->integrate_dw; break;
        case VR_INTEGRATE_DX: value[i] = md->integrate_dx; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
        int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_INTEGRATE_DW: md->integrate_dw = value[i]; break;
        case VR_INTEGRATE_DX: md->integrate_dx = value[i]; break;

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
