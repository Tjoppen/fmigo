#include <string>
#include <fmitcp/Client.h>
#include <fmitcp/fmitcp-common.h>
#ifdef USE_MPI
#include <mpi.h>
#include "common/mpi_tools.h"
#include "server/FMIServer.h"
#endif
#include <zmq.hpp>
#include <fmitcp/serialize.h>
#include <stdlib.h>
#include <signal.h>
#include <sstream>
#include "master/FMIClient.h"
#include "common/common.h"
#include "master/WeakMasters.h"
#include "master/parseargs.h"
#ifdef ENABLE_SC
#include <sc/BallJointConstraint.h>
#include <sc/LockConstraint.h>
#include <sc/ShaftConstraint.h>
#include <sc/MultiWayConstraint.h>
#include "master/StrongMaster.h"
#endif
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <fstream>
#include "control.pb.h"
#include "master/globals.h"
#ifdef USE_MATIO
#include <matio.h>
#endif

using namespace fmitcp_master;
using namespace fmitcp;
using namespace fmitcp::serialize;
using namespace sc;
using namespace common;

jm_log_level_enu_t fmigo_loglevel = jm_log_level_warning;
bool alwaysComputeNumericalDirectionalDerivatives = false;

#ifdef USE_MPI
static vector<FMIClient*> setupClients(int numFMUs) {
    vector<FMIClient*> clients;
    for (int x = 0; x < numFMUs; x++) {
        FMIClient* client = new FMIClient(x+1 /* world_rank */, x /* clientId */);
        clients.push_back(client);
    }
    return clients;
}
#else
static vector<FMIClient*> setupClients(vector<string> fmuURIs, zmq::context_t &context) {
    vector<FMIClient*> clients;
    int clientId = 0;
    for (auto it = fmuURIs.begin(); it != fmuURIs.end(); it++, clientId++) {
        // Assume URI to client
        FMIClient *client = new FMIClient(context, clientId, *it);

        if (!client) {
            fatal("Failed to connect client with URI %s\n", it->c_str());
        }

        clients.push_back(client);
    }
    return clients;
}
#endif

static vector<WeakConnection> setupWeakConnections(vector<connection> connections, vector<FMIClient*> clients) {
    vector<WeakConnection> weakConnections;
    for(auto conn: connections){
        weakConnections.push_back(WeakConnection(conn, clients[conn.fromFMU], clients[conn.toFMU]));
    }
    return weakConnections;
}

#ifdef ENABLE_SC
static StrongConnector* findOrCreateBallLockConnector(FMIClient *client,
        int posX, int posY, int posZ,
        int accX, int accY, int accZ,
        int forceX, int forceY, int forceZ,
        int quatX, int quatY, int quatZ, int quatW,
        int angAccX, int angAccY, int angAccZ,
        int torqueX, int torqueY, int torqueZ) {
    for (int x = 0; x < client->numConnectors(); x++) {
        StrongConnector *sc = client->getConnector(x);
        if (sc->matchesBallLockConnector(
                posX, posY, posZ, accX, accY, accZ, forceX, forceY, forceZ,
                quatX, quatY, quatZ, quatW, angAccX, angAccY, angAccZ, torqueX, torqueY, torqueZ)) {
            return sc;
        }
    }
    StrongConnector *sc = client->createConnector();
    sc->setPositionValueRefs           (posX, posY, posZ);
    sc->setAccelerationValueRefs       (accX, accY, accZ);
    sc->setForceValueRefs              (forceX, forceY, forceZ);
    sc->setQuaternionValueRefs         (quatX, quatY, quatZ, quatW);
    sc->setAngularAccelerationValueRefs(angAccX, angAccY, angAccZ);
    sc->setTorqueValueRefs             (torqueX, torqueY, torqueZ);
    return sc;
}

static StrongConnector* findOrCreateShaftConnector(FMIClient *client,
        int angle, int angularVel, int angularAcc, int torque) {
    for (int x = 0; x < client->numConnectors(); x++) {
        StrongConnector *sc = client->getConnector(x);
        if (sc->matchesShaftConnector(angle, angularVel, angularAcc, torque)) {
            debug("Match! id = %i\n", sc->m_index);
            return sc;
        }
    }
    StrongConnector *sc = client->createConnector();
    sc->setShaftAngleValueRef          (angle);
    sc->setAngularVelocityValueRefs    (angularVel, -1, -1);
    sc->setAngularAccelerationValueRefs(angularAcc, -1, -1);
    sc->setTorqueValueRefs             (torque,     -1, -1);
    return sc;
}
#endif

static int vrFromKeyName(FMIClient* client, string key){

  if(isVR(key))
    return atoi(key.c_str());

  const variable_map& vars = client->getVariables();

  switch (vars.count(key)){
  case 0:{
    fatal("client(%d):%s\n", client->m_id, key.c_str());
  }
  case 1:  return vars.find(key)->second.vr;
  default:{
    fatal("Not uniq - client(%d):%s\n", client->m_id, key.c_str());
  }
  }
}

#define toVR(i, index)                                   \
  vrFromKeyName(clients[it->fmus[i]],it->vrORname[index])

