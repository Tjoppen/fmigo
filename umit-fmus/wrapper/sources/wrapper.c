#ifndef WIN32
#include <unistd.h>
#else
#include <direct.h>
#endif

//link fmilib statically
#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "gsl-interface.h"

#include "modelDescription.h"

//having hand-written getters and setters
#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters

// This check serves two purposes:
// - protect against senseless zero-real FMUs
// - avoid compilation problems on Windows (me_simulation::partials becoming [0])
#if NUMBER_OF_REALS == 0
#error NUMBER_OF_REALS == 0 does not make sense for ModelExchange
#endif

#if NUMBER_OF_REAL_OUTPUTS == 0
#error NUMBER_OF_REAL_OUTPUTS == 0 is suspicious
#endif

static const int real_output_vrs[] = REAL_OUTPUT_VRS;

typedef struct Backup
{
    double t;
    double h;
    double dydt[NUMBER_OF_STATES+NUMBER_OF_REAL_OUTPUTS];
    fmi2_real_t ei[NUMBER_OF_EVENT_INDICATORS];
    fmi2_real_t ei_b[NUMBER_OF_EVENT_INDICATORS];
    fmi2_real_t x[NUMBER_OF_STATES+NUMBER_OF_REAL_OUTPUTS];
    unsigned long failed_steps;
    fmi2_event_info_t eventInfo;
}Backup;

typedef struct {
    fmi2ValueReference unknown, known, vr;
} partial_t;

typedef struct {
    cgsl_simulation sim;

    //we're guaranteed never to need more than (number of reals)^2 partials
    //this could be optimized slightly, using number of real inputs * number of real outputs
    //doing it this way avoids a bunch of allocations
    partial_t partials[NUMBER_OF_REALS*NUMBER_OF_REALS];
    size_t npartials;

    fmi2_import_t *FMU;
    char* dir;
    jm_callbacks m_jmCallbacks;
    Backup m_backup;
} me_simulation;

void restoreStates(cgsl_simulation *sim, Backup *backup);
void storeStates(cgsl_simulation *sim, Backup *backup);
void runIteration(cgsl_simulation *sim, double t, double dt);
void prepare(cgsl_simulation *sim, fmi2_import_t *FMU, enum cgsl_integrator_ids integrator);

#include "math.h"
#ifndef max
#define max(a,b) ((a>b) ? a : b)
#endif
#ifndef min
#define min(a,b) ((a<b) ? a : b)
#endif

typedef struct TimeLoop
{
    fmi2_real_t t_safe, dt_new, t_crossed, t_end;
} TimeLoop;

typedef struct fmu_parameters {
    double t_ok;
    double t_past;
    int count;                    /* number of function evaluations */

    bool stateEvent;

    fmi2_import_t *FMU;
    Backup m_backup;
} fmu_parameters;

//#define signbits(a,b) ((a > 0)? ( (b > 0) ? 1 : 0) : (b<=0)? 1: 0)
/** get_p
 *  Extracts the parameters from the model
 *
 *  @param m Model
 */
fmu_parameters* get_p(cgsl_model* m){
    return (fmu_parameters*)(m->parameters);
}

/** getSafeAndCrossed()
 *  Extracts safe and crossed time found by fmu_function
 */
void getSafeAndCrossed(cgsl_simulation *sim, TimeLoop *timeLoop){
    fmu_parameters *p = get_p(sim->model);
    timeLoop->t_safe    = p->t_ok;//max( timeLoop->t_safe,    t_ok);
    timeLoop->t_crossed = p->t_past;//min( timeLoop->t_crossed, t_past);
}

bool past_event(fmi2_real_t* a, fmi2_real_t* b, int i){

    i--;
    for(;i>=0;i--){
        if(signbit( a[i] ) != signbit( b[i] ))
            return true;
    }
    return false;
}

/** fmu_function
 *  function needed by cgsl_simulation to get and set the current
 *  states and derivatives
 *
 *  @param t Input time
 *  @param x Input states vector
 *  @param dxdt Output derivatives
 *  @param params Contains model specific parameters
 */
