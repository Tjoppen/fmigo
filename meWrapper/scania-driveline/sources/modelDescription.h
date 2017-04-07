/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER wrapper_drivetrainMFunctionOnly
#define MODEL_GUID "86883902-8d3d-4a3c-87fe-0f3b711a8616"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 38
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 2
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    tq_retarder; //VR=0
    fmi2Real    tq_fricLoss; //VR=1
    fmi2Real    tq_env; //VR=2
    fmi2Real    gear_ratio; //VR=3
    fmi2Real    tq_clutchMax; //VR=4
    fmi2Real    tq_losses; //VR=5
    fmi2Real    r_tire; //VR=6
    fmi2Real    m_vehicle; //VR=7
    fmi2Real    final_gear_ratio; //VR=8
    fmi2Real    w_eng; //VR=9
    fmi2Real    tq_eng; //VR=10
    fmi2Real    J_eng; //VR=11
    fmi2Real    J_neutral; //VR=12
    fmi2Real    tq_clutch; //VR=13
    fmi2Real    v_vehicle; //VR=14
    fmi2Real    w_out; //VR=15
    fmi2Real    w_shaft; //VR=16
    fmi2Real    tq_outTransmission; //VR=17
    fmi2Real    v_driveWheel; //VR=18
    fmi2Real    a_inshaft; //VR=19
    fmi2Real    a_wheel; //VR=20
    fmi2Real    ContinuousState_0; //VR=21
    fmi2Real    ContinuousState_1; //VR=22
    fmi2Real    ContinuousState_2; //VR=23
    fmi2Real    EventIndicator_0; //VR=24
    fmi2Real    EventIndicator_1; //VR=25
    fmi2Real    EventIndicator_2; //VR=26
    fmi2Real    EventIndicator_3; //VR=27
    fmi2Real    EventIndicator_4; //VR=28
    fmi2Real    EventIndicator_5; //VR=29
    fmi2Real    Integrator_InitialCondition; //VR=33
    fmi2Real    Integrator_LowerSaturationLimit; //VR=34
    fmi2Real    Integrator_UpperSaturationLimit; //VR=35
    fmi2Real    Integrator1_InitialCondition; //VR=36
    fmi2Real    Integrator1_LowerSaturationLimit; //VR=37
    fmi2Real    Integrator1_UpperSaturationLimit; //VR=38
    fmi2Real    Transfer_Fcn_A; //VR=39
    fmi2Real    Transfer_Fcn_C; //VR=40


    fmi2Char    fmu[32]; //VR=0
    fmi2Char    directional[20]; //VR=1
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.000000, //tq_retarder
    0.000000, //tq_fricLoss
    0.000000, //tq_env
    0.000000, //gear_ratio
    0.000000, //tq_clutchMax
    0.000000, //tq_losses
    0.000000, //r_tire
    0.000000, //m_vehicle
    0.000000, //final_gear_ratio
    0.000000, //w_eng
    0.000000, //tq_eng
    0.000000, //J_eng
    0.000000, //J_neutral
    0.000000, //tq_clutch
    0.000000, //v_vehicle
    0.000000, //w_out
    0.000000, //w_shaft
    0.000000, //tq_outTransmission
    0.000000, //v_driveWheel
    0.000000, //a_inshaft
    0.000000, //a_wheel
    0.000000, //ContinuousState_0
    0.000000, //ContinuousState_1
    0.000000, //ContinuousState_2
    0.000000, //EventIndicator_0
    0.000000, //EventIndicator_1
    0.000000, //EventIndicator_2
    0.000000, //EventIndicator_3
    0.000000, //EventIndicator_4
    0.000000, //EventIndicator_5
    0.000000, //Integrator_InitialCondition
    0.000000, //Integrator_LowerSaturationLimit
    0.000000, //Integrator_UpperSaturationLimit
    0.000000, //Integrator1_InitialCondition
    0.000000, //Integrator1_LowerSaturationLimit
    0.000000, //Integrator1_UpperSaturationLimit
    -4.000000, //Transfer_Fcn_A
    4.000000, //Transfer_Fcn_C


    "fmu/drivetrainMFunctionOnly.fmu", //fmu
    "", //directional
};


#define VR_TQ_RETARDER 0
#define VR_TQ_FRICLOSS 1
#define VR_TQ_ENV 2
#define VR_GEAR_RATIO 3
#define VR_TQ_CLUTCHMAX 4
#define VR_TQ_LOSSES 5
#define VR_R_TIRE 6
#define VR_M_VEHICLE 7
#define VR_FINAL_GEAR_RATIO 8
#define VR_W_ENG 9
#define VR_TQ_ENG 10
#define VR_J_ENG 11
#define VR_J_NEUTRAL 12
#define VR_TQ_CLUTCH 13
#define VR_V_VEHICLE 14
#define VR_W_OUT 15
#define VR_W_SHAFT 16
#define VR_TQ_OUTTRANSMISSION 17
#define VR_V_DRIVEWHEEL 18
#define VR_A_INSHAFT 19
#define VR_A_WHEEL 20
#define VR_CONTINUOUSSTATE_0 21
#define VR_CONTINUOUSSTATE_1 22
#define VR_CONTINUOUSSTATE_2 23
#define VR_EVENTINDICATOR_0 24
#define VR_EVENTINDICATOR_1 25
#define VR_EVENTINDICATOR_2 26
#define VR_EVENTINDICATOR_3 27
#define VR_EVENTINDICATOR_4 28
#define VR_EVENTINDICATOR_5 29
#define VR_INTEGRATOR_INITIALCONDITION 33
#define VR_INTEGRATOR_LOWERSATURATIONLIMIT 34
#define VR_INTEGRATOR_UPPERSATURATIONLIMIT 35
#define VR_INTEGRATOR1_INITIALCONDITION 36
#define VR_INTEGRATOR1_LOWERSATURATIONLIMIT 37
#define VR_INTEGRATOR1_UPPERSATURATIONLIMIT 38
#define VR_TRANSFER_FCN_A 39
#define VR_TRANSFER_FCN_C 40


#define VR_FMU 0
#define VR_DIRECTIONAL 1

//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    fmi2_import_get_real(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    if( *getFMU() != NULL)
        fmi2_import_set_real(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    fmi2_import_get_integer(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    if( *getFMU() != NULL)
        fmi2_import_set_integer(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    fmi2_import_get_boolean(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    if( *getFMU() != NULL)
        fmi2_import_set_boolean(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2GetString(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    fmi2_import_get_string(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    if( *getFMU() != NULL)
        fmi2_import_set_string(*getFMU(),vr,nvr,value);
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
