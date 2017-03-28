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
using namespace model_exchange;

namespace fmitcp_master {

//aka parallel stepper
class JacobiMaster : public BaseMaster {
 private:
    ModelExchangeStepper* m_modelexchange;
public:
    JacobiMaster(vector<FMIClient*> clients, vector<WeakConnection> weakConnections) :
            BaseMaster(clients, weakConnections) {
        fprintf(stderr, "JacobiMaster\n");
    }

    void prepare() {
        std::vector<WeakConnection> me_weakConnections;
        std::vector<FMIClient*>     me_clients;
        for(auto client: m_clients)
            if(client->getFmuKind() == fmi2_fmu_kind_me)
                me_clients.push_back(client);
        //TODO make sure that solve loops for model exchange only uses me_weakConnections
        for(auto wc: m_weakConnections)
            if(wc.from->getFmuKind() == fmi2_fmu_kind_me &&
               wc.to->getFmuKind()   == fmi2_fmu_kind_me )
                me_weakConnections.push_back(wc);
        m_modelexchange = new ModelExchangeStepper(me_clients,me_weakConnections,this);
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

        for( auto client: m_clients){
            switch (client->getFmuKind()){
                case fmi2_fmu_kind_cs:
                    send(client, fmi2_import_do_step(t, dt, true));
                    break;
                case fmi2_fmu_kind_me:
                    m_modelexchange->runIteration(t,dt);
                    break;
                default:
                    fprintf(stderr,"Fatal: fmigo only supports co-simulation and model exchange fmus\n");
                    exit(1);
                }
        }
        wait();
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

}// namespace fmitcp_master


#endif /* WEAKMASTERS_H_ */