static int fmu_function(double t, const double x[], double dxdt[], void* params)
{
    // make local variables
    fmu_parameters* p = (fmu_parameters*)params;

    ++p->count; /* count function evaluations */
    if(p->stateEvent)return GSL_SUCCESS;

    fmi2_import_set_time(p->FMU, t);
    fmi2_import_set_continuous_states(p->FMU, x, NUMBER_OF_STATES);

    fmi2_import_get_derivatives(p->FMU, dxdt, NUMBER_OF_STATES);
    //filter outputs
    fmi2_import_get_real(p->FMU, real_output_vrs, NUMBER_OF_REAL_OUTPUTS, &dxdt[NUMBER_OF_STATES]);

    if(NUMBER_OF_EVENT_INDICATORS){
        fmi2_import_get_event_indicators(p->FMU, p->m_backup.ei, NUMBER_OF_EVENT_INDICATORS);
        if(past_event(p->m_backup.ei_b, p->m_backup.ei, NUMBER_OF_EVENT_INDICATORS)){
            p->stateEvent = true;
            p->t_past = t;
            return GSL_SUCCESS;
        } else{
            p->stateEvent = false;
            p->t_ok = t;
        }
    }

    return GSL_SUCCESS;
}

static void me_model_free(cgsl_model *model) {
  fmu_parameters* p = get_p(model);
  free(p);
  cgsl_model_default_free(model);
}

/** init_fmu_model
 *  Setup all parameters and function pointers needed by fmu_model
 *
 *  @param m The fmu_model we are working on
 *  @param client A vector with clients
 */

void init_fmu_model(cgsl_model **m, fmi2_import_t *FMU){
    fmu_parameters* p = (fmu_parameters*)calloc(1,sizeof(fmu_parameters));

    p->FMU        = FMU;

    //sanity check NUMBER_OF_STATES and NUMBER_OF_EVENT_INDICATORS
    int nx = fmi2_import_get_number_of_continuous_states(p->FMU);
    int ni = fmi2_import_get_number_of_event_indicators(p->FMU);

    if (nx != NUMBER_OF_STATES) {
      fprintf(stderr,
        "fmi2_import_get_number_of_continuous_states != NUMBER_OF_STATES (%i != %i)\n",
        nx, NUMBER_OF_STATES
      );
      exit(1);
    }
    if (ni != NUMBER_OF_EVENT_INDICATORS) {
      fprintf(stderr,
        "fmi2_import_get_number_of_event_indicators != NUMBER_OF_EVENT_INDICATORS (%i != %i)\n",
        ni, NUMBER_OF_EVENT_INDICATORS
      );
      exit(1);
    }

    p->t_ok       = 0;
    p->t_past     = 0;
    p->stateEvent = false;
    p->count      = 0;

    *m = cgsl_model_default_alloc(NUMBER_OF_STATES+NUMBER_OF_REAL_OUTPUTS, NULL, p, fmu_function, NULL, NULL, NULL, 0);
    (*m)->free = me_model_free;

    p->m_backup.t = 0;
    p->m_backup.h = 0;

    fmi2_status_t status;
    status = fmi2_import_get_continuous_states(p->FMU, (*m)->x,        NUMBER_OF_STATES);
    status = fmi2_import_get_continuous_states(p->FMU, (*m)->x_backup, NUMBER_OF_STATES);
}

/** prepare()
 *  Setup everything
 */
void prepare(cgsl_simulation *sim, fmi2_import_t *FMU, enum cgsl_integrator_ids integrator) {
    init_fmu_model(&sim->model, FMU);
    // set up a gsl_simulation for each client
    *sim = cgsl_init_simulation(sim->model,
                                 integrator, /* integrator: Runge-Kutta Prince Dormand pair order 7-8 */
                                 1e-10,
                                 0,
                                 0,
                                 0, NULL
                                 );
}

