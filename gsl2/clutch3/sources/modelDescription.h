/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER clutch3
#define MODEL_GUID "{2e25fc3c-fa3d-4ad0-8d4b-b1895541fc9a}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 34
#define NUMBER_OF_INTEGERS 4
#define NUMBER_OF_BOOLEANS 6
#define NUMBER_OF_STRINGS 1
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    x0_e; //VR=0
    fmi2Real    v0_e; //VR=1
    fmi2Real    dx0_e; //VR=2
    fmi2Real    x0_s; //VR=3
    fmi2Real    v0_s; //VR=4
    fmi2Real    dx0_s; //VR=5
    fmi2Real    k_ec; //VR=6
    fmi2Real    gamma_ec; //VR=7
    fmi2Real    k_sc; //VR=9
    fmi2Real    gamma_sc; //VR=10
    fmi2Real    mass_e; //VR=12
    fmi2Real    gamma_e; //VR=13
    fmi2Real    mass_s; //VR=14
    fmi2Real    gamma_s; //VR=15
    fmi2Real    clutch_damping; //VR=16
    fmi2Real    gear_k; //VR=18
    fmi2Real    gear_d; //VR=19
    fmi2Real    x_in_e; //VR=20
    fmi2Real    v_in_e; //VR=21
    fmi2Real    force_in_e; //VR=22
    fmi2Real    force_in_ex; //VR=23
    fmi2Real    x_in_s; //VR=24
    fmi2Real    v_in_s; //VR=25
    fmi2Real    force_in_s; //VR=26
    fmi2Real    force_in_sx; //VR=27
    fmi2Real    clutch_position; //VR=28
    fmi2Real    x_e; //VR=30
    fmi2Real    v_e; //VR=31
    fmi2Real    a_e; //VR=32
    fmi2Real    force_e; //VR=33
    fmi2Real    x_s; //VR=34
    fmi2Real    v_s; //VR=35
    fmi2Real    a_s; //VR=36
    fmi2Real    force_s; //VR=37
    fmi2Integer gear; //VR=29
    fmi2Integer filter_length; //VR=98
    fmi2Integer n_steps; //VR=100
    fmi2Integer integrator; //VR=201
    fmi2Boolean integrate_dx_e; //VR=8
    fmi2Boolean integrate_dx_s; //VR=11
    fmi2Boolean is_gearbox; //VR=17
    fmi2Boolean octave_output; //VR=97
    fmi2Boolean reset_dx_e; //VR=202
    fmi2Boolean reset_dx_s; //VR=203
    fmi2Char    octave_output_file[500]; //VR=202
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.000000, //x0_e
    0.000000, //v0_e
    0.000000, //dx0_e
    0.000000, //x0_s
    0.000000, //v0_s
    0.000000, //dx0_s
    0.000000, //k_ec
    0.000000, //gamma_ec
    0.000000, //k_sc
    0.000000, //gamma_sc
    1.000000, //mass_e
    1.000000, //gamma_e
    1.000000, //mass_s
    1.000000, //gamma_s
    1.000000, //clutch_damping
    10000.000000, //gear_k
    0.000000, //gear_d
    0.000000, //x_in_e
    0.000000, //v_in_e
    0.000000, //force_in_e
    0.000000, //force_in_ex
    0.000000, //x_in_s
    0.000000, //v_in_s
    0.000000, //force_in_s
    0.000000, //force_in_sx
    0.000000, //clutch_position
    0.000000, //x_e
    0.000000, //v_e
    0.000000, //a_e
    0.000000, //force_e
    0.000000, //x_s
    0.000000, //v_s
    0.000000, //a_s
    0.000000, //force_s
    1, //gear
    0, //filter_length
    0, //n_steps
    2, //integrator
    0, //integrate_dx_e
    0, //integrate_dx_s
    0, //is_gearbox
    0, //octave_output
    0, //reset_dx_e
    0, //reset_dx_s
    "clutch3.m", //octave_output_file
};


#define VR_X0_E 0
#define VR_V0_E 1
#define VR_DX0_E 2
#define VR_X0_S 3
#define VR_V0_S 4
#define VR_DX0_S 5
#define VR_K_EC 6
#define VR_GAMMA_EC 7
#define VR_K_SC 9
#define VR_GAMMA_SC 10
#define VR_MASS_E 12
#define VR_GAMMA_E 13
#define VR_MASS_S 14
#define VR_GAMMA_S 15
#define VR_CLUTCH_DAMPING 16
#define VR_GEAR_K 18
#define VR_GEAR_D 19
#define VR_X_IN_E 20
#define VR_V_IN_E 21
#define VR_FORCE_IN_E 22
#define VR_FORCE_IN_EX 23
#define VR_X_IN_S 24
#define VR_V_IN_S 25
#define VR_FORCE_IN_S 26
#define VR_FORCE_IN_SX 27
#define VR_CLUTCH_POSITION 28
#define VR_X_E 30
#define VR_V_E 31
#define VR_A_E 32
#define VR_FORCE_E 33
#define VR_X_S 34
#define VR_V_S 35
#define VR_A_S 36
#define VR_FORCE_S 37
#define VR_GEAR 29
#define VR_FILTER_LENGTH 98
#define VR_N_STEPS 100
#define VR_INTEGRATOR 201
#define VR_INTEGRATE_DX_E 8
#define VR_INTEGRATE_DX_S 11
#define VR_IS_GEARBOX 17
#define VR_OCTAVE_OUTPUT 97
#define VR_RESET_DX_E 202
#define VR_RESET_DX_S 203
#define VR_OCTAVE_OUTPUT_FILE 202

