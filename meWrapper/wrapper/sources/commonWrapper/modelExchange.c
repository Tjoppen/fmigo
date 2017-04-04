#include "modelExchange.h"
//#include "include/fmi2.h"
#include "math.h"
#ifndef max
#define max(a,b) ((a>b) ? a : b)
#endif
#ifndef min
#define min(a,b) ((a<b) ? a : b)
#endif
static fmi2_import_t* FMU;
static cgsl_simulation m_sim;
static TimeLoop timeLoop;
static fmu_model m_model;
#define MEFMU *getfmi2Instance()
//#define signbits(a,b) ((a > 0)? ( (b > 0) ? 1 : 0) : (b<=0)? 1: 0)
/* //#define get_storage m_baseMaster->get_storage */
/* /\** get_p */
/*  *  Extracts the parameters from the model */
/*  * */
/*  *  @param m Model */
/*  *\/ */
fmi2_import_t** getfmi2Instance(){
    return &FMU;
}
double* getContinuousStates(){
    return m_sim.model->x;
}
void meGetReal(const fmi2_value_reference_t* vr, int nvr,  fmi2_real_t* value){
    fmi2_import_get_real(MEFMU,vr,nvr,value);
}

inline fmu_parameters* get_p(fmu_model* m){
    return (fmu_parameters*)(m->model->parameters);
}

void getSafeAndCrossed();
bool past_event(fmi2_real_t* a, fmi2_real_t* b, int i){

    i--;
    for(;i>=0;i--){
        if(signbit( a[i] ) != signbit( b[i] ))
            return true;
    }
    return false;
}

