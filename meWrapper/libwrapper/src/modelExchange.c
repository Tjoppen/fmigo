#include "modelExchange.h"
//#include "include/fmi2.h"
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
    int nx;
    int ni;

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
    fmi2_import_set_continuous_states(p->FMU, x, p->nx);

    fmi2_import_get_derivatives(p->FMU, dxdt, p->nx);

    if(p->ni){
        fmi2_import_get_event_indicators(p->FMU, p->m_backup.ei, p->ni);
        if(past_event(p->m_backup.ei_b, p->m_backup.ei, p->ni)){
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

void allocateBackup(Backup *backup, void *params){
    fmu_parameters *p = (fmu_parameters*)params;
    backup->ei          = (fmi2_real_t*)calloc(p->ni, sizeof(fmi2_real_t));
    backup->ei_b        = (fmi2_real_t*)calloc(p->ni, sizeof(fmi2_real_t));
    backup->x           = (double*)calloc(p->nx, sizeof(double));
    backup->dydt        = (double*)calloc(p->nx, sizeof(double));
    if(!backup->ei || !backup->ei_b || !backup->x || !backup->dydt){
        //freeFMUModel(m);
        perror("WeakMaster:ModelExchange:allocateBackup ERROR -  could not allocate memory");
        exit(1);
    }
}

void freeBackup(Backup *backup) {
  free(backup->ei);
  free(backup->ei_b);
  free(backup->x);
  free(backup->dydt);
}

static void me_model_free(cgsl_model *model) {
  fmu_parameters* p = get_p(model);
  freeBackup(&p->m_backup);
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
    p->nx         = fmi2_import_get_number_of_continuous_states(p->FMU);
    p->ni         = fmi2_import_get_number_of_event_indicators(p->FMU);
    p->t_ok       = 0;
    p->t_past     = 0;
    p->stateEvent = false;
    p->count      = 0;

    allocateBackup(&p->m_backup, p);

    *m = cgsl_model_default_alloc(p->nx, NULL, p, fmu_function, NULL, NULL, NULL, 0);
    (*m)->free = me_model_free;

    p->m_backup.t = 0;
    p->m_backup.h = 0;

    fmi2_status_t status;
    status = fmi2_import_get_continuous_states(p->FMU, (*m)->x,        p->nx);
    status = fmi2_import_get_continuous_states(p->FMU, (*m)->x_backup, p->nx);
    status = fmi2_import_get_event_indicators (p->FMU, p->m_backup.ei_b,  p->ni);
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


    memcpy(sim->model->x,backup->x,p->nx * sizeof(sim->model->x[0]));

    memcpy(sim->i.evolution->dydt_out, backup->dydt,
           sim->model->n_variables * sizeof(backup->dydt[0]));

    sim->i.evolution->failed_steps = backup->failed_steps;
    sim->t = backup->t;
    sim->h = backup->h;

    fmi2_import_set_time(p->FMU, sim->t);
    fmi2_import_set_continuous_states(p->FMU, sim->model->x, p->nx);

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

    fmi2_import_get_continuous_states(p->FMU, backup->x,    p->nx);
    fmi2_import_get_event_indicators (p->FMU, backup->ei_b, p->ni);
    memcpy(sim->model->x,backup->x,p->nx * sizeof(backup->x[0]));

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
    while(!hasStateEvent(sim) && !(absmin(backup->ei,p->ni) < tol || timeLoop->dt_new < tol)){
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

    if(p->ni){
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
