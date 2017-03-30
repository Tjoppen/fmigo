#ifndef MODELEXCHANGE_H
#define MODELEXCHANGE_H
#define storage_alloc m_baseMaster->storage_alloc
//#define get_storage m_baseMaster->get_storage
using namespace fmitcp::serialize;
using namespace fmitcp_master;
using namespace fmitcp;
namespace model_exchange{
typedef struct TimeLoop
{
    fmi2_real_t t_safe, dt_new, t_crossed, t_end;
} TimeLoop;
struct Backup
{
    double t;
    double h;
    double *dydt;
    unsigned long failed_steps;
};
struct fmu_parameters{

    double t_ok;
    double t_past;
    int count;                    /* number of function evaluations */

    BaseMaster* baseMaster;       /* BaseMaster object pointer */
    std::vector<FMIClient*> clients;            /* FMIClient vector */
    std::vector<int> intv;            /* FMIClient vector */
    //std::vector<WeakConnection> weakConnections;

    bool stateEvent;
    Backup backup;
    bool sim_started;
};
struct fmu_model{
    cgsl_model *model;
};
class ModelExchangeStepper {
cgsl_simulation m_sim;
fmu_parameters m_p;
BaseMaster* m_baseMaster;
fmu_model m_model;
TimeLoop timeLoop;
std::vector<WeakConnection> m_weakConnections;
std::vector<FMIClient*> m_clients;

/** get_p
 *  Extracts the parameters from the model
 *
 *  @param m Model
 */
inline fmu_parameters* get_p(cgsl_model* m){
    return (fmu_parameters*)(m->get_model_parameters(m));
}
inline fmu_parameters* get_p(fmu_model& m){
    return ((fmu_parameters *)(m.model->get_model_parameters(m.model)));
}
inline fmu_parameters* get_p(fmu_model* m){
    return (fmu_parameters*)(m->model->get_model_parameters(m->model));
}
inline fmu_parameters* get_p(cgsl_simulation s){
    return get_p(s.model);
}

 public:
/** ModelExchangeStepper()
 *  Class initializer
 *
 *  @param client A client
 *  @param weakConnections WeakConnections.. not used
 *  @param baseMaster The baseMaster we're working for
 */
ModelExchangeStepper(FMIClient* client, std::vector<WeakConnection> weakConnections, BaseMaster* baseMaster){
    m_baseMaster = baseMaster;
    m_clients.push_back(client);
    m_weakConnections = weakConnections;
    prepare();
}

/** ModelExchangeStepper()
 *  Class initializer
 *
 *  @param clients Vector with clients
 *  @param weakConnections WeakConnections.. not used
 *  @param baseMaster The baseMaster we're working for
 */
ModelExchangeStepper(std::vector<FMIClient*> clients, std::vector<WeakConnection> weakConnections, BaseMaster* baseMaster){
    m_baseMaster = baseMaster;
    m_clients = clients;
    m_weakConnections = weakConnections;
    prepare();
}

/** ~ModelExchangeStepper()
 *  Class destructor
 */
~ModelExchangeStepper(){
    cgsl_free_simulation(m_sim);
    free(m_p.backup.dydt);
}

/** runIteration()
 *  @param t The current time
 *  @param dt The timestep to be taken
 */
void runIteration(double t, double dt) {
    timeLoop.t_safe = t;
    timeLoop.t_end = t + dt;
    timeLoop.dt_new = dt;
    getSafeTime(m_clients, t, timeLoop.dt_new);
    get_p(m_sim)->sim_started = true;
    newDiscreteStates();
    int iter = 2;
    while( timeLoop.t_safe < timeLoop.t_end ){
        step(m_sim);
        if (hasStateEvent(m_sim)){
            getSafeAndCrossed();

            // restore and step to before the event
            restoreStates(m_sim);
            timeLoop.dt_new = timeLoop.t_safe - m_sim.t;
            step(m_sim);

            // store and step to the event
            if(!hasStateEvent(m_sim)) storeStates(m_sim);
            timeLoop.dt_new = timeLoop.t_crossed - m_sim.t;
            step(m_sim);

            // step closer to the event location
            if(hasStateEvent(m_sim))
                stepToEvent(m_sim);

        }
        else {
            timeLoop.t_safe = m_sim.t;
            timeLoop.t_crossed = timeLoop.t_end;
        }

        safeTimeStep(m_sim);
        if(hasStateEvent(m_sim))
            newDiscreteStates();
        storeStates(m_sim);
    }
}
 private:
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

