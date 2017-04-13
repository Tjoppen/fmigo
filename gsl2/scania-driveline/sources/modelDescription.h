/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER drivetrain_G5IO_m_function_only
#define MODEL_GUID "{df4d45a1-9637-428f-9266-e6cb20143adb}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 0
#define NUMBER_OF_REALS 41
#define NUMBER_OF_INTEGERS 1
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    current_time; //VR=0
    fmi2Real    w_inShaftNeutral; //VR=1
    fmi2Real    w_wheel; //VR=2
    fmi2Real    w_inShaftOld; //VR=3
    fmi2Real    tq_retarder; //VR=4
    fmi2Real    tq_fricLoss; //VR=5
    fmi2Real    tq_env; //VR=6
    fmi2Real    gear_ratio; //VR=7
    fmi2Real    tq_clutchMax; //VR=8
    fmi2Real    tq_losses; //VR=9
    fmi2Real    r_tire; //VR=10
    fmi2Real    m_vehicle; //VR=11
    fmi2Real    final_gear_ratio; //VR=12
    fmi2Real    w_eng; //VR=13
    fmi2Real    tq_eng; //VR=14
    fmi2Real    J_eng; //VR=15
    fmi2Real    J_neutral; //VR=16
    fmi2Real    tq_brake; //VR=17
    fmi2Real    ts; //VR=18
    fmi2Real    r_slipFilt; //VR=19
    fmi2Real    w_inShaftDer; //VR=20
    fmi2Real    w_wheelDer; //VR=21
    fmi2Real    tq_clutch; //VR=22
    fmi2Real    v_vehicle; //VR=23
    fmi2Real    w_out; //VR=24
    fmi2Real    w_inShaft; //VR=25
    fmi2Real    tq_outTransmission; //VR=26
    fmi2Real    v_driveWheel; //VR=27
    fmi2Real    r_slip; //VR=28
    fmi2Real    k1; //VR=30
    fmi2Real    f_shaft_out; //VR=31
    fmi2Real    w_shaft_in; //VR=32
    fmi2Real    w_wheel_out; //VR=33
    fmi2Real    f_wheel_in; //VR=34
    fmi2Real    k2; //VR=35
    fmi2Real    w_shaft_out; //VR=36
    fmi2Real    f_shaft_in; //VR=37
    fmi2Real    f_wheel_out; //VR=38
    fmi2Real    w_wheel_in; //VR=39
    fmi2Real    a_wheel; //VR=40
    fmi2Real    a_shaft_out; //VR=41
    fmi2Integer simulation_ticks; //VR=0


} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.000000, //current_time
    0.000000, //w_inShaftNeutral
    0.000000, //w_wheel
    0.000000, //w_inShaftOld
    0.000000, //tq_retarder
    0.000000, //tq_fricLoss
    0.000000, //tq_env
    1.000000, //gear_ratio
    0.000000, //tq_clutchMax
    0.000000, //tq_losses
    1.000000, //r_tire
    100.000000, //m_vehicle
    1.000000, //final_gear_ratio
    0.000000, //w_eng
    0.000000, //tq_eng
    1.000000, //J_eng
    1.000000, //J_neutral
    0.000000, //tq_brake
    1.000000, //ts
    0.000000, //r_slipFilt
    0.000000, //w_inShaftDer
    0.000000, //w_wheelDer
    0.000000, //tq_clutch
    0.000000, //v_vehicle
    0.000000, //w_out
    0.000000, //w_inShaft
    0.000000, //tq_outTransmission
    0.000000, //v_driveWheel
    0.000000, //r_slip
    100.000000, //k1
    0.000000, //f_shaft_out
    0.000000, //w_shaft_in
    0.000000, //w_wheel_out
    0.000000, //f_wheel_in
    10.000000, //k2
    0.000000, //w_shaft_out
    0.000000, //f_shaft_in
    0.000000, //f_wheel_out
    0.000000, //w_wheel_in
    0.000000, //a_wheel
    0.000000, //a_shaft_out
    0, //simulation_ticks


};


#define VR_CURRENT_TIME 0
#define VR_W_INSHAFTNEUTRAL 1
#define VR_W_WHEEL 2
#define VR_W_INSHAFTOLD 3
#define VR_TQ_RETARDER 4
#define VR_TQ_FRICLOSS 5
#define VR_TQ_ENV 6
#define VR_GEAR_RATIO 7
#define VR_TQ_CLUTCHMAX 8
#define VR_TQ_LOSSES 9
#define VR_R_TIRE 10
#define VR_M_VEHICLE 11
#define VR_FINAL_GEAR_RATIO 12
#define VR_W_ENG 13
#define VR_TQ_ENG 14
#define VR_J_ENG 15
#define VR_J_NEUTRAL 16
#define VR_TQ_BRAKE 17
#define VR_TS 18
#define VR_R_SLIPFILT 19
#define VR_W_INSHAFTDER 20
#define VR_W_WHEELDER 21
#define VR_TQ_CLUTCH 22
#define VR_V_VEHICLE 23
#define VR_W_OUT 24
#define VR_W_INSHAFT 25
#define VR_TQ_OUTTRANSMISSION 26
#define VR_V_DRIVEWHEEL 27
#define VR_R_SLIP 28
#define VR_K1 30
#define VR_F_SHAFT_OUT 31
#define VR_W_SHAFT_IN 32
#define VR_W_WHEEL_OUT 33
#define VR_F_WHEEL_IN 34
#define VR_K2 35
#define VR_W_SHAFT_OUT 36
#define VR_F_SHAFT_IN 37
#define VR_F_WHEEL_OUT 38
#define VR_W_WHEEL_IN 39
#define VR_A_WHEEL 40
#define VR_A_SHAFT_OUT 41
#define VR_SIMULATION_TICKS 0