/** restoreStates()
 *  Restores all values needed by the simulations to restart
 *  before the event
 *
 *  @param sim The simulation
 */
void restoreStates(cgsl_simulation *sim, Backup *backup){
    fmu_parameters* p = get_p(sim->model);
    //restore previous states


    memcpy(sim->model->x,backup->x,(NUMBER_OF_STATES+NUMBER_OF_REAL_OUTPUTS) * sizeof(sim->model->x[0]));

    memcpy(sim->i.evolution->dydt_out, backup->dydt,
           sim->model->n_variables * sizeof(backup->dydt[0]));

    sim->i.evolution->failed_steps = backup->failed_steps;
    sim->t = backup->t;
    sim->h = backup->h;

    fmi2_import_set_time(p->FMU, sim->t);
    fmi2_import_set_continuous_states(p->FMU, sim->model->x, NUMBER_OF_STATES);

    gsl_odeiv2_evolve_reset(sim->i.evolution);
    gsl_odeiv2_step_reset(sim->i.step);
    gsl_odeiv2_driver_reset(sim->i.driver);
}

/** storeStates()
 *  Stores all values needed by the simulations to restart
 *  from a state before an event
 *
 *  @param sim The simulation
 */
void storeStates(cgsl_simulation *sim, Backup *backup){
    fmu_parameters* p = get_p(sim->model);

    fmi2_import_get_continuous_states(p->FMU, backup->x,    NUMBER_OF_STATES);
    fmi2_import_get_event_indicators (p->FMU, backup->ei_b, NUMBER_OF_EVENT_INDICATORS);
    memcpy(sim->model->x,backup->x,(NUMBER_OF_STATES+NUMBER_OF_REAL_OUTPUTS) * sizeof(backup->x[0]));

    backup->failed_steps = sim->i.evolution->failed_steps;
    backup->t = sim->t;
    backup->h = sim->h;

    memcpy(backup->dydt, sim->i.evolution->dydt_out,
           sim->model->n_variables * sizeof(backup->dydt[0]));
}

/** hasStateEvent()
 *  Retrieve stateEvent status
 *  Returns true if at least one simulation crossed an event
 *
 *  @param sim The simulation
 */
bool hasStateEvent(cgsl_simulation *sim){
    return get_p(sim->model)->stateEvent;
}

/** getGoldenNewTime()
 *  Calculates a time step which brings solution closer to the event
 *  Uses the golden ratio to get t_crossed and t_safe to converge
 *  to the event time
 *
 *  @param sim The simulation
 */
void getGoldenNewTime(cgsl_simulation *sim, Backup* backup, TimeLoop *timeLoop){
    // golden ratio
    double phi = (1 + sqrt(5)) / 2;
    /* passed solution, need to reduce tEnd */
    if(hasStateEvent(sim)){
        getSafeAndCrossed(sim, timeLoop);
        restoreStates(sim, backup);
        timeLoop->dt_new = (timeLoop->t_crossed - sim->t) / phi;
    } else { // havent passed solution, increase step
        storeStates(sim, backup);
        timeLoop->t_safe = max(timeLoop->t_safe, sim->t);
        timeLoop->dt_new = timeLoop->t_crossed - sim->t - (timeLoop->t_crossed - timeLoop->t_safe) / phi;
    }
}

/** me_step()
 *  Run cgsl_step_to the simulation
 *
 *  @param sim The simulation
 */
void me_step(cgsl_simulation *sim, TimeLoop *timeLoop){
    fmu_parameters *p = get_p(sim->model);
    p->stateEvent = false;
    p->t_past = max(p->t_past, sim->t + timeLoop->dt_new);
    p->t_ok = sim->t;

    cgsl_step_to(sim, sim->t, timeLoop->dt_new);
}

fmi2_real_t absmin(fmi2_real_t* v, size_t n){
    fmi2_real_t min = v[0];
    for(; n>1; n--){
        if(min > v[n-1]) min = v[n-1];
    }
    return min;
}
/** stepToEvent()
 *  To be runned when an event is crossed.
 *  Finds the event and returns a state immediately after the event
 *
 *  @param sim The simulation
 */