#ifdef ENABLE_SC
static Solver* setupConstraintsAndSolver(vector<strongconnection> strongConnections, vector<FMIClient*> clients) {
    if (strongConnections.size() == 0) {
        return NULL;
    }
    Solver *solver = new Solver;

    for (auto it = strongConnections.begin(); it != strongConnections.end(); it++) {
        //NOTE: this leaks memory, but I don't really care since it's only setup
        Constraint *con;

        if (it->type == "ball" || it->type == "lock") {
            if (it->vrORname.size() != 38) {
                fatal("Bad %s specification: need 38 VRs ([XYZpos + XYZacc + XYZforce + Quat + XYZrotAcc + XYZtorque] x 2), got %zu\n",
                        it->type == "ball" ? "ball joint" : "lock", it->vrORname.size());
            }

            StrongConnector *scA = findOrCreateBallLockConnector(clients[it->fmus[0]],
                                       toVR(0,0), toVR(0,1), toVR(0,2),
                                       toVR(0,3), toVR(0,4), toVR(0,5),
                                       toVR(0,6), toVR(0,7), toVR(0,8),
                                       toVR(0,9), toVR(0,10), toVR(0,11), toVR(0,12),
                                       toVR(0,13), toVR(0,14), toVR(0,15),
                                       toVR(0,16), toVR(0,17), toVR(0,18) );

            StrongConnector *scB = findOrCreateBallLockConnector(clients[it->fmus[1]],
                                       toVR(1,19), toVR(1,20), toVR(1,21),
                                       toVR(1,22), toVR(1,23), toVR(1,24),
                                       toVR(1,25), toVR(1,26), toVR(1,27),
                                       toVR(1,28), toVR(1,29), toVR(1,30), toVR(1,31),
                                       toVR(1,32), toVR(1,33), toVR(1,34),
                                       toVR(1,35), toVR(1,36), toVR(1,37) );

            con = it->type == "ball" ? new BallJointConstraint(scA, scB, Vec3(), Vec3())
                                     : new LockConstraint(scA, scB, Vec3(), Vec3(), Quat(), Quat());
        } else if (it->type == "shaft") {
            if (it->vrORname.size() != 8) {
                fatal("Bad shaft specification: need 8 VRs ([shaft angle + angular velocity + angular acceleration + torque] x 2)\n");
            }

            StrongConnector *scA = findOrCreateShaftConnector(clients[it->fmus[0]],
                                       toVR(0,0), toVR(0,1), toVR(0,2), toVR(0,3));

            StrongConnector *scB = findOrCreateShaftConnector(clients[it->fmus[1]],
                                       toVR(1,4), toVR(1,5), toVR(1,6), toVR(1,7));

            con = new ShaftConstraint(scA, scB);
        } else if (it->type == "multiway") {
            if (it->vrORname.size() != it->fmus.size() * 5) {
                fatal("Bad multiway constraint specification: each FMU must have exactly 5 values. "
                      "The first four are VRs like shaft constraints (shaft angle + angular velocity + angular acceleration + torque), "
                      "last in each group of five is the weight. Example: -C multiway,3,0,1,2,phi,omega,alpha,tau,-1,omega,alpha,tau,2,omega,alpha,tau,2\n");
            }

            vector<Connector*> scs;
            vector<double> weights;
            for (size_t i = 0; i < it->fmus.size(); i++) {
                StrongConnector *scA = findOrCreateShaftConnector(clients[it->fmus[i]],
                                            toVR(i,i*5), toVR(i,i*5+1), toVR(i,i*5+2), toVR(i,i*5+3));
                weights.push_back(atof(it->vrORname[i*5+4].c_str()));
                scs.push_back(scA);
            }
            con = new MultiWayConstraint(scs, weights);
        } else {
            fatal("Unknown strong connector type: %s\n", it->type.c_str());
        }

        solver->addConstraint(con);
    }

    for (auto it = clients.begin(); it != clients.end(); it++) {
        solver->addSlave(*it);
   }

   return solver;
}
#endif

static param param_from_vr_type_string(int vr, fmi2_base_type_enu_t type, string s, string varname="") {
    param p;
    p.valueReference = vr;
    p.type = type;

    switch (p.type) {
    case fmi2_base_type_real:
        if (!isReal(s)) {
            if (varname.length()) {
                fatal("Value \"%s\" for variable %s does not look like a Real\n", s.c_str(), varname.c_str());
            } else {
                fatal("Value \"%s\" for VR %i does not look like a Real\n", s.c_str(), vr);
            }
        }
        p.realValue = atof(s.c_str());
        break;
    case fmi2_base_type_int:
        if (!isInteger(s)) {
            if (varname.length()) {
                fatal("Value \"%s\" for variable %s does not look like an Integer\n", s.c_str(), varname.c_str());
            } else {
                fatal("Value \"%s\" for VR %i does not look like an Integer\n", s.c_str(), vr);
            }
        }
        p.intValue = atoi(s.c_str());
        break;
    case fmi2_base_type_bool:
        if (!isBoolean(s)) {
            if (varname.length()) {
                fatal("Value \"%s\" for variable %s does not look like a Boolean\n", s.c_str(), varname.c_str());
            } else {
                fatal("Value \"%s\" for VR %i does not look like a Boolean\n", s.c_str(), vr);
            }
        }
        p.boolValue = (s == "true");
        break;
    case fmi2_base_type_str:  p.stringValue = s; break;
    case fmi2_base_type_enum: fatal("An enum snuck its way into -p\n");
    }

    return p;
}

