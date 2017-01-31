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
#include <FMI2/fmi2_types.h>
#include "common/gsl_interface.h"
//#include <sys/stat.h>
//#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <algorithm>

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

        // why not wait here?? wait()

        sendWait(m_clients, fmi2_import_do_step(0, 0, t, dt, true));

    }
};

//aka serial stepper
class GaussSeidelMaster : public BaseMaster {
    map<FMIClient*, OutputRefsType> clientGetXs;  //one OutputRefsType for each client
    std::vector<int> stepOrder;
public:
    GaussSeidelMaster(vector<FMIClient*> clients, vector<WeakConnection> weakConnections, std::vector<int> stepOrder) :
        BaseMaster(clients,weakConnections), stepOrder(stepOrder) {
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
            sendWait(client, fmi2_import_do_step(0, 0, t, dt, true));
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

        FILE* result_file;
        long size_of_file;

    } ;
    typedef struct fmu_parameters{

        double t_ok;
        double t_past;
        int count;                    /* number of function evaluations */
        int nz;                       /* number of event indicators, from XML file */
        int nx;                       /* number of states, from XML file */

        BaseMaster* baseMaster;       /* BaseMaster object pointer */
        FMIClient* client;            /* FMIClient object pointer */

        bool stateEvent;
        bool skip;
        Backup backup;
    }fmu_parameters;
    struct fmu_model{
      cgsl_model model;
    };
    vector<cgsl_simulation*> m_sims;
    vector<WeakConnection> m_weakConnections;
    enum INTEGRATORTYPE m_integratorType;
    double m_tolerance;
    TimeLoop timeLoop;