void stepToEvent(cgsl_simulation *sim, Backup *backup, TimeLoop *timeLoop){
    double tol = 1e-9;
    fmu_parameters* p = get_p(sim->model);
    while(!hasStateEvent(sim) && !(absmin(backup->ei,NUMBER_OF_EVENT_INDICATORS) < tol || timeLoop->dt_new < tol)){
        getGoldenNewTime(sim, backup, timeLoop);
        me_step(sim, timeLoop);
        if(timeLoop->dt_new == 0){
            fprintf(stderr,"stepToEvent: dt == 0, abort\n");
            exit(1);
        }
        if(hasStateEvent(sim)){
            // step back to where event occured
            restoreStates(sim, backup);
            getSafeAndCrossed(sim, timeLoop);
            timeLoop->dt_new = timeLoop->t_safe - sim->t;
            me_step(sim, timeLoop);
            if(!hasStateEvent(sim)) storeStates(sim, backup);
            else{
                fprintf(stderr,"stepToEvent: failed at stepping to safe time, aborting\n");
                exit(1);
            }
            timeLoop->dt_new = timeLoop->t_crossed - sim->t;
            me_step(sim, timeLoop);
            if(!hasStateEvent(sim)){
                fprintf(stderr,"stepToEvent: failed at stepping to event \n");
                exit(1);
            }
        }
    }
}

/** newDiscreteStates()
 *  Should be used where a new discrete state ends and another begins.
 *  Store the current state of the simulation
 */
void newDiscreteStates(cgsl_simulation *sim, Backup *backup){
    fmu_parameters* p = get_p(sim->model);
    // start at a new state
    fmi2_import_enter_event_mode(p->FMU);

    // todo loop until newDiscreteStatesNeeded == false

    fmi2_event_info_t eventInfo;
    eventInfo.newDiscreteStatesNeeded = true;
    eventInfo.terminateSimulation = false;

    if(NUMBER_OF_EVENT_INDICATORS){
        while(eventInfo.newDiscreteStatesNeeded){
            fmi2_import_new_discrete_states(p->FMU, &eventInfo);
            if(eventInfo.terminateSimulation){
                fprintf(stderr,"modelExchange.c: terminated simulation\n");
                exit(1);
            }
        }
    }

    fmi2_import_enter_continuous_time_mode(p->FMU);

    // store the current state of all running FMUs
    storeStates(sim, backup);
}

/** safeTimeStep()
 *  Make sure we take small first step when we're at on event
 *  @param sim The simulation
 */
void safeTimeStep(cgsl_simulation *sim, TimeLoop *timeLoop){
    // if sims has a state event do not step to far
    if(hasStateEvent(sim)){
        double absmin = 0;//m_baseMaster->get_storage().absmin(STORAGE::indicators);
        timeLoop->dt_new = sim->h * (absmin > 0 ? absmin:0.00001);
    }else
        timeLoop->dt_new = timeLoop->t_end - sim->t;
}

/** getSafeTime()
 *
 *  @param clients Vector with clients
 *  @param t The current time
 *  @param dt Timestep, input and output
 */
void getSafeTime(cgsl_simulation *sim, double t, double *dt, Backup *backup){
    if(backup->eventInfo.nextEventTimeDefined)
        *dt = min(*dt, backup->eventInfo.nextEventTime - t);
}

/** runIteration()
 *  @param t The current time
 *  @param dt The timestep to be taken
 */
