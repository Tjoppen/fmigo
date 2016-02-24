/*
 * StrongMaster.cpp
 *
 *  Created on: Aug 12, 2014
 *      Author: thardin
 */

#include "master/StrongMaster.h"
#include "master/FMIClient.h"
#include <fmitcp/serialize.h>
#include "common/common.h"

using namespace fmitcp_master;
using namespace fmitcp;
using namespace fmitcp::serialize;
using namespace sc;

StrongMaster::StrongMaster(vector<FMIClient*> slaves, vector<WeakConnection*> weakConnections, Solver strongCouplingSolver) :
        JacobiMaster(slaves, weakConnections),
        m_strongCouplingSolver(strongCouplingSolver) {
    fprintf(stderr, "StrongMaster\n");
}

void StrongMaster::prepare() {
    m_strongCouplingSolver.prepare();
    clientWeakRefs = getOutputWeakRefs(m_weakConnections);
}

void StrongMaster::getDirectionalDerivative(FMIClient *client, Equation *eq, void (Equation::*getSeed)(Vec3&), vector<int> accelerationRefs, vector<int> forceRefs) {
    Vec3 seedVec;
    (eq->*getSeed)(seedVec);

    vector<double> seed;
    seed.push_back(seedVec.x());

    if (accelerationRefs.size() == 1) {
        //HACKHACK: special handling for 1-dimensional couplings (shafts)
        accelerationRefs.resize(1);
        forceRefs.resize(1);
    } else {
        seed.push_back(seedVec.y());
        seed.push_back(seedVec.z());
    }

    block(client, fmi2_import_get_directional_derivative(0, 0, accelerationRefs, forceRefs, seed));
}

void StrongMaster::getSpatialAngularDirectionalDerivatives(FMIClient *client, Equation *eq, StrongConnector *sc, void (Equation::*getSpatialSeed)(Vec3&), void (Equation::*getRotationalSeed)(Vec3&)) {
    if (eq->m_isSpatial) {
    if (sc->hasAcceleration() && sc->hasForce()) {
        getDirectionalDerivative(client, eq, getSpatialSeed, sc->getAccelerationValueRefs(), sc->getForceValueRefs());
    } else {
        fprintf(stderr, "Strong coupling requires acceleration outputs for now\n");
        exit(1);
    }
    }

    if (eq->m_isRotational) {
    if (sc->hasAngularAcceleration() && sc->hasTorque()) {
        getDirectionalDerivative(client, eq, getRotationalSeed, sc->getAngularAccelerationValueRefs(), sc->getTorqueValueRefs());
    } else {
        fprintf(stderr, "Strong coupling requires angular acceleration outputs for now\n");
        exit(1);
    }
    }
}