    for(auto client: p->clients){
        p->FMIGO_ME_SET_TIME(client);
        p->FMIGO_ME_SET_CONTINUOUS_STATES(client, x);
    }
    p->FMIGO_ME_WAIT();
    p->baseMaster->solveLoops();

    for(auto client: p->clients)
        p-> FMIGO_ME_GET_DERIVATIVES(client);
    p->FMIGO_ME_WAIT();

    for(auto client: p->clients)
        p->baseMaster->get_storage().get_current_derivatives(dxdt, client->getId());

    for(auto client: p->clients){
        if(client->getNumEventIndicators())
            p->FMIGO_ME_GET_EVENT_INDICATORS(client);
    }

    p->FMIGO_ME_WAIT();

    //p->baseMaster->get_storage().print(states);
    //fprintf(stderr,"x[0] %f x[1] %f\n",x[0],x[1]);
    for(auto client: p->clients){
        if(client->getNumEventIndicators()){
            if(p->baseMaster->get_storage().past_event(client->getId())){
                p->stateEvent = true;
                p->t_past = t;
                return GSL_SUCCESS;
            } else{
                p->stateEvent = false;
                p->t_ok = t;
            }
        }
    }
    return GSL_SUCCESS;
}


/** allocate Memory
 *  Allocates memory needed by the fmu_model
 *
 *  @param m The fmu_model
 *  @param clients Vector with clients
 */
void allocateMemory(fmu_model &m, const std::vector<FMIClient*> &clients){
    m.model = (cgsl_model*)calloc(1,sizeof(cgsl_model));
    storage_alloc(clients);
    m.model->n_variables = m_baseMaster->get_storage().get_current_states().size();
    if(m.model->n_variables == 0){
        cerr << "ModelExchangeStepper nothing to integrate" << endl;
        exit(0);
    }

    m.model->x = (double*)calloc(m.model->n_variables, sizeof(double));
    m.model->x_backup = (double*)calloc(m.model->n_variables, sizeof(double));

    m_p.backup.dydt = (double*)calloc(m.model->n_variables, sizeof(double));

    if(!m.model->x || !m_p.backup.dydt){
        //freeFMUModel(m);
        perror("WeakMaster:ModelExchange:allocateMemory ERROR -  could not allocate memory");
        exit(1);
    }
}
static inline void *get_model_parameters(const cgsl_model *m){
    return m->parameters;
}

/** init_fmu_model
 *  Setup all parameters and function pointers needed by fmu_model
 *
 *  @param m The fmu_model we are working on
 *  @param client A vector with clients
 */
void init_fmu_model(fmu_model &m,  const std::vector<FMIClient*> &clients){
    allocateMemory(m, clients);
    m.model->parameters = (void*)&m_p;
    m.model->get_model_parameters = get_model_parameters;
    fmu_parameters* p = get_p(m);

    p->t_ok = 0;
    p->t_past = 0;
    p->baseMaster = m_baseMaster;
    p->stateEvent = false;
    p->sim_started = false;
    //p.clients.resize(0,0);
    p->count = 0;

    p->backup.t = 0;
    p->backup.h = 0;
    p->clients = m_clients;

    m.model->function = fmu_function;
    m.model->jacobian = NULL;
    m.model->post_step = NULL;
    m.model->pre_step = NULL;
    m.model->free = cgsl_model_default_free;//freeFMUModel;

    for(auto client: clients)
        p->FMIGO_ME_GET_CONTINUOUS_STATES(client);
    wait();

    for(auto client: clients)
        m_baseMaster->get_storage().get_current_states(m.model->x, client->getId());
}

/** epce_post_step()
 *  Sync_out
 *  Does not work yet
 */
static int epce_post_step(int n, const double outputs[], void * params) {
    // make local variables
    fmu_parameters* p = (fmu_parameters*)params;
    if( p->sim_started ){
        ++p->count; /* count function evaluations */
        Data& states = p->baseMaster->get_storage().get_current_states();

        fprintf(stderr,"\nepce_post_step %f %f \n",outputs[0],outputs[1]);
        // extract current states to restore after outputs are changed
        for(auto client: p->clients)
            p->FMIGO_ME_GET_CONTINUOUS_STATES(client);
        std::vector<double> tmp(p->baseMaster->get_storage().get_current_states().size());
        p->baseMaster->get_storage().get_current_states(tmp.data());

        //set filtered states
        for(auto client: p->clients){
          p->FMIGO_ME_SET_CONTINUOUS_STATES(client, outputs);
        }
        p->FMIGO_ME_WAIT();
        p->baseMaster->solveLoops();

        // send get_ to update outputs. TODO use one function call instead
        for(auto client: p->clients)
            p->FMIGO_ME_GET_DERIVATIVES(client);
        p->FMIGO_ME_WAIT();

        //reset old states
        for(auto client: p->clients){
          p->FMIGO_ME_SET_CONTINUOUS_STATES(client,tmp.data());
        }
    }

    return GSL_SUCCESS;
}

/** prepare()
 *  Setup everything
 */
void prepare() {
#ifdef USE_GPL
    // set up a gsl_simulation for each client
    init_fmu_model(m_model, m_clients);
    fmu_parameters* p = get_p(m_model);
    int filter_length = m_baseMaster->get_storage().get_current_states().size();
    cgsl_model* e_model = cgsl_epce_default_model_init(m_model.model,  /* model */
                                                       2,
                                                       epce_post_step,
                                                       p);

    //m_sim = (cgsl_simulation *)malloc(sizeof(cgsl_simulation));
    m_sim = cgsl_init_simulation(
#ifdef MODEL_EXCHANGE_FILTER
                                 e_model,
#else
                                 m_model.model,
#endif
                                 rk8pd, /* integrator: Runge-Kutta Prince Dormand pair order 7-8 */
                                 1e-10,
                                 0,
                                 0,
                                 0, NULL
                                 );
    // might not be needed
    m_baseMaster->get_storage().sync();
#endif
}

/** restoreStates()
 *  Restores all values needed by the simulations to restart
 *  before the event
 *
 *  @param sim The simulation
 */
void restoreStates(cgsl_simulation &sim){
    m_baseMaster->get_storage().cycle();
    m_baseMaster->get_storage().sync();
    fmu_parameters* p = get_p(sim);
    //restore previous states

    for(auto client: m_clients)
        m_baseMaster->get_storage().get_current_states(sim.model->x,client->getId());

    memcpy(sim.i.evolution->dydt_out, p->backup.dydt,
           sim.model->n_variables * sizeof(p->backup.dydt[0]));

    sim.i.evolution->failed_steps = p->backup.failed_steps;
    sim.t = p->backup.t;
    sim.h = p->backup.h;
}

/** storeStates()
 *  Stores all values needed by the simulations to restart
 *  from a state before an event
 *
 *  @param sim The simulation
 */
void storeStates(cgsl_simulation &sim){
    fmu_parameters* p = get_p(sim);
    for(auto client: m_clients)
        p->FMIGO_ME_GET_CONTINUOUS_STATES(client);

    p->backup.failed_steps = sim.i.evolution->failed_steps;
    p->backup.t = sim.t;
    p->backup.h = sim.h;

    memcpy(p->backup.dydt, sim.i.evolution->dydt_out,
           sim.model->n_variables * sizeof(p->backup.dydt[0]));

    wait();
    for(auto client: m_clients)
        m_baseMaster->get_storage().get_current_states(sim.model->x,client->getId());
    m_baseMaster->get_storage().sync();
}

/** hasStateEvent()
 *  Retrieve stateEvent status
 *  Returns true if at least one simulation crossed an event
 *
 *  @param sim The simulation
 */
bool hasStateEvent(cgsl_simulation &sim){
    return get_p(sim)->stateEvent;
}

/** getGoldenNewTime()
 *  Calculates a time step which brings solution closer to the event
 *  Uses the golden ratio to get t_crossed and t_safe to converge
 *  to the event time
 *
 *  @param sim The simulation
 */
void getGoldenNewTime(cgsl_simulation &sim){
    // golden ratio
    double phi = (1 + sqrt(5)) / 2;
    /* passed solution, need to reduce tEnd */
    if(hasStateEvent(sim)){
        getSafeAndCrossed();
        restoreStates(sim);
        timeLoop.dt_new = (timeLoop.t_crossed - sim.t) / phi;
    } else { // havent passed solution, increase step
        storeStates(sim);
        timeLoop.t_safe = max(timeLoop.t_safe, sim.t);
        timeLoop.dt_new = timeLoop.t_crossed - sim.t - (timeLoop.t_crossed - timeLoop.t_safe) / phi;
    }
}

/** step()
 *  Run cgsl_step_to the simulation
 *
 *  @param sim The simulation
 */
void step(cgsl_simulation &sim){
    fmu_parameters *p;
    p = get_p(sim);
    p->stateEvent = false;
    p->t_past = max(p->t_past, sim.t + timeLoop.dt_new);
    p->t_ok = sim.t;

    cgsl_step_to(&sim, sim.t, timeLoop.dt_new);
}

/** stepToEvent()
 *  To be runned when an event is crossed.
 *  Finds the event and returns a state immediately after the event
 *
 *  @param sim The simulation
 */
void stepToEvent(cgsl_simulation &sim){
    double tol = 1e-9;
    while(!hasStateEvent(sim) &&
            !(m_baseMaster->get_storage().absmin(STORAGE::indicators) < tol || timeLoop.dt_new < tol)){
        getGoldenNewTime(sim);
        step(sim);
        if(timeLoop.dt_new == 0){
            fprintf(stderr,"stepToEvent: dt == 0, abort\n");
            exit(1);
        }
        if(hasStateEvent(sim)){
            // step back to where event occured
            restoreStates(sim);
            getSafeAndCrossed();
            timeLoop.dt_new = timeLoop.t_safe - sim.t;
            step(sim);
            if(!hasStateEvent(sim)) storeStates(sim);
            else{
                fprintf(stderr,"stepToEvent: failed at stepping to safe time, aborting\n");
                exit(1);
            }
            timeLoop.dt_new = timeLoop.t_crossed - sim.t;
            step(sim);
            if(!hasStateEvent(sim)){
                cerr << "stepToEvent: failed at stepping to event " << endl;
                exit(1);
            }
        }
    }
}

/** newDiscreteStates()
 *  Should be used where a new discrete state ends and another begins.
 *  Store the current state of the simulation
 */
void newDiscreteStates(){
    fmu_parameters* p = get_p(m_sim);
    // start at a new state
    p->FMIGO_ME_ENTER_EVENT_MODE(m_clients);
    // todo loop until newDiscreteStatesNeeded == false
    bool newDiscreteStatesNeeded = true;

    while(newDiscreteStatesNeeded){
        newDiscreteStatesNeeded = false;
        p->FMIGO_ME_NEW_DISCRETE_STATES(m_clients);
        for(auto client: m_clients){
            newDiscreteStatesNeeded += client->m_event_info.newDiscreteStatesNeeded;
            if(client->m_event_info.terminateSimulation){
                fprintf(stderr,"modelExchange.cpp: client %d terminated simulation\n",client->getId());
                exit(1);
            }
        }
    }

    p->FMIGO_ME_ENTER_CONTINUOUS_TIME_MODE(m_clients);

    for(auto client: m_clients)
        p->FMIGO_ME_GET_EVENT_INDICATORS(client);

    wait();

    // store the current state of all running FMUs
    storeStates(m_sim);
}

/** getSafeAndCrossed()
 *  Extracts safe and crossed time found by fmu_function
 */
void getSafeAndCrossed(){
    fmu_parameters *p = get_p(m_sim);
    timeLoop.t_safe    = p->t_ok;//max( timeLoop.t_safe,    t_ok);
    timeLoop.t_crossed = p->t_past;//min( timeLoop.t_crossed, t_past);
}

/** safeTimeStep()
 *  Make sure we take small first step when we're at on event
 *  @param sim The simulation
 */
void safeTimeStep(cgsl_simulation &sim){
    // if sims has a state event do not step to far
    if(hasStateEvent(sim)){
        double absmin = m_baseMaster->get_storage().absmin(STORAGE::indicators);
        timeLoop.dt_new = sim.h * (absmin > 0 ? absmin:0.00001);
    }else
        timeLoop.dt_new = timeLoop.t_end - sim.t;
}

/** getSafeTime()
 *
 *  @param clients Vector with clients
 *  @param t The current time
 *  @param dt Timestep, input and output
 */
void getSafeTime(const std::vector<FMIClient*> clients, double t, double &dt){
    for(auto client: clients)
        if(client->m_event_info.nextEventTimeDefined)
            dt = min(dt, t - client->m_event_info.nextEventTime);
}

};
}
#endif
