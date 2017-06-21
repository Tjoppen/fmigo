/*
 * StrongMaster.cpp
 *
 *  Created on: Aug 12, 2014
 *      Author: thardin
 */

#include "master/StrongMaster.h"
#include "master/FMIClient.h"
#include <fmitcp/serialize.h>
#include <sstream>
#include "common/common.h"
#include "master/globals.h"

using namespace fmitcp_master;
using namespace fmitcp;
using namespace fmitcp::serialize;
using namespace sc;

// HACKHACK

StrongMaster::~StrongMaster(){
}

StrongMaster::StrongMaster(zmq::context_t &context, vector<FMIClient*> clients, vector<WeakConnection> weakConnections, Solver strongCouplingSolver, bool holonomic) :
        JacobiMaster(context, clients, weakConnections),
        m_strongCouplingSolver(strongCouplingSolver), holonomic(holonomic) {
    info("StrongMaster (%s)\n", holonomic ? "holonomic" : "non-holonomic");
}

void StrongMaster::prepare() {
    m_strongCouplingSolver.prepare();
    JacobiMaster::prepare();
}

void StrongMaster::getDirectionalDerivative(FMIClient *client, Vec3 seedVec, vector<int> accelerationRefs, vector<int> forceRefs) {
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

    sendWait(client, fmi2_import_get_directional_derivative(accelerationRefs, forceRefs, seed));
}

