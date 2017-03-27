/*
 * WeakMasters.h
 *
 *  Created on: Aug 12, 2014
 *      Author: thardin
 */

#ifndef WEAKMASTERS_H_
#define WEAKMASTERS_H_
#include "master/BaseMaster.h"
#include "master/WeakConnection.h"
#include "common/common.h"
#include <fmitcp/serialize.h>
#ifdef USE_GPL
#include "gsl-interface.h"
#endif

#ifdef DEBUG_MODEL_EXCHANGE
#include <unistd.h>
#include <sstream>
#endif

using namespace fmitcp::serialize;

namespace fmitcp_master {

//aka parallel stepper
class JacobiMaster : public BaseMaster {
public:
    JacobiMaster(vector<FMIClient*> clients, vector<WeakConnection> weakConnections) :
            BaseMaster(clients, weakConnections) {
        fprintf(stderr, "JacobiMaster\n");
    }

    void prepare() {
    }

    void runIteration(double t, double dt) {
        //get connection outputs
        for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
            it->first->sendGetX(it->second);
        }
        wait();

        //print values
        /*for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
            int i = 0;
            for (auto it2 = it->first->m_getRealValues.begin(); it2 != it->first->m_getRealValues.end(); it2++, i++) {
                fprintf(stderr, "%i real VR %i = %f\n", it->first->getId(), it->second[i], *it2);
            }
        }*/

        //set connection inputs, pipeline with do_step()
        const InputRefsValuesType refValues = getInputWeakRefsAndValues(m_weakConnections);

        // redirect outputs to inputs
        for (auto it = refValues.begin(); it != refValues.end(); it++) {
            it->first->sendSetX(it->second);
        }

        sendWait(m_clients, fmi2_import_do_step(t, dt, true));
    }
};

//aka serial stepper
class GaussSeidelMaster : public BaseMaster {
    map<FMIClient*, OutputRefsType> clientGetXs;  //one OutputRefsType for each client
    std::vector<int> stepOrder;
public:
    GaussSeidelMaster(vector<FMIClient*> clients, vector<WeakConnection> weakConnections, std::vector<int> stepOrder) :
        BaseMaster(clients, weakConnections), stepOrder(stepOrder) {
        fprintf(stderr, "GSMaster\n");
    }

    void prepare() {
        for (size_t x = 0; x < m_weakConnections.size(); x++) {
            WeakConnection wc = m_weakConnections[x];
            clientGetXs[wc.to][wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
        }
    }

    void runIteration(double t, double dt) {
        for (int o : stepOrder) {
            FMIClient *client = m_clients[o];

            for (auto it = clientGetXs[client].begin(); it != clientGetXs[client].end(); it++) {
                it->first->sendGetX(it->second);
            }
            wait();
            const SendSetXType refValues = getInputWeakRefsAndValues(m_weakConnections, client);
            client->sendSetX(refValues);
            sendWait(client, fmi2_import_do_step(t, dt, true));
        }
    }
};

class ModelExchangeStepper : public BaseMaster {
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

#ifdef DEBUG_MODEL_EXCHANGE
        FILE* result_file;
        long size_of_file;
#endif
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
    cgsl_simulation m_sim;
    fmu_parameters m_p;
    fmu_model m_model;
    TimeLoop timeLoop;
    bool restored = false;

