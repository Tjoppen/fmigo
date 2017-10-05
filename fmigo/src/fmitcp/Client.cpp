#include "Client.h"
#include "fmitcp-common.h"
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#ifdef USE_MPI
#include "common/mpi_tools.h"
#endif
#include "common/common.h"
#include "master/globals.h"
#include "master/BaseMaster.h"
#include <fmitcp/serialize.h>

using namespace std;
using namespace fmitcp;
using namespace fmitcp_proto;

template<typename T, typename R> vector<T> repeated_to_vector(R *repeated_vector) {
    vector<T> values;
    for(int i=0; i < repeated_vector().size(); i++)
        values.push_back(repeated_vector(i));
    return values;
}

template<typename T, typename R> vector<T> values_to_vector(R &r) {
    return vector<T>(r.values().data(), &r.values().data()[r.values_size()]);
}

//fmi_status_t and jm_status_enu_t have different "ok" values
static bool statusIsOK(fmitcp_proto::jm_status_enu_t jm) {
    return jm == fmitcp_proto::jm_status_success;
}

static bool statusIsOK(fmitcp_proto::fmi2_status_t fmi2) {
    return fmi2 == fmitcp_proto::fmi2_status_ok;
}

template<typename T, typename R> void handle_get_value_res(Client *c, R &r, set<int>& outgoing, unordered_map<int,T>& dest) {
  if (!statusIsOK(r.status())) {
      debug("< %s(values=...,status=%d)\n",r.GetTypeName().c_str(), r.status());
      fatal("FMI call %s failed with status=%d\nMaybe a connection or <Output> was specified incorrectly?",
          r.GetTypeName().c_str(), r.status());
  }

  size_t x = 0;
  for (int vr : outgoing) {
    dest.insert(make_pair(vr, r.values(x)));
    x++;
  }
  outgoing.clear();

}