static param_map resolve_string_params(const vector<deque<string> > &params, vector<FMIClient*> clients) {
    param_map ret;

    int i = 0;
    for (auto parts : params) {
        int fmuIndex = atoi(parts[parts.size()-3].c_str());

        if (fmuIndex < 0 || (size_t)fmuIndex >= clients.size()) {
            fatal("Parameter %d refers to FMU %d, which does not exist.\n", i, fmuIndex);
        }

        FMIClient *client = clients[fmuIndex];
        param p;
        fmi2_base_type_enu_t type = fmi2_base_type_real;

        if (parts.size() == 3) {
            //FMU,VR,value  [type=real]
            //FMU,NAME,value
            int vr = vrFromKeyName(client, parts[1]);
            string name = "";

            if (!isVR(parts[1])) {
                const variable_map& vars = client->getVariables();
                name = parts[1];
                type = vars.find(parts[1])->second.type;
            }

            p = param_from_vr_type_string(vr, type, parts[2], name);
        } else if (parts.size() == 4) {
            //type,FMU,VR,value
            type = type_from_char(parts[0]);

            if (!isVR(parts[2])) {
                fatal("Must use VRs when specifying parameters with explicit types (-p %s,%s,%s,%s)\n",
                    parts[0].c_str(), parts[1].c_str(), parts[2].c_str(), parts[3].c_str());
            }

            int vr = atoi(parts[2].c_str());
            p = param_from_vr_type_string(vr, type, parts[3]);
        }

        ret[make_pair(fmuIndex,type)].push_back(p);
        i++;
    }

    return ret;
}

map<double, param_map > param_mapFromCSV(fmigo_csv_fmu csvfmus, vector<FMIClient*> clients){
  map<double, param_map > pairmap;
    for (auto it = csvfmus.begin(); it != csvfmus.end(); it++) {
      variable_map vmap = clients[it->first]->getVariables();
      for (auto time: it->second.time) {
        for (auto header: it->second.headers) {
          string s = it->second.matrix[time][header];
          param p = param_from_vr_type_string(vmap[header].vr, vmap[header].type, s, header);
          int fmuIndex = it->first;
          pairmap[atof(time.c_str())][make_pair(fmuIndex,p.type)].push_back(p);
        }
      }
    }

  return pairmap;
}

//if initialization == true, send only values with initial=exact or causality=input
static void sendUserParams(BaseMaster *master, vector<FMIClient*> clients,
                           map<pair<int,fmi2_base_type_enu_t>,
                           vector<param> > params,
                           bool initialization = false) {
    for (auto it = params.begin(); it != params.end(); it++) {
        FMIClient *client = clients[it->first.first];
        const variable_vr_map& vr_map = client->getVRVariables();
        vector<int> vrs;
        for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
          auto it3 = vr_map.find(make_pair(it2->valueReference, it->first.second));

          if (it3 == vr_map.end()) {
            fatal("Couldn't find variable VR=%i type=%i\n",
                  it2->valueReference, it->first.second);
          }
          //We used to check for initial="calculated" here, but Dymola FMUs have VRs
          //with multiple ScalarVariable associated, which makes checks for that more involved.
          //The reason is that they have some of these SVs without causality or initial defined,
          //so they default to causality="local" and initial="calculated". FMIL then picks these up
          //and delivers the first of them to us, not the input one which has an appropriate value for initial.

          //skip non-exact, non-inputs during initialization
          if (initialization &&
              !(it3->second.initial   == fmi2_initial_enu_exact ||
                it3->second.causality == fmi2_causality_enu_input)) {
            debug("Skipping VR=%i (initial = %i, causality = %i)\n", it2->valueReference, it3->second.initial, it3->second.causality);
            continue;
          }

          vrs.push_back(it2->valueReference);
          debug("Sending VR=%i type=%i\n", it2->valueReference, it->first.second);
        }

        if (vrs.size() == 0) {
          continue;
        }

        switch (it->first.second) {
        case fmi2_base_type_real: {
            vector<double> values;
            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                values.push_back(it2->realValue);
            }
            client->queueMessage(fmi2_import_set_real(vrs, values));
            break;
        }
        case fmi2_base_type_enum:
        case fmi2_base_type_int: {
            vector<int> values;
            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                values.push_back(it2->intValue);
            }
            client->queueMessage(fmi2_import_set_integer(vrs, values));
            break;
        }
        case fmi2_base_type_bool: {
            vector<bool> values;
            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                values.push_back(it2->boolValue);
            }
            client->queueMessage(fmi2_import_set_boolean(vrs, values));
            break;
        }
        case fmi2_base_type_str: {
            vector<string> values;
            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                values.push_back(it2->stringValue);
            }
            client->queueMessage(fmi2_import_set_string(vrs, values));
            break;
        }
        }
    }
}

static string getFieldnames(vector<FMIClient*> clients) {
    ostringstream oss;
    char separator = fmigo::globals::getSeparator();
    switch (fmigo::globals::fileFormat){
    default:
    case csv:   oss << "#t"; break;
    case tikz:  oss << "t";  break;
    }

    for (auto client : clients) {
        ostringstream prefix;
        prefix << separator << "fmu" << client->m_id << "_";
        for (const variable& var : client->getOutputs()) {
            oss << prefix.str() << var.name;
        }
    }
    return oss.str();
}

