/*
 * WeakMasters.h
 *
 *  Created on: Aug 12, 2014
 *      Author: thardin
 */

#ifndef WEAKMASTERS_H_
#define WEAKMASTERS_H_
#define USE_GSL
#include "master/BaseMaster.h"
#include "master/WeakConnection.h"
#include "common/common.h"
#include <fmitcp/serialize.h>
#include "common/fmi2.h"
#include "common/gsl_interface.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

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

        for (auto it = refValues.begin(); it != refValues.end(); it++) {
            it->first->sendSetX(it->second);
        }

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
        //fprintf(stderr, "\n\n");
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
        //fprintf(stderr, "\n\n");
    }
};
 
//aka serial stepper
class ModelExchangeStepper : public BaseMaster {
    typedef struct TimeLoop
    {
        fmi2Real t_start, t_safe, t_new, t_crossed, t_end;
    } TimeLoop;
    typedef struct backup
    {
        double* x; 
        double t;
        unsigned long failed_steps;

        FILE* result_file;
        long size_of_file;

    } backup;
    typedef struct fmu_parameters{

        double* z;                    /* state event indicators */
        int count;                    /* number of function evaluations */
        int nz;                       /* number of event indicators, from XML file */
        int nx;                       /* number of states, from XML file */
        int loggingOn;                /* loggingOn == true => logging is enabled */
        char separator;               /* not used */
        char* resultFile;             /* path and file name of where to store result */
        fmi2String* categories;       /* only used in setDebugLogging */
        int nCategories;              /* only used in setDebugLogging */

        BaseMaster* baseMaster;
        FMIClient* client;
      //        fmi2Component c;

        fmi2Real t_start, t_safe, t_new, t_crossed, t_end;
        fmi2EventInfo event;

        bool stateEvent;
        backup m_backup;
    }fmu_parameters;
    struct fmu_model{
      cgsl_model model;
    };
    vector<cgsl_simulation> m_sims;
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


    char* getModelResultPath(const char* path)
    {
        size_t n = strlen(path);
        char* p = (char*)malloc(n * sizeof(char));

        // copy path to p, tp for destruction only.
        void* tp = memcpy(p, path, n);

        // pointer to be
        char* name = p;

        // find file name of path 'path'
        while((p = strchr(p, '/')) != NULL)
            name = ++p;

        // remove file extension
        if((p = strchr(name, '.')) != NULL)
            *p = '\0';

        // allocate memory for resultFile
        size_t s = strlen(name);
        char* resultFile = (char*)calloc(s + 10, sizeof(char));

        // create the resultFile
        sprintf(resultFile, "data/");

        // if there is no directory, create it
        mkdir(resultFile, 0700);

        // add path and resultFile
        sprintf(resultFile, "%s%s.mat", resultFile, name);

        // free copy of path
        free(tp);
        return resultFile;
    }

    std::vector<double> doubleToVector(const double* v, int n){
      std::vector<double> vector;
      for(int i = 0; i < n; i++)
        vector.push_back(v[i]);
      return vector;
    }
    
    static int fmu_function(double t, const double x[], double dxdt[], void* params)
    {
        // make local variables
        fmu_parameters* p = (fmu_parameters*)params;
        //        fmi2Component c = p->c; // instance of the fmu
        int nx = p->nx;
        /*should make sure we are in continuous state */

        ++p->count; /* count function evaluations */
        p->baseMaster->sendWait(p->client, fmi2_import_set_time(0,0,t));
        p->baseMaster->sendWait(p->client, fmi2_import_set_continuous_states(0,0,x,nx));
        p->baseMaster->sendWait(p->client, fmi2_import_get_derivatives(0,0,nx));

        // maybe just send all three and then wait?
        return GSL_SUCCESS;
    }

    fmu_parameters* getParameters(cgsl_model* m){
        return (fmu_parameters*)(m->parameters);
    }
    fmu_parameters* getParameters(fmu_model* m){
        return (fmu_parameters*)(m->model.parameters);
    }
    void freeFMUModel(fmu_model*m){
        if(m == NULL) return;

        fmu_parameters* p = getParameters(m);
        if(p != NULL)             return;
        if(p->z != NULL)          free(p->z);
        if(p->resultFile != NULL) free(p->resultFile);
        if(m->model.x != NULL)    free(m->model.x);

        free(p);
        free(m);
    }