void Client::clientData(const char* data, long size){
    fmitcp_message_Type type = parseType(data, size);

    data += 2;
    size -= 2;

    if ((m_master ? m_master->m_pendingRequests : m_pendingRequests) == 0) {
        fatal("Got response while m_pendingRequests = 0\n");
    }
    if (m_master) {
        m_master->m_pendingRequests--;
    } else {
        m_pendingRequests--;
    }

//useful for providing hints in case of failure
#define CHECK_WITH_STR(type, str) {\
        type##_res r;\
        r.ParseFromArray(data, size);\
        debug("< "#type"_res(status=%d)\n",r.status()); \
        if (!statusIsOK(r.status())) {\
            fatal("FMI call "#type"() failed with status=%d\n" str, r.status()); \
        }\
        on_##type##_res(r.status());\
    }

#define NORMAL_CASE(type) CHECK_WITH_STR(type, "")
#define NOSTAT_CASE(type) {\
        type##_res r;\
        r.ParseFromArray(data, size);\
        debug("< "#type"_res()\n");\
        on_##type##_res();\
    }

#define CLIENT_VALUE_CASE(type){                                        \
    type##_res r;                                                       \
    r.ParseFromArray(data, size);                                       \
    std::ostringstream stream;                                          \
    stream << "< "#type"_res(value=" << r.value() << ")\n";             \
    debug( stream.str().c_str());            \
    on_##type##_res( r.value());                         \
    }

#define SETX_HINT "Maybe a parameter or a connection was specified incorrectly?\n"
    // Check type and run the corresponding event handler
    switch (type) {
    case type_fmi2_import_get_version_res: {
        fmi2_import_get_version_res r; r.ParseFromArray(data, size);
        debug("< fmi2_import_get_version_res(version=%s)\n", r.version().c_str());
        on_fmi2_import_get_version_res(r.version());

        break;
    }
    case type_fmi2_import_set_debug_logging_res:            NORMAL_CASE(fmi2_import_set_debug_logging); break;
    case type_fmi2_import_instantiate_res:                  NORMAL_CASE(fmi2_import_instantiate); break;
    case type_fmi2_import_free_instance_res:                NOSTAT_CASE(fmi2_import_free_instance); break;
    case type_fmi2_import_setup_experiment_res:             NORMAL_CASE(fmi2_import_setup_experiment); break;
    case type_fmi2_import_enter_initialization_mode_res:    NORMAL_CASE(fmi2_import_enter_initialization_mode); break;
    case type_fmi2_import_exit_initialization_mode_res:     NORMAL_CASE(fmi2_import_exit_initialization_mode); break;
    case type_fmi2_import_terminate_res:                    NORMAL_CASE(fmi2_import_terminate); break;
    case type_fmi2_import_reset_res:                        NORMAL_CASE(fmi2_import_reset); break;
    case type_fmi2_import_get_real_res: {
        fmi2_import_get_real_res r;
        r.ParseFromArray(data, size);
        handle_get_value_res(this, r, m_outgoing_reals, m_reals);
        break;
    }
    case type_fmi2_import_get_integer_res: {
        fmi2_import_get_integer_res r;
        r.ParseFromArray(data, size);
        handle_get_value_res(this, r, m_outgoing_ints, m_ints);
        break;
    }
    case type_fmi2_import_get_boolean_res: {
        fmi2_import_get_boolean_res r;
        r.ParseFromArray(data, size);
        handle_get_value_res(this, r, m_outgoing_bools, m_bools);
        break;
    }
    case type_fmi2_import_get_string_res: {
        fmi2_import_get_string_res r;
        r.ParseFromArray(data, size);
        handle_get_value_res(this, r, m_outgoing_strings, m_strings);
        break;
    }
    case type_fmi2_import_set_real_res:                     CHECK_WITH_STR(fmi2_import_set_real, SETX_HINT); break;
    case type_fmi2_import_set_integer_res:                  CHECK_WITH_STR(fmi2_import_set_integer, SETX_HINT); break;
    case type_fmi2_import_set_boolean_res:                  CHECK_WITH_STR(fmi2_import_set_boolean, SETX_HINT); break;
    case type_fmi2_import_set_string_res:                   CHECK_WITH_STR(fmi2_import_set_string, SETX_HINT); break;
    case type_fmi2_import_get_fmu_state_res: {
        fmi2_import_get_fmu_state_res r; r.ParseFromArray(data, size);
        debug("< fmi2_import_get_fmu_state_res(stateId=%d,status=%d)\n", r.stateid(), r.status());
        on_fmi2_import_get_fmu_state_res(r.stateid(),r.status());

        break;
    }
    case type_fmi2_import_set_fmu_state_res:                NORMAL_CASE(fmi2_import_set_fmu_state); break;
    case type_fmi2_import_free_fmu_state_res: {
        debug("This command is TODO\n");
        break;
    }
    case type_fmi2_import_set_free_last_fmu_state_res: {
        break;
    }
    // case type_fmi2_import_serialized_fmu_state_size_res: {
    //     debug("This command is TODO\n");
    //     break;
    // }
    // case type_fmi2_import_serialize_fmu_state_res: {
    //     debug("This command is TODO\n");
    //     break;
    // }
    // case type_fmi2_import_de_serialize_fmu_state_res: {
    //     debug("This command is TODO\n");
    //     break;
    // }
    case type_fmi2_import_get_directional_derivative_res: {
        fmi2_import_get_directional_derivative_res r; r.ParseFromArray(data, size);
        std::vector<double> dz;
        for(int i=0; i<r.dz_size(); i++)
            dz.push_back(r.dz(i));
        debug("< fmi2_import_get_directional_derivative_res(dz=...,status=%d)\n", r.status());
        on_fmi2_import_get_directional_derivative_res(dz,r.status());

        break;

      /* Model exchange */
    }case type_fmi2_import_enter_event_mode_res:            NORMAL_CASE(fmi2_import_enter_event_mode); break;
    case type_fmi2_import_new_discrete_states_res:{

        fmi2_import_new_discrete_states_res r; r.ParseFromArray(data, size);
#ifdef DEBUG
        ::fmi2_event_info_t eventInfo = protoEventInfoToFmi2EventInfo(r.eventinfo());

        debug("< fmi2_import_new_discrete_states_res(newDiscreteStatesNeeded=%d, terminateSimulation=%d, nominalsOfContinuousStatesChanged=%d, valuesOfContinuousStatusChanged=%d, nextEventTimeDefined=%d, nextEventTime=%f )\n",
                     eventInfo.newDiscreteStatesNeeded,
                     eventInfo.terminateSimulation,
                     eventInfo.nominalsOfContinuousStatesChanged,
                     eventInfo.valuesOfContinuousStatesChanged,
                     eventInfo.nextEventTimeDefined,
                     eventInfo.nextEventTime
                     );
#endif

        on_fmi2_import_new_discrete_states_res(r.eventinfo());

        break;


    }case type_fmi2_import_enter_continuous_time_mode_res:  NORMAL_CASE(fmi2_import_enter_continuous_time_mode); break;
    case type_fmi2_import_completed_integrator_step_res: {
        fmi2_import_completed_integrator_step_res r; r.ParseFromArray(data, size);
        debug("< fmi2_import_completed_integrator_step_res(mid=%d,callEventUpdate=%d,status=%d)\n", r.calleventupdate(), r.status());
        on_fmi2_import_completed_integrator_step_res(r.calleventupdate(),r.status());

        break;
    }
    case type_fmi2_import_set_time_res:                     NORMAL_CASE(fmi2_import_set_time); break;
    case type_fmi2_import_set_continuous_states_res:        NORMAL_CASE(fmi2_import_set_continuous_states); break;
    case type_fmi2_import_get_event_indicators_res: {
        debug("This command is NOT TESTED\n");
        fmi2_import_get_event_indicators_res r; r.ParseFromArray(data, size);
        std::vector<double> z;
        for(int i=0; i<r.z_size(); i++)
            z.push_back(r.z(i));

        on_fmi2_import_get_event_indicators_res(z,r.status());
        debug("< fmi2_import_get_event_indicators_res(mid=%d, status=%d)\n", r.status());

        break;
    }
    case type_fmi2_import_get_continuous_states_res: {
        debug("This command is NOT TESTED\n");
        fmi2_import_get_continuous_states_res r; r.ParseFromArray(data, size);
        std::vector<double> x;
        for(int i=0; i<r.x_size(); i++)
            x.push_back(r.x(i));
        on_fmi2_import_get_continuous_states_res(x,r.status());
        debug("< fmi2_import_get_continuous_states_res(mid=%d, states=%d)\n", r.status());
        break;
    }
    case type_fmi2_import_get_derivatives_res: {
        debug("This command is NOT TESTED\n");
        fmi2_import_get_derivatives_res r; r.ParseFromArray(data, size);
        std::vector<double> derivatives;
        for(int i=0; i<r.derivatives_size(); i++)
            derivatives.push_back(r.derivatives(i));
        on_fmi2_import_get_derivatives_res(derivatives,r.status());
        debug("< fmi2_import_get_derivatives_res(mid=%d, status=%d)\n", r.status());
        break;
    }
    case type_fmi2_import_get_nominal_continuous_states_res: {
        debug("This command is NOT TESTED\n");
        fmi2_import_get_nominal_continuous_states_res r; r.ParseFromArray(data, size);
        std::vector<double> x;
        for(int i=0; i<r.nominal_size(); i++)
            x.push_back(r.nominal(i));
        on_fmi2_import_get_nominal_continuous_states_res(x,r.status());
        debug("< fmi2_import_get_nominal_continuous_states_res(mid=%d, states=%d)\n", r.status());
        break;
    }
      /* Co-simulation */
    case type_fmi2_import_set_real_input_derivatives_res:   NORMAL_CASE(fmi2_import_set_real_input_derivatives); break;
    case type_fmi2_import_get_real_output_derivatives_res: {
        fmi2_import_get_real_output_derivatives_res r; r.ParseFromArray(data, size);
        debug("< fmi2_import_get_real_output_derivatives_res(mid=%d,status=%d,values=...)\n",r.status());
        on_fmi2_import_get_real_output_derivatives_res(r.status(),values_to_vector<double>(r));
        break;
    }
    case type_fmi2_import_do_step_res:                      NORMAL_CASE(fmi2_import_do_step); break;
    case type_fmi2_import_cancel_step_res:                  NORMAL_CASE(fmi2_import_cancel_step); break;
    case type_fmi2_import_get_status_res:                   CLIENT_VALUE_CASE(fmi2_import_get_status); break;
    case type_fmi2_import_get_real_status_res:              CLIENT_VALUE_CASE(fmi2_import_get_real_status); break;
    case type_fmi2_import_get_integer_status_res:           CLIENT_VALUE_CASE(fmi2_import_get_integer_status); break;
    case type_fmi2_import_get_boolean_status_res:           CLIENT_VALUE_CASE(fmi2_import_get_boolean_status); break;
    case type_fmi2_import_get_string_status_res:            CLIENT_VALUE_CASE(fmi2_import_get_string_status); break;
    case type_fmi2_kinematic_res: {
        last_kinematic.ParseFromArray(data, size);
        break;
    }
    case type_get_xml_res: {

        get_xml_res r; r.ParseFromArray(data, size);
        debug("< get_xml_res(mid=%d,xml=...)\n");
        on_get_xml_res( r.loglevel(), r.xml());

        break;
    }
    default:
        error("Message type not recognized: %d!\n",type);
        break;
    }
}