    map<FMIClient*, OutputRefsType> clientGetXs;  //one OutputRefsType for each client
    std::vector<int> stepOrder;
 public:
 ModelExchangeStepper(std::vector<FMIClient*> clients, std::vector<WeakConnection> weakConnections) :
    BaseMaster(clients,weakConnections)
    {
      fprintf(stderr, "ModelExchangeStepper\n");
    }
    ~ModelExchangeStepper(){
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

        //p->baseMaster->get_storage().print(p->baseMaster->get_storage().get_current_states());
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

    /** allocate Memory
     *  Allocates memory needed by the fmu_model
     *
     *  @param m Pointer to a fmu_model
     */
    void allocateMemory(fmu_model &m, const std::vector<FMIClient*> &clients){
        m.model = (cgsl_model*)calloc(1,sizeof(cgsl_model));
        storage_alloc(clients);
        m.model->n_variables = get_storage().get_current_states().size();
        if(m.model->n_variables == 0){
            cerr << "ModelExchangeStepper nothing to integrate" << endl;
            exit(0);
        }

        m.model->x = (double*)calloc(m.model->n_variables, sizeof(double));
        m.model->x_backup = (double*)calloc(m.model->n_variables, sizeof(double));

        m_p.backup.dydt = (double*)calloc(m.model->n_variables, sizeof(double));
        //m_p.backup.result_file = (FILE*)calloc(1, sizeof(FILE));

        if(!m.model->x || !m_p.backup.dydt){
            //freeFMUModel(m);
            perror("WeakMaster:ModelExchange:allocateMemory ERROR -  could not allocate memory");
            exit(1);
        }
    }

    static void *get_model_parameters(const cgsl_model *m){
        return m->parameters;
    }

    /** init_fmu_model
     *  Creates the fmu_model and sets the parameters
     *
     *  @param client A pointer to a fmi client
     */
    void init_fmu_model(fmu_model &m,  const std::vector<FMIClient*> &clients){
        allocateMemory(m, clients);
        m.model->parameters = (void*)&m_p;
        m.model->get_model_parameters = get_model_parameters;
        fmu_parameters* p = get_p(m);

#ifdef DEBUG_MODEL_EXCHANGE
        ostringstream prefix;
        prefix << "/home/jonas/work/umit/data/resultFile.mat";
        if ( ( p->backup.result_file = fopen(prefix.str().c_str(), "w+") ) == NULL){
            cerr << "Could not open file " << prefix.str() << endl;
            exit(1);
        }
#endif
        p->t_ok = 0;
        p->t_past = 0;
        p->baseMaster = this;
        p->stateEvent = false;
        p->sim_started = false;
        //p.clients.resize(0,0);
        p->count = 0;

        p->backup.t = 0;
        p->backup.h = 0;
#ifdef DEBUG_MODEL_EXCHANGE
        p->backup.size_of_file = 0;
#endif
        p->clients = m_clients;

        m.model->function = fmu_function;
        m.model->jacobian = NULL;
        m.model->post_step = NULL;
        m.model->pre_step = NULL;
        m.model->free = cgsl_model_default_free;//freeFMUModel;

        for(auto client: clients)
            send(client, fmi2_import_get_continuous_states((int)client->getNumContinuousStates()));
        wait();

        for(auto client: clients)
            get_storage().get_current_states(m.model->x, client->getId());
    }

#define STATIC_GET_CLIENT_OFFSET(name)                                  \
   p->baseMaster->get_storage().get_offset(client->getId(), STORAGE::name)
#define STATIC_SET_(name, name2, data)                                       \
    p->baseMaster->send(client, fmi2_import_set_##name##_##name2(     \
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

    void prepare() {
        for (size_t x = 0; x < m_weakConnections.size(); x++) {
            WeakConnection wc = m_weakConnections[x];
            clientGetXs[wc.to][wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
        }
#ifdef USE_GPL
        /* This is the step control which determines tolerances. */

        // set up a gsl_simulation for each client
        init_fmu_model(m_model, m_clients);
        fmu_parameters* p = get_p(m_model);
        int filter_length = get_storage().get_current_states().size();
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
#ifdef DEBUG_MODEL_EXCHANGE
                                     1,     /* write to file: YES! */
                                     p->backup.result_file
#else
                                     0, NULL
#endif
                                     );
        // might not be needed
        get_storage().sync();
#endif
    }

    /** restoreStates
     *  restores all values needed by the simulations to restart
     *  from a known safe time.
     *
     *  @param sim The simulation
     */
    void restoreStates(cgsl_simulation &sim){
        get_storage().cycle();
        get_storage().sync();
        fmu_parameters* p = get_p(sim);
        //restore previous states

        for(auto client: m_clients)
            get_storage().get_current_states(sim.model->x,client->getId());

        memcpy(sim.i.evolution->dydt_out, p->backup.dydt,
               sim.model->n_variables * sizeof(p->backup.dydt[0]));

        sim.i.evolution->failed_steps = p->backup.failed_steps;
        sim.t = p->backup.t;
        sim.h = p->backup.h;

#ifdef DEBUG_MODEL_EXCHANGE
        // reset position in the result file
        fseek(p->backup.result_file, p->backup.size_of_file, SEEK_SET);

        // truncate the result file to have the previous result file size
        int trunc = ftruncate(fileno(p->backup.result_file), p->backup.size_of_file);
        if(0 > trunc )
            perror("storeStates: ftruncate");
#endif
    }

    /** storeStates
     *  stores all values needed by the simulations to restart
     *  from a known safe time.
     *
     *  @param sim The simulation
     */
    void storeStates(cgsl_simulation &sim){
        fmu_parameters* p = get_p(sim);
        for(auto client: m_clients)
            send(client, fmi2_import_get_continuous_states((int)client->getNumContinuousStates()));

        p->backup.failed_steps = sim.i.evolution->failed_steps;
        p->backup.t = sim.t;
        p->backup.h = sim.h;

        memcpy(p->backup.dydt, sim.i.evolution->dydt_out,
               sim.model->n_variables * sizeof(p->backup.dydt[0]));

#ifdef DEBUG_MODEL_EXCHANGE
        p->backup.size_of_file = ftell(p->backup.result_file);
#endif

        wait();
        for(auto client: m_clients)
            get_storage().get_current_states(sim.model->x,client->getId());
        get_storage().sync();
    }

    /** hasStateEvent:
     ** returns true if at least one simulation has an event
     *
     *  @param sim The simulation
     */
    bool hasStateEvent(cgsl_simulation &sim){
        return get_p(sim)->stateEvent;
    }

    /** getSafeTime:
     *  caluclates a "safe" time, uses the golden ratio to get
     *  t_crossed and t_safe to converge towards same value
     *
     *  @param sim A cgsl simulation
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

    /** step
     *  run cgsl_step_to on all simulations
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

    /** stepToEvent
     *  if there is an event, find the event and return
     *  the time at where the time event occured
     *
     *  @param sim The simulation
     *  @return Returns the time immediatly after the event
     */
    void stepToEvent(cgsl_simulation &sim){
        double tol = 1e-9;
        while(!(hasStateEvent(sim) &&
                (get_storage().absmin(STORAGE::indicators) < tol || timeLoop.dt_new < tol))){
            getGoldenNewTime(sim);
            step(sim);
            if(timeLoop.dt_new == 0) exit(23);
            if(hasStateEvent(sim)){
                // step back to where event occured
                restoreStates(sim);
                getSafeAndCrossed();
                timeLoop.dt_new = timeLoop.t_safe - sim.t;
                step(sim);
                if(!hasStateEvent(sim)) storeStates(sim);
                else{
                    cerr << "failed at stepping to safe timestep "<< sim.t << " " << timeLoop.t_safe << " " << timeLoop.t_crossed << endl;
                    restoreStates(sim);
                    step(sim);
                    restoreStates(sim);
                    step(sim);
                    cerr << "failed at stepping to safe timestep "<< sim.t << " " << timeLoop.t_safe << " " << timeLoop.t_crossed << endl;
                }
                timeLoop.dt_new = timeLoop.t_crossed - sim.t;
                step(sim);
                if(!hasStateEvent(sim)){
                    cerr << "failed at stepping to event " << endl;
                    exit(22);
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
    void newDiscreteStates(){
        // start at a new state
        sendWait(m_clients, fmi2_import_enter_event_mode());
        // todo loop until newDiscreteStatesNeeded == false
        bool newDiscreteStatesNeeded = true;

        while(newDiscreteStatesNeeded){
            newDiscreteStatesNeeded = false;
            sendWait(m_clients, fmi2_import_new_discrete_states());
            for(auto client: m_clients)
                    newDiscreteStatesNeeded += client->m_event_info.newDiscreteStatesNeeded;
        }

        sendWait(m_clients, fmi2_import_enter_continuous_time_mode());

        for(auto client: m_clients)
            send(client, fmi2_import_get_event_indicators((int)client->getNumEventIndicators()));

        wait();

        // store the current state of all running FMUs
        storeStates(m_sim);
    }

    void printStates(void){
      fprintf(stderr,"      states     ");
      get_storage().print(STORAGE::states);
      fprintf(stderr,"      indicator  ");
      get_storage().print(STORAGE::indicators);
    }
    void getSafeAndCrossed(){
        fmu_parameters *p = get_p(m_sim);
        timeLoop.t_safe    = p->t_ok;//max( timeLoop.t_safe,    t_ok);
        timeLoop.t_crossed = p->t_past;//min( timeLoop.t_crossed, t_past);
    }

    void safeTimeStep(cgsl_simulation &sim){
        // if sims has a state event do not step to far
        if(hasStateEvent(sim)){
            double absmin = get_storage().absmin(STORAGE::indicators);
            timeLoop.dt_new = sim.h * (absmin > 0 ? absmin:0.00001);
        }else
            timeLoop.dt_new = timeLoop.t_end - sim.t;
    }

    void getSafeTime(const std::vector<FMIClient*> clients, double &dt){
        for(auto client: clients)
            if(client->m_event_info.nextEventTimeDefined)
                dt = min(dt,client->m_event_info.nextEventTime);
    }

    void runIteration(double t, double dt) {
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
};
}// namespace fmitcp_master


#endif /* WEAKMASTERS_H_ */