void runIteration(cgsl_simulation *sim, double t, double dt) {
    TimeLoop timeLoop;
    fmu_parameters* p = get_p(sim->model);

    timeLoop.t_safe = t;
    timeLoop.t_crossed = t; //not used before getSafeAndCrossed() I think, but best to be safe
    timeLoop.t_end = t + dt;
    timeLoop.dt_new = dt;

    getSafeTime(sim, t, &timeLoop.dt_new, &p->m_backup);
    newDiscreteStates(sim, &p->m_backup);

    while( timeLoop.t_safe < timeLoop.t_end ){
        me_step(sim, &timeLoop);

        if (hasStateEvent(sim)){

            getSafeAndCrossed(sim, &timeLoop);

            // restore and step to before the event
            restoreStates(sim, &p->m_backup);
            timeLoop.dt_new = timeLoop.t_safe - sim->t;
            me_step(sim, &timeLoop);

            // store and step to the event
            if(!hasStateEvent(sim)) storeStates(sim, &p->m_backup);
            timeLoop.dt_new = timeLoop.t_crossed - sim->t;
            me_step(sim, &timeLoop);

            // step closer to the event location
            if(hasStateEvent(sim))
                stepToEvent(sim, &p->m_backup, &timeLoop);

        }
        else {
            timeLoop.t_safe = sim->t;
            timeLoop.t_crossed = timeLoop.t_end;
        }

        safeTimeStep(sim, &timeLoop);
        if(hasStateEvent(sim))
            newDiscreteStates(sim, &p->m_backup);
        storeStates(sim, &p->m_backup);
    }
}

#define SIMULATION_TYPE    me_simulation
#include "fmuTemplate.h"
#include "hypotmath.h"

#define SIMULATION_INSTANTIATE      wrapper_instantiate
#define SIMULATION_SETUP_EXPERIMENT wrapper_setup_experiment
#define SIMULATION_SET              wrapper_set
#define SIMULATION_GET              wrapper_get
#define SIMULATION_ENTER_INIT       wrapper_enter_init
#define SIMULATION_EXIT_INIT        wrapper_exit_init
#define SIMULATION_FREE             wrapper_free

#include "strlcpy.h"

static fmi2Status generated_fmi2GetReal(ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    return (fmi2Status)fmi2_import_get_real(comp->s.simulation.FMU,vr,nvr,value);
}

static fmi2Status generated_fmi2SetReal(ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    if( comp->s.simulation.FMU != NULL)
        return (fmi2Status)fmi2_import_set_real(comp->s.simulation.FMU,vr,nvr,value);
    return fmi2Error;
}

//fmi2GetInteger and fmi2SetInteger become special because we've introduced integrator
static fmi2Status generated_fmi2GetInteger(ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    size_t x = 0;

    for (x = 0; x < nvr; x++) {
        if (vr[x] == VR_INTEGRATOR) {
            value[x] = md->integrator;
        } else if (fmi2_import_get_integer(comp->s.simulation.FMU, &vr[x], 1, &value[x]) != fmi2OK) {
            return fmi2Error;
        }
    }

    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    if( comp->s.simulation.FMU != NULL) {
        size_t x;

        //only call fmi2SetInteger() on the FMU if it's an integer it expects
        //this fixes FMUs created by FMI Toolbox crashing
        for (x = 0; x < nvr; x++) {
            if (vr[x] == VR_INTEGRATOR) {
                md->integrator = value[x];
            } else if (fmi2_import_set_integer(comp->s.simulation.FMU,&vr[x],1,&value[x]) != fmi2OK) {
                return fmi2Error;
            }
        }

        return fmi2OK;
    }
    return fmi2Error;
}

static fmi2Status generated_fmi2GetBoolean(ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    return (fmi2Status)fmi2_import_get_boolean(comp->s.simulation.FMU,vr,nvr,value);
}

static fmi2Status generated_fmi2SetBoolean(ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    if( comp->s.simulation.FMU != NULL)
        return (fmi2Status)fmi2_import_set_boolean(comp->s.simulation.FMU,vr,nvr,value);
    return fmi2Error;
}

static fmi2Status generated_fmi2GetString(ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    return (fmi2Status)fmi2_import_get_string(comp->s.simulation.FMU,vr,nvr,value);
}