    void allocateMemory(fmu_model* m){
        fmu_parameters* p = getParameters(m);
        int nx = p->client->getNumContinuousStates();
        int nz = p->client->getNumEventIndicators();
        p->nx = nx;
        p->nz = nz;
        p->m_backup.x = (double*)calloc(nx, sizeof(double));
        m->model.x    = (double*)calloc(nx, sizeof(double));
        if(nz > 0) p->z = (double*)calloc(nz, sizeof(double));
        else p->z = NULL;
  
        if((!m->model.x ) || (nz > 0 && !p->z)) {
            freeFMUModel(m);
            perror("WeakMaster:ModelExchange:allocateMemory ERROR -  could not allocate memory");
            exit(1);
        }
    }

    cgsl_model* init_fmu_model(FMIClient* client){
      fmu_parameters* p = (fmu_parameters*)malloc(sizeof(fmu_parameters));
      fmu_model* m = (fmu_model*)malloc(sizeof(fmu_model));

      m->model.parameters = (void*)p;
      p->client = client;
      // TODO one result file for each fmu
      p->resultFile = getModelResultPath("resultFile");

      p->baseMaster = this;
      // done in main     sendWait(m_clients, fmi2_import_instantiate(0));
      size_t dummy = client->getNumEventIndicators(); 
      allocateMemory(m);

      // done in main     initialization

      m->model.function = fmu_function;
      m->model.jacobian = NULL;
      // m->model.free = NULL;//freeFMUModel; 
    }
    void prepare() {
        for (size_t x = 0; x < m_weakConnections.size(); x++) {
            WeakConnection wc = m_weakConnections[x];
            clientGetXs[wc.to][wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
        }
#ifdef USE_GSL
        /* This is the step control which determines tolerances. */
        cgsl_step_control_parameters step_control;
        /* = (cgsl_step_control_parameters){ */
        /*     .eps_rel = 1e-6,          /\* relative tolerance *\/ */
        /*     .eps_abs = 1e-6,          /\* absolute tolerance *\/ */
        /*     .id = step_control_y_new, /\* step control strategy *\/ */
        /*     .start = 1e-8             /\* first step *\/ */
        /* }; */
        step_control.eps_rel = 1e-6;
        step_control.eps_abs = 1e-6;
        step_control.id = step_control_y_new;
        step_control.start = 1e-8;

        for(FMIClient* client: m_clients)
          {
            cgsl_model* cgsl = init_fmu_model(client);
            cgsl_simulation sim = cgsl_init_simulation(cgsl,  /* model */
                                                       rk8pd, /* integrator: Runge-Kutta Prince Dormand pair order 7-8 */
                                                       1,     /* write to file: YES! */
                                                       NULL,  
                                                       step_control);
            m_sims.push_back(sim);
          }
#endif
    }

    void storeStates(std::vector<cgsl_simulation> *sims){
#ifdef USE_GSL
      for(auto sim : *sims) storeStates(&sim);
#endif
    }
    void storeStates(cgsl_simulation* sim){
#ifdef USE_GSL
        fmu_parameters* p = getParameters(sim->model); 
        memcpy(sim->model->x, p->m_backup.x, p->nx * sizeof(sim->model->x[0]));
        sim->i.evolution->failed_steps = p->m_backup.failed_steps;
        sim->t = p->m_backup.t;

        // reset position in the result file
        fseek(p->m_backup.result_file, p->m_backup.size_of_file, SEEK_SET);

        // truncate the result file to have the previous result file size
        ftruncate(fileno(p->m_backup.result_file), p->m_backup.size_of_file);

#endif
    }
    void restoreStates(std::vector<cgsl_simulation> *sims){
#ifdef USE_GSL
      for(auto sim : *sims) restoreStates(&sim);
#endif
    }
    void restoreStates(cgsl_simulation *sim){
#ifdef USE_GSL
        fmu_parameters* p = getParameters(sim->model); 
        p->baseMaster->sendWait(p->client, fmi2_import_get_continuous_states(0,0,p->nx));
            
        memcpy(p->m_backup.x, sim->model->x, p->nx * sizeof(sim->model->x[0]));

        // if statement not needed if timestep is choosen more carefully
        if(p->t_start == sim->t && p->nz > 0){
            p->baseMaster->sendWait(p->client, fmi2_import_get_event_indicators(0,0, p->nz));
        }
        p->m_backup.failed_steps = sim->i.evolution->failed_steps;
        p->m_backup.t = sim->t;
        p->m_backup.size_of_file = ftell(p->m_backup.result_file);

#endif
    }


    /** resetIntegratorTimeVariables: simple function to restore values of t_ */
    void resetIntegratorTimeVariables(double t_new)
    {
        timeLoop.t_safe = timeLoop.t_start;
        timeLoop.t_crossed = timeLoop.t_end;
        timeLoop.t_new = t_new;//min(sim->h, max(max((sim->t - prevTimeEvent)/500,0),t_end));
    }


    /* getStateEvent: Tries to retrieve event indicators, if successful the signbit of all
    * event indicators z are compaired with event indicators in p->z */
    fmi2Status getStateEvent(std::vector<cgsl_simulation> sims){
      for(auto sim: sims) getStateEvent(sim);
    }
    fmi2Status getStateEvent(cgsl_simulation sim)
    {
        fmu_parameters* p = getParameters(sim.model);
        p->stateEvent = 0;
        if(p->nz > 0){ 
            double z[p->nz];
            if(p->nz > 0){
                p->baseMaster->sendWait(p->client, fmi2_import_get_event_indicators(0,0,p->nz));
            }
            /* compare signbit of previous state and the current
            * return at first difference */
            /* int i; */
            /* for(i = 0; i < p->nz; i++) */
            /* if(signbit(z[i]) != signbit(p->z[i])) { */
            /*     p->event.stateEvent = 1; */
            /*     return ; */
            /* } */
        }
    }

    double getSafeTime(cgsl_simulation sim, TimeLoop* timeLoop){

        // golden ratio
        double phi = (1 + sqrt(5)) / 2;

        fmu_parameters* p = getParameters(sim.model);
        /* passed solution, need to reduce tEnd */
        if(p->stateEvent) {
            timeLoop->t_crossed = timeLoop->t_new;
            timeLoop->t_new = timeLoop->t_safe + (timeLoop->t_crossed - timeLoop->t_safe) / phi;

        } else { // havent passed solution, increase step
            timeLoop->t_safe = timeLoop->t_new;
            timeLoop->t_new = timeLoop->t_crossed - (timeLoop->t_crossed - timeLoop->t_safe) / phi;
        }

        return timeLoop->t_new;
    }
    bool hasStateEvent(std::vector<cgsl_simulation> sims){
      fmu_parameters* p;
      for(auto sim:sims){
        p = getParameters(sim.model);
        if(p->stateEvent)
          return true;
      }
      return false;

    }

    void step(std::vector<cgsl_simulation> *sims, TimeLoop* timeLoop){
        for(auto sim: *sims)
            cgsl_step_to(&sim, sim.t, getSafeTime(sim, timeLoop)); // stepTo(sim,p));
    }

    void reduceSims(std::vector<cgsl_simulation> *sims){
        for(int it = 0; it < sims.size();it++){
          p = getParameters(sims[it].model);
          if(!p->stateEvent)
            sims.erase(sims.begin() + it--);
        }
    }
    
    double findEventTime(std::vector<cgsl_simulation> sims, TimeLoop* timeLoop){
        restoreStates(&sims);
        double tol = 1e-6;
        fmu_parameters* p;

        //remove all simulations that doesn't have an event
        reduceSims(&sims);
          
        while(timeLoop->t_crossed - timeLoop->t_safe > tol) {
            step(&sims, timeLoop); // stepTo(sim,p));
            
            getStateEvent(sims);
            if(hasStateEvent(sims))
                restoreStates(&sims);
                reduceSims(&sims);
            else
                storeStates(&sims);
        }
        return timeLoop->t_crossed;
    }

    void runIteration(double t, double dt) {
        timeLoop.t_start = t;
        timeLoop.t_end = t + dt;
        
        // set start values for the time integration
        resetIntegratorTimeVariables(timeLoop.t_end);

        // start at a new state
        sendWait(m_clients, fmi2_import_new_discrete_states(0,0));

        // store the current state of all running FMUs
        storeStates(&m_sims);

        while( timeLoop.t_safe < timeLoop.t_end ){
            step(&m_sims, &timeLoop); // stepTo(sim,p));
            
            if( getStateEvent(m_sims) ){
                timeLoop.t_new = findEventTime(m_sims, &timeLoop);
                step(&m_sims, &timeLoop); // stepTo(sim,p));
            }
            
        }
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
        //fprintf(stderr, "\n\n");
    }
};
}// namespace fmitcp_master


#endif /* WEAKMASTERS_H_ */