template<typename RFType, typename From> void addVectorToRepeatedField(RFType* rf, const From& from) {
    for (auto f : from) {
        *rf->Add() = f;
    }
}

struct MatlabOutput {
    vector<vector<double> >  reals;
    vector<vector<int> >     ints;
    vector<vector<uint8_t> > bools;
};

#ifdef USE_MATIO
// Reserves space for Matlab output
static MatlabOutput reserveMatlabOutput(const vector<FMIClient*>& clients, int maxSamples, int nsteps) {
    MatlabOutput mo;

    //allocate a little more than we think we need
    int expected_rows = maxSamples > 0 ? maxSamples : nsteps;
    int extra = expected_rows / 100 + 1;
    if (expected_rows > INT_MAX - extra) {
        expected_rows = INT_MAX;
    } else {
        expected_rows += extra;
    }

    //initialize reals with time column
    mo.reals.push_back(vector<double>());
    mo.reals.back().reserve(expected_rows);

    for (auto client : clients) {
        for (const variable& var : client->getOutputs()) {
            switch (var.type) {
            case fmi2_base_type_real:
                mo.reals.push_back(vector<double>());
                mo.reals.back().reserve(expected_rows);
                break;
            case fmi2_base_type_int:
                mo.ints.push_back(vector<int>());
                mo.ints.back().reserve(expected_rows);
                break;
            case fmi2_base_type_bool:
                mo.bools.push_back(vector<uint8_t>());
                mo.bools.back().reserve(expected_rows);
                break;
            case fmi2_base_type_str:
                warning("Strings are ignored in Matlab output for now\n");
                break;
            case fmi2_base_type_enum:
                fatal("Enums are not allowed in Matlab output for now\n");
                break;
            }
        }
    }

    return mo;
}


static void writeMatlabOutput(const vector<FMIClient*>& clients, const MatlabOutput& mo, const std::string& outFilePath) {
    mat_t *matfp = Mat_CreateVer(outFilePath.c_str(), NULL, MAT_FT_MAT5);

    if (!matfp) {
        fatal("Failed to open %s\n", outFilePath.c_str());
    }

    int realofs = 0;
    int intofs = 0;
    int boolofs = 0;

    //there's always a time column, grab the number of rows from it, then write it
    size_t rows = mo.reals[0].size();
    size_t dims[2] = {rows,1};
    size_t struct_dims[2] = {1,1};
    matio_compression comp = fmigo::globals::fileFormat == mat5 ? MAT_COMPRESSION_NONE : MAT_COMPRESSION_ZLIB;

    matvar_t *timevar = Mat_VarCreate("t", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, (void*)mo.reals[0].data(), MAT_F_DONT_COPY_DATA);
    Mat_VarWrite(matfp, timevar, comp);
    Mat_VarFree(timevar);

    realofs++;

    for (auto client : clients) {
        vector<string> fieldnames;
        vector<const char*> fieldnames_c_str;
        int num_fields = 0;

        for (const variable& var : client->getOutputs()) {
            if (var.type != fmi2_base_type_str) {
                fieldnames.push_back(var.name);
            }
        }
        for (const string& str : fieldnames) {
            fieldnames_c_str.push_back(str.c_str());
        }

        ostringstream oss;
        oss << "fmu" << client->m_id;
        matvar_t *fmuvar = Mat_VarCreateStruct(oss.str().c_str(), 2, struct_dims, fieldnames_c_str.data(), fieldnames_c_str.size());

        int x = 0;
        for (const variable& var : client->getOutputs()) {
            //ignore string outputs
            if (var.type != fmi2_base_type_str) {
                matvar_t *field = NULL;
                switch (var.type) {
                case fmi2_base_type_real:
                    field = Mat_VarCreate(NULL, MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, (void*)mo.reals[realofs++].data(), MAT_F_DONT_COPY_DATA);
                    break;
                case fmi2_base_type_int:
                    field = Mat_VarCreate(NULL, MAT_C_INT32,  MAT_T_INT32,  2, dims, (void*)mo.ints[intofs++].data(),   MAT_F_DONT_COPY_DATA);
                    break;
                case fmi2_base_type_bool:
                    field = Mat_VarCreate(NULL, MAT_C_UINT8,  MAT_T_UINT8,  2, dims, (void*)mo.bools[boolofs++].data(), MAT_F_DONT_COPY_DATA);
                    break;
                default:
                    fatal("Unexpected variable type in writeMatlabOutput()\n");
                }
                Mat_VarSetStructFieldByName(fmuvar, fieldnames_c_str[x], 0, field);
                x++;
            }
        }

        Mat_VarWrite(matfp, fmuvar, comp);
        Mat_VarFree(fmuvar);
    }

    Mat_Close(matfp);
}
#endif