void StrongMaster::runIteration(double t, double dt) {
    //get weak connector outputs
    for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
        send(it->first, fmi2_import_get_real(0, 0, it->second));
    }
    PRINT_HDF5_DELTA("get_weak_reals");
    wait();
    PRINT_HDF5_DELTA("get_weak_reals_wait");

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
            exit(1);
            m_slaves[i]->m_getDirectionalDerivativeValues.clear();
        }

        const vector<int> valueRefs = m_slaves[i]->getStrongConnectorValueReferences();
        send(m_slaves[i], fmi2_import_get_real(0, 0, valueRefs));
    }
    PRINT_HDF5_DELTA("get_strong_reals");
    wait();
    PRINT_HDF5_DELTA("get_strong_reals_wait");

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

    //get future velocities:
    //0. save FMU states
    //1. zero forces
    //2. step
    //3. get velocity
    //4. restore FMU states
    send(m_slaves, fmi2_import_get_fmu_state(0, 0));

    //zero forces
    //if we don't do this then the forces would explode
    for (int i=0; i<m_slaves.size(); i++){
        FMIClient *client = m_slaves[i];
        for (int j = 0; j < client->numConnectors(); j++) {
            StrongConnector *sc = client->getConnector(j);
            vector<int> fvrs = sc->getForceValueRefs();
            vector<int> tvrs = sc->getTorqueValueRefs();
            fvrs.insert(fvrs.end(), tvrs.begin(), tvrs.end());

            vector<double> vec(fvrs.size(), 0.0);

            send(client, fmi2_import_set_real(0, 0, fvrs, vec));
        }
    }

    send(m_slaves, fmi2_import_do_step(0, 0, t, dt, false));

    //do about the same thing we did a little bit further up, but store the results in future values
    for(int i=0; i<m_slaves.size(); i++){
        const vector<int> valueRefs = m_slaves[i]->getStrongConnectorValueReferences();
        send(m_slaves[i], fmi2_import_get_real(0, 0, valueRefs));
    }

    PRINT_HDF5_DELTA("get_future_values");
    wait();
    PRINT_HDF5_DELTA("get_future_values_wait");

    //set FUTURE connector values (velocities only)
    for (int i=0; i<m_slaves.size(); i++){
        FMIClient *client = m_slaves[i];
        vector<int> vrs = client->getStrongConnectorValueReferences();
        client->setConnectorFutureVelocities(vrs, client->m_getRealValues);
    }

    //restore
    for (int i=0; i<m_slaves.size(); i++){
        FMIClient *client = m_slaves[i];
        send(client, fmi2_import_set_fmu_state(0, 0, client->m_stateId));
        send(client, fmi2_import_free_fmu_state(0, 0, client->m_stateId));
    }

    //get directional derivatives
    vector<Equation*> eqs;
    m_strongCouplingSolver.getEquations(&eqs);

    for (size_t j = 0; j < eqs.size(); ++j){
        Equation * eq = eqs[j];
        StrongConnector *scA = (StrongConnector*)eq->getConnA();
        StrongConnector *scB = (StrongConnector*)eq->getConnB();
        FMIClient * slaveA = (FMIClient *)scA->m_userData;
        FMIClient * slaveB = (FMIClient *)scB->m_userData;

        getSpatialAngularDirectionalDerivatives(slaveA, eq, scA, &Equation::getSpatialJacobianSeedA, &Equation::getRotationalJacobianSeedA);
        getSpatialAngularDirectionalDerivatives(slaveB, eq, scB, &Equation::getSpatialJacobianSeedB, &Equation::getRotationalJacobianSeedB);
    }
    PRINT_HDF5_DELTA("get_directional_derivs");
    wait();
    PRINT_HDF5_DELTA("get_directional_derivs_wait");

    for (size_t j = 0; j < eqs.size(); ++j){
        Equation * eq = eqs[j];
        StrongConnector *scA = (StrongConnector*)eq->getConnA();
        StrongConnector *scB = (StrongConnector*)eq->getConnB();
        FMIClient * slaveA = (FMIClient *)scA->m_userData;
        FMIClient * slaveB = (FMIClient *)scB->m_userData;

        if (eq->m_isSpatial) {
            if (scA->getAccelerationValueRefs().size() == 1) {
                eq->setSpatialJacobianA(    slaveA->m_getDirectionalDerivativeValues.front()[0], 0, 0);
                eq->setSpatialJacobianB(    slaveB->m_getDirectionalDerivativeValues.front()[0], 0, 0);
            } else {
                eq->setSpatialJacobianA(    slaveA->m_getDirectionalDerivativeValues.front()[0],
                                            slaveA->m_getDirectionalDerivativeValues.front()[1],
                                            slaveA->m_getDirectionalDerivativeValues.front()[2]);
                eq->setSpatialJacobianB(    slaveB->m_getDirectionalDerivativeValues.front()[0],
                                            slaveB->m_getDirectionalDerivativeValues.front()[1],
                                            slaveB->m_getDirectionalDerivativeValues.front()[2]);
            }
            slaveA->m_getDirectionalDerivativeValues.pop_front();
            slaveB->m_getDirectionalDerivativeValues.pop_front();
        } else {
            eq->setSpatialJacobianA(0,0,0);
            eq->setSpatialJacobianB(0,0,0);
        }

        if (eq->m_isRotational) {
            //1-D?
            if (scA->getAngularAccelerationValueRefs().size() == 1) {
                eq->setRotationalJacobianA( slaveA->m_getDirectionalDerivativeValues.front()[0], 0, 0);
                eq->setRotationalJacobianB( slaveB->m_getDirectionalDerivativeValues.front()[0], 0, 0);
            } else {
                eq->setRotationalJacobianA( slaveA->m_getDirectionalDerivativeValues.front()[0],
                                            slaveA->m_getDirectionalDerivativeValues.front()[1],
                                            slaveA->m_getDirectionalDerivativeValues.front()[2]);
                eq->setRotationalJacobianB( slaveB->m_getDirectionalDerivativeValues.front()[0],
                                            slaveB->m_getDirectionalDerivativeValues.front()[1],
                                            slaveB->m_getDirectionalDerivativeValues.front()[2]);
            }
            slaveA->m_getDirectionalDerivativeValues.pop_front();
            slaveB->m_getDirectionalDerivativeValues.pop_front();
        } else {
            eq->setRotationalJacobianA(0,0,0);
            eq->setRotationalJacobianB(0,0,0);
        }
    }
    PRINT_HDF5_DELTA("distribute_directional_derivs");

    //TODO: figure out if future velocities need to be set when we have proper directional derivatives..

    //compute strong coupling forces
    m_strongCouplingSolver.solve();
    PRINT_HDF5_DELTA("run_solver");

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
                printf(",%f,%f,%f", sc->m_torque.x(),sc->m_torque.y(),sc->m_torque.z());
                vec.push_back(sc->m_torque.x());
                vec.push_back(sc->m_torque.y());
                vec.push_back(sc->m_torque.z());
            }

            vector<int> fvrs = sc->getForceValueRefs();
            vector<int> tvrs = sc->getTorqueValueRefs();
            fvrs.insert(fvrs.end(), tvrs.begin(), tvrs.end());

            send(client, fmi2_import_set_real(0, 0, fvrs, vec));
        }
    }
    PRINT_HDF5_DELTA("send_strong_forces");

    //set weak connector inputs
    for (auto it = refValues.begin(); it != refValues.end(); it++) {
        send(it->first, fmi2_import_set_real(0, 0, it->second.first, it->second.second));
    }
    PRINT_HDF5_DELTA("send_weak_reals");

    //do actual step
#ifdef ENABLE_DEMO_HACKS
    //TODO: always do newStep=true? Keep it a demo hack for now
    block(m_slaves, fmi2_import_do_step(0, 0, t, dt, true));
#else
    block(m_slaves, fmi2_import_do_step(0, 0, t, dt, false));
#endif
    PRINT_HDF5_DELTA("do_step");
}