//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->x0_e; break;
        case 1: value[i] = md->v0_e; break;
        case 2: value[i] = md->dx0_e; break;
        case 3: value[i] = md->x0_s; break;
        case 4: value[i] = md->v0_s; break;
        case 5: value[i] = md->dx0_s; break;
        case 6: value[i] = md->k_ec; break;
        case 7: value[i] = md->gamma_ec; break;
        case 9: value[i] = md->k_sc; break;
        case 10: value[i] = md->gamma_sc; break;
        case 12: value[i] = md->mass_e; break;
        case 13: value[i] = md->gamma_e; break;
        case 14: value[i] = md->mass_s; break;
        case 15: value[i] = md->gamma_s; break;
        case 16: value[i] = md->clutch_damping; break;
        case 18: value[i] = md->gear_k; break;
        case 19: value[i] = md->gear_d; break;
        case 20: value[i] = md->x_in_e; break;
        case 21: value[i] = md->v_in_e; break;
        case 22: value[i] = md->force_in_e; break;
        case 23: value[i] = md->force_in_ex; break;
        case 24: value[i] = md->x_in_s; break;
        case 25: value[i] = md->v_in_s; break;
        case 26: value[i] = md->force_in_s; break;
        case 27: value[i] = md->force_in_sx; break;
        case 28: value[i] = md->clutch_position; break;
        case 30: value[i] = md->x_e; break;
        case 31: value[i] = md->v_e; break;
        case 32: value[i] = md->a_e; break;
        case 33: value[i] = md->force_e; break;
        case 34: value[i] = md->x_s; break;
        case 35: value[i] = md->v_s; break;
        case 36: value[i] = md->a_s; break;
        case 37: value[i] = md->force_s; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->x0_e = value[i]; break;
        case 1: md->v0_e = value[i]; break;
        case 2: md->dx0_e = value[i]; break;
        case 3: md->x0_s = value[i]; break;
        case 4: md->v0_s = value[i]; break;
        case 5: md->dx0_s = value[i]; break;
        case 6: md->k_ec = value[i]; break;
        case 7: md->gamma_ec = value[i]; break;
        case 9: md->k_sc = value[i]; break;
        case 10: md->gamma_sc = value[i]; break;
        case 12: md->mass_e = value[i]; break;
        case 13: md->gamma_e = value[i]; break;
        case 14: md->mass_s = value[i]; break;
        case 15: md->gamma_s = value[i]; break;
        case 16: md->clutch_damping = value[i]; break;
        case 18: md->gear_k = value[i]; break;
        case 19: md->gear_d = value[i]; break;
        case 20: md->x_in_e = value[i]; break;
        case 21: md->v_in_e = value[i]; break;
        case 22: md->force_in_e = value[i]; break;
        case 23: md->force_in_ex = value[i]; break;
        case 24: md->x_in_s = value[i]; break;
        case 25: md->v_in_s = value[i]; break;
        case 26: md->force_in_s = value[i]; break;
        case 27: md->force_in_sx = value[i]; break;
        case 28: md->clutch_position = value[i]; break;
        case 30: md->x_e = value[i]; break;
        case 31: md->v_e = value[i]; break;
        case 32: md->a_e = value[i]; break;
        case 33: md->force_e = value[i]; break;
        case 34: md->x_s = value[i]; break;
        case 35: md->v_s = value[i]; break;
        case 36: md->a_s = value[i]; break;
        case 37: md->force_s = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 29: value[i] = md->gear; break;
        case 98: value[i] = md->filter_length; break;
        case 100: value[i] = md->n_steps; break;
        case 201: value[i] = md->integrator; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 29: md->gear = value[i]; break;
        case 98: md->filter_length = value[i]; break;
        case 100: md->n_steps = value[i]; break;
        case 201: md->integrator = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 8: value[i] = md->integrate_dx_e; break;
        case 11: value[i] = md->integrate_dx_s; break;
        case 17: value[i] = md->is_gearbox; break;
        case 97: value[i] = md->octave_output; break;
        case 202: value[i] = md->reset_dx_e; break;
        case 203: value[i] = md->reset_dx_s; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 8: md->integrate_dx_e = value[i]; break;
        case 11: md->integrate_dx_s = value[i]; break;
        case 17: md->is_gearbox = value[i]; break;
        case 97: md->octave_output = value[i]; break;
        case 202: md->reset_dx_e = value[i]; break;
        case 203: md->reset_dx_s = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetString(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 202: value[i] = md->octave_output_file; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 202: if (strlcpy(md->octave_output_file, value[i], sizeof(md->octave_output_file)) >= sizeof(md->octave_output_file)) { return fmi2Error; } break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