// Writes global time all <Outputs> in all clients to output file in desired format
// In case of Matlab output the values are merely cached, since we must write values in columns
static void printOutputs(double t, BaseMaster *master, vector<FMIClient*>& clients, MatlabOutput &mo, FILE *outfile, const bool matlab_output) {
    char separator = fmigo::globals::getSeparator();

    for (auto client : clients) {
        size_t nvars = client->getOutputs().size();
        SendGetXType getX;

        getX[fmi2_base_type_real].reserve(nvars);
        getX[fmi2_base_type_int ].reserve(nvars);
        getX[fmi2_base_type_bool].reserve(nvars);
        getX[fmi2_base_type_str ].reserve(nvars);

        for (const variable& var : client->getOutputs()) {
            getX[var.type].push_back(var.vr);
        }

        client->queueX(getX);
    }
    master->queueValueRequests();
    master->wait();

    //for matlab output
    int realofs = 0;
    int intofs = 0;
    int boolofs = 0;

    if (matlab_output) {
        mo.reals[realofs++].push_back(t);
    } else {
        fprintf(outfile, "%+.16le", t);
    }

    for (size_t x = 0; x < clients.size(); x++) {
        FMIClient *client = clients[x];
        for (const variable& out : client->getOutputs()) {
            switch (out.type) {
            case fmi2_base_type_real:
                if (matlab_output) {
                    mo.reals[realofs++].push_back(client->getReal(out.vr));
                } else {
                    fprintf(outfile, "%c%+.16le", separator, client->getReal(out.vr));
                }
                break;
            case fmi2_base_type_int:
                if (matlab_output) {
                    mo.ints[intofs++].push_back(client->getInt(out.vr));
                } else {
                    fprintf(outfile, "%c%i", separator, client->getInt(out.vr));
                }
                break;
            case fmi2_base_type_bool:
                if (matlab_output) {
                    mo.bools[boolofs++].push_back(client->getBool(out.vr));
                } else {
                    fprintf(outfile, "%c%i", separator, client->getBool(out.vr));
                }
                break;
            case fmi2_base_type_str: {
                if (!matlab_output) {
                ostringstream oss;
                string s = client->getString(out.vr);
                for(char c: s){
                    switch (c){
                    case '"': oss << "\"\""; break;
                    default: oss << c;
                    }
                }
                fprintf(outfile, "%c\"%s\"", separator, oss.str().c_str());
                }
                break;
            }
            case fmi2_base_type_enum:
                fatal("Enum outputs not allowed for now\n");
            }
        }
    }
}

/**
   This will push all the data from all FMUs including global data on a port.
 */
static void pushResults(int step, double t, double endTime, double timeStep, zmq::socket_t& push_socket, BaseMaster *master, vector<FMIClient*>& clients, bool pushEverything) {
    //collect data
    control_proto::results_message results;
    map<FMIClient*, SendGetXType> clientVariables;

    results.set_version(1);
    results.set_step(step);
    results.set_t(t);
    results.set_t_end(endTime);
    results.set_dt(timeStep);

    for (auto client : clients) {
        const variable_map& vars = client->getVariables();
        SendGetXType getVariables;

        //figure out what values we need to fetch from the FMU
        for (auto var : vars) {
            //only put in values which are
            if (    pushEverything ||
                    var.second.causality == fmi2_causality_enu_input ||
                    var.second.causality == fmi2_causality_enu_output) {
                getVariables[var.second.type].push_back(var.second.vr);
            }
        }

        client->queueX(getVariables);
        clientVariables[client] = getVariables;
    }
    master->queueValueRequests();
    master->wait();

    for (auto cv : clientVariables) {
        control_proto::fmu_results *fmu_res = results.add_results();

        fmu_res->set_fmu_id(cv.first->m_id);

        addVectorToRepeatedField(fmu_res->mutable_reals()->mutable_vrs(),       cv.second[fmi2_base_type_real]);
        addVectorToRepeatedField(fmu_res->mutable_reals()->mutable_values(),    cv.first->getReals(cv.second[fmi2_base_type_real]));
        addVectorToRepeatedField(fmu_res->mutable_ints()->mutable_vrs(),        cv.second[fmi2_base_type_int]);
        addVectorToRepeatedField(fmu_res->mutable_ints()->mutable_values(),     cv.first->getInts(cv.second[fmi2_base_type_int]));
        addVectorToRepeatedField(fmu_res->mutable_bools()->mutable_vrs(),       cv.second[fmi2_base_type_bool]);
        addVectorToRepeatedField(fmu_res->mutable_bools()->mutable_values(),    cv.first->getBools(cv.second[fmi2_base_type_bool]));
        addVectorToRepeatedField(fmu_res->mutable_strings()->mutable_vrs(),     cv.second[fmi2_base_type_str]);
        addVectorToRepeatedField(fmu_res->mutable_strings()->mutable_values(),  cv.first->getStrings(cv.second[fmi2_base_type_str]));
    }

    string str = results.SerializeAsString();
    zmq::message_t rep(str.length());
    memcpy(rep.data(), str.data(), str.length());
    push_socket.send(rep);
}

/**
   Utility so variable names can be used on the command line.
 */
