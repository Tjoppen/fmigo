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
#include "fmitcp.pb.h"
#include <algorithm>

using namespace fmitcp_master;
using namespace fmitcp;
using namespace fmitcp::serialize;
using namespace sc;

StrongMaster::StrongMaster(zmq::context_t &context, vector<FMIClient*> clients, vector<WeakConnection> weakConnections,
                           Solver *strongCouplingSolver, bool holonomic, const std::vector<Rend>& rends) :
        JacobiMaster(context, clients, weakConnections),
        m_strongCouplingSolver(strongCouplingSolver), holonomic(holonomic), rends(rends) {
    info("StrongMaster (%s)\n", holonomic ? "holonomic" : "non-holonomic");

    if (rends.size() < 2) {
        fatal("rends too small: %i\n", (int)rends.size());
    }

    counters.resize(rends.size());

    //populate clientrend
    //sanity check rends while we're at it - it should contain all client IDs in children and parents
    set<int> children, parents;

    clientrend.resize(m_clients.size());
    for (int i = 0; i < (int)rends.size(); i++) {
        for (int id : rends[i].parents) {
            clientrend[id] = i;
            if (parents.count(id)) {
                fatal("parent id=%i occurs more than once in execution order\n", id);
            }
            parents.insert(id);
            if (id < 0 || (size_t)id >= clients.size()) {
                fatal("parent id=%i in execution order is outside the valid range\n", id);
            }
        }
        for (int id : rends[i].children) {
            if (children.count(id)) {
                fatal("child id=%i occurs more than once in execution order\n", id);
            }
            children.insert(id);
            if (id < 0 || (size_t)id >= clients.size()) {
                fatal("child id=%i in execution order is outside the valid range\n", id);
            }
        }
    }

    if (children.size() != clients.size()) {
        fatal("execution order child count %i != FMU count %i\n", (int)children.size(), (int)clients.size());
    }
    if (parents.size() != clients.size()) {
        fatal("execution order parent count %i != FMU count %i\n", (int)parents.size(), (int)clients.size());
    }

    for (auto wc : m_weakConnections) {
        clientGetXs[wc.to->m_id][wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
    }
}

StrongMaster::~StrongMaster() {
    if (m_strongCouplingSolver) {
        delete m_strongCouplingSolver;
    }
}

void StrongMaster::prepare() {
    JacobiMaster::prepare();

    if (m_strongCouplingSolver) {
        m_strongCouplingSolver->prepare();

        //check that every FMU involved in an equation has get/set functionality
        //also put their IDs in the kinematic set
        for (sc::Equation *eq : m_strongCouplingSolver->getEquations()) {
            for (sc::Connector *fc : eq->m_connectors) {
                FMIClient *client = dynamic_cast<FMIClient*>(fc->m_slave);
                kins.insert(client->m_id);

                if (!client->hasCapability(fmi2_cs_canGetAndSetFMUstate)) {
                    fatal("FMU %i (%s) is part of a kinematic constraint but lacks rollback functionality (canGetAndSetFMUstate=\"false\")\n",
                        client->m_id, client->getModelName().c_str());
                }
            }
        }
    }

    forces.resize(getNumForces());
}

void StrongMaster::getDirectionalDerivative(fmitcp_proto::fmi2_kinematic_req& kin, const Vec3& seedVec, const vector<int>& accelerationRefs, const vector<int>& forceRefs) {
    fmitcp_proto::fmi2_import_get_directional_derivative_req* get_derivs = kin.add_get_derivs();

    for (size_t x = 0; x < accelerationRefs.size(); x++) {
      get_derivs->add_z_ref(accelerationRefs[x]);
      get_derivs->add_v_ref(forceRefs[x]);
      get_derivs->add_dv(seedVec[x]);
    }
}

//"convenience" function for filling out setX entries in fmi2_kinematic_req
template<typename T, typename set_x_req>
void fill_kinematic_req(
          const SendSetXType& it,
          fmitcp_proto::fmi2_kinematic_req& req,
          fmi2_base_type_enu_t type,
          set_x_req* (fmitcp_proto::fmi2_kinematic_req::*mutable_x)(),
          T MultiValue::*member) {
    if (it.count(type)) {
        const pair<vector<int>, vector<MultiValue> > vrs_values = it.find(type)->second;
        set_x_req* values = (req.*mutable_x)();

        for (int vr : vrs_values.first) {
            values->add_valuereferences(vr);
        }
        for (T value : vectorToBaseType(vrs_values.second, member)) {
            values->add_values(value);
        }
    }
}

void StrongMaster::crankIt(double t, double dt, const std::set<int>& target) {
    //crank system until open set contains target,
    //or until the end if target is empty
    debug("into  crankIt: %zu %zu %zu %zu\n", done.size(), open.size(), todo.size(), target.size());
    while (done.size() < m_clients.size() &&
            (target.size() == 0 ||
             !std::includes(open.begin(), open.end(), target.begin(), target.end()))) {
        debug("      -crankIt: %zu %zu %zu %zu\n", done.size(), open.size(), todo.size(), target.size());

        //only step those which are not in the target set
        set<int> toStep;
        for (int id : open) {
            if (target.count(id) == 0) {
                toStep.insert(id);
            }
        }

        if (toStep.size() == 0) {
            fatal("toStep = 0? Stepping order must be unfulfillable\n");
        }

        //request inputs for the FMUs we're about to step
        for (int id : toStep) {
            for (auto it : clientGetXs[id]) {
                it.first->queueX(it.second);
            }
        }

        queueValueRequests();
        wait();

        //grab the values that we requested
        InputRefsValuesType refValues = getInputWeakRefsAndValues(m_weakConnections, toStep);

        //distribute inputs, step
        for (int id : toStep) {
            m_clients[id]->sendSetX(refValues[m_clients[id]]);
            queueMessage(m_clients[id], fmi2_import_do_step(t, dt, true));
        }

        //all cached values are now bork
        deleteCachedValues(true, toStep);

        moveCranked(toStep);
    }
    debug(" out  crankIt: %zu %zu %zu %zu\n", done.size(), open.size(), todo.size(), target.size());
}

void StrongMaster::moveCranked(std::set<int> cranked) {
    set<int> triggered_rends;
    for (int id : cranked) {
        //poke the corresponding rend
        int v = --counters[clientrend[id]];
        if (v < 0) {
            fatal("negative rend counter\n");
        } else if (v == 0) {
            triggered_rends.insert(clientrend[id]);
        }

        done.insert(id);
        open.erase(id);
    }

    //deal with triggered rends
    for (int t : triggered_rends) {
        for (int id : rends[t].children) {
            if (todo.count(id) != 1) {
                string s = executionOrderToString(rends);
                info(s.c_str());
                fatal("rend child %i not in todo set\n", id);
            }
            todo.erase(id);
            open.insert(id);
        }
    }
}

void StrongMaster::stepKinematicFmus(double t, double dt) {
    //get weak connector outputs
    for (int id : open) {
        for (auto it : clientGetXs[id]) {
            it.first->queueX(it.second);
        }
    }

    //get strong connector inputs
    for (int id : open) {
        const vector<int>& valueRefs = m_clients[id]->getStrongConnectorValueReferences();
        m_clients[id]->queueReals(valueRefs);
    }

    //these two are usually no-ops since we ask for a pre-fetch at the end of the function
    queueValueRequests();
    wait();

    //disentangle received values for set_real() further down (before do_step())
    //we shouldn't set_real() for these until we've gotten directional derivatives
    //this sets StrongMaster apart from the weak masters
    InputRefsValuesType refValues = getInputWeakRefsAndValues(m_weakConnections, open);

    vector<fmitcp_proto::fmi2_kinematic_req> kin(open.size(), fmitcp_proto::fmi2_kinematic_req());
    vector<int> kin_ofs(open.size(), 0);  //current offset into derivs
    map<int,int> id2kin; //maps FMU IDs into indices into kin

    //set weak connector inputs
    int kini = 0;
    for (int id : open) {
        id2kin[id] = kini;
        SendSetXType it = refValues[m_clients[id]];
        fill_kinematic_req(it, kin[kini], fmi2_base_type_real, &fmitcp_proto::fmi2_kinematic_req::mutable_reals,   &MultiValue::r);
        fill_kinematic_req(it, kin[kini], fmi2_base_type_int,  &fmitcp_proto::fmi2_kinematic_req::mutable_ints,    &MultiValue::i);
        fill_kinematic_req(it, kin[kini], fmi2_base_type_bool, &fmitcp_proto::fmi2_kinematic_req::mutable_bools,   &MultiValue::b);
        fill_kinematic_req(it, kin[kini], fmi2_base_type_str,  &fmitcp_proto::fmi2_kinematic_req::mutable_strings, &MultiValue::s);
        kini++;
    }

    //set connector values
    for (int id : open) {
        FMIClient *client = m_clients[id];
        const vector<int>& vrs = client->getStrongConnectorValueReferences();
        client->setConnectorValues(vrs, client->getReals(vrs));
    }

    //update constraints since connector values changed
    if (m_strongCouplingSolver) {
        m_strongCouplingSolver->updateConstraints();
    }

    //get future velocities:
    //0. save FMU states
    //1. step
    //2. get velocity
    //3. restore FMU states

    //first filter out FMUs with save/load functionality
    for (int id : open) {
        FMIClient *client = m_clients[id];
        if (client->hasCapability(fmi2_cs_canGetAndSetFMUstate)) {
            for (int vr : client->getStrongConnectorValueReferences()) {
                kin[id2kin[id]].add_future_velocity_vrs(vr);
            }
            kin[id2kin[id]].set_currentcommunicationpoint(t);
            kin[id2kin[id]].set_communicationstepsize(dt);
        }
    }

    //get directional derivatives
    //this is a two-step process which is important to get the order of correct
    if (m_strongCouplingSolver) {
    for (sc::Equation *eq : m_strongCouplingSolver->getEquations()) {
        for (sc::Connector *fc : eq->m_connectors) {
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

                //step 0 = send fmi2_import_get_directional_derivative() requests
                if (eq->m_isSpatial) {
                    if (accelerationConnector->hasAcceleration() && forceConnector->hasForce()) {
                        getDirectionalDerivative(kin[id2kin[client->m_id]], eq->jacobianElementForConnector(forceConnector).getSpatial(), accelerationConnector->getAccelerationValueRefs(), forceConnector->getForceValueRefs());
                    } else {
                        fatal("Strong coupling requires acceleration outputs for now\n");
                    }
                }

                if (eq->m_isRotational) {
                    if (accelerationConnector->hasAngularAcceleration() && forceConnector->hasTorque()) {
                        getDirectionalDerivative(kin[id2kin[client->m_id]], eq->jacobianElementForConnector(forceConnector).getRotational(), accelerationConnector->getAngularAccelerationValueRefs(), forceConnector->getTorqueValueRefs());
                    } else {
                        fatal("Strong coupling requires angular acceleration outputs for now\n");
                    }
                }
            }
        }
    }
    }

    for (int id : open) {
        if (kin[id2kin[id]].has_reals() ||
            kin[id2kin[id]].has_ints() ||
            kin[id2kin[id]].has_bools() ||
            kin[id2kin[id]].has_strings() ||
            kin[id2kin[id]].future_velocity_vrs_size()) {
          m_clients[id]->queueMessage(pack(fmitcp_proto::type_fmi2_kinematic_req, kin[id2kin[id]]));
        }
    }

    wait();

    if (m_strongCouplingSolver) {
    for (sc::Equation *eq : m_strongCouplingSolver->getEquations()) {
        for (sc::Connector *fc : eq->m_connectors) {
            StrongConnector *forceConnector = dynamic_cast<StrongConnector*>(fc);
            FMIClient *client = dynamic_cast<FMIClient*>(forceConnector->m_slave);
            if (!open.count(client->m_id)) {
                fatal("Kinematic FMU %i not in open set\n", client->m_id);
            }
            for (int x = 0; x < client->numConnectors(); x++) {
                StrongConnector *accelerationConnector = dynamic_cast<StrongConnector*>(forceConnector->m_slave->getConnector(x));

                //step 1 = put returned directional derivatives in the correct place in the sparse mobility matrix
                int I = accelerationConnector->m_index;
                int J = eq->m_index;
                JacobianElement &el = m_strongCouplingSolver->m_mobilities[make_pair(I,J)];

                if (eq->m_isSpatial) {
                    const fmitcp_proto::fmi2_import_get_directional_derivative_res& res = client->last_kinematic.derivs(kin_ofs[id2kin[client->m_id]]++);
                    if (accelerationConnector->getAccelerationValueRefs().size() == 1) {
                        el.setSpatial(    res.dz(0), 0,         0);
                    } else {
                        el.setSpatial(    res.dz(0), res.dz(1), res.dz(2));
                    }
                } else {
                    el.setSpatial(0,0,0);
                }

                if (eq->m_isRotational) {
                    const fmitcp_proto::fmi2_import_get_directional_derivative_res& res = client->last_kinematic.derivs(kin_ofs[id2kin[client->m_id]]++);
                    //1-D?
                    if (accelerationConnector->getAngularAccelerationValueRefs().size() == 1) {
                        //debug("J(%i,%i) = %f\n", I, J, client->m_getDirectionalDerivativeValues.front()[0]);
                        el.setRotational( res.dz(0), 0,         0);
                    } else {
                        el.setRotational( res.dz(0), res.dz(1), res.dz(2));
                    }
                } else {
                    el.setRotational(0,0,0);
                }
            }
        }
    }
    }

    //set FUTURE connector values (velocities only)
    for (int id : open) {
        if (kin[id2kin[id]].future_velocity_vrs_size()) {
            const vector<int>& vrs = m_clients[id]->getStrongConnectorValueReferences();
            vector<double> reals;
            reals.reserve(kin[id2kin[id]].future_velocity_vrs_size());
            for (int x = 0; x < kin[id2kin[id]].future_velocity_vrs_size(); x++) {
              reals.push_back(m_clients[id]->last_kinematic.future_velocities(x));
            }
            m_clients[id]->setConnectorFutureVelocities(vrs, reals);
        }
    }

    //compute strong coupling forces
    if (m_strongCouplingSolver) {
        m_strongCouplingSolver->solve(holonomic);
    }

    //distribute forces
    char separator = fmigo::globals::getSeparator();
    //offset into this->forces
    int forceofs = 0;
    for (int id : open) {
        FMIClient *client = m_clients[id];
        for (int j = 0; j < client->numConnectors(); j++) {
            StrongConnector *sc = client->getConnector(j);
            vector<double> vec;
            vec.reserve(6);

            vector<int> fvrs = sc->getForceValueRefs();
            vector<int> tvrs = sc->getTorqueValueRefs();

            //dump force/torque
            if (sc->hasForce()) {
                this->forces[forceofs++] = sc->m_force.x();
                vec.push_back(sc->m_force.x());

                if (fvrs.size() > 1) {
                  this->forces[forceofs++] = sc->m_force.y();
                  this->forces[forceofs++] = sc->m_force.z();
                  vec.push_back(sc->m_force.y());
                  vec.push_back(sc->m_force.z());
                }
            }

            if (sc->hasTorque()) {
                //only set/print one torque for shafts
                this->forces[forceofs++] = sc->m_torque.x();
                vec.push_back(sc->m_torque.x());

                if (tvrs.size() > 1) {
                  this->forces[forceofs++] = sc->m_torque.y();
                  this->forces[forceofs++] = sc->m_torque.z();
                  vec.push_back(sc->m_torque.y());
                  vec.push_back(sc->m_torque.z());
                }
            }

            fvrs.insert(fvrs.end(), tvrs.begin(), tvrs.end());

            queueMessage(client, fmi2_import_set_real(fvrs, vec));
        }
    }

    if ((size_t)forceofs != forces.size()) {
      fatal("forceofs != forces.size()\n");
    }

    //do actual step
    //noSetFMUStatePriorToCurrentPoint = true
    //In other words: do the step, commit the results (basically, we're not going back)
    for (int id : open) {
        queueMessage(m_clients[id], fmi2_import_do_step(t, dt, true));
    }

    //do_step() makes values old
    deleteCachedValues(true, open);

    //pass open by value so a copy happens
    moveCranked(open);
}