//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->current_time; break;
        case 1: value[i] = md->w_inShaftNeutral; break;
        case 2: value[i] = md->w_wheel; break;
        case 3: value[i] = md->w_inShaftOld; break;
        case 4: value[i] = md->tq_retarder; break;
        case 5: value[i] = md->tq_fricLoss; break;
        case 6: value[i] = md->tq_env; break;
        case 7: value[i] = md->gear_ratio; break;
        case 8: value[i] = md->tq_clutchMax; break;
        case 9: value[i] = md->tq_losses; break;
        case 10: value[i] = md->r_tire; break;
        case 11: value[i] = md->m_vehicle; break;
        case 12: value[i] = md->final_gear_ratio; break;
        case 13: value[i] = md->w_eng; break;
        case 14: value[i] = md->tq_eng; break;
        case 15: value[i] = md->J_eng; break;
        case 16: value[i] = md->J_neutral; break;
        case 17: value[i] = md->tq_brake; break;
        case 18: value[i] = md->ts; break;
        case 19: value[i] = md->r_slipFilt; break;
        case 20: value[i] = md->w_inShaftDer; break;
        case 21: value[i] = md->w_wheelDer; break;
        case 22: value[i] = md->tq_clutch; break;
        case 23: value[i] = md->v_vehicle; break;
        case 24: value[i] = md->w_out; break;
        case 25: value[i] = md->w_inShaft; break;
        case 26: value[i] = md->tq_outTransmission; break;
        case 27: value[i] = md->v_driveWheel; break;
        case 28: value[i] = md->r_slip; break;
        case 30: value[i] = md->k1; break;
        case 31: value[i] = md->f_shaft_out; break;
        case 32: value[i] = md->w_shaft_in; break;
        case 33: value[i] = md->w_wheel_out; break;
        case 34: value[i] = md->f_wheel_in; break;
        case 35: value[i] = md->k2; break;
        case 36: value[i] = md->w_shaft_out; break;
        case 37: value[i] = md->f_shaft_in; break;
        case 38: value[i] = md->f_wheel_out; break;
        case 39: value[i] = md->w_wheel_in; break;
        case 40: value[i] = md->a_wheel; break;
        case 41: value[i] = md->a_shaft_out; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->current_time = value[i]; break;
        case 1: md->w_inShaftNeutral = value[i]; break;
        case 2: md->w_wheel = value[i]; break;
        case 3: md->w_inShaftOld = value[i]; break;
        case 4: md->tq_retarder = value[i]; break;
        case 5: md->tq_fricLoss = value[i]; break;
        case 6: md->tq_env = value[i]; break;
        case 7: md->gear_ratio = value[i]; break;
        case 8: md->tq_clutchMax = value[i]; break;
        case 9: md->tq_losses = value[i]; break;
        case 10: md->r_tire = value[i]; break;
        case 11: md->m_vehicle = value[i]; break;
        case 12: md->final_gear_ratio = value[i]; break;
        case 13: md->w_eng = value[i]; break;
        case 14: md->tq_eng = value[i]; break;
        case 15: md->J_eng = value[i]; break;
        case 16: md->J_neutral = value[i]; break;
        case 17: md->tq_brake = value[i]; break;
        case 18: md->ts = value[i]; break;
        case 19: md->r_slipFilt = value[i]; break;
        case 20: md->w_inShaftDer = value[i]; break;
        case 21: md->w_wheelDer = value[i]; break;
        case 22: md->tq_clutch = value[i]; break;
        case 23: md->v_vehicle = value[i]; break;
        case 24: md->w_out = value[i]; break;
        case 25: md->w_inShaft = value[i]; break;
        case 26: md->tq_outTransmission = value[i]; break;
        case 27: md->v_driveWheel = value[i]; break;
        case 28: md->r_slip = value[i]; break;
        case 30: md->k1 = value[i]; break;
        case 31: md->f_shaft_out = value[i]; break;
        case 32: md->w_shaft_in = value[i]; break;
        case 33: md->w_wheel_out = value[i]; break;
        case 34: md->f_wheel_in = value[i]; break;
        case 35: md->k2 = value[i]; break;
        case 36: md->w_shaft_out = value[i]; break;
        case 37: md->f_shaft_in = value[i]; break;
        case 38: md->f_wheel_out = value[i]; break;
        case 39: md->w_wheel_in = value[i]; break;
        case 40: md->a_wheel = value[i]; break;
        case 41: md->a_shaft_out = value[i]; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: value[i] = md->simulation_ticks; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 0: md->simulation_ticks = value[i]; break;
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

static fmi2Status generated_fmi2GetString(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