static int connectionNamesToVr(std::vector<connection> &connections,
#ifdef ENABLE_SC
                        vector<strongconnection> &strongConnections,
#endif
                        const vector<FMIClient*> clients // could not get this to compile when defined in parseargs
                        ){
    for(size_t i = 0; i < connections.size(); i++){
      connections[i].fromOutputVR = vrFromKeyName(clients[connections[i].fromFMU], connections[i].fromOutputVRorNAME);
      connections[i].toInputVR = vrFromKeyName(clients[connections[i].toFMU], connections[i].toInputVRorNAME);

      if (connections[i].needs_type) {
          //resolve types
          connections[i].fromType = clients[connections[i].fromFMU]->getVariables().find(connections[i].fromOutputVRorNAME)->second.type;
          connections[i].toType   = clients[connections[i].toFMU  ]->getVariables().find(connections[i].toInputVRorNAME   )->second.type;
      }
    }

#ifdef ENABLE_SC
    for(size_t i = 0; i < strongConnections.size(); i++){
      size_t j = 0;
      if(strongConnections[i].vrORname.size()%2 != 0 && strongConnections[i].type != "multiway"){
        fatal("strong connection needs even number of connections for fmu0 and fmu1\n");
      }
    }
#endif
  return 0;
}

#ifdef USE_MPI
static void run_server(string fmuPath, int rank, string hdf5Filename) {
    FMIServer server(fmuPath, rank, hdf5Filename);
    std::string recv_str;

    for (;;) {
        int rank, tag;
        server.m_timer.rotate("pre_wait");
        mpi_recv_string(0, &rank, &tag, recv_str);
        server.m_timer.rotate("wait");

        //shutdown command?
        if (tag == 1) {
            break;
        }

        //let Server handle packet, send reply back to master
#if CLIENTDATA_NEW == 0
        std::string str = server.clientData(recv_str.c_str(), recv_str.length());
        if (str.length() > 0) {
          server.m_timer.rotate("pre_send");
          MPI_Send((void*)str.c_str(), str.length(), MPI_CHAR, rank, tag, MPI_COMM_WORLD);
          server.m_timer.rotate("send");
        }
#else
        const vector<char>& str = server.clientData(recv_str.c_str(), recv_str.length());
        if (str.size() > 0) {
          server.m_timer.rotate("pre_send");
          MPI_Send((void*)&str[0], str.size(), MPI_CHAR, rank, tag, MPI_COMM_WORLD);
          server.m_timer.rotate("send");
        }
#endif
    }

    MPI_Finalize();
}
#endif