void StrongMaster::runIteration(double t, double dt) {
    //execution order stuff
    //first set up the rend counters and all three sets
    done.clear();
    open = rends[0].children;
    todo.clear();

    for (size_t x = 0; x < rends.size(); x++) {
        counters[x] = (int)rends[x].parents.size();
    }

    for (FMIClient *client : m_clients) {
        if (open.count(client->m_id) == 0) {
            todo.insert(client->m_id);
        }
    }

    //crank system until kins \in open
    crankIt(t, dt, kins);

    //only bother doing anything more if we have some FMUs left to step
    if (open.size() > 0) {
        stepKinematicFmus(t, dt);

        //crank the rest of the system
        crankIt(t, dt, set<int>());
    } else if (todo.size() > 0) {
        //probably broken execution order XML parsing if we got here
        fatal("open.size() == 0 but todo.size() == %i\n", (int)todo.size());
    }

    //pre-fetch values for next step
    for (int id : rends[0].children) {
        for (auto it : clientGetXs[id]) {
            it.first->queueX(it.second);
        }
    }
    for(size_t i=0; i<m_clients.size(); i++){
        const vector<int>& valueRefs = m_clients[i]->getStrongConnectorValueReferences();
        m_clients[i]->queueReals(valueRefs);
    }
}

string StrongMaster::getFieldNames() const {
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
                if (sc->getAccelerationValueRefs().size() > 1) {
                  oss << separator << basename.str() << "force_y";
                  oss << separator << basename.str() << "force_z";
                }
            }

            if (sc->hasTorque()) {
                oss << separator << basename.str() << "torque_x";
                if (sc->getAngularAccelerationValueRefs().size() > 1) {
                  oss << separator << basename.str() << "torque_y";
                  oss << separator << basename.str() << "torque_z";
                }
            }
        }
    }
    if ( m_strongCouplingSolver )
      oss << m_strongCouplingSolver->getViolationsNames(separator);
    return oss.str();
}

int StrongMaster::getNumForces() const {
  int ret = 0;
  for (size_t i=0; i<m_clients.size(); i++){
    FMIClient *client = m_clients[i];
    for (int j = 0; j < client->numConnectors(); j++) {
      StrongConnector *sc = client->getConnector(j);
      if (sc->hasForce()) {
        ret += sc->getAccelerationValueRefs().size();
      }
      if (sc->hasTorque()) {
        ret += sc->getAngularAccelerationValueRefs().size();
      }
    }
  }
  return ret;
}

void StrongMaster::writeFields(bool last) {
  char sep = fmigo::globals::getSeparator();
  if ( last ) {
    //finish off with zeroes for any extra forces
    int n = getNumForces();
    for (int i = 0; i < n; i++) {
      fprintf(fmigo::globals::outfile, "%c0", sep);
    }
  } else {
    for (double f : this->forces) {
      fprintf(fmigo::globals::outfile, "%c%+.16le", sep, f);
    }
  }

  if (m_strongCouplingSolver) {
    m_strongCouplingSolver->writeViolations(fmigo::globals::outfile, sep);
  }
}