    map<FMIClient*, OutputRefsType> clientGetXs;  //one OutputRefsType for each client
    std::vector<int> stepOrder;
 public:
 ModelExchangeStepper(vector<FMIClient*> clients, vector<WeakConnection> weakConnections, double relativeTolerance, enum INTEGRATORTYPE integratorType  ) :
    BaseMaster(clients,weakConnections), m_tolerance(relativeTolerance), m_integratorType(integratorType)
    {
      fprintf(stderr, "ModelExchangeStepper\n");
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

        if(p->stateEvent)return GSL_SUCCESS;

        ++p->count; /* count function evaluations */
        p->baseMaster->send(p->client, fmi2_import_set_time(0,0,t));
        p->baseMaster->send(p->client, fmi2_import_set_continuous_states(0,0,x,p->nx));
        p->baseMaster->wait();
        p->baseMaster->sendWait(p->client, fmi2_import_get_derivatives(0,0,p->nx));
        p->baseMaster->get_storage().get_derivatives(dxdt, p->client->getId());

        if(p->nz){
            p->baseMaster->sendWait(p->client, fmi2_import_get_event_indicators(0,0,p->nz));
            if(p->baseMaster->get_storage().past_event(p->client->getId())){
                p->stateEvent = true;
                p->t_past = t;
                //p->debuggingvar = true;
                return GSL_CONTINUE;
            } else{
                p->stateEvent = false;
                p->t_ok = t;
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
        return (fmu_parameters*)(m->parameters);
    }
    inline fmu_parameters* get_p(fmu_model* m){
        return (fmu_parameters*)(m->model.parameters);
    }
    inline fmu_parameters* get_p(cgsl_simulation* s){
        return get_p(s->model);
    }

    /** freeFMUModel
     *  Free all memory allocated by allacateMemory(fmu_model* m)
     *
     *  @param m Pointer to a fmu_model
     */
    void freeFMUModel(fmu_model* m){
        if(m == NULL) return;

        fmu_parameters* p = get_p(m);
        if( p != NULL)             return;
        if( m->model.x != NULL)    free(m->model.x);
        if( p->backup.dydt != NULL)    free(p->backup.dydt);

        free(p);
        free(m);
    }
    void freeSim(void){
      fmu_parameters *p;
      while(m_sims.size()){
        freeFMUModel((fmu_model *)m_sims[0]->model);
        free(m_sims[0]);
        m_sims.pop_back();
      }
    }

    /** allocate Memory
     *  Allocates memory needed by the fmu_model
     *
     *  @param m Pointer to a fmu_model
     */
    void allocateMemory(fmu_model* m){
        bool allocFail = false;
        fmu_parameters* p = get_p(m);
        m->model.x = (double*)calloc(p->nx, sizeof(double));
        p->backup.dydt = (double*)calloc(p->nx, sizeof(double));

        if(!m->model.x){
            freeFMUModel(m);
            perror("WeakMaster:ModelExchange:allocateMemory ERROR -  could not allocate memory");
            exit(1);
        }
    }

    /** init_fmu_model
     *  Creates the fmu_model and sets the parameters
     *
     *  @param client A pointer to a fmi client
     */
    cgsl_model* init_fmu_model(FMIClient* client){
        fmu_parameters* p = (fmu_parameters*)malloc(sizeof(fmu_parameters));
        fmu_model* m = (fmu_model*)malloc(sizeof(fmu_model));

        m->model.parameters = (void*)p;
        p->client = client;
        // TODO one result file for each fmu

        ostringstream prefix;
        prefix << "data/resultFile" << client->getId() << ".mat";
        if ( ( p->backup.result_file = fopen(prefix.str().c_str(), "w+") ) == NULL){
            cout << "Could not open file " << prefix.str() << endl;
            exit(1);
        }
        p->baseMaster = this;
        p->nx = p->client->getNumContinuousStates();
        p->nz = p->client->getNumEventIndicators();
        m->model.n_variables = p->nx;

        allocateMemory(m);

        m->model.function = fmu_function;
        m->model.jacobian = NULL;
        m->model.free = NULL;//freeFMUModel;
        sendWait(p->client, fmi2_import_get_continuous_states(0,0,p->nx));
        get_storage().get_states(m->model.x, p->client->getId());

        return(cgsl_model*)m;
    }


    void prepare() {
#ifdef USE_GPL
        /* This is the step control which determines tolerances. */
        cgsl_step_control_parameters step_control;
        step_control.eps_rel = 1e-6;
        step_control.eps_abs = 1e-6;
        step_control.id = step_control_y_new;
        step_control.start = 1e-8;

        // allocate memory needed by messages
        fmu_alloc();

        // set up a gsl_simulation for each client
        for(FMIClient* client: m_clients) {
            cgsl_model* cgsl = init_fmu_model(client);
            fmu_parameters* p = get_p(cgsl);
            cgsl_simulation* sim = (cgsl_simulation *)malloc(sizeof(cgsl_simulation));
            *sim = cgsl_init_simulation(cgsl,  /* model */
                                        rk8pd, /* integrator: Runge-Kutta Prince Dormand pair order 7-8 */
                                        1,     /* write to file: YES! */
                                        p->backup.result_file,
                                        step_control);
            m_sims.push_back(sim);
        }

        // might not be needed
        get_storage().sync();
#endif
    }

    /** restoreStates
     *  restores all values needed by the simulations to restart
     *  from a known safe time.
     *
     *  @param sims Vector of all simulations
     */
    void restoreStates(std::vector<cgsl_simulation*> &sims){
        get_storage().cycle();
        get_storage().sync();
        for(auto sim : sims) {
            fmu_parameters* p = get_p(sim);
            //restore previous states
            get_storage().get_states(sim->model->x,p->client->getId());
            memcpy(sim->i.evolution->dydt_out, p->backup.dydt,
                   p->nx * sizeof(p->backup.dydt[0]));

            sim->i.evolution->failed_steps = p->backup.failed_steps;
            sim->t = p->backup.t;
            sim->h = p->backup.h;

            // reset position in the result file
            fseek(p->backup.result_file, p->backup.size_of_file, SEEK_SET);

            // truncate the result file to have the previous result file size
            int trunc = ftruncate(fileno(p->backup.result_file), p->backup.size_of_file);
            if(0 > trunc )
                perror("storeStates: ftruncate");
        }
    }

    /** storeStates
     *  stores all values needed by the simulations to restart
     *  from a known safe time.
     *
     *  @param sims Vector of all simulations
     */
    void storeStates(std::vector<cgsl_simulation*> &sims){
        for(auto sim : sims){
            fmu_parameters* p = get_p(sim);
            send(p->client, fmi2_import_get_continuous_states(0,0,p->nx));
            p->backup.failed_steps = sim->i.evolution->failed_steps;
            p->backup.t = sim->t;
            p->backup.h = sim->h;
            memcpy(p->backup.dydt, sim->i.evolution->dydt_out, p->nx * sizeof(p->backup.dydt[0]));
            p->backup.size_of_file = ftell(p->backup.result_file);
        }
        wait();
        for(auto sim : sims){
            fmu_parameters* p = get_p(sim);
            get_storage().get_states(sim->model->x,p->client->getId());
        }
        get_storage().sync();
    }

    /** getStateEvent: Tries to retrieve event indicators, if successful the signbit of all
     *  event indicators z are compaired with event indicators in p->z
     *
     *  @param sims Vector of all simulations
     */
    void getStateEvent(std::vector<cgsl_simulation*> &sims){
        fmu_parameters *p;
        for(auto sim: m_sims){
            p = get_p(sim);
            p->stateEvent = get_storage().past_event(p->client->getId());
        }
    }

    /** hasStateEvent:
     ** returns true if at least one simulation has an event
     *
     *  @param sims Vector of all simulations
     */
    bool hasStateEvent(std::vector<cgsl_simulation*> &sims){
      fmu_parameters* p;
      for(auto sim:sims){
          if(get_p(sim)->stateEvent)
              return true;
      }
      return false;
    }

    /** getSafeTime:
     *  caluclates a "safe" time, uses the golden ratio to get
     *  t_crossed and t_safe to converge towards same value
     *
     *  @param sim A cgsl simulation
     */
    void getGoldenNewTime(std::vector<cgsl_simulation*> &sims){
        // golden ratio
        double phi = (1 + sqrt(5)) / 2;
        /* passed solution, need to reduce tEnd */
        if(hasStateEvent(sims)){
            getSafeAndCrossed();
            restoreStates(sims);
            timeLoop.dt_new = (timeLoop.t_crossed - sims[0]->t) / phi;
        } else { // havent passed solution, increase step
            storeStates(sims);
            timeLoop.t_safe = max(timeLoop.t_safe, sims[0]->t);
            timeLoop.dt_new = timeLoop.t_crossed - sims[0]->t - (timeLoop.t_crossed - timeLoop.t_safe) / phi;
        }
    }

    /** step
     *  run cgsl_step_to on all simulations
     *
     *  @param sims Vector of all simulations
     */
    void step(std::vector<cgsl_simulation*> &sims){
        fmu_parameters *p;
        for(auto sim: sims){
            p = get_p(sim);
            p->stateEvent = false;
            p->t_past = max(p->t_past, sim->t + timeLoop.dt_new);
            p->t_ok = sim->t;
            cgsl_step_to(sim, sim->t, sim->t + timeLoop.dt_new);
        }
    }

    /** reduceSims
     *  Forget about all simulations that will not reach the event
     *  during current time step.
     *
     *  @param sims Vector of all simulations
     */
    void reduceSims(std::vector<cgsl_simulation*> &sims){
        if(hasStateEvent(sims)){
            fmu_parameters* p;
            for(auto sim:sims){
                p = get_p(sim);
                p->skip = !p->stateEvent;
            }
        }else
            for(auto sim:sims)
                get_p(sim)->skip = false;
    }

    /** stepToEvent
     *  if there is an event, find the event and return
     *  the time at where the time event occured
     *
     *  @param sims Vector of all simulations
     *  @return Returns the time immediatly after the event
     */
    void stepToEvent(std::vector<cgsl_simulation*> &sims){
        double tol = 1e-9;
        while(!(hasStateEvent(sims) &&
                (get_storage().absmin(get_storage().get_indicators()) < tol || timeLoop.dt_new < tol))){
            getGoldenNewTime(sims);
            step(sims);
            if(timeLoop.dt_new == 0) exit(23);
            if(hasStateEvent(sims)){
                // step back to where event occured
                restoreStates(sims);
                getSafeAndCrossed();
                timeLoop.dt_new = timeLoop.t_safe - sims[0]->t;
                step(sims);
                if(!hasStateEvent(sims)) storeStates(sims);
                else cout << "failed at stepping to safe timestep " << endl;

                timeLoop.dt_new = timeLoop.t_crossed - sims[0]->t;
                step(sims);
                if(!hasStateEvent(sims)){
                    cout << "failed at stepping to event " << endl;
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
        sendWait(m_clients, fmi2_import_enter_event_mode(0,0));
        // todo loop until newDiscreteStatesNeeded == false
        sendWait(m_clients, fmi2_import_new_discrete_states(0,0));
        sendWait(m_clients, fmi2_import_enter_continuous_time_mode(0,0));
        fmu_parameters *p;
        for(auto sim: m_sims){
            p = get_p(sim);
            send(p->client, fmi2_import_get_event_indicators(0,0,p->nz));
        }
        wait();

        // store the current state of all running FMUs
        storeStates(m_sims);
    }

    void printStates(void){
      fprintf(stderr,"      states     ");
      get_storage().print(get_storage().get_states());
      fprintf(stderr,"      indicator  ");
      get_storage().print(get_storage().get_indicators());
      fprintf(stderr,"      bindicator ");
      get_storage().print(get_storage().get_backup_indicators());
    }
    void getSafeAndCrossed(){
        fmu_parameters *p = get_p(m_sims[0]->model);
        double t_ok = p->t_ok;
        double t_past = p->t_past;
        for(auto sim: m_sims){
            p = get_p(sim);
            t_ok   = min( p->t_ok,    t_ok);
            t_past = min( p->t_past, t_past);
        }
        timeLoop.t_safe    = t_ok;//max( timeLoop.t_safe,    t_ok);
        timeLoop.t_crossed = t_past;//min( timeLoop.t_crossed, t_past);
    }

    double safeTimeStep(std::vector<cgsl_simulation*> &sims){
        double t = sims[0]->t;
        double h = sims[0]->h;
        for(auto sim: sims) h = min(h,sim->h);

        double absmin = get_storage().absmin(get_storage().get_indicators());

        // if sims has a state event do not step to far
        if(hasStateEvent(sims)){
            timeLoop.dt_new = h * (absmin > 0 ? absmin:0.00001);
        }else
            timeLoop.dt_new = timeLoop.t_end - t;
    }

    void runIteration(double t, double dt) {
        timeLoop.t_end = t + dt;

        newDiscreteStates();

        while( timeLoop.t_safe < timeLoop.t_end ){
            if (hasStateEvent(m_sims)){
                getSafeAndCrossed();

                // restore and step to before the event
                restoreStates(m_sims);
                timeLoop.dt_new = timeLoop.t_safe - m_sims[0]->t;
                step(m_sims);

                // store and step to the event
                if(!hasStateEvent(m_sims)) storeStates(m_sims);
                timeLoop.dt_new = timeLoop.t_crossed - m_sims[0]->t;
                step(m_sims);

                // step closer to the event location
                if(hasStateEvent(m_sims))
                    stepToEvent(m_sims);

            }else{
                timeLoop.t_safe = m_sims[0]->t;
                timeLoop.t_crossed = timeLoop.t_end;
            }

            safeTimeStep(m_sims);
            if(hasStateEvent(m_sims))
               newDiscreteStates();
            storeStates(m_sims);
            step(m_sims);
        }
    }
};
}// namespace fmitcp_master


#endif /* WEAKMASTERS_H_ */
