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
    fmitcp::int_set kins;

    //rend counters
    //think of these like TTLs - when one reaches zero then
    //that rend's children are added to the open set
    std::vector<int> counters;

    //for keeping track of what outputs from which clients each client will want values from
    std::map<int, OutputRefsType> clientGetXs;

    void getDirectionalDerivative(fmitcp_proto::fmi2_kinematic_req& kin, const sc::Vec3& seedVec, const std::vector<int>& accelerationRefs, const std::vector<int>& forceRefs);

    //crank the system until the open set contains target
    //if target is empty then the system is cranked until all FMUs have been executed
    fmitcp::int_set done;
    fmitcp::int_set open;
    fmitcp::int_set todo;
    void crankIt(double t, double dt, const fmitcp::int_set& target);

    //this moves the IDs in the cranked set from open to done,
    //and figures out if any rends were triggered
    //if so those rends children are moved from todo to open
    //pass cranked by value since it is called with open in runIteration
    void moveCranked(const fmitcp::int_set& cranked);

    //steps kinematic FMUs
    //all kinematic FMUs must be in the open set before calling this function
    void stepKinematicFmus(double t, double dt);

    //computed forces, for writeFields()
    std::vector<double> forces;
    int getNumForces() const;

    //for avoiding allocations in getInputWeakRefsAndValues()
    InputRefsValuesType m_refValues;

    //subset of m_weakConnections which are just real -> real without scaling
    //like -c foo,x,bar,y
    struct simpleconnection {
        int fromOutputVR;
        //to save on dereferencing "from" FMIClient*
        std::unordered_map<int, double> *fromRealsPtr;
    };
    std::unordered_map<FMIClient*, std::vector<simpleconnection> > m_simpleConnections;
    std::unordered_map<FMIClient*, std::vector<int> > m_simpleInputsVRs;

    //all other connections
    std::vector<WeakConnection> m_complexConnections;

    //resets m_refValues and fills with values via m_simpleConnections
    void initRefValues(const fmitcp::int_set& cset);
public:
    StrongMaster(zmq::context_t &context, std::vector<FMIClient*> slaves, std::vector<WeakConnection> weakConnections,
                 sc::Solver *strongCouplingSolver, bool holonomic, const std::vector<Rend>& rends);
    ~StrongMaster();
    void prepare();
    void runIteration(double t, double dt);

    //StrongMaster adds some extra columns to the CSV output, this returns the names of those columns
    //the returned strings begins with a space
    std::string getFieldNames() const;

    virtual void writeFields(bool last, FILE *outfile) ;
};
}


#endif /* STRONGMASTER_H_ */