static fmi2Status generated_fmi2SetString(ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    if( comp->s.simulation.FMU != NULL) {
        size_t x;

        for (x = 0; x < nvr; x++) {
            if (vr[x] == VR_OCTAVE_OUTPUT) {
                strlcpy(comp->s.md.octave_output, value[x], sizeof(comp->s.md.octave_output));
            } else if (fmi2_import_set_string(comp->s.simulation.FMU,&vr[x],1,&value[x]) != fmi2OK) {
                return fmi2Error;
            }
        }

        return fmi2OK;
    }
    return fmi2Error;
}

fmi2Status wrapper_get ( me_simulation *sim) {
    storeStates(&sim->sim, &sim->m_backup);
    return fmi2OK;
}

fmi2Status wrapper_set ( me_simulation *sim) {
    restoreStates(&sim->sim, &sim->m_backup);
    return fmi2OK;
}

//#include <stdio.h>
static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial){
    size_t x;
    state_t *s = &comp->s;

    if ( fmi2_import_get_capability(comp->s.simulation.FMU, fmi2_me_providesDirectionalDerivatives ) ){
      fmi2_value_reference_t vrp []  = {vr};
      fmi2_value_reference_t wrtp[]  = {wrt};
      fmi2_value_reference_t zrefp[] = {0};
      fmi2Real dv [] = {1.0};
      // TODO: move this logic to the fmuTemplate_impl.h
      return (fmi2Status)fmi2_import_get_directional_derivative(
        comp->s.simulation.FMU,
        wrtp, 1,  //v_ref aka known ("Value references for the seed vector")
        vrp, 1,   //z_ref aka unknown ("Value references for the derivatives/outputs to be processed")
        dv,       //dv
        partial   //dz
      );
    }

    //could speed this up with binary search or some kind of simple hash map
    for (x = 0; x < s->simulation.npartials; x++) {
        if (vr  == s->simulation.partials[x].unknown &&
            wrt == s->simulation.partials[x].known) {
            return generated_fmi2GetReal(comp, &s->md, &s->simulation.partials[x].vr, 1, partial);
        }
    }

    //compute d(vr)/d(wrt) = (vr1 - vr0) / (wrt1 - wrt0)
    //since the FMU is ME we don't need to doStep() for the values of vr to update when changing wrt
    fmi2Real dwrt, wrt0, wrt1, vr0, vr1;
    fmi2Real hi, lo;

    //save state
    if (wrapper_get(&s->simulation) != fmi2OK) return fmi2Error;

    if (generated_fmi2GetReal(comp, &s->md, &wrt, 1, &wrt0) != fmi2OK) return fmi2Error;
    if (generated_fmi2GetReal(comp, &s->md, &vr,  1, &vr0) != fmi2OK) return fmi2Error;

    //binary search for a "lagom" dwrt
    //we want one that moves vr1 a small distance away from vr0
    //try dwrt roughly between DBL_MIN and DBL_MAX
    //this typically takes about 12 iterations
    lo = 1.0e-307;
    hi = 1.0e307;

    int i = 0;
    while (hi / lo > 1.5) {
      i++;
      dwrt = sqrt(hi) * sqrt(lo);
      wrt1 = wrt0 + dwrt;

      if (generated_fmi2SetReal(comp, &s->md, &wrt, 1, &wrt1) != fmi2OK) return fmi2Error;
      if (generated_fmi2GetReal(comp, &s->md, &vr,  1, &vr1) != fmi2OK) return fmi2Error;

      fmi2Real res = fabs(vr1 - vr0);
      fmi2Real ref = 0.5*fabs(vr0);

      //fprintf(stderr, "lo=%e dwrt=%e hi=%e hi/lo=%e -> %e vs %e", lo, dwrt, hi, hi/lo, res, ref);

      if (res < ref) {
        //difference too small - move lo up
        //fprintf(stderr, " up\n");
        lo = dwrt;
      } else {
        //difference too big - move hi down
        //fprintf(stderr, " down\n");
        hi = dwrt;
      }
    }

    //restore state
    if (wrapper_set(&s->simulation) != fmi2OK) return fmi2Error;

    *partial = (vr1 - vr0) / dwrt;
    //fprintf(stderr, "numerical try %i: %+.16le = (%f - %f) / %f  [wrt0 = %f, wrt = %i] hi/lo = %e\n", i, *partial, vr1, vr0, dwrt, wrt0, wrt, hi/lo);
    return fmi2OK;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    //fprintf(stderr,"do step run iteration\n");
    runIteration(&s->simulation.sim, currentCommunicationPoint,communicationStepSize);
}

