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
#include "common/gsl_interface.h"
using namespace fmitcp::serialize;

namespace fmitcp_master {

//aka parallel stepper
class JacobiMaster : public BaseMaster {
protected:
    vector<WeakConnection> m_weakConnections;
    OutputRefsType clientWeakRefs;

public:
    JacobiMaster(vector<FMIClient*> clients, vector<WeakConnection> weakConnections) :
            BaseMaster(clients), m_weakConnections(weakConnections) {
        fprintf(stderr, "JacobiMaster\n");
    }

    void prepare() {
        clientWeakRefs = getOutputWeakRefs(m_weakConnections);
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
    vector<WeakConnection> m_weakConnections;
    map<FMIClient*, OutputRefsType> clientGetXs;  //one OutputRefsType for each client
    std::vector<int> stepOrder;
public:
    GaussSeidelMaster(vector<FMIClient*> clients, vector<WeakConnection> weakConnections, std::vector<int> stepOrder) :
        BaseMaster(clients), m_weakConnections(weakConnections), stepOrder(stepOrder) {
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
    vector<WeakConnection> m_weakConnections;
    enum INTEGRATORTYPE m_integratorType;  
    double m_tolerance;
    struct fmu_model{
        int cgsl;
    };
    vector<cgsl_simulation> sims;
    
    map<FMIClient*, OutputRefsType> clientGetXs;  //one OutputRefsType for each client
    std::vector<int> stepOrder;
public:
 ModelExchangeStepper(vector<FMIClient*> clients, vector<WeakConnection> weakConnections, double relativeTolerance, enum INTEGRATORTYPE integratorType  ) :
    BaseMaster(clients), m_weakConnections(weakConnections), m_tolerance(relativeTolerance), m_integratorType(integratorType)
    {
        fprintf(stderr, "ModelExchangeStepper\n");
    }

    void prepare() {
        for (size_t x = 0; x < m_weakConnections.size(); x++) {
            WeakConnection wc = m_weakConnections[x];
            clientGetXs[wc.to][wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
        }

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


        cgsl_model* cgsl;
        cgsl_simulation sim = cgsl_init_simulation(cgsl,  /* model */
                                                   rk8pd, /* integrator: Runge-Kutta Prince Dormand pair order 7-8 */
                                                   1,     /* write to file: YES! */
                                                   NULL,  
                                                   step_control);
        sims.push_back(sim);
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
}// namespace fmitcp_master


#endif /* WEAKMASTERS_H_ */
