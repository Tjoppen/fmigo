/*
 * StrongMaster.h
 *
 *  Created on: Aug 12, 2014
 *      Author: thardin
 */

#ifndef STRONGMASTER_H_
#define STRONGMASTER_H_

#include "WeakMasters.h"
#include "sc/Solver.h"

namespace fmitcp_master {
class WeakConnection;
class StrongMaster : public JacobiMaster {
    sc::Solver *m_strongCouplingSolver;
    bool holonomic;

    void getDirectionalDerivative(fmitcp_proto::fmi2_kinematic_req& kin, const sc::Vec3& seedVec, const std::vector<int>& accelerationRefs, const std::vector<int>& forceRefs);
public:
    StrongMaster(zmq::context_t &context, std::vector<FMIClient*> slaves, std::vector<WeakConnection> weakConnections, sc::Solver *strongCouplingSolver, bool holonomic);
    ~StrongMaster();
    void prepare();
    void runIteration(double t, double dt);

    //StrongMaster adds some extra columns to the CSV output, this returns the names of those columns
    //the returned strings begins with a space
    std::string getForceFieldnames() const;

    int getNumForceOutputs() const;
};
}


#endif /* STRONGMASTER_H_ */
