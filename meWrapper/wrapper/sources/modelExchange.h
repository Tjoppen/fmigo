#ifndef MODELEXCHANGE_H
#define MODELEXCHANGE_H
#include "modelDescription.h"
#include "modelExchangeFmiInterface.h"
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
    fmi2Component m_fmi2Component;
    //fmi2_callback_functions_t m_fmi2CallbackFunctions;
    fmi2CallbackFunctions m_fmi2CallbackFunctions;
    fmi2_import_variable_list_t *m_fmi2Variables, *m_fmi2Outputs;
    jm_callbacks m_jmCallbacks;
}Wrapper;

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
     *  returns true if at least one simulation has an event
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
#endif
