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
#ifdef USE_LACEWING
    WeakMaster(fmitcp::EventPump *pump, vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections) : BaseMaster(pump, slaves),
#else
    WeakMaster(vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections) : BaseMaster(slaves),
#endif
            m_weakConnections(weakConnections) {
    }
};

//aka parallel stepper
class JacobiMaster : public WeakMaster {
public:
#ifdef USE_LACEWING
    JacobiMaster(fmitcp::EventPump *pump, vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections) : WeakMaster(pump, slaves, weakConnections) {
#else
    JacobiMaster(vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections) : WeakMaster(slaves, weakConnections) {
#endif
        fprintf(stderr, "JacobiMaster\n");
    }

    void transferWeakValues() {
        //get connection outputs
        const map<FMIClient*, vector<int> > clientWeakRefs = getOutputWeakRefs(m_weakConnections);

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
    }

    void runIteration(double t, double dt) {
        transferWeakValues();

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
public:
#ifdef USE_LACEWING
    GaussSeidelMaster(fmitcp::EventPump *pump, vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections) : WeakMaster(pump, slaves, weakConnections) {
#else
    GaussSeidelMaster(vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections) : WeakMaster(slaves, weakConnections) {
#endif
        fprintf(stderr, "GSMaster\n");
    }

    void runIteration(double t, double dt) {
        map<FMIClient*, vector<int> > clientWeakRefs = getOutputWeakRefs(m_weakConnections);

        for (auto it = m_slaves.begin(); it != m_slaves.end(); it++) {
            if (clientWeakRefs[*it].size() > 0) {
            //get connection outputs
            block(*it, fmi2_import_get_real(0, 0, clientWeakRefs[*it]));

            //set connection inputs, pipeline with do_step()
            const pair<vector<int>, vector<double> > refValues = getInputWeakRefsAndValues(m_weakConnections)[*it];

            /*int i = 0;
            for (auto it2 = refValues.second.begin(); it2 != refValues.second.end(); it2++, i++) {
                fprintf(stderr, "%i real VR %i = %f\n", (*it)->getId(), refValues.first[i], *it2);
            }*/

            send(*it, fmi2_import_set_real(0, 0, refValues.first, refValues.second));
            }

#ifdef ENABLE_DEMO_HACKS
            //AgX requires newStep=true
            block(*it, &FMIClient::fmi2_import_do_step, 0, 0, t, dt, true);
#else
            block(*it, fmi2_import_do_step(0, 0, t, dt, false));
#endif
        }
    }
};
}


#endif /* WEAKMASTERS_H_ */
