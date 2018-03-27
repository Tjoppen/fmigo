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

    //first rend is root
    //last rend is terminal
    const std::vector<Rend> rends;

    //maps client -> rend
    std::vector<int> clientrend;

    //kinematic FMU IDs
    std::set<int> kins;

    //rend counters
    //think of these like TTLs - when one reaches zero then
    //that rend's children are added to the open set
    std::vector<int> counters;

    //for keeping track of what outputs from which clients each client will want values from
    std::map<int, OutputRefsType> clientGetXs;

    void getDirectionalDerivative(fmitcp_proto::fmi2_kinematic_req& kin, const sc::Vec3& seedVec, const std::vector<int>& accelerationRefs, const std::vector<int>& forceRefs);

    //crank the system until the open set contains target
    //if target is empty then the system is cranked until all FMUs have been executed
    std::set<int> done;
    std::set<int> open;
    std::set<int> todo;
    void crankIt(double t, double dt, const std::set<int>& target);

    //this moves the IDs in the cranked set from open to done,
    //and figures out if any rends were triggered
    //if so those rends children are moved from todo to open
    //pass cranked by value since it is called with open in runIteration
    void moveCranked(std::set<int> cranked);

    //steps kinematic FMUs
    //all kinematic FMUs must be in the open set before calling this function
    void stepKinematicFmus(double t, double dt);

    //computed forces, for writeFields()
    std::vector<double> forces;
    int getNumForces() const;
public:
    StrongMaster(zmq::context_t &context, std::vector<FMIClient*> slaves, std::vector<WeakConnection> weakConnections,
                 sc::Solver *strongCouplingSolver, bool holonomic, const std::vector<Rend>& rends);
    ~StrongMaster();
    void prepare();
    void runIteration(double t, double dt);

    //StrongMaster adds some extra columns to the CSV output, this returns the names of those columns
    //the returned strings begins with a space
    std::string getFieldNames() const;

    virtual void writeFields(bool last) ;
};
}


#endif /* STRONGMASTER_H_ */