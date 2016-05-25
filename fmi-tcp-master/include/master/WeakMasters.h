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

        block(m_clients, fmi2_import_do_step(0, 0, t, dt, true));
    }
};

//aka serial stepper
class GaussSeidelMaster : public BaseMaster {
    vector<WeakConnection> m_weakConnections;
    OutputRefsType clientWeakRefs;
    std::vector<int> stepOrder;
public:
    GaussSeidelMaster(vector<FMIClient*> clients, vector<WeakConnection> weakConnections, std::vector<int> stepOrder) :
        BaseMaster(clients), m_weakConnections(weakConnections), stepOrder(stepOrder) {
        fprintf(stderr, "GSMaster\n");
    }

    void prepare() {
        clientWeakRefs = getOutputWeakRefs(m_weakConnections);
        //work out what clients and VRs each client has to grab from,
        //and where to put the retrieved values
        /*for (int o : stepOrder) {
            FMIClient *client = m_clients[o];

            for (size_t x = 0; x < m_weakConnections.size(); x++) {
                const WeakConnection& wc = m_weakConnections[x];
                if (wc.to == client) {
                    //connection has client as destination - remember it
                    conns[wc.to].vrMap[wc.from].push_back(wc.conn.fromOutputVR);
                    conns[wc.to].vrs.push_back(wc.conn.toInputVR);
                }
            }
        }*/
    }

    void runIteration(double t, double dt) {
        //fprintf(stderr, "\n\n");
        for (int o : stepOrder) {
            FMIClient *client = m_clients[o];

            client->sendGetX(clientWeakRefs[client]);
            wait();
            const SendSetXType refValues = getInputWeakRefsAndValues(m_weakConnections, client);
            client->sendSetX(refValues);
            block(client, fmi2_import_do_step(0, 0, t, dt, true));
        }
        //fprintf(stderr, "\n\n");
    }
};
}


#endif /* WEAKMASTERS_H_ */
