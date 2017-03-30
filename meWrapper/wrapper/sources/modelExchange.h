#ifndef MODELEXCHANGE_H
#define MODELEXCHANGE_H
#include <fmilib.h>
#include "modelDescription.h"
#include "fmuTemplate.h"
#include "modelExchangeFmiInterface.h"
#include <stdbool.h>
#include "gsl-interface.h"
typedef struct TimeLoop
{
    fmi2_real_t t_safe, dt_new, t_crossed, t_end;
} TimeLoop;
typedef struct Backup
{
    double t;
    double h;
    double *dydt;
    unsigned long failed_steps;
}Backup;
typedef struct fmu_parameters{
    int nx;
    int ni;

    fmi2Real* ei;
    fmi2Real* ei_backup;
    fmi2EventInfo eventInfo;
    double t_ok;
    double t_past;
    int count;                    /* number of function evaluations */

    bool stateEvent;
    Backup backup;
}fmu_parameters;
typedef struct fmu_model{
    cgsl_model *model;
}fmu_model;
cgsl_simulation m_sim;
fmu_parameters m_p;
fmu_model m_model;
TimeLoop timeLoop;
typedef struct wrapper{
    fmi_import_context_t* m_context;
    fmi_version_enu_t m_version;
    fmi2_import_t* m_fmi2Instance;
    fmi2_callback_functions_t m_fmi2CallbackFunctions;
    fmi2_import_variable_list_t *m_fmi2Variables, *m_fmi2Outputs;
    jm_callbacks m_jmCallbacks;
}Wrapper;

Wrapper wrapper;

    void runIteration(double t, double dt);

    void prepare();

    /** allocate Memory
     *  Allocates memory needed by the fmu_model
     *
     *  @param m Pointer to a fmu_model
     */
    void allocateMemory(fmu_model *m);

    static void *get_model_parameters(const cgsl_model *m){
        return m->parameters;
    }

    /** init_fmu_model
     *  Creates the fmu_model and sets the parameters
     *
     *  @param client A pointer to a fmi client
     */
    void init_fmu_model(fmu_model *m);

#define STATIC_GET_CLIENT_OFFSET(name)                                  \
   p->baseMaster->get_storage().get_offset(client->getId(), STORAGE::name)
#define STATIC_SET_(name, name2, data)                                       \
    p->baseMaster->send(client, fmi2_import_set_##name##_##name2(     \
                                                       data + STATIC_GET_CLIENT_OFFSET(name2), \
                                                       client->getNumContinuousStates()));
#define STATIC_GET_(name)                                               \
    p->baseMaster->send(client, fmi2_import_get_##name((int)client->getNumContinuousStates()))

    /** restoreStates
     *  restores all values needed by the simulations to restart
     *  from a known safe time.
     *
     *  @param sim The simulation
     */
    void restoreStates(cgsl_simulation *sim);

    /** storeStates
     *  stores all values needed by the simulations to restart
     *  from a known safe time.
     *
     *  @param sim The simulation
     */
    void storeStates(cgsl_simulation *sim);

    /** hasStateEvent:
     ** returns true if at least one simulation has an event
     *
     *  @param sim The simulation
     */
    bool hasStateEvent(cgsl_simulation *sim);

    /** getSafeTime:
     *  caluclates a "safe" time, uses the golden ratio to get
     *  t_crossed and t_safe to converge towards same value
     *
     *  @param sim A cgsl simulation
     */
    void getGoldenNewTime(cgsl_simulation *sim);

    /** step
     *  run cgsl_step_to on all simulations
     *
     *  @param sim The simulation
     */
    void step(cgsl_simulation *sim);

    /** stepToEvent
     *  if there is an event, find the event and return
     *  the time at where the time event occured
     *
     *  @param sim The simulation
     *  @return Returns the time immediatly after the event
     */
    void stepToEvent(cgsl_simulation *sim);

    /** newDiscreteStates
     *  Should be used where a new discrete state ends
     *  and another begins. Resets the loop variables
     *  and store all states of the simulation
     *
     *  @param t The current time
     *  @param t_new New next time
     */
    void newDiscreteStates();

    void printStates(void);

    void getSafeAndCrossed();

    void safeTimeStep(cgsl_simulation *sim);

    void getSafeTime(double t, double *dt);

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
#endif
