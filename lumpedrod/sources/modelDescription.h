/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.

#define MODEL_IDENTIFIER lumpedrod
#define MODEL_GUID "{b8998512-96a7-4e6d-8350-6d1f9aeae4a1}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 32
#define NUMBER_OF_INTEGERS 1
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real theta1; //VR=0
    fmi2Real theta2; //VR=1
    fmi2Real omega1; //VR=2
    fmi2Real omega2; //VR=3
    fmi2Real alpha1; //VR=4
    fmi2Real alpha2; //VR=5
    fmi2Real dtheta1; //VR=6
    fmi2Real dtheta2; //VR=7
    fmi2Real out_torque1; //VR=8
    fmi2Real out_torque2; //VR=9
    fmi2Real tau1; //VR=10
    fmi2Real tau2; //VR=11
    fmi2Real theta_drive1; //VR=12
    fmi2Real omega_drive1; //VR=13
    fmi2Real theta_drive2; //VR=14
    fmi2Real omega_drive2; //VR=15
    fmi2Real J; //VR=16
    fmi2Real compliance; //VR=17
    fmi2Real D; //VR=18
    fmi2Real K_drive1; //VR=19
    fmi2Real D_drive1; //VR=20
    fmi2Real K_drive2; //VR=21
    fmi2Real D_drive2; //VR=22
    fmi2Real driver_sign1; //VR=23
    fmi2Real driver_sign2; //VR=24
    fmi2Real integrate_dt1; //VR=25
    fmi2Real integrate_dt2; //VR=26
    fmi2Real step; //VR=27
    fmi2Real theta01; //VR=29
    fmi2Real theta02; //VR=30
    fmi2Real omega01; //VR=31
    fmi2Real omega02; //VR=32
    fmi2Integer n_elements; //VR=28

} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.0, //theta1
    0.0, //theta2
    0.0, //omega1
    0.0, //omega2
    0.0, //alpha1
    0.0, //alpha2
    0.0, //dtheta1
    0.0, //dtheta2
    0.0, //out_torque1
    0.0, //out_torque2
    0.0, //tau1
    0.0, //tau2
    0.0, //theta_drive1
    0.0, //omega_drive1
    0.0, //theta_drive2
    0.0, //omega_drive2
    10.0, //J
    0.0001, //compliance
    2.0, //D
    0.0, //K_drive1
    0.0, //D_drive1
    0.0, //K_drive2
    0.0, //D_drive2
    1.0, //driver_sign1
    1.0, //driver_sign2
    1.0, //integrate_dt1
    1.0, //integrate_dt2
    0.1, //step
    0.0, //theta01
    0.0, //theta02
    0.0, //omega01
    0.0, //omega02
    10, //n_elements

};


#define VR_THETA1 0
#define VR_THETA2 1
#define VR_OMEGA1 2
#define VR_OMEGA2 3
#define VR_ALPHA1 4
#define VR_ALPHA2 5
#define VR_DTHETA1 6
#define VR_DTHETA2 7
#define VR_OUT_TORQUE1 8
#define VR_OUT_TORQUE2 9
#define VR_TAU1 10
#define VR_TAU2 11
#define VR_THETA_DRIVE1 12
#define VR_OMEGA_DRIVE1 13
#define VR_THETA_DRIVE2 14
#define VR_OMEGA_DRIVE2 15
#define VR_J 16
#define VR_COMPLIANCE 17
#define VR_D 18
#define VR_K_DRIVE1 19
#define VR_D_DRIVE1 20
#define VR_K_DRIVE2 21
#define VR_D_DRIVE2 22
#define VR_DRIVER_SIGN1 23
#define VR_DRIVER_SIGN2 24
#define VR_INTEGRATE_DT1 25
#define VR_INTEGRATE_DT2 26
#define VR_STEP 27
#define VR_THETA01 29
#define VR_THETA02 30
#define VR_OMEGA01 31
#define VR_OMEGA02 32
#define VR_N_ELEMENTS 28


//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->theta1; break;
        case 1: value[i] = md->theta2; break;
        case 2: value[i] = md->omega1; break;
        case 3: value[i] = md->omega2; break;
        case 4: value[i] = md->alpha1; break;
        case 5: value[i] = md->alpha2; break;
        case 6: value[i] = md->dtheta1; break;
        case 7: value[i] = md->dtheta2; break;
        case 8: value[i] = md->out_torque1; break;
        case 9: value[i] = md->out_torque2; break;
        case 10: value[i] = md->tau1; break;
        case 11: value[i] = md->tau2; break;
        case 12: value[i] = md->theta_drive1; break;
        case 13: value[i] = md->omega_drive1; break;
        case 14: value[i] = md->theta_drive2; break;
        case 15: value[i] = md->omega_drive2; break;
        case 16: value[i] = md->J; break;
        case 17: value[i] = md->compliance; break;
        case 18: value[i] = md->D; break;
        case 19: value[i] = md->K_drive1; break;
        case 20: value[i] = md->D_drive1; break;
        case 21: value[i] = md->K_drive2; break;
        case 22: value[i] = md->D_drive2; break;
        case 23: value[i] = md->driver_sign1; break;
        case 24: value[i] = md->driver_sign2; break;
        case 25: value[i] = md->integrate_dt1; break;
        case 26: value[i] = md->integrate_dt2; break;
        case 27: value[i] = md->step; break;
        case 29: value[i] = md->theta01; break;
        case 30: value[i] = md->theta02; break;
        case 31: value[i] = md->omega01; break;
        case 32: value[i] = md->omega02; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->theta1 = value[i]; break;
        case 1: md->theta2 = value[i]; break;
        case 2: md->omega1 = value[i]; break;
        case 3: md->omega2 = value[i]; break;
        case 4: md->alpha1 = value[i]; break;
        case 5: md->alpha2 = value[i]; break;
        case 6: md->dtheta1 = value[i]; break;
        case 7: md->dtheta2 = value[i]; break;
        case 8: md->out_torque1 = value[i]; break;
        case 9: md->out_torque2 = value[i]; break;
        case 10: md->tau1 = value[i]; break;
        case 11: md->tau2 = value[i]; break;
        case 12: md->theta_drive1 = value[i]; break;
        case 13: md->omega_drive1 = value[i]; break;
        case 14: md->theta_drive2 = value[i]; break;
        case 15: md->omega_drive2 = value[i]; break;
        case 16: md->J = value[i]; break;
        case 17: md->compliance = value[i]; break;
        case 18: md->D = value[i]; break;
        case 19: md->K_drive1 = value[i]; break;
        case 20: md->D_drive1 = value[i]; break;
        case 21: md->K_drive2 = value[i]; break;
        case 22: md->D_drive2 = value[i]; break;
        case 23: md->driver_sign1 = value[i]; break;
        case 24: md->driver_sign2 = value[i]; break;
        case 25: md->integrate_dt1 = value[i]; break;
        case 26: md->integrate_dt2 = value[i]; break;
        case 27: md->step = value[i]; break;
        case 29: md->theta01 = value[i]; break;
        case 30: md->theta02 = value[i]; break;
        case 31: md->omega01 = value[i]; break;
        case 32: md->omega02 = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 28: value[i] = md->n_elements; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 28: md->n_elements = value[i]; break;
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
