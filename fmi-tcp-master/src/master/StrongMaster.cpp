/*
 * StrongMaster.cpp
 *
 *  Created on: Aug 12, 2014
 *      Author: thardin
 */

#include "master/StrongMaster.h"
#include "master/FMIClient.h"
#include <fmitcp/serialize.h>

using namespace fmitcp_master;
using namespace fmitcp;
using namespace fmitcp::serialize;
using namespace sc;

#ifdef USE_LACEWING
StrongMaster::StrongMaster(EventPump *pump, vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections, Solver strongCouplingSolver) :
        JacobiMaster(pump, slaves, weakConnections),
#else
StrongMaster::StrongMaster(vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections, Solver strongCouplingSolver) :
        JacobiMaster(slaves, weakConnections),
#endif
        m_strongCouplingSolver(strongCouplingSolver) {
    fprintf(stderr, "StrongMaster\n");
}


void StrongMaster::getDirectionalDerivative(FMIClient *client, Equation *eq, void (Equation::*getSeed)(Vec3&), vector<int> accelerationRefs, vector<int> forceRefs) {
    Vec3 seedVec;
    (eq->*getSeed)(seedVec);

    vector<double> seed;
    seed.push_back(seedVec.x());
    seed.push_back(seedVec.y());
    seed.push_back(seedVec.z());

    send(client, fmi2_import_get_directional_derivative(0, 0, accelerationRefs, forceRefs, seed));
}

void StrongMaster::getSpatialAngularDirectionalDerivatives(FMIClient *client, Equation *eq, StrongConnector *sc, void (Equation::*getSpatialSeed)(Vec3&), void (Equation::*getRotationalSeed)(Vec3&)) {
    if (sc->hasAcceleration() && sc->hasForce()) {
        getDirectionalDerivative(client, eq, getSpatialSeed, sc->getAccelerationValueRefs(), sc->getForceValueRefs());
    } else {
        fprintf(stderr, "Strong coupling requires acceleration outputs for now\n");
        exit(1);
    }

    if (sc->hasAngularAcceleration() && sc->hasTorque()) {
        getDirectionalDerivative(client, eq, getRotationalSeed, sc->getAngularAccelerationValueRefs(), sc->getTorqueValueRefs());
    } else {
        fprintf(stderr, "Strong coupling requires angular acceleration outputs for now\n");
        exit(1);
    }
}

