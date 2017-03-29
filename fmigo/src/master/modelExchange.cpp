#include "master/modelExchange.h"
#define storage_alloc m_baseMaster->storage_alloc
//#define get_storage m_baseMaster->get_storage
using namespace fmitcp::serialize;
using namespace model_exchange;

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

ModelExchangeStepper::ModelExchangeStepper(std::vector<FMIClient*> clients, std::vector<WeakConnection> weakConnections, BaseMaster* baseMaster){
    m_baseMaster = baseMaster;
    m_clients = clients;
    m_weakConnections = weakConnections;
    prepare();
}

ModelExchangeStepper::~ModelExchangeStepper(){
    cgsl_free_simulation(m_sim);
    free(m_p.backup.dydt);
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

    for(auto client: p->clients){
        p->baseMaster->send(client, fmi2_import_set_time(t));
        p->baseMaster->send(client, fmi2_import_set_continuous_states(x + p->baseMaster->get_storage().get_offset(client->getId(), STORAGE::states),
                                                                      client->getNumContinuousStates()));
    }
    p->baseMaster->wait();
    p->baseMaster->solveLoops();

    for(auto client: p->clients)
        p->baseMaster->send(client, fmi2_import_get_derivatives((int)client->getNumContinuousStates()));
    p->baseMaster->wait();

    for(auto client: p->clients)
        p->baseMaster->get_storage().get_current_derivatives(dxdt, client->getId());

    for(auto client: p->clients){
        if(client->getNumEventIndicators())
            p->baseMaster->send(client, fmi2_import_get_event_indicators((int)client->getNumEventIndicators()));
    }

    p->baseMaster->wait();

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
 *  @param m Pointer to a fmu_model
 */
void ModelExchangeStepper::allocateMemory(fmu_model &m, const std::vector<FMIClient*> &clients){
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

/** init_fmu_model
 *  Creates the fmu_model and sets the parameters
 *
 *  @param client A pointer to a fmi client
 */
void ModelExchangeStepper::init_fmu_model(fmu_model &m,  const std::vector<FMIClient*> &clients){
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
        m_baseMaster->send(client, fmi2_import_get_continuous_states((int)client->getNumContinuousStates()));
    wait();

    for(auto client: clients)
        m_baseMaster->get_storage().get_current_states(m.model->x, client->getId());
}

#define STATIC_GET_CLIENT_OFFSET(name)                                  \
    p->baseMaster->get_storage().get_offset(client->getId(), STORAGE::name)
#define STATIC_SET_(name, name2, data)                                  \
    p->baseMaster->send(client, fmi2_import_set_##name##_##name2(       \
                                                                 data + STATIC_GET_CLIENT_OFFSET(name2), \
                                                                 client->getNumContinuousStates()));
#define STATIC_GET_(name)                                               \
    p->baseMaster->send(client, fmi2_import_get_##name((int)client->getNumContinuousStates()))

static int epce_post_step(int n, const double outputs[], void * params) {
    // make local variables
    fmu_parameters* p = (fmu_parameters*)params;
    if( p->sim_started ){
        ++p->count; /* count function evaluations */
        Data& states = p->baseMaster->get_storage().get_current_states();

        fprintf(stderr,"\nepce_post_step %f %f \n",outputs[0],outputs[1]);
        // extract current states to restore after outputs are changed
        for(auto client: p->clients)
            STATIC_GET_(continuous_states);
        std::vector<double> tmp(p->baseMaster->get_storage().get_current_states().size());
        p->baseMaster->get_storage().get_current_states(tmp.data());

        //set filtered states
        for(auto client: p->clients){
            STATIC_SET_(continuous,states,outputs);
        }
        p->baseMaster->wait();
        p->baseMaster->solveLoops();

        // send get_ to update outputs. TODO use one function call instead
        for(auto client: p->clients)
            STATIC_GET_(derivatives);
        p->baseMaster->wait();

        //reset old states
        for(auto client: p->clients){
            STATIC_SET_(continuous,states,tmp.data());
        }
    }

    return GSL_SUCCESS;
}

void ModelExchangeStepper::prepare() {
    if(m_clients.size() == 0)
        return;
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

/** restoreStates
 *  restores all values needed by the simulations to restart
 *  from a known safe time.
 *
 *  @param sim The simulation
 */
void ModelExchangeStepper::restoreStates(cgsl_simulation &sim){
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

/** storeStates
 *  stores all values needed by the simulations to restart
 *  from a known safe time.
 *
 *  @param sim The simulation
 */
void ModelExchangeStepper::storeStates(cgsl_simulation &sim){
    fmu_parameters* p = get_p(sim);
    for(auto client: m_clients)
        m_baseMaster->send(client, fmi2_import_get_continuous_states((int)client->getNumContinuousStates()));

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

/** hasStateEvent:
 ** returns true if at least one simulation has an event
 *
 *  @param sim The simulation
 */
bool ModelExchangeStepper::hasStateEvent(cgsl_simulation &sim){
    return get_p(sim)->stateEvent;
}

/** getSafeTime:
 *  caluclates a "safe" time, uses the golden ratio to get
 *  t_crossed and t_safe to converge towards same value
 *
 *  @param sim A cgsl simulation
 */
void ModelExchangeStepper::getGoldenNewTime(cgsl_simulation &sim){
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

/** step
 *  run cgsl_step_to on all simulations
 *
 *  @param sim The simulation
 */
void ModelExchangeStepper::step(cgsl_simulation &sim){
    fmu_parameters *p;
    p = get_p(sim);
    p->stateEvent = false;
    p->t_past = max(p->t_past, sim.t + timeLoop.dt_new);
    p->t_ok = sim.t;

    cgsl_step_to(&sim, sim.t, timeLoop.dt_new);
}

/** stepToEvent
 *  if there is an event, find the event and return
 *  the time at where the time event occured
 *
 *  @param sim The simulation
 *  @return Returns the time immediatly after the event
 */
void ModelExchangeStepper::stepToEvent(cgsl_simulation &sim){
    double tol = 1e-9;
    m_baseMaster->get_storage().print(states);
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

/** newDiscreteStates
 *  Should be used where a new discrete state ends
 *  and another begins. Resets the loop variables
 *  and store all states of the simulation
 *
 *  @param t The current time
 *  @param t_new New next time
 */
void ModelExchangeStepper::newDiscreteStates(){
    // start at a new state
    m_baseMaster->sendWait(m_clients, fmi2_import_enter_event_mode());
    // todo loop until newDiscreteStatesNeeded == false
    bool newDiscreteStatesNeeded = true;

    while(newDiscreteStatesNeeded){
        newDiscreteStatesNeeded = false;
        m_baseMaster->sendWait(m_clients, fmi2_import_new_discrete_states());
        for(auto client: m_clients){
            newDiscreteStatesNeeded += client->m_event_info.newDiscreteStatesNeeded;
            if(client->m_event_info.terminateSimulation){
                fprintf(stderr,"modelExchange.cpp: client %d terminated simulation\n",client->getId());
                exit(1);
            }
        }
    }

    m_baseMaster->sendWait(m_clients, fmi2_import_enter_continuous_time_mode());

    for(auto client: m_clients)
        m_baseMaster->send(client, fmi2_import_get_event_indicators((int)client->getNumEventIndicators()));

    wait();

    // store the current state of all running FMUs
    storeStates(m_sim);
}

void ModelExchangeStepper::printStates(void){
    fprintf(stderr,"      states     ");
    m_baseMaster->get_storage().print(STORAGE::states);
    fprintf(stderr,"      indicator  ");
    m_baseMaster->get_storage().print(STORAGE::indicators);
}
void ModelExchangeStepper::getSafeAndCrossed(){
    fmu_parameters *p = get_p(m_sim);
    timeLoop.t_safe    = p->t_ok;//max( timeLoop.t_safe,    t_ok);
    timeLoop.t_crossed = p->t_past;//min( timeLoop.t_crossed, t_past);
}

void ModelExchangeStepper::safeTimeStep(cgsl_simulation &sim){
    // if sims has a state event do not step to far
    if(hasStateEvent(sim)){
        double absmin = m_baseMaster->get_storage().absmin(STORAGE::indicators);
        timeLoop.dt_new = sim.h * (absmin > 0 ? absmin:0.00001);
    }else
        timeLoop.dt_new = timeLoop.t_end - sim.t;
}

void ModelExchangeStepper::getSafeTime(const std::vector<FMIClient*> clients, double &dt){
    for(auto client: clients)
        if(client->m_event_info.nextEventTimeDefined)
            dt = min(dt,client->m_event_info.nextEventTime);
}

void ModelExchangeStepper::runIteration(double t, double dt) {
    if(m_clients.size() == 0)
        return;

    timeLoop.t_safe = t;
    timeLoop.t_end = t + dt;
    timeLoop.dt_new = dt;
    getSafeTime(m_clients, timeLoop.dt_new);
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
                {
                    fprintf(stderr,"stepToEvent\n");
                stepToEvent(m_sim);
                    fprintf(stderr,"stepToEvent done\n");
                }

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
