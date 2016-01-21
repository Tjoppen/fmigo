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
    sc::Solver m_strongCouplingSolver;
    map<FMIClient*, vector<int> > clientWeakRefs;

    void getDirectionalDerivative(FMIClient *client, sc::Equation *eq, void (sc::Equation::*getSeed)(sc::Vec3&), std::vector<int> accelerationRefs, std::vector<int> forceRefs);
    void getSpatialAngularDirectionalDerivatives(FMIClient *client, sc::Equation *eq, StrongConnector *sc, void (sc::Equation::*getSpatialSeed)(sc::Vec3&), void (sc::Equation::*getRotationalSeed)(sc::Vec3&));
public:
#ifdef USE_LACEWING
    StrongMaster(fmitcp::EventPump *pump, std::vector<FMIClient*> slaves, std::vector<WeakConnection*> weakConnections, sc::Solver strongCouplingSolver);
#else
    StrongMaster(std::vector<FMIClient*> slaves, std::vector<WeakConnection*> weakConnections, sc::Solver strongCouplingSolver);
#endif
    void prepare();
    void runIteration(double t, double dt);
};
}


#endif /* STRONGMASTER_H_ */
