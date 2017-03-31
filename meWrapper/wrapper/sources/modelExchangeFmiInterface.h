#include "gsl-interface.h"

#define FMIGO_ME_SET_TIME(client) baseMaster->send(client, fmi2_import_set_time(t))
#define FMIGO_ME_SET_CONTINUOUS_STATES(client,data) baseMaster->send(client, fmi2_import_set_continuous_states(data + p->baseMaster->get_storage().get_offset(client->getId(), STORAGE::states), client->getNumContinuousStates()))
#define FMIGO_ME_GET_DERIVATIVES(client) baseMaster->send(client, fmi2_import_get_derivatives((int)client->getNumContinuousStates()))
#define FMIGO_ME_GET_EVENT_INDICATORS(client) baseMaster->send(client, fmi2_import_get_event_indicators((int)client->getNumEventIndicators()))
#define FMIGO_ME_GET_CONTINUOUS_STATES(client) baseMaster->send(client, fmi2_import_get_continuous_states((int)client->getNumContinuousStates()))


#define FMIGO_ME_ENTER_EVENT_MODE(clients) baseMaster->sendWait(clients, fmi2_import_enter_event_mode())
#define FMIGO_ME_NEW_DISCRETE_STATES(clients) baseMaster->sendWait(clients, fmi2_import_new_discrete_states())
#define FMIGO_ME_ENTER_CONTINUOUS_TIME_MODE(clients) baseMaster->sendWait(clients, fmi2_import_enter_continuous_time_mode())

#define FMIGO_ME_WAIT() baseMaster->wait()


const char* fmi2GetTypesPlatform();
const char* fmi2GetVersion();
fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]);
fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions *functions, fmi2Boolean visible, fmi2Boolean loggingOn);
void fmi2FreeInstance(fmi2Component c);
fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime);
fmi2Status fmi2EnterInitializationMode(fmi2Component c);
fmi2Status fmi2ExitInitializationMode(fmi2Component c);
fmi2Status fmi2Terminate(fmi2Component c);
fmi2Status fmi2Reset(fmi2Component c);
fmi2Status fmi2GetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]);
fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]);
fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]);
fmi2Status fmi2GetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]);
fmi2Status fmi2SetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]);
fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]);
fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]);
fmi2Status fmi2SetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]);
fmi2Status fmi2GetFMUstate (fmi2Component c, fmi2FMUstate* FMUstate);
fmi2Status fmi2SetFMUstate (fmi2Component c, fmi2FMUstate FMUstate);
fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate);
fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t *size);
fmi2Status fmi2SerializeFMUstate (fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte serializedState[], size_t size);
fmi2Status fmi2DeSerializeFMUstate (fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate);
fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown, const fmi2ValueReference vKnown_ref[] , size_t nKnown, const fmi2Real dvKnown[], fmi2Real dvUnknown[]);
fmi2Status fmi2EnterEventMode(fmi2Component c);
fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo *eventInfo);
fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c);
fmi2Status fmi2CompletedIntegratorStep(fmi2Component c, fmi2Boolean noSetFMUStatePriorToCurrentPoint, fmi2Boolean *enterEventMode, fmi2Boolean *terminateSimulation);
fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time);
fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx);
fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx);
fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni);
fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real states[], size_t nx);
fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx);