void StrongMaster::runIteration(double t, double dt) {
  //get weak connector outputs
    for (auto it = clientWeakRefs.begin(); it != clientWeakRefs.end(); it++) {
        it->first->sendGetX(it->second);
    }
    PRINT_HDF5_DELTA("get_weak_values");
    wait();
    PRINT_HDF5_DELTA("get_weak_values_wait");

    //disentangle received values for set_real() further down (before do_step())
    //we shouldn't set_real() for these until we've gotten directional derivatives
    //this sets StrongMaster apart from the weak masters
    const InputRefsValuesType refValues = getInputWeakRefsAndValues(m_weakConnections);

    //set weak connector inputs
    for (auto it = refValues.begin(); it != refValues.end(); it++) {
        it->first->sendSetX(it->second);
    }
    PRINT_HDF5_DELTA("send_weak_values");

    //get strong connector inputs
    //TODO: it'd be nice if these get_real() were pipelined with the get_real()s done above
    for(size_t i=0; i<m_clients.size(); i++){
        //check m_getDirectionalDerivativeValues while we're at it
        if (m_clients[i]->m_getDirectionalDerivativeValues.size() > 0) {
            fatal("Client %zu had %zu unprocessed directional derivative results\n", i,
                    m_clients[i]->m_getDirectionalDerivativeValues.size());
        }

        const vector<int> valueRefs = m_clients[i]->getStrongConnectorValueReferences();
        send(m_clients[i], fmi2_import_get_real(valueRefs));
    }
    PRINT_HDF5_DELTA("get_strong_reals");
    wait();
    PRINT_HDF5_DELTA("get_strong_reals_wait");

    //set connector values
    for (size_t i=0; i<m_clients.size(); i++){
        FMIClient *client = m_clients[i];
        vector<int> vrs = client->getStrongConnectorValueReferences();
        /*debug("m_getRealValues:\n");
        for (int j = 0; j < client->m_getRealValues.size(); j++) {
             debug("VR %i = %f\n", vrs[j], client->m_getRealValues[j]);
        }*/

        client->setConnectorValues(vrs, vector<double>(client->m_getRealValues.begin(), client->m_getRealValues.end()));
    }

    //update constraints since connector values changed
    m_strongCouplingSolver.updateConstraints();

    //get future velocities:
    //0. save FMU states
    //1. zero forces
    //2. step
    //3. get velocity
    //4. restore FMU states

    //first filter out FMUs with save/load functionality
    std::vector<FMIClient*> saveLoadClients;
    for (size_t i=0; i<m_clients.size(); i++){
        FMIClient *client = m_clients[i];
        if (client->hasCapability(fmi2_cs_canGetAndSetFMUstate)) {
            saveLoadClients.push_back(client);
        }
    }
    send(saveLoadClients, fmi2_import_get_fmu_state());

    //zero forces
    //if we don't do this then the forces would explode
    for (size_t i=0; i<saveLoadClients.size(); i++){
        FMIClient *client = saveLoadClients[i];
        for (int j = 0; j < client->numConnectors(); j++) {
            StrongConnector *sc = client->getConnector(j);
            vector<int> fvrs = sc->getForceValueRefs();
            vector<int> tvrs = sc->getTorqueValueRefs();
            fvrs.insert(fvrs.end(), tvrs.begin(), tvrs.end());

            vector<double> vec(fvrs.size(), 0.0);

            send(client, fmi2_import_set_real(fvrs, vec));
        }
    }

    //noSetFMUStatePriorToCurrentPoint = false
    //This signals to the FMU that we might restore it to a state prior to currentCommunicationPoint=t
    //In other words: do the step, but don't commit the results
    send(saveLoadClients, fmi2_import_do_step(t, dt, false));

#if 0 
    //restore
    for (size_t i=0; i<saveLoadClients.size(); i++){
        FMIClient *client = saveLoadClients[i];
        send(client, fmi2_import_set_fmu_state(client->m_stateId));
    }

    send(saveLoadClients, fmi2_import_do_step(t, dt, false));
#endif

    //do about the same thing we did a little bit further up, but store the results in future values
    for(size_t i=0; i<saveLoadClients.size(); i++){
        const vector<int> valueRefs = saveLoadClients[i]->getStrongConnectorValueReferences();
        send(saveLoadClients[i], fmi2_import_get_real(valueRefs));
    }

    PRINT_HDF5_DELTA("get_future_values");
    wait();
    PRINT_HDF5_DELTA("get_future_values_wait");

    //set FUTURE connector values (velocities only)
    for (size_t i=0; i<saveLoadClients.size(); i++){
        FMIClient *client = saveLoadClients[i];
        vector<int> vrs = client->getStrongConnectorValueReferences();
        client->setConnectorFutureVelocities(vrs, vector<double>(client->m_getRealValues.begin(), client->m_getRealValues.end()));
    }

    //restore
    for (size_t i=0; i<saveLoadClients.size(); i++){
        FMIClient *client = saveLoadClients[i];
        send(client, fmi2_import_set_fmu_state(client->m_stateId));
        send(client, fmi2_import_free_fmu_state(client->m_stateId));
    }

    //get directional derivatives
    //this is a two-step process which is important to get the order of correct
    for (int step = 0; step < 2; step++) {
        for (sc::Equation *eq : m_strongCouplingSolver.getEquations()) {
            for (sc::Connector *fc : eq->getConnectors()) {
                StrongConnector *forceConnector = dynamic_cast<StrongConnector*>(fc);
                FMIClient *client = dynamic_cast<FMIClient*>(forceConnector->m_slave);
                for (int x = 0; x < client->numConnectors(); x++) {
                    StrongConnector *accelerationConnector = dynamic_cast<StrongConnector*>(forceConnector->m_slave->getConnector(x));

                        //HACKHACK: use the presence of shaft angle VR to distinguish connector type
                        if (accelerationConnector->hasShaftAngle() != forceConnector->hasShaftAngle()) {
                            //the reason this is a problem is because we can't always get the positional mobilities for rotational constraints and vice versa
                            //in theory we can though, by just putting zeroes in the relevant places
                            //it's just very hairy, so i'm not doing it right now
                            fatal("Can't deal with different types of kinematic connections to the same FMU\n");
                        }

                        if (step == 0) {
                            //step 0 = send fmi2_import_get_directional_derivative() requests
                            if (eq->m_isSpatial) {
                                if (accelerationConnector->hasAcceleration() && forceConnector->hasForce()) {
                                    getDirectionalDerivative(client, eq->jacobianElementForConnector(forceConnector).getSpatial(), accelerationConnector->getAccelerationValueRefs(), forceConnector->getForceValueRefs());
                                } else {
                                    fatal("Strong coupling requires acceleration outputs for now\n");
                                }
                            }

                            if (eq->m_isRotational) {
                                if (accelerationConnector->hasAngularAcceleration() && forceConnector->hasTorque()) {
                                    getDirectionalDerivative(client, eq->jacobianElementForConnector(forceConnector).getRotational(), accelerationConnector->getAngularAccelerationValueRefs(), forceConnector->getTorqueValueRefs());
                                } else {
                                    fatal("Strong coupling requires angular acceleration outputs for now\n");
                                }
                            }
                        } else {
                            //step 1 = put returned directional derivatives in the correct place in the sparse mobility matrix
                            int I = accelerationConnector->m_index;
                            int J = eq->m_index;
                            JacobianElement &el = m_strongCouplingSolver.m_mobilities[make_pair(I,J)];

                            if (eq->m_isSpatial) {
                                if (accelerationConnector->getAccelerationValueRefs().size() == 1) {
                                    el.setSpatial(    client->m_getDirectionalDerivativeValues.front()[0], 0, 0);
                                } else {
                                    el.setSpatial(    client->m_getDirectionalDerivativeValues.front()[0],
                                                      client->m_getDirectionalDerivativeValues.front()[1],
                                                      client->m_getDirectionalDerivativeValues.front()[2]);
                                }
                                client->m_getDirectionalDerivativeValues.pop_front();
                            } else {
                                el.setSpatial(0,0,0);
                            }

                            if (eq->m_isRotational) {
                                //1-D?
                                if (accelerationConnector->getAngularAccelerationValueRefs().size() == 1) {
                                    //debug("J(%i,%i) = %f\n", I, J, client->m_getDirectionalDerivativeValues.front()[0]);
                                    el.setRotational( client->m_getDirectionalDerivativeValues.front()[0], 0, 0);
                                } else {
                                    el.setRotational( client->m_getDirectionalDerivativeValues.front()[0],
                                                      client->m_getDirectionalDerivativeValues.front()[1],
                                                      client->m_getDirectionalDerivativeValues.front()[2]);
                                }
                                client->m_getDirectionalDerivativeValues.pop_front();
                            } else {
                                el.setRotational(0,0,0);
                            }
                        }
                }
            }
        }
        if (step == 0) {
            PRINT_HDF5_DELTA("get_directional_derivs");
            wait();
            PRINT_HDF5_DELTA("get_directional_derivs_wait");
        } else {
            PRINT_HDF5_DELTA("distribute_directional_derivs");
        }
    }

    //compute strong coupling forces
    m_strongCouplingSolver.solve(holonomic);
    PRINT_HDF5_DELTA("run_solver");

    //distribute forces
    char separator = fmigo::globals::getSeparator();
    for (size_t i=0; i<m_clients.size(); i++){
        FMIClient *client = m_clients[i];
        for (int j = 0; j < client->numConnectors(); j++) {
            StrongConnector *sc = client->getConnector(j);
            vector<double> vec;

            //dump force/torque
            if (sc->hasForce()) {
                if (fmigo::globals::fileFormat != none) {
                    printf("%c%+.16le%c%+.16le%c%+.16le", separator, sc->m_force.x(), separator, sc->m_force.y(), separator, sc->m_force.z());
                }
                vec.push_back(sc->m_force.x());
                vec.push_back(sc->m_force.y());
                vec.push_back(sc->m_force.z());
            }

            if (sc->hasTorque()) {
                if (fmigo::globals::fileFormat != none) {
                    printf("%c%+.16le%c%+.16le%c%+.16le", separator, sc->m_torque.x(), separator, sc->m_torque.y(), separator, sc->m_torque.z());
                }
                vec.push_back(sc->m_torque.x());
                vec.push_back(sc->m_torque.y());
                vec.push_back(sc->m_torque.z());
            }

            vector<int> fvrs = sc->getForceValueRefs();
            vector<int> tvrs = sc->getTorqueValueRefs();
            fvrs.insert(fvrs.end(), tvrs.begin(), tvrs.end());

            send(client, fmi2_import_set_real(fvrs, vec));
        }
    }
    PRINT_HDF5_DELTA("send_strong_forces");

    //do actual step
    //noSetFMUStatePriorToCurrentPoint = true
    //In other words: do the step, commit the results (basically, we're not going back)
    sendWait(m_clients, fmi2_import_do_step(t, dt, true));
    PRINT_HDF5_DELTA("do_step");
}