/* /\** fmu_function */
/*  *  function needed by cgsl_simulation to get and set the current */
/*  *  states and derivatives */
/*  * */
/*  *  @param t Input time */
/*  *  @param x Input states vector */
/*  *  @param dxdt Output derivatives */
/*  *  @param params Contains model specific parameters */
/*  *\/ */
static int fmu_function(double t, const double x[], double dxdt[], void* params)
{
    // make local variables
    fmu_parameters* p = (fmu_parameters*)params;

    ++p->count; /* count function evaluations */
    if(p->stateEvent)return GSL_SUCCESS;

    fmi2_import_set_time(MEFMU,t);
    fmi2_import_set_continuous_states(MEFMU,x,p->nx);

    fmi2_import_get_derivatives(MEFMU,dxdt,p->nx);

    if(p->ni){
        fmi2_import_get_event_indicators(MEFMU,p->ei,p->ni);
        if(past_event(p->ei,p->ei_backup,p->ni)){
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

/* /\** allocate Memory */
/*  *  Allocates memory needed by the fmu_model */
/*  * */
/*  *  @param m The fmu_model */
/*  *  @param clients Vector with clients */
/*  *\/ */
void allocateMemory(fmu_model *m){
    //fprintf(stderr,"init_fmu_model %p \n",p->fmi2Instance);
    m->model = (cgsl_model*)calloc(1,sizeof(cgsl_model));
    fmu_parameters* p = (fmu_parameters*)calloc(1,sizeof(fmu_parameters));
    m->model->parameters = p;
    m->model->n_variables = fmi2_import_get_number_of_continuous_states(MEFMU);;
    if(m->model->n_variables == 0){
        fprintf(stderr,"ModelExchangeStepper nothing to integrate\n");
        exit(0);
    }

    p->nx = m->model->n_variables;

    p->ni = fmi2_import_get_number_of_event_indicators(MEFMU);
    p->ei             = (fmi2_real_t*)calloc(p->ni, sizeof(fmi2_real_t));
    p->ei_backup      = (fmi2_real_t*)calloc(p->ni, sizeof(fmi2_real_t));
    m->model->x        = (double*)calloc(m->model->n_variables, sizeof(double));
    m->model->x_backup = (double*)calloc(m->model->n_variables, sizeof(double));
    p->backup.dydt    = (double*)calloc(m->model->n_variables, sizeof(double));

    if(!m->model->x || !m->model->x_backup || !p->backup.dydt){
        //freeFMUModel(m);
        perror("WeakMaster:ModelExchange:allocateMemory ERROR -  could not allocate memory");
        exit(1);
    }
}

/* /\** init_fmu_model */
/*  *  Setup all parameters and function pointers needed by fmu_model */
/*  * */
/*  *  @param m The fmu_model we are working on */
/*  *  @param client A vector with clients */
/*  *\/ */

void init_fmu_model(fmu_model *m){
    //(p->fmi2Instance) = MEFMU;
    allocateMemory(m);
    fmu_parameters* p = get_p(m);
    //m->model->get_model_parameters = get_model_parameters;

    p->t_ok = 0;
    p->t_past = 0;
    p->stateEvent = false;
    p->count = 0;

    p->backup.t = 0;
    p->backup.h = 0;

    m->model->function = fmu_function;
    m->model->jacobian = NULL;
    m->model->post_step = NULL;
    m->model->pre_step = NULL;
    m->model->free = cgsl_model_default_free;//freeFMUModel;

    fmi2_status_t status = fmi2_import_get_continuous_states(MEFMU, m->model->x, p->nx);

    status = fmi2_import_get_event_indicators(MEFMU,p->ei_backup,p->ni);
    memcpy(m->model->x_backup,m->model->x,m->model->n_variables);
}

/* /\** prepare() */
/*  *  Setup everything */
/*  *\/ */
void prepare() {
    init_fmu_model(&m_model);
    // set up a gsl_simulation for each client
    fmu_parameters* p = get_p(&m_model);

    //m_sim = (cgsl_simulation *)malloc(sizeof(cgsl_simulation));
    m_sim = cgsl_init_simulation(m_model.model,
                                 rk8pd, /* integrator: Runge-Kutta Prince Dormand pair order 7-8 */
                                 1e-10,
                                 0,
                                 0,
                                 0, NULL
                                 );
#ifdef USE_GPL
#endif
}

/* /\** restoreStates() */
/*  *  Restores all values needed by the simulations to restart */
/*  *  before the event */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
void restoreStates(cgsl_simulation *sim){
    fmu_parameters* p = get_p((fmu_model*)&sim->model);
    //restore previous states


    memcpy(sim->model->x,sim->model->x_backup,sim->model->n_variables);

    memcpy(sim->i.evolution->dydt_out, p->backup.dydt,
           sim->model->n_variables * sizeof(p->backup.dydt[0]));

    sim->i.evolution->failed_steps = p->backup.failed_steps;
    sim->t = p->backup.t;
    sim->h = p->backup.h;
}

/* /\** storeStates() */
/*  *  Stores all values needed by the simulations to restart */
/*  *  from a state before an event */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
void storeStates(cgsl_simulation *sim){
    fmu_parameters* p = get_p((fmu_model*)&sim->model);

    fmi2_import_get_continuous_states(MEFMU,sim->model->x_backup,p->nx);
    fmi2_import_get_event_indicators(MEFMU,p->ei_backup,p->ni);
    memcpy(sim->model->x,sim->model->x_backup,p->nx * sizeof(sim->model->x[0]));

    p->backup.failed_steps = sim->i.evolution->failed_steps;
    p->backup.t = sim->t;
    p->backup.h = sim->h;

    memcpy(p->backup.dydt, sim->i.evolution->dydt_out,
           sim->model->n_variables * sizeof(p->backup.dydt[0]));
}

/* /\** hasStateEvent() */
/*  *  Retrieve stateEvent status */
/*  *  Returns true if at least one simulation crossed an event */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
bool hasStateEvent(cgsl_simulation *sim){
    return get_p((fmu_model*)&sim->model)->stateEvent;
}

/* /\** getGoldenNewTime() */
/*  *  Calculates a time step which brings solution closer to the event */
/*  *  Uses the golden ratio to get t_crossed and t_safe to converge */
/*  *  to the event time */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
void getGoldenNewTime(cgsl_simulation *sim){
    // golden ratio
    double phi = (1 + sqrt(5)) / 2;
    /* passed solution, need to reduce tEnd */
    if(hasStateEvent(sim)){
        getSafeAndCrossed();
        restoreStates(sim);
        timeLoop.dt_new = (timeLoop.t_crossed - sim->t) / phi;
    } else { // havent passed solution, increase step
        storeStates(sim);
        timeLoop.t_safe = max(timeLoop.t_safe, sim->t);
        timeLoop.dt_new = timeLoop.t_crossed - sim->t - (timeLoop.t_crossed - timeLoop.t_safe) / phi;
    }
}

/* /\** me_step() */
/*  *  Run cgsl_step_to the simulation */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
void me_step(cgsl_simulation *sim){
    fmu_parameters *p;
    p = get_p((fmu_model*)(&sim->model));
    p->stateEvent = false;
    p->t_past = max(p->t_past, sim->t + timeLoop.dt_new);
    p->t_ok = sim->t;

    cgsl_step_to(sim, sim->t, timeLoop.dt_new);
}

fmi2_real_t absmin(fmi2_real_t* v, size_t n){
    fmi2_real_t min = v[0];
    for(; n>1; n--){
        if(min > v[n-1]) min = v[n-1];
    }
    return min;
}
/* /\** stepToEvent() */
/*  *  To be runned when an event is crossed. */
/*  *  Finds the event and returns a state immediately after the event */
/*  * */
/*  *  @param sim The simulation */
/*  *\/ */
void stepToEvent(cgsl_simulation *sim){
    double tol = 1e-9;
    fmu_parameters* p = get_p((fmu_model*)&sim->model);
    while(!hasStateEvent(sim) && !(absmin(p->ei,p->ni) < tol || timeLoop.dt_new < tol)){
        getGoldenNewTime(sim);
        me_step(sim);
        if(timeLoop.dt_new == 0){
            fprintf(stderr,"stepToEvent: dt == 0, abort\n");
            exit(1);
        }
        if(hasStateEvent(sim)){
            // step back to where event occured
            restoreStates(sim);
            getSafeAndCrossed();
            timeLoop.dt_new = timeLoop.t_safe - sim->t;
            me_step(sim);
            if(!hasStateEvent(sim)) storeStates(sim);
            else{
                fprintf(stderr,"stepToEvent: failed at stepping to safe time, aborting\n");
                exit(1);
            }
            timeLoop.dt_new = timeLoop.t_crossed - sim->t;
            me_step(sim);
            if(!hasStateEvent(sim)){
                fprintf(stderr,"stepToEvent: failed at stepping to event \n");
                exit(1);
            }
        }
    }
}

/* /\** newDiscreteStates() */
/*  *  Should be used where a new discrete state ends and another begins. */
/*  *  Store the current state of the simulation */
/*  *\/ */
void newDiscreteStates(){
    fmu_parameters* p = get_p((fmu_model*)&m_sim.model);
    // start at a new state
    fmi2_import_enter_event_mode(MEFMU);

    // todo loop until newDiscreteStatesNeeded == false

    fmi2_event_info_t eventInfo;
    eventInfo.newDiscreteStatesNeeded = true;
    eventInfo.terminateSimulation = false;

    while(eventInfo.newDiscreteStatesNeeded){
        fmi2_import_new_discrete_states(MEFMU,&eventInfo);

        if(eventInfo.terminateSimulation){
                fprintf(stderr,"modelExchange.c: terminated simulation\n");
                exit(1);
        }
    }

    fmi2_import_enter_continuous_time_mode(MEFMU);

    // store the current state of all running FMUs
    storeStates(&m_sim);
}

/* /\** getSafeAndCrossed() */
/*  *  Extracts safe and crossed time found by fmu_function */
/*  *\/ */
void getSafeAndCrossed(){
    fmu_parameters *p = get_p((fmu_model*)&m_sim.model);
    timeLoop.t_safe    = p->t_ok;//max( timeLoop.t_safe,    t_ok);
    timeLoop.t_crossed = p->t_past;//min( timeLoop.t_crossed, t_past);
}

/* /\** safeTimeStep() */
/*  *  Make sure we take small first step when we're at on event */
/*  *  @param sim The simulation */
/*  *\/ */
void safeTimeStep(cgsl_simulation *sim){
    // if sims has a state event do not step to far
    if(hasStateEvent(sim)){
        double absmin = 0;//m_baseMaster->get_storage().absmin(STORAGE::indicators);
        timeLoop.dt_new = sim->h * (absmin > 0 ? absmin:0.00001);
    }else
        timeLoop.dt_new = timeLoop.t_end - sim->t;
}

/* /\** getSafeTime() */
/*  * */
/*  *  @param clients Vector with clients */
/*  *  @param t The current time */
/*  *  @param dt Timestep, input and output */
/*  *\/ */
void getSafeTime(double t, double *dt){
    fmu_parameters* p = get_p((fmu_model*)&m_sim.model);
    if(p->backup.eventInfo.nextEventTimeDefined)
        *dt = min(*dt, t - p->backup.eventInfo.nextEventTime);
}

/* /\** runIteration() */
/*  *  @param t The current time */
/*  *  @param dt The timestep to be taken */
/*  *\/ */
void runIteration(double t, double dt) {
    timeLoop.t_safe = t;
    timeLoop.t_end = t + dt;
    timeLoop.dt_new = dt;
    getSafeTime(t, &timeLoop.dt_new);
    newDiscreteStates();
    int iter = 2;
    while( timeLoop.t_safe < timeLoop.t_end ){
        me_step(&m_sim);

        if (hasStateEvent(&m_sim)){

            getSafeAndCrossed();

            // restore and step to before the event
            restoreStates(&m_sim);
            timeLoop.dt_new = timeLoop.t_safe - m_sim.t;
            me_step(&m_sim);

            // store and step to the event
            if(!hasStateEvent(&m_sim)) storeStates(&m_sim);
            timeLoop.dt_new = timeLoop.t_crossed - m_sim.t;
            me_step(&m_sim);

            // step closer to the event location
            if(hasStateEvent(&m_sim))
                {
                    fprintf(stderr,"stepToEvent: ");
                stepToEvent(&m_sim);
                fprintf(stderr," done %0.32f %d\n", m_sim.t, hasStateEvent(&m_sim));

                }

        }
        else {
            timeLoop.t_safe = m_sim.t;
            timeLoop.t_crossed = timeLoop.t_end;
        }

        safeTimeStep(&m_sim);
        if(hasStateEvent(&m_sim))
            newDiscreteStates();
        storeStates(&m_sim);
    }
    fprintf(stderr,"runIteration: time %f\n",m_sim.t);
}
