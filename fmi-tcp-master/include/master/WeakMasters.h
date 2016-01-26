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
class WeakMaster : public BaseMaster {
protected:
    vector<WeakConnection*> m_weakConnections;
public:
    WeakMaster(vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections) : BaseMaster(slaves),
            m_weakConnections(weakConnections) {
    }
};

//aka parallel stepper
class JacobiMaster : public WeakMaster {
    map<FMIClient*, vector<int> > clientWeakRefs;

public:
    JacobiMaster(vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections) : WeakMaster(slaves, weakConnections) {
        fprintf(stderr, "JacobiMaster\n");
    }

    void prepare() {
        clientWeakRefs = getOutputWeakRefs(m_weakConnections);
    }

    void runIteration(double t, double dt) {
        //get connection outputs
        for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
            send(it->first, fmi2_import_get_real(0, 0, it->second));
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
        const map<FMIClient*, pair<vector<int>, vector<double> > > refValues = getInputWeakRefsAndValues(m_weakConnections);

        for (auto it = refValues.begin(); it != refValues.end(); it++) {
            send(it->first, fmi2_import_set_real(0, 0, it->second.first, it->second.second));
        }

#ifdef ENABLE_DEMO_HACKS
        //AgX requires newStep=true
        block(m_slaves, fmi2_import_do_step(0, 0, t, dt, true));
#else
        block(m_slaves, fmi2_import_do_step(0, 0, t, dt, false));
#endif
    }
};

//aka serial stepper
class GaussSeidelMaster : public WeakMaster {
    std::vector<int> stepOrder;
    struct ClientThingy {
        std::map<FMIClient*, std::vector<int> > vrMap;
        std::vector<int> vrs;
    };
    std::map<FMIClient*, ClientThingy> conns;
public:
    GaussSeidelMaster(vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections, std::vector<int> stepOrder) :
        WeakMaster(slaves, weakConnections), stepOrder(stepOrder) {
        fprintf(stderr, "GSMaster\n");
    }

    void prepare() {
        //work out what clients and VRs each client has to grab from,
        //and where to put the retrieved values
        for (int o : stepOrder) {
            FMIClient *client = m_slaves[o];

            for (size_t x = 0; x < m_weakConnections.size(); x++) {
                WeakConnection *wc = m_weakConnections[x];
                if (wc->getSlaveB() == client) {
                    //connection has client as destination - remember it
                    conns[wc->getSlaveB()].vrMap[wc->getSlaveA()].push_back(wc->getValueRefA());
                    conns[wc->getSlaveB()].vrs.push_back(wc->getValueRefB());
                }
            }
        }
    }

    void runIteration(double t, double dt) {
        //fprintf(stderr, "\n\n");
        for (int o : stepOrder) {
            FMIClient *client = m_slaves[o];

            //fprintf(stderr, "client %i:", client->getId());
            if (conns[client].vrs.size() > 0) {
                for (auto it : conns[client].vrMap) {
                    /*fprintf(stderr, " %i(", it.first->getId());
                    for (int vr : it.second) {
                        fprintf(stderr, "%i ", vr);
                    }
                    fprintf(stderr, "\b)");*/
                    send(it.first, fmi2_import_get_real(0, 0, it.second));
                }

                /*fprintf(stderr, " ->");
                for (int vr : conns[client].vrs) {
                    fprintf(stderr, " %i", vr);
                }
                fprintf(stderr, "\n");*/

                wait();

                std::vector<double> values;
                for (auto it : conns[client].vrMap) {
                    values.insert(values.end(), it.first->m_getRealValues.begin(), it.first->m_getRealValues.end());
                }

                send(client, fmi2_import_set_real(0, 0, conns[client].vrs, values));
            }

#ifdef ENABLE_DEMO_HACKS
            //AgX requires newStep=true
            block(client, &FMIClient::fmi2_import_do_step, 0, 0, t, dt, true);
#else
            block(client, fmi2_import_do_step(0, 0, t, dt, false));
#endif
        }
        //fprintf(stderr, "\n\n");
    }
};
}


#endif /* WEAKMASTERS_H_ */