string StrongMaster::getForceFieldnames() const {
    char separator = fmigo::globals::getSeparator();

    ostringstream oss;
    for (size_t i=0; i<m_clients.size(); i++){
        FMIClient *client = m_clients[i];
        for (int j = 0; j < client->numConnectors(); j++) {
            StrongConnector *sc = client->getConnector(j);
            ostringstream basename;
            basename << "fmu" << i << "_conn" << j << "_";

            if (sc->hasForce()) {
                oss << separator << basename.str() << "force_x";
                oss << separator << basename.str() << "force_y";
                oss << separator << basename.str() << "force_z";
            }

            if (sc->hasTorque()) {
                oss << separator << basename.str() << "torque_x";
                oss << separator << basename.str() << "torque_y";
                oss << separator << basename.str() << "torque_z";
            }
        }
    }
    return oss.str();
}

int StrongMaster::getNumForceOutputs() const {
  int ret = 0;
  for (size_t i=0; i<m_clients.size(); i++){
    FMIClient *client = m_clients[i];
    for (int j = 0; j < client->numConnectors(); j++) {
      StrongConnector *sc = client->getConnector(j);
      if (sc->hasForce()) {
        ret += 3;
      }
      if (sc->hasTorque()) {
        ret += 3;
      }
    }
  }
  return ret;
}
