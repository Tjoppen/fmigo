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
#include <sc/BallJointConstraint.h>
#include <sc/LockConstraint.h>
#include <sc/ShaftConstraint.h>
#include "master/StrongMaster.h"
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <fstream>
#include "control.pb.h"
#include "master/globals.h"

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

#define toVR(type, index)                                   \
  vrFromKeyName(clients[it->type##FMU],it->vrORname[index])

static Solver* setupConstraintsAndSolver(vector<strongconnection> strongConnections, vector<FMIClient*> clients) {
    if (strongConnections.size() == 0) {
        return NULL;
    }
    Solver *solver = new Solver;

    for (auto it = strongConnections.begin(); it != strongConnections.end(); it++) {
        //NOTE: this leaks memory, but I don't really care since it's only setup
        Constraint *con;
        char t = tolower(it->type[0]);

        switch (t) {
        case 'b':
        case 'l':
        {
            if (it->vrORname.size() != 38) {
                fatal("Bad %s specification: need 38 VRs ([XYZpos + XYZacc + XYZforce + Quat + XYZrotAcc + XYZtorque] x 2), got %zu\n",
                        t == 'b' ? "ball joint" : "lock", it->vrORname.size());
            }

            StrongConnector *scA = findOrCreateBallLockConnector(clients[it->fromFMU],
                                       toVR(from,0), toVR(from,1), toVR(from,2),
                                       toVR(from,3), toVR(from,4), toVR(from,5),
                                       toVR(from,6), toVR(from,7), toVR(from,8),
                                       toVR(from,9), toVR(from,10), toVR(from,11), toVR(from,12),
                                       toVR(from,13), toVR(from,14), toVR(from,15),
                                       toVR(from,16), toVR(from,17), toVR(from,18) );

            StrongConnector *scB = findOrCreateBallLockConnector(clients[it->toFMU],
                                       toVR(to,19), toVR(to,20), toVR(to,21),
                                       toVR(to,22), toVR(to,23), toVR(to,24),
                                       toVR(to,25), toVR(to,26), toVR(to,27),
                                       toVR(to,28), toVR(to,29), toVR(to,30), toVR(to,31),
                                       toVR(to,32), toVR(to,33), toVR(to,34),
                                       toVR(to,35), toVR(to,36), toVR(to,37) );

            con = t == 'b' ? new BallJointConstraint(scA, scB, Vec3(), Vec3())
                           : new LockConstraint(scA, scB, Vec3(), Vec3(), Quat(), Quat());

            break;
        }
        case 's':
        {
            if (it->vrORname.size() != 8) {
                fatal("Bad shaft specification: need 8 VRs ([shaft angle + angular velocity + angular acceleration + torque] x 2)\n");
            }

            StrongConnector *scA = findOrCreateShaftConnector(clients[it->fromFMU],
                                       toVR(from,0), toVR(from,1), toVR(from,2), toVR(from,3));

            StrongConnector *scB = findOrCreateShaftConnector(clients[it->toFMU],
                                       toVR(to,4), toVR(to,5), toVR(to,6), toVR(to,7));

            con = new ShaftConstraint(scA, scB);
            break;
        }
        default:
            fatal("Unknown strong connector type: %s\n", it->type.c_str());
        }

        solver->addConstraint(con);
    }

    for (auto it = clients.begin(); it != clients.end(); it++) {
        solver->addSlave(*it);
   }

   return solver;
}

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
        oss << client->getSpaceSeparatedFieldNames(prefix.str());
    }
    return oss.str();
}

template<typename RFType, typename From> void addVectorToRepeatedField(RFType* rf, const From& from) {
    for (auto f : from) {
        *rf->Add() = f;
    }
}

static void printOutputs(double t, BaseMaster *master, vector<FMIClient*>& clients) {
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

    fprintf(fmigo::globals::outfile, "%+.16le", t);
    for (size_t x = 0; x < clients.size(); x++) {
        FMIClient *client = clients[x];
        for (const variable& out : client->getOutputs()) {
            switch (out.type) {
            case fmi2_base_type_real:
                fprintf(fmigo::globals::outfile, "%c%+.16le", separator, client->getReal(out.vr));
                break;
            case fmi2_base_type_int:
                fprintf(fmigo::globals::outfile, "%c%i", separator, client->getInt(out.vr));
                break;
            case fmi2_base_type_bool:
                fprintf(fmigo::globals::outfile, "%c%i", separator, client->getBool(out.vr));
                break;
            case fmi2_base_type_str: {
                ostringstream oss;
                string s = client->getString(out.vr);
                for(char c: s){
                    switch (c){
                    case '"': oss << "\"\""; break;
                    default: oss << c;
                    }
                }
                fprintf(fmigo::globals::outfile, "%c\"%s\"", separator, oss.str().c_str());
                break;
            }
            case fmi2_base_type_enum:
                fatal("Enum outputs not allowed for now\n");
            }
        }
    }
}


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
        addVectorToRepeatedField(fmu_res->mutable_ints()->mutable_values(),     cv.first->getReals(cv.second[fmi2_base_type_int]));
        addVectorToRepeatedField(fmu_res->mutable_bools()->mutable_vrs(),       cv.second[fmi2_base_type_bool]);
        addVectorToRepeatedField(fmu_res->mutable_bools()->mutable_values(),    cv.first->getReals(cv.second[fmi2_base_type_bool]));
        addVectorToRepeatedField(fmu_res->mutable_strings()->mutable_vrs(),     cv.second[fmi2_base_type_str]);
        addVectorToRepeatedField(fmu_res->mutable_strings()->mutable_values(),  cv.first->getReals(cv.second[fmi2_base_type_str]));
    }

    string str = results.SerializeAsString();
    zmq::message_t rep(str.length());
    memcpy(rep.data(), str.data(), str.length());
    push_socket.send(rep);
}