void StrongMaster::runIteration(double t, double dt) {
    //get weak connector outputs
    const map<FMIClient*, vector<int> > clientWeakRefs = getOutputWeakRefs(m_weakConnections);

    for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
        send(it->first, fmi2_import_get_real(0, 0, it->second));
    }
    wait();

    //disentangle received values for set_real() further down (before do_step())
    //we shouldn't set_real() for these until we've gotten directional derivatives
    //this sets StrongMaster apart from the weak masters
    const map<FMIClient*, pair<vector<int>, vector<double> > > refValues = getInputWeakRefsAndValues(m_weakConnections);

    //get strong connector inputs
    //TODO: it'd be nice if these get_real() were pipelined with the get_real()s done above
    for(int i=0; i<m_slaves.size(); i++){
        //check m_getDirectionalDerivativeValues while we're at it
        if (m_slaves[i]->m_getDirectionalDerivativeValues.size() > 0) {
            fprintf(stderr, "WARNING: Client %i had %zu unprocessed directional derivative results\n", i,
                    m_slaves[i]->m_getDirectionalDerivativeValues.size());
            m_slaves[i]->m_getDirectionalDerivativeValues.clear();
        }

        const vector<int> valueRefs = m_slaves[i]->getStrongConnectorValueReferences();
        send(m_slaves[i], fmi2_import_get_real(0, 0, valueRefs));
    }
    wait();

    //set connector values
    for (int i=0; i<m_slaves.size(); i++){
        FMIClient *client = m_slaves[i];
        vector<int> vrs = client->getStrongConnectorValueReferences();
        /*fprintf(stderr, "m_getRealValues:\n");
        for (int j = 0; j < client->m_getRealValues.size(); j++) {
            fprintf(stderr, "VR %i = %f\n", vrs[j], client->m_getRealValues[j]);
        }*/

        client->setConnectorValues(vrs, client->m_getRealValues);
    }

    //update constraints since connector values changed
    m_strongCouplingSolver.updateConstraints();

    //get directional derivatives
    vector<Equation*> eqs;
    m_strongCouplingSolver.getEquations(&eqs);

    for (size_t j = 0; j < eqs.size(); ++j){
        Equation * eq = eqs[j];
        StrongConnector *scA = (StrongConnector*)eq->getConnA();
        StrongConnector *scB = (StrongConnector*)eq->getConnB();
        FMIClient * slaveA = (FMIClient *)scA->m_userData;
        FMIClient * slaveB = (FMIClient *)scB->m_userData;

#ifdef ENABLE_DEMO_HACKS
        if (eqs.size() != 1) {
            fprintf(stderr, "Only one equation supported in demo mode\n");
            exit(1);
        }
        Vec3 seedA; eq->getRotationalJacobianSeedA(seedA);
        Vec3 seedB; eq->getRotationalJacobianSeedB(seedB);

        //inverse moments of inertia
        double J1inv = 1/1000.0;
        double J2inv = 1/1200.0;

        seedA = seedA * J1inv;
        seedB = seedB * J2inv;

        vector<double> a;
        vector<double> b;
        a.push_back(seedA.x()); a.push_back(seedA.y()); a.push_back(seedA.z());
        b.push_back(seedB.x()); b.push_back(seedB.y()); b.push_back(seedB.z());

        slaveA->m_getDirectionalDerivativeValues.push_back(a);
        slaveB->m_getDirectionalDerivativeValues.push_back(b);
#else
        getSpatialAngularDirectionalDerivatives(slaveA, eq, scA, &Equation::getSpatialJacobianSeedA, &Equation::getRotationalJacobianSeedA);
        getSpatialAngularDirectionalDerivatives(slaveB, eq, scB, &Equation::getSpatialJacobianSeedB, &Equation::getRotationalJacobianSeedB);
#endif
    }
    wait();

    for (size_t j = 0; j < eqs.size(); ++j){
        Equation * eq = eqs[j];
        StrongConnector *scA = (StrongConnector*)eq->getConnA();
        StrongConnector *scB = (StrongConnector*)eq->getConnB();
        FMIClient * slaveA = (FMIClient *)scA->m_userData;
        FMIClient * slaveB = (FMIClient *)scB->m_userData;

#ifdef ENABLE_DEMO_HACKS
        if (slaveA->m_getDirectionalDerivativeValues.size() == 1 &&
            slaveB->m_getDirectionalDerivativeValues.size() == 1) {

            eq->setSpatialJacobianA(0,0,0);
            eq->setRotationalJacobianA(slaveA->m_getDirectionalDerivativeValues[0][0], slaveA->m_getDirectionalDerivativeValues[0][1], slaveA->m_getDirectionalDerivativeValues[0][2]);
            eq->setSpatialJacobianB(0,0,0);
            eq->setRotationalJacobianB(slaveB->m_getDirectionalDerivativeValues[0][0], slaveB->m_getDirectionalDerivativeValues[0][1], slaveB->m_getDirectionalDerivativeValues[0][2]);

            //dump directional derivatives
            printf(",%f,%f,%f,%f,%f,%f",
                    slaveA->m_getDirectionalDerivativeValues[0][0], slaveA->m_getDirectionalDerivativeValues[0][1], slaveA->m_getDirectionalDerivativeValues[0][2],
                    slaveB->m_getDirectionalDerivativeValues[0][0], slaveB->m_getDirectionalDerivativeValues[0][1], slaveB->m_getDirectionalDerivativeValues[0][2]);

            slaveA->m_getDirectionalDerivativeValues.pop_front();
            slaveB->m_getDirectionalDerivativeValues.pop_front();
        } else
#endif
        if (slaveA->m_getDirectionalDerivativeValues.size() < 2 ||
            slaveB->m_getDirectionalDerivativeValues.size() < 2) {
            fprintf(stderr, "Not enough results: %zu %zu\n", slaveA->m_getDirectionalDerivativeValues.size(),
                                                             slaveB->m_getDirectionalDerivativeValues.size());
            exit(1);
        } else {
        eq->setSpatialJacobianA(    slaveA->m_getDirectionalDerivativeValues[0][0],
                                    slaveA->m_getDirectionalDerivativeValues[0][1],
                                    slaveA->m_getDirectionalDerivativeValues[0][2]);
        eq->setRotationalJacobianA( slaveA->m_getDirectionalDerivativeValues[1][0],
                                    slaveA->m_getDirectionalDerivativeValues[1][1],
                                    slaveA->m_getDirectionalDerivativeValues[1][2]);
        eq->setSpatialJacobianB(    slaveB->m_getDirectionalDerivativeValues[0][0],
                                    slaveB->m_getDirectionalDerivativeValues[0][1],
                                    slaveB->m_getDirectionalDerivativeValues[0][2]);
        eq->setRotationalJacobianB( slaveB->m_getDirectionalDerivativeValues[1][0],
                                    slaveB->m_getDirectionalDerivativeValues[1][1],
                                    slaveB->m_getDirectionalDerivativeValues[1][2]);

        //dump directional derivatives
        for (int x = 0; x < 2; x++) {
            for (int y = 0; y < 3; y++) {
                printf(",%f", slaveA->m_getDirectionalDerivativeValues[x][y]);
            }
        }
        for (int x = 0; x < 2; x++) {
            for (int y = 0; y < 3; y++) {
                printf(",%f", slaveB->m_getDirectionalDerivativeValues[x][y]);
            }
        }

        for (int x = 0; x < 2; x++) {
            slaveA->m_getDirectionalDerivativeValues.pop_front();
            slaveB->m_getDirectionalDerivativeValues.pop_front();
        }
        }
    }

    //TODO: figure out if future velocities need to be set when we have proper directional derivatives..

    //compute strong coupling forces
    m_strongCouplingSolver.solve();

    //distribute forces
    for (int i=0; i<m_slaves.size(); i++){
        FMIClient *client = m_slaves[i];
        for (int j = 0; j < client->numConnectors(); j++) {
            StrongConnector *sc = client->getConnector(j);
            vector<double> vec;

            //dump force/torque
            if (sc->hasForce()) {
                printf(",%f,%f,%f", sc->m_force.x(), sc->m_force.y(), sc->m_force.z());
                vec.push_back(sc->m_force.x());
                vec.push_back(sc->m_force.y());
                vec.push_back(sc->m_force.z());
            }

            if (sc->hasTorque()) {
#ifdef ENABLE_DEMO_HACKS
                printf(",%f", sc->m_torque.y());
                vec.push_back(sc->m_torque.y());
#else
                printf(",%f,%f,%f", sc->m_torque.x(),sc->m_torque.y(),sc->m_torque.z());
                vec.push_back(sc->m_torque.x());
                vec.push_back(sc->m_torque.y());
                vec.push_back(sc->m_torque.z());
#endif
            }

            vector<int> fvrs = sc->getForceValueRefs();
            vector<int> tvrs = sc->getTorqueValueRefs();
            fvrs.insert(fvrs.end(), tvrs.begin(), tvrs.end());

            send(client, fmi2_import_set_real(0, 0, fvrs, vec));
        }
    }

    //set weak connector inputs
    for (auto it = refValues.begin(); it != refValues.end(); it++) {
        send(it->first, fmi2_import_set_real(0, 0, it->second.first, it->second.second));
    }

    //do actual step
#ifdef ENABLE_DEMO_HACKS
    //TODO: always do newStep=true? Keep it a demo hack for now
    block(m_slaves, fmi2_import_do_step(0, 0, t, dt, true));
#else
    block(m_slaves, fmi2_import_do_step(0, 0, t, dt, false));
#endif
}