//extern "C"{
void jmCallbacksLogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
    fprintf(stderr, "[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}

static fmi2Status wrapper_instantiate(ModelInstance *comp)  {
    fprintf(stderr,"Init Wrapper\n");
    state_t *s = &comp->s;
    //char *m_fmuLocation;
    char m_resourcePath[1024];
    char m_fmuPath[1024];
    const char* m_instanceName;
    fmi_version_enu_t m_version;
    fmi_import_context_t* m_context;

    comp->s.simulation.m_jmCallbacks.malloc = malloc;
    comp->s.simulation.m_jmCallbacks.calloc = calloc;
    comp->s.simulation.m_jmCallbacks.realloc = realloc;
    comp->s.simulation.m_jmCallbacks.free = free;
    comp->s.simulation.m_jmCallbacks.logger = jmCallbacksLogger;
    comp->s.simulation.m_jmCallbacks.log_level = 0;
    comp->s.simulation.m_jmCallbacks.context = 0;

    comp->s.simulation.m_jmCallbacks.logger(NULL,"modulename",0,"jm_string");

    strlcpy(m_fmuPath, comp->fmuResourceLocation, sizeof(m_fmuPath));
    strlcat(m_fmuPath, "/", sizeof(m_fmuPath));
    strlcat(m_fmuPath, fmuFilename, sizeof(m_fmuPath));

    if (!(comp->s.simulation.dir = fmi_import_mk_temp_dir(&comp->s.simulation.m_jmCallbacks, NULL, "wrapper_"))) {
        fprintf(stderr, "fmi_import_mk_temp_dir() failed\n");
        exit(1);
    }

    m_context = fmi_import_allocate_context(&comp->s.simulation.m_jmCallbacks);
    fprintf(stderr," cont :%p\n",m_context);
    // unzip the real fmu
    m_version = fmi_import_get_fmi_version(m_context, m_fmuPath, comp->s.simulation.dir);
    fprintf(stderr,"%s\n",m_fmuPath);
    fprintf(stderr,"wrapper: got version %d\n",m_version);

    if ((m_version <= fmi_version_unknown_enu) || (m_version >= fmi_version_unsupported_enu)) {

        fmi_import_free_context(m_context);
        fmi_import_rmdir(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
        exit(1);
    }
    if (m_version == fmi_version_2_0_enu) { // FMI 2.0
        // parse the xml file
        /* fprintf(stderr,"dir: %s\n",dir); */
        comp->s.simulation.FMU = fmi2_import_parse_xml(m_context, comp->s.simulation.dir, 0);
        fprintf(stderr,"dir: %s\n",comp->s.simulation.dir);
        if(!comp->s.simulation.FMU) {
        fprintf(stderr,"dir: %s\n",comp->s.simulation.dir);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
            return fmi2Error;
        }

        // check FMU kind
        fmi2_fmu_kind_enu_t fmuType = fmi2_import_get_fmu_kind(comp->s.simulation.FMU);
        if(fmuType != fmi2_fmu_kind_me) {
            fprintf(stderr,"Wrapper only supports model exchange\n");
            fmi2_import_free(comp->s.simulation.FMU);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
            return fmi2Error;
        }
        // FMI callback functions
        const fmi2_callback_functions_t m_fmi2CallbackFunctions = {fmi2_log_forwarding, calloc, free, 0, 0};

        // Load the binary (dll/so)
        jm_status_enu_t status = fmi2_import_create_dllfmu(comp->s.simulation.FMU, fmuType, &m_fmi2CallbackFunctions);
        if (status == jm_status_error) {
            fmi2_import_free(comp->s.simulation.FMU);
            fmi_import_free_context(m_context);
            fmi_import_rmdir(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
            return fmi2Error;
        }
        m_instanceName = fmi2_import_get_model_name(comp->s.simulation.FMU);
        {
            //m_fmuLocation = fmi_import_create_URL_from_abs_path(&comp->s.simulation.m_jmCallbacks, m_fmuPath);
        }
        fprintf(stderr,"have instancename %s\n",m_instanceName);

        {
            char *temp = fmi_import_create_URL_from_abs_path(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
            strlcpy(m_resourcePath, temp, sizeof(m_resourcePath));
            strlcat(m_resourcePath, "/resources", sizeof(m_resourcePath));
            comp->s.simulation.m_jmCallbacks.free(temp);
        }
    } else {
        // todo add FMI 1.0 later on.
        fmi_import_free_context(m_context);
        fmi_import_rmdir(&comp->s.simulation.m_jmCallbacks, comp->s.simulation.dir);
        return fmi2Error;
    }
    fmi_import_free_context(m_context);

    fmi2_status_t status = fmi2_import_instantiate(comp->s.simulation.FMU , m_instanceName, fmi2_model_exchange, m_resourcePath, 0);
    if(status == fmi2Error){
        fprintf(stderr,"Wrapper: instatiate faild\n");
        return fmi2Error;
    }

    return fmi2OK;
}

static fmi2Status wrapper_setup_experiment(ModelInstance *comp,
        fmi2Boolean toleranceDefined, fmi2Real tolerance,
        fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {
    // Calling this on Dymola FMUs with toleranceDefined = true gave this:
    //fmi2SetupExperiment: tolerance control not supported for fmuType fmi2ModelExchange, setting toleranceDefined to fmi2False
    return (fmi2Status)fmi2_import_setup_experiment(comp->s.simulation.FMU, false, 0.0, startTime, stopTimeDefined, stopTime) ;
}

static fmi2Status wrapper_enter_init(ModelInstance *comp) {
    return (fmi2Status)fmi2_import_enter_initialization_mode(comp->s.simulation.FMU) ;
}

static fmi2Status wrapper_exit_init(ModelInstance *comp) {
    FILE *fp;
    fmi2Status status;

    //PATH_MAX is usually 512 or so, this should be enough
    char path[1024];

    prepare(&comp->s.simulation.sim, comp->s.simulation.FMU, comp->s.md.integrator);
    status = (fmi2Status)fmi2_import_exit_initialization_mode(comp->s.simulation.FMU);
    if(status == fmi2Error){
        return status;
    }
    status = (fmi2Status)fmi2_import_enter_continuous_time_mode(comp->s.simulation.FMU);
    if(status == fmi2Error){
        return status;
    }

    strlcpy(path, comp->fmuResourceLocation, sizeof(path));
    strlcat(path, "/directional.txt",        sizeof(path));

    fp = fopen(path, "r");

    if (fp) {
        //read partials from directional.txt
        me_simulation *me = &comp->s.simulation;

        for (;;) {
            partial_t *p = &me->partials[me->npartials];
            int n = fscanf(fp, "%u %u %u", &p->unknown, &p->known, &p->vr);

            if (n < 3) {
                break;
            }

            if (++me->npartials >= sizeof(me->partials)/sizeof(me->partials[0])) {
                break;
            }
        }

        fclose(fp);
    }

    return fmi2OK;
}



static void wrapper_free(me_simulation me) {
  fmi2_import_terminate(me.FMU);
  fmi2_import_free_instance(me.FMU);
  fmi2_import_destroy_dllfmu(me.FMU);
  fmi2_import_free(me.FMU);
  fmi_import_rmdir(&me.m_jmCallbacks, me.dir);
  free(me.dir);
  cgsl_free_simulation(me.sim);
}

#include "fmuTemplate_impl.h"