static int connectionNamesToVr(std::vector<connection> &connections,
                        vector<strongconnection> &strongConnections,
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

    for(size_t i = 0; i < strongConnections.size(); i++){
      size_t j = 0;
      if(strongConnections[i].vrORname.size()%2 != 0){
        fatal("strong connection needs even number of connections for fmu0 and fmu1\n");
      }
    }
  return 0;
}

#ifdef USE_MPI
static void run_server(string fmuPath, string hdf5Filename) {
    FMIServer server(fmuPath, hdf5Filename);

    for (;;) {
        int rank, tag;
        server.m_timer.rotate("pre_wait");
        std::string recv_str = mpi_recv_string(MPI_ANY_SOURCE, &rank, &tag);
        server.m_timer.rotate("wait");

        //shutdown command?
        if (tag == 1) {
            break;
        }

        //let Server handle packet, send reply back to master
        std::string str = server.clientData(recv_str.c_str(), recv_str.length());
        if (str.length() > 0) {
          server.m_timer.rotate("pre_send");
          MPI_Send((void*)str.c_str(), str.length(), MPI_CHAR, rank, tag, MPI_COMM_WORLD);
          server.m_timer.rotate("send");
        }
    }

    MPI_Finalize();
}
#endif

int main(int argc, char *argv[] ) {
    //count everything from here to before the main loop into "setup"
    fmigo::globals::timer.dont_rotate = true;

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
    double relaxation = 4,
           compliance = 0;
    vector<string> fmuURIs;
    vector<connection> connections;
    vector<deque<string> > params;
    char csv_separator = ',';
    string outFilePath = "";
    METHOD method = jacobi;
    int realtimeMode = 0;
    vector<int> stepOrder;
    vector<int> fmuVisibilities;
    vector<strongconnection> scs;
    string fieldnameFilename;
    bool holonomic = true;
    bool useHeadersInCSV = false;
    int command_port = 0, results_port = 0;
    bool startPaused = false, solveLoops = false;
    fmigo_csv_fmu csv_fmu;
    string hdf5Filename;

    parseArguments(
            argc, argv, &fmuURIs, &connections, &params, &endTime, &timeStep,
            &fmigo_loglevel, &csv_separator, &outFilePath, &fmigo::globals::fileFormat,
            &method, &realtimeMode, &stepOrder, &fmuVisibilities,
            &scs, &hdf5Filename, &fieldnameFilename, &holonomic, &compliance,
            &command_port, &results_port, &startPaused, &solveLoops, &useHeadersInCSV, &csv_fmu
    );

    bool zmqControl = command_port > 0 && results_port > 0;

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
          run_server(fmuURIs[0], hdf5Filename);
        } else {
          run_server(fmuURIs[world_rank-1], hdf5Filename);
        }
        return 0;
    }
    //world_rank == 0 below
#endif

    if (outFilePath.size()) {
        fmigo::globals::outfile = fopen(outFilePath.c_str(), "w");

        if (!fmigo::globals::outfile) {
            fatal("Failed to open %s\n", outFilePath.c_str());
        }
    }

    zmq::context_t context(1);
    zmq::socket_t push_socket(context, ZMQ_PUSH);

#ifdef USE_MPI
    vector<FMIClient*> clients = setupClients(world_size-1);
#else
    //without this the maximum number of clients tops out at 300 on Linux,
    //around 63 on Windows (according to Web searches)
