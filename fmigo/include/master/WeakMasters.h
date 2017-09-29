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
#include "master/modelExchange.h"
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
class JacobiMaster : public model_exchange::ModelExchangeStepper {
public:
    JacobiMaster(zmq::context_t &context, vector<FMIClient*> clients, vector<WeakConnection> weakConnections) :
            model_exchange::ModelExchangeStepper(context, clients, weakConnections) {
        info("JacobiMaster\n");
    }

    void prepare() {
#ifdef USE_GPL
      prepareME();
#endif
    }

    void runIteration(double t, double dt) {
        clearGetValues();
        //get connection outputs
        for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
            it->first->sendGetX(it->second);
        }
        wait();

        //set connection inputs, pipeline with do_step()
        const InputRefsValuesType refValues = getInputWeakRefsAndValues(m_weakConnections);

        // redirect outputs to inputs
        for (auto it = refValues.begin(); it != refValues.end(); it++) {
            it->first->sendSetX(it->second);
        }

        //this pipelines with the sendGetX() + wait() in printOutputs() in main.cpp
        
        send(cs_clients, fmi2_import_do_step(t, dt, true));
#ifdef USE_GPL
        solveME(t,dt);
#endif
    }
};

//aka serial stepper
class GaussSeidelMaster : public model_exchange::ModelExchangeStepper {
    map<FMIClient*, OutputRefsType> clientGetXs;  //one OutputRefsType for each client
    std::vector<int> stepOrder;
public:
    GaussSeidelMaster(zmq::context_t &context, vector<FMIClient*> clients, vector<WeakConnection> weakConnections, std::vector<int> stepOrder) :
            model_exchange::ModelExchangeStepper(context, clients, weakConnections), stepOrder(stepOrder) {
        info("GSMaster\n");
    }

    void prepare() {
        for (size_t x = 0; x < m_weakConnections.size(); x++) {
            WeakConnection wc = m_weakConnections[x];
            clientGetXs[wc.to][wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
        }
#ifdef USE_GPL
        prepareME();
#endif
    }

    void runIteration(double t, double dt) {
        for (int o : stepOrder) {
            FMIClient *client = m_clients[o];

            clearGetValues();
            for (auto it = clientGetXs[client].begin(); it != clientGetXs[client].end(); it++) {
                it->first->sendGetX(it->second);
            }
            wait();
            const SendSetXType refValues = getInputWeakRefsAndValues(m_weakConnections, client);
            client->sendSetX(refValues);
            if (client->getFmuKind() == fmi2_fmu_kind_cs) {
                sendWait(client, fmi2_import_do_step(t, dt, true));
            } //else modelExchange, solve all further down simultaneously
        }
#ifdef USE_GPL
        solveME(t,dt);
#endif
    }
};

}// namespace fmitcp_master


#endif /* WEAKMASTERS_H_ */
