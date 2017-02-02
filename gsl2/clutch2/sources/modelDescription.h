#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER clutch2
#define MODEL_GUID "{6985712a-2b8e-4ebb-9b6c-05d89b0ec1c8}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 34
#define NUMBER_OF_INTEGERS 2
#define NUMBER_OF_BOOLEANS 4
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real x0_e; //VR=0
    fmi2Real v0_e; //VR=1
    fmi2Real dx0_e; //VR=2
    fmi2Real x0_s; //VR=3
    fmi2Real v0_s; //VR=4
    fmi2Real dx0_s; //VR=5
    fmi2Real k_ec; //VR=6
    fmi2Real gamma_ec; //VR=7
    fmi2Real k_sc; //VR=9
    fmi2Real gamma_sc; //VR=10
    fmi2Real mass_e; //VR=12
    fmi2Real gamma_e; //VR=13
    fmi2Real mass_s; //VR=14
    fmi2Real gamma_s; //VR=15
    fmi2Real clutch_damping; //VR=16
    fmi2Real gear_k; //VR=18
    fmi2Real gear_d; //VR=19
    fmi2Real x_in_e; //VR=20
    fmi2Real v_in_e; //VR=21
    fmi2Real force_in_e; //VR=22
    fmi2Real force_in_ex; //VR=23
    fmi2Real x_in_s; //VR=24
    fmi2Real v_in_s; //VR=25
    fmi2Real force_in_s; //VR=26
    fmi2Real force_in_sx; //VR=27
    fmi2Real clutch_position; //VR=28
    fmi2Real x_e; //VR=30
    fmi2Real v_e; //VR=31
    fmi2Real a_e; //VR=32
    fmi2Real force_e; //VR=33
    fmi2Real x_s; //VR=34
    fmi2Real v_s; //VR=35
    fmi2Real a_s; //VR=36
    fmi2Real force_s; //VR=37
    fmi2Integer filter_length; //VR=98
    fmi2Integer gear; //VR=29
    fmi2Boolean integrate_dx_e; //VR=8
    fmi2Boolean is_gearbox; //VR=17
    fmi2Boolean integrate_dx_s; //VR=11
    fmi2Boolean octave_output; //VR=97
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //x0_e
    0.0, //v0_e
    0.0, //dx0_e
    0.0, //x0_s
    0.0, //v0_s
    0.0, //dx0_s
    0.0, //k_ec
    0.0, //gamma_ec
    0.0, //k_sc
    0.0, //gamma_sc
    1.0, //mass_e
    1.0, //gamma_e
    1.0, //mass_s
    1.0, //gamma_s
    1.0, //clutch_damping
    10000.0, //gear_k
    0.0, //gear_d
    0.0, //x_in_e
    0.0, //v_in_e
    0.0, //force_in_e
    0.0, //force_in_ex
    0.0, //x_in_s
    0.0, //v_in_s
    0.0, //force_in_s
    0.0, //force_in_sx
    0.0, //clutch_position
    0, //x_e
    0, //v_e
    0, //a_e
    0, //force_e
    0, //x_s
    0, //v_s
    0, //a_s
    0, //force_s
    0, //filter_length
    1, //gear
    0, //integrate_dx_e
    0, //is_gearbox
    0, //integrate_dx_s
    0, //octave_output
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
#define VR_FILTER_LENGTH 98
#define VR_GEAR 29
#define VR_INTEGRATE_DX_E 8
#define VR_IS_GEARBOX 17
#define VR_INTEGRATE_DX_S 11
#define VR_OCTAVE_OUTPUT 97

