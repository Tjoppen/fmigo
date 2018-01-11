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
        //get connection outputs
        //if we're lucky they we already have up-to-date values
        //in that case queueX(), queueValueRequests() and wait() all become no-ops
        for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
            it->first->queueX(it->second);
        }
        queueValueRequests();
        wait();

        //set connection inputs, pipeline with do_step()
        const InputRefsValuesType refValues = getInputWeakRefsAndValues(m_weakConnections);

        // redirect outputs to inputs
        for (auto it = refValues.begin(); it != refValues.end(); it++) {
            it->first->sendSetX(it->second);
        }

        //this pipelines with the sendGetX() + wait() in printOutputs() in main.cpp
        
        queueMessage(cs_clients, fmi2_import_do_step(t, dt, true));
        deleteCachedValues();
#ifdef USE_GPL
        solveME(t,dt);
#endif

        //pre-request values
        //this causes the CSV printing to fetch output so we have them ready when re-entering this function
        for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
            it->first->queueX(it->second);
        }
    }
};

}// namespace fmitcp_master


#endif /* WEAKMASTERS_H_ */