#ifdef USE_MPI
Client::Client(int world_rank) : world_rank(world_rank) {
#else
Client::Client(zmq::context_t &context, string uri) : m_socket(context, ZMQ_DEALER) {
    messages = 0;
    debug("connecting to %s\n", uri.c_str());
    m_socket.connect(uri.c_str());
    debug("connected\n");
#endif
    m_pendingRequests = 0;
    m_master = NULL;
    GOOGLE_PROTOBUF_VERIFY_VERSION;
}

Client::~Client(){
    if (m_pendingRequests) {
        fatal("Had m_pendingRequests=%i in Client::~Client()\n", m_pendingRequests);
    }
    google::protobuf::ShutdownProtobufLibrary();
}

void Client::sendMessage(std::string s){
    fmigo::globals::timer.rotate("pre_sendMessage");
    messages++;
    if (m_master) {
        m_master->m_pendingRequests++;
    } else {
        m_pendingRequests++;
    }
#ifdef USE_MPI
    MPI_Send((void*)s.c_str(), s.length(), MPI_CHAR, world_rank, 0, MPI_COMM_WORLD);
    fmigo::globals::timer.rotate("MPI_Send");
#else
    //ZMQ_DEALERs must send two-part messages with the first part being zero-length
    zmq::message_t zero(0);
    m_socket.send(zero, ZMQ_SNDMORE);

    zmq::message_t msg(s.size());
    memcpy(msg.data(), s.data(), s.size());
    m_socket.send(msg);
    fmigo::globals::timer.rotate("zmq::socket::send");
#endif
}

void Client::sendMessageBlocking(std::string s) {
    sendMessage(s);
    receiveAndHandleMessage();
}

void Client::receiveAndHandleMessage() {
    fmigo::globals::timer.rotate("pre_wait");
#ifdef USE_MPI
    std::string str = mpi_recv_string(world_rank, NULL, NULL);
    fmigo::globals::timer.rotate("wait");
    clientData(str.c_str(), str.length());
#else
    //expect to recv a delimiter
    zmq::message_t delim;
    m_socket.recv(&delim);

    if (delim.size() != 0) {
        fatal("Expected to recv zero-length delimiter, got %zu B instead\n", delim.size());
    }

    zmq::message_t msg;
    m_socket.recv(&msg);
    fmigo::globals::timer.rotate("wait");
    clientData(static_cast<char*>(msg.data()), msg.size());
#endif
}

void Client::deleteCachedValues() {
  m_reals.clear();
  m_ints.clear();
  m_bools.clear();
  m_strings.clear();
}

template<typename T> void queueFoo(const vector<int>& vrs,
                                   const unordered_map<int,T>& values,
                                   set<int>& outgoing) {
  //only queue values which we haven't seen yet
  for (int vr : vrs) {
    auto it = values.find(vr);
    if (it == values.end()) {
      outgoing.insert(vr);
    }
  }
}

void Client::queueReals(const vector<int>& vrs) {
  queueFoo(vrs, m_reals, m_outgoing_reals);
}
void Client::queueInts(const vector<int>& vrs) {
  queueFoo(vrs, m_ints, m_outgoing_ints);
}
void Client::queueBools(const vector<int>& vrs) {
  queueFoo(vrs, m_bools, m_outgoing_bools);
}
void Client::queueStrings(const vector<int>& vrs) {
  queueFoo(vrs, m_strings, m_outgoing_strings);
}

static vector<int> setToVector(const set<int> &s) {
  vector<int> ret;
  ret.reserve(s.size());
  for (int v : s) {
    ret.push_back(v);
  }
  return ret;
}

void Client::sendValueRequests() {
  if (m_outgoing_reals.size()) {
    sendMessage(fmitcp::serialize::fmi2_import_get_real(setToVector(m_outgoing_reals)));
  }
  if (m_outgoing_ints.size()) {
    sendMessage(fmitcp::serialize::fmi2_import_get_integer(setToVector(m_outgoing_ints)));
  }
  if (m_outgoing_bools.size()) {
    sendMessage(fmitcp::serialize::fmi2_import_get_boolean(setToVector(m_outgoing_bools)));
  }
  if (m_outgoing_strings.size()) {
    sendMessage(fmitcp::serialize::fmi2_import_get_string(setToVector(m_outgoing_strings)));
  }
}

template<typename T> vector<T> getFoo(const vector<int>& vrs,
                                      const unordered_map<int,T>& values) {
  vector<T> ret;
  for (int vr : vrs) {
    auto it = values.find(vr);
    if (it == values.end()) {
      fatal("VR %i was not requested\n", vr);
    }
    ret.push_back(it->second);
  }
  return ret;
}

vector<double> Client::getReals(const vector<int>& vrs) const {
  return getFoo(vrs, m_reals);
}
vector<int> Client::getInts(const vector<int>& vrs) const {
  return getFoo(vrs, m_ints);
}
vector<bool> Client::getBools(const vector<int>& vrs) const {
  return getFoo(vrs, m_bools);
}
vector<string> Client::getStrings(const vector<int>& vrs) const {
  return getFoo(vrs, m_strings);
}

double Client::getReal(int vr) const {
  return getReals(std::vector<int>(1, vr))[0];
}
int Client::getInt(int vr) const {
  return getInts(std::vector<int>(1, vr))[0];
}
bool Client::getBool(int vr) const {
  return getBools(std::vector<int>(1, vr))[0];
}
string Client::getString(int vr) const {
  return getStrings(std::vector<int>(1, vr))[0];
}