//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0_E: value[i] = md->x0_e; break;
        case VR_V0_E: value[i] = md->v0_e; break;
        case VR_DX0_E: value[i] = md->dx0_e; break;
        case VR_X0_S: value[i] = md->x0_s; break;
        case VR_V0_S: value[i] = md->v0_s; break;
        case VR_DX0_S: value[i] = md->dx0_s; break;
        case VR_K_EC: value[i] = md->k_ec; break;
        case VR_GAMMA_EC: value[i] = md->gamma_ec; break;
        case VR_K_SC: value[i] = md->k_sc; break;
        case VR_GAMMA_SC: value[i] = md->gamma_sc; break;
        case VR_MASS_E: value[i] = md->mass_e; break;
        case VR_GAMMA_E: value[i] = md->gamma_e; break;
        case VR_MASS_S: value[i] = md->mass_s; break;
        case VR_GAMMA_S: value[i] = md->gamma_s; break;
        case VR_CLUTCH_DAMPING: value[i] = md->clutch_damping; break;
        case VR_GEAR_K: value[i] = md->gear_k; break;
        case VR_GEAR_D: value[i] = md->gear_d; break;
        case VR_X_IN_E: value[i] = md->x_in_e; break;
        case VR_V_IN_E: value[i] = md->v_in_e; break;
        case VR_FORCE_IN_E: value[i] = md->force_in_e; break;
        case VR_FORCE_IN_EX: value[i] = md->force_in_ex; break;
        case VR_X_IN_S: value[i] = md->x_in_s; break;
        case VR_V_IN_S: value[i] = md->v_in_s; break;
        case VR_FORCE_IN_S: value[i] = md->force_in_s; break;
        case VR_FORCE_IN_SX: value[i] = md->force_in_sx; break;
        case VR_CLUTCH_POSITION: value[i] = md->clutch_position; break;
        case VR_X_E: value[i] = md->x_e; break;
        case VR_V_E: value[i] = md->v_e; break;
        case VR_A_E: value[i] = md->a_e; break;
        case VR_FORCE_E: value[i] = md->force_e; break;
        case VR_X_S: value[i] = md->x_s; break;
        case VR_V_S: value[i] = md->v_s; break;
        case VR_A_S: value[i] = md->a_s; break;
        case VR_FORCE_S: value[i] = md->force_s; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0_E: md->x0_e = value[i]; break;
        case VR_V0_E: md->v0_e = value[i]; break;
        case VR_DX0_E: md->dx0_e = value[i]; break;
        case VR_X0_S: md->x0_s = value[i]; break;
        case VR_V0_S: md->v0_s = value[i]; break;
        case VR_DX0_S: md->dx0_s = value[i]; break;
        case VR_K_EC: md->k_ec = value[i]; break;
        case VR_GAMMA_EC: md->gamma_ec = value[i]; break;
        case VR_K_SC: md->k_sc = value[i]; break;
        case VR_GAMMA_SC: md->gamma_sc = value[i]; break;
        case VR_MASS_E: md->mass_e = value[i]; break;
        case VR_GAMMA_E: md->gamma_e = value[i]; break;
        case VR_MASS_S: md->mass_s = value[i]; break;
        case VR_GAMMA_S: md->gamma_s = value[i]; break;
        case VR_CLUTCH_DAMPING: md->clutch_damping = value[i]; break;
        case VR_GEAR_K: md->gear_k = value[i]; break;
        case VR_GEAR_D: md->gear_d = value[i]; break;
        case VR_X_IN_E: md->x_in_e = value[i]; break;
        case VR_V_IN_E: md->v_in_e = value[i]; break;
        case VR_FORCE_IN_E: md->force_in_e = value[i]; break;
        case VR_FORCE_IN_EX: md->force_in_ex = value[i]; break;
        case VR_X_IN_S: md->x_in_s = value[i]; break;
        case VR_V_IN_S: md->v_in_s = value[i]; break;
        case VR_FORCE_IN_S: md->force_in_s = value[i]; break;
        case VR_FORCE_IN_SX: md->force_in_sx = value[i]; break;
        case VR_CLUTCH_POSITION: md->clutch_position = value[i]; break;
        case VR_X_E: md->x_e = value[i]; break;
        case VR_V_E: md->v_e = value[i]; break;
        case VR_A_E: md->a_e = value[i]; break;
        case VR_FORCE_E: md->force_e = value[i]; break;
        case VR_X_S: md->x_s = value[i]; break;
        case VR_V_S: md->v_s = value[i]; break;
        case VR_A_S: md->a_s = value[i]; break;
        case VR_FORCE_S: md->force_s = value[i]; break;
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
        case VR_GEAR: value[i] = md->gear; break;
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
        case VR_GEAR: md->gear = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_INTEGRATE_DX_E: value[i] = md->integrate_dx_e; break;
        case VR_IS_GEARBOX: value[i] = md->is_gearbox; break;
        case VR_INTEGRATE_DX_S: value[i] = md->integrate_dx_s; break;
        case VR_OCTAVE_OUTPUT: value[i] = md->octave_output; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_INTEGRATE_DX_E: md->integrate_dx_e = value[i]; break;
        case VR_IS_GEARBOX: md->is_gearbox = value[i]; break;
        case VR_INTEGRATE_DX_S: md->integrate_dx_s = value[i]; break;
        case VR_OCTAVE_OUTPUT: md->octave_output = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