static bool hasModelExchangeFMUs(vector<FMIClient*> clients) {
    for(FMIClient *client : clients) {
        if (client->getFmuKind() == fmi2_fmu_kind_me) {
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[] ) {
    //count everything from here to before the main loop into "setup"
    fmigo::globals::timer.dont_rotate = true;
    FILE *outfile = stdout;

    //check endianness
    int e = 1;
    if (*(char*)&e != 1) {
        //workaround: use protobuf for get_real/set_real/do_step
        fatal("Big-endian machines not supported\n");
    }

#ifdef USE_MPI
    MPI_Init(NULL, NULL);

    //world = master at 0, FMUs at 1..N
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
#endif

    double timeStep = 0.1;
    double endTime = 10;
    double relativeTolerance = 0.0001;
    double relaxation = 2,
           compliance = 0;
    vector<string> fmuURIs;
    vector<connection> connections;
    vector<deque<string> > params;
    string outFilePath = "";
    int realtimeMode = 0;
    vector<Rend> executionOrder;
    vector<int> fmuVisibilities;
#ifdef ENABLE_SC
    vector<strongconnection> scs;
#endif
    string fieldnameFilename;
    bool holonomic = true;
    bool useHeadersInCSV = false;
    int command_port = 0, results_port = 0;
    bool startPaused = false, solveLoops = false;
    fmigo_csv_fmu csv_fmu;
    string hdf5Filename;
    int maxSamples = -1;
    bool writeSolverFields = false;
    MatlabOutput mo;

    parseArguments(
            argc, argv, &fmuURIs, &connections, &params, &endTime, &timeStep,
            &fmigo_loglevel, &outFilePath, &fmigo::globals::fileFormat,
            &realtimeMode, &executionOrder, &fmuVisibilities,
#ifdef ENABLE_SC
            &scs,
#endif
            &hdf5Filename, &fieldnameFilename, &holonomic, &compliance,
            &command_port, &results_port, &startPaused, &solveLoops, &useHeadersInCSV, &csv_fmu, &maxSamples, &relaxation,
            &writeSolverFields
    );

#ifdef USE_MPI
    if (world_rank > 0) {
        //we're a server
        //In MPI mode, treat fmuURIs as a list of paths,
        //or as the single FMU filename to use for this node.
        //See parseargs.cpp for more information.
        if (fmuURIs.size() == 0 || (fmuURIs.size() > 1 && (size_t)world_rank > fmuURIs.size())) {
          //checked in parseArguments()
          fatal("This should never happen\n");
        } else if (fmuURIs.size() == 1) {
          run_server(fmuURIs[0], world_rank, hdf5Filename);
        } else {
          run_server(fmuURIs[world_rank-1], world_rank, hdf5Filename);
        }
        return 0;
    }
    //world_rank == 0 below
#endif

    const bool matlab_output = fmigo::globals::fileFormat == mat5 || fmigo::globals::fileFormat == mat5_zlib;
#ifndef USE_MATIO
    if (matlab_output) {
        fatal("Matlab output not enabled, recompile with -DUSE_MATIO=ON\n");
    }
#endif

    if (outFilePath.size()) {
      //when outputing .mat libmatio takes care of opening the output file,
      //so keep outfile pointing at stdout (we don't use it but it avoids
      //the fclose() further down being called)
      if (!matlab_output) {
        outfile = fopen(outFilePath.c_str(), "w");

        if (!outfile) {
            fatal("Failed to open %s\n", outFilePath.c_str());
        }
      }
    } else {
#ifdef USE_MATIO
        if (matlab_output) {
            fatal("Cannot write Matlab output to stdout\n");
        }
#endif
    }

    zmq::context_t context(1);
    zmq::socket_t push_socket(context, ZMQ_PUSH);

#ifdef USE_MPI
    vector<FMIClient*> clients = setupClients(world_size-1);
#else
    //without this the maximum number of clients tops out at 300 on Linux,
    //around 63 on Windows (according to Web searches)
#ifdef ZMQ_MAX_SOCKETS
    zmq_ctx_set((void *)context, ZMQ_MAX_SOCKETS, fmuURIs.size() + !!command_port + !!results_port);
#endif
    vector<FMIClient*> clients = setupClients(fmuURIs, context);
    info("Successfully connected to all %zu servers\n", fmuURIs.size());
#endif

    //catch any ZMQ exceptions
    try {
    //get modelDescription XML
    //important to be able to resolve variable names
    for (auto it = clients.begin(); it != clients.end(); it++) {
        (*it)->sendMessageBlocking(get_xml());
    }

    connectionNamesToVr(
        connections,
#ifdef ENABLE_SC
        scs,
#endif
        clients
    );
    vector<WeakConnection> weakConnections = setupWeakConnections(connections, clients);

    BaseMaster *master = NULL;
    string fieldnames = getFieldnames(clients);

#ifdef ENABLE_SC
    if (hasModelExchangeFMUs(clients)) {
        if (scs.size()) {
            fatal("Cannot do ModelExchange and kinematic coupling at the same time currently\n");
        }
#endif
        master = (BaseMaster*)new JacobiMaster(context, clients, weakConnections);
#ifdef ENABLE_SC
    } else {
        Solver *solver = setupConstraintsAndSolver(
            scs,
            clients
        );
        if (solver) {
            solver->setSpookParams(relaxation,compliance,timeStep);
        }
        StrongMaster *sm = new StrongMaster(context, clients, weakConnections, solver, holonomic, executionOrder);
        master = sm;
    }
#endif

    master->zmqControl = command_port > 0;

    if (master->zmqControl > 0) {
        info("Init ZMQ control on port %i\n", command_port);
        char addr[128];
        snprintf(addr, sizeof(addr), "tcp://*:%i", command_port);
        master->rep_socket.bind(addr);

        if (results_port > 0) {
          info("Init ZMQ results on port %i\n", results_port);
          snprintf(addr, sizeof(addr), "tcp://*:%i", results_port);
          push_socket.bind(addr);
        }
    } else if (startPaused) {
        fatal("-Z requires -z\n");
    }

    

    //init
    for (size_t x = 0; x < clients.size(); x++) {
        //set visibility based on command line
        clients[x]->queueMessage(fmi2_import_instantiate2( x < fmuVisibilities.size() ? fmuVisibilities[x] : false));
    }

    master->queueMessage(clients, fmi2_import_setup_experiment(true, relativeTolerance, 0, endTime >= 0, endTime));

    /**
     * From the FMI 2.0 spec:
     *
     * fmi2SetXXX can be called on any variable with variability ≠ "constant"
     * before initialization (before calling fmi2EnterInitializationMode) if
     * • initial = "exact" or "approx" [in order to set the corresponding start value].
     *
     * Since initial can be "exact", "approx" or "calculated" this means any
     * non-constant non-calculated variable is allowed to be set. There is a
     * check inside sendUserParams() making sure the user isn't stupidly
     * trying set calculated parameters.
     */
    sendUserParams(master, clients, resolve_string_params(params, clients));
    master->wait();

    for (FMIClient *client : clients) {
      client->m_fmuState = control_proto::fmu_state_State_initializing;
    }

    master->queueMessage(clients, fmi2_import_enter_initialization_mode());

    /**
     * From the FMI 2.0 spec:
     *
     * fmi2SetXXX can be called on any variable with variability ≠ "constant"
     * during initialization (after calling fmi2EnterInitializationMode and
     * before fmi2ExitInitializationMode is called) if
     * • initial = "exact" [in order to set the corresponding start value], or if
     * • causality = "input" [in order to provide new values for inputs]
     *
     * We probably don't need to send parameters with initial=exact more than
     * once, but it probably doesn't hurt.
     */
    sendUserParams(master, clients, resolve_string_params(params, clients), true);

    map<double, param_map> csvParam = param_mapFromCSV(csv_fmu, clients);

    if (solveLoops) {
      //solve initial algebraic loops
      master->solveLoops();
    }

    master->queueMessage(clients, fmi2_import_exit_initialization_mode());
    master->wait();

    //prepare solver and all that
    master->prepare();

    if (writeSolverFields) {
      fieldnames += master->getFieldNames();
    }

    int step = 0;
    double fsteps = round(endTime / timeStep);
    int nsteps;
    if (fsteps < 0) {
        nsteps = 0;
    } else if (fsteps >= INT_MAX) {
        nsteps = INT_MAX;
    } else {
        nsteps = (int)fsteps;
    }

    /// reduce amount of output if wanted.
    int write_period = (maxSamples>0) ?
      (int) ceil( (double) nsteps / (double) maxSamples) : 1;

    if ((fmigo::globals::fileFormat == csv && useHeadersInCSV) || fmigo::globals::fileFormat == tikz) {
        fprintf(outfile, "%s\n",fieldnames.c_str());
    } else if (matlab_output) {
#ifdef USE_MATIO
        mo = reserveMatlabOutput(clients, maxSamples, nsteps);
#endif
    }

    if (fieldnameFilename.length() > 0) {
        ofstream ofs(fieldnameFilename.c_str());
        ofs << fieldnames;
        ofs << endl;
    }

    if (results_port > 0) {
        pushResults(step, 0, endTime, timeStep, push_socket, master, clients, true);
    }

    for (FMIClient *client : clients) {
      client->m_fmuState = control_proto::fmu_state_State_running;
    }

    //switch to running mode, pause if we should
    master->initing = false;
    master->running = true;
    master->paused = startPaused; 
    master->resetT1();

    fmigo::globals::timer.dont_rotate = false;
    fmigo::globals::timer.rotate("setup");

    //whether to suppress output of the current line
    bool suppress_output = false;

    //run
    while ((endTime < 0 || step < nsteps) && master->running) {
        suppress_output = step % write_period != 0;

        master->t = step * endTime / nsteps;
        master->handleZmqControl();

        if (!master->running) {
            //termination requested
            break;
        }

        if (master->paused) {
            continue;
        }

        if (csvParam.size() > 0) {
            //zero order hold
            auto it = csvParam.upper_bound(master->t);
            if (it != csvParam.begin()) {
                it--;
            }
            sendUserParams(master, clients, it->second);
        }

        if (fmigo::globals::fileFormat != none && !suppress_output) {
            printOutputs(master->t, master, clients, mo, outfile, matlab_output);
        }

        if (realtimeMode) {
            master->waitupT1(timeStep);
        }

        master->runIteration(master->t, timeStep);

        step++;

        if (results_port > 0) {
            pushResults(step, master->t+timeStep, endTime, timeStep, push_socket, master, clients, false);
        }

        if (fmigo::globals::fileFormat != none && !suppress_output) {
            if (!matlab_output) {
                if (writeSolverFields) {
                    master->writeFields(false, outfile);
                }
                fprintf(outfile, "\n");
            }
        }
    }
    fmigo::globals::timer.rotate("pre_shutdown");

    if (fmigo::globals::fileFormat != none && !suppress_output) {
        printOutputs(endTime, master, clients, mo, outfile, matlab_output);

        if (!matlab_output) {
            if (writeSolverFields) {
                master->writeFields(true, outfile);
            }

            fprintf(outfile, "\n");
        }
    }

#ifdef USE_MATIO
    if (matlab_output) {
        writeMatlabOutput(clients, mo, outFilePath);
    }
#endif

    for (FMIClient *client : clients) {
      client->terminate();
    }

    //clean up
    delete master;

    for (size_t x = 0; x < clients.size(); x++) {
        delete clients[x];
    }

#ifdef USE_MPI
    //send shutdown message (tag = 1)
    for (int x = 1; x < world_size; x++) {
        MPI_Send(NULL, 0, MPI_CHAR, x, 1, MPI_COMM_WORLD);
    }
#endif

    if (outfile != stdout) {
        fclose(outfile);
    }
    fflush(stdout);
    fflush(stderr);

#ifdef FMIGO_PRINT_TIMINGS
    fmigo::globals::timer.rotate("shutdown");
    set<string> onetime = {
      "setup",
      "pre_shutdown",
      "shutdown",
      "wait", //not actually onetime, but don't want time spent waiting to be part of percentage
      "MPI_Send",
      "zmq::socket::send",
    };
    double total = 0, total2 = 0;

    for (auto kv : fmigo::globals::timer.m_durations) {
      if (!onetime.count(kv.first)) {
        total += kv.second;
      } else {
        info("master: %20s: %10" PRIi64 " µs\n", kv.first.c_str(), (int64_t)kv.second);
      }
      total2 += kv.second;
    }

    for (auto kv : fmigo::globals::timer.m_durations) {
      if (!onetime.count(kv.first)) {
        info("master: %20s: %10" PRIi64 " µs (%5.2lf%%)\n", kv.first.c_str(), (int64_t)kv.second, 100*kv.second/total);
      }
    }
    info("master:                Total: %10.2lf seconds (%.1lf ms user time)\n", total2 * 1e-6, total * 1e-3);
#endif

    } catch (zmq::error_t& e) {
      fatal("zmq::error_t in %s: %s\n", argv[0], e.what());
    }

#ifdef USE_MPI
    // finalize last of all, per mpich documentation (https://www.mpich.org/static/docs/v3.1/www3/MPI_Finalize.html):
    //
    //   The number of processes running after this routine is called is undefined;
    //   it is best not to perform much more than a return rc after calling MPI_Finalize.
    //
    // OpenMPI's docs do not say anything like this (https://docs.open-mpi.org/en/v5.0.0/man-openmpi/man3/MPI_Finalize.3.html)
    // but it probably doesn't hurt to do as mpich devs suggest
    MPI_Finalize();
#endif
    return 0;
}