#ifdef ZMQ_MAX_SOCKETS
    zmq_ctx_set((void *)context, ZMQ_MAX_SOCKETS, fmuURIs.size() + (zmqControl ? 2 : 0));
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

    connectionNamesToVr(connections,scs,clients);
    vector<WeakConnection> weakConnections = setupWeakConnections(connections, clients);
    Solver *solver = setupConstraintsAndSolver(scs, clients);

    BaseMaster *master = NULL;
    string fieldnames = getFieldnames(clients);

    if (scs.size()) {
        if (method != jacobi) {
            fatal("Can only do Jacobi stepping for weak connections when also doing strong coupling\n");
        }

        solver->setSpookParams(relaxation,compliance,timeStep);
        StrongMaster *sm = new StrongMaster(context, clients, weakConnections, solver, holonomic);
        master = sm;
        fieldnames += sm->getForceFieldnames();
    } else {
        master = (method == gs) ?           (BaseMaster*)new GaussSeidelMaster(context, clients, weakConnections, stepOrder) :
                                            (BaseMaster*)new JacobiMaster(context, clients, weakConnections);
    }

    if (zmqControl) {
        info("Init zmq control on ports %i and %i\n", command_port, results_port);
        char addr[128];
        snprintf(addr, sizeof(addr), "tcp://*:%i", command_port);
        master->rep_socket.bind(addr);
        snprintf(addr, sizeof(addr), "tcp://*:%i", results_port);
        push_socket.bind(addr);
    } else if (startPaused) {
        fatal("-Z requires -z\n");
    }

    master->zmqControl = zmqControl;

    if (useHeadersInCSV || fmigo::globals::fileFormat == tikz) {
        fprintf(fmigo::globals::outfile, "%s\n",fieldnames.c_str());
    }

    if (fieldnameFilename.length() > 0) {
        ofstream ofs(fieldnameFilename.c_str());
        ofs << fieldnames;
        ofs << endl;
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

    //double t = 0;
    int step = 0;
    int nsteps = (int)round(endTime / timeStep);

    if (zmqControl) {
        pushResults(step, 0, endTime, timeStep, push_socket, master, clients, true);
    }

    for (FMIClient *client : clients) {
      client->m_fmuState = control_proto::fmu_state_State_running;
    }

    //switch to running mode, pause if we should
    master->initing = false;
    master->running = true;
    master->paused = startPaused;

    fmigo::globals::timer.dont_rotate = false;
    fmigo::globals::timer.rotate("setup");

    #ifdef WIN32
        LARGE_INTEGER freq, t1;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&t1);
    #else
        timeval t1;
        gettimeofday(&t1, NULL);
    #endif

    //run
    while ((endTime < 0 || step < nsteps) && master->running) {
        double t = step * endTime / nsteps;

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
            auto it = csvParam.upper_bound(t);
            if (it != csvParam.begin()) {
                it--;
            }
            sendUserParams(master, clients, it->second);
        }

        if (realtimeMode) {
            double t_wall;

            //delay loop
            for (;;) {
#ifdef WIN32
                LARGE_INTEGER t2;
                QueryPerformanceCounter(&t2);
                t_wall = (t2.QuadPart - t1.QuadPart) / (double)freq.QuadPart;

                if (t_wall >= t)
                    break;

                Yield();
#else
                timeval t2;
                gettimeofday(&t2, NULL);

                t_wall = ((double)t2.tv_sec - t1.tv_sec) + 1.0e-6 * ((double)t2.tv_usec - t1.tv_usec);
                int us = 1000000 * (t - t_wall);

                if (us <= 0)
                    break;

                usleep(us);
#endif
            }
        }

        if (fmigo::globals::fileFormat != none) {
            printOutputs(t, master, clients);
        }

        master->runIteration(t, timeStep);

        step++;

        if (zmqControl) {
            pushResults(step, t+timeStep, endTime, timeStep, push_socket, master, clients, false);
        }

        if (fmigo::globals::fileFormat != none) {
            fprintf(fmigo::globals::outfile, "\n");
        }
    }
    fmigo::globals::timer.rotate("pre_shutdown");

    if (fmigo::globals::fileFormat != none) {
      printOutputs(endTime, master, clients);
      char separator = fmigo::globals::getSeparator();

      //finish off with zeroes for any extra forces
      int n = master->getNumForceOutputs();
      for (int i = 0; i < n; i++) {
          fprintf(fmigo::globals::outfile, "%c0", separator);
      }

      fprintf(fmigo::globals::outfile, "\n");
    }

    for (FMIClient *client : clients) {
      client->terminate();
    }

    //clean up
    delete master;

    for (size_t x = 0; x < clients.size(); x++) {
        delete clients[x];
    }

#ifdef USE_MPI
    //send shutdown message (tag = 1), finalize
    for (int x = 1; x < world_size; x++) {
        MPI_Send(NULL, 0, MPI_CHAR, x, 1, MPI_COMM_WORLD);
    }
    MPI_Finalize();
#endif

    if (fmigo::globals::outfile != stdout) {
        fclose(fmigo::globals::outfile);
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

    } catch (zmq::error_t e) {
      fatal("zmq::error_t in %s: %s\n", argv[0], e.what());
    }

    return 0;
}
