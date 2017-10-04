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

template<typename T, typename R> deque<T> values_to_deque(R &r) {
    return deque<T>(r.values().data(), &r.values().data()[r.values_size()]);
}

template<> deque<string> values_to_deque<string,fmi2_import_get_string_res> (fmi2_import_get_string_res &r) {
    deque<string> values;
    for(int i=0; i<r.values_size(); i++)
        values.push_back(r.values(i));
    return values;
}

//fmi_status_t and jm_status_enu_t have different "ok" values
static bool statusIsOK(fmitcp_proto::jm_status_enu_t jm) {
    return jm == fmitcp_proto::jm_status_success;
}

static bool statusIsOK(fmitcp_proto::fmi2_status_t fmi2) {
    return fmi2 == fmitcp_proto::fmi2_status_ok;
}

template<typename T, typename R> void handle_get_value_res(Client *c, void (Client::*callback)(const deque<T>&,fmitcp_proto::fmi2_status_t), R &r) {
    std::deque<T> values = values_to_deque<T>(r);
    if (!statusIsOK(r.status())) {
        debug("< %s(values=...,status=%d)\n",r.GetTypeName().c_str(), r.status());
        fatal("FMI call %s failed with status=%d\nMaybe a connection or <Output> was specified incorrectly?",
            r.GetTypeName().c_str(), r.status());
    }
    (c->*callback)(values,r.status());
}

void Client::clientData(const char* data, long size){
    fmitcp_message_Type type = parseType(data, size);

    data += 2;
    size -= 2;

    if (m_pendingRequests == 0) {
        fatal("Got response while m_pendingRequests = 0\n");
    }
    m_pendingRequests--;

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
        handle_get_value_res(this, &Client::on_fmi2_import_get_real_res, r);
        break;
    }
    case type_fmi2_import_get_integer_res: {
        fmi2_import_get_integer_res r;
        r.ParseFromArray(data, size);
        handle_get_value_res(this, &Client::on_fmi2_import_get_integer_res, r);
        break;
    }
    case type_fmi2_import_get_boolean_res: {
        fmi2_import_get_boolean_res r;
        r.ParseFromArray(data, size);
        handle_get_value_res(this, &Client::on_fmi2_import_get_boolean_res, r);
        break;
    }
    case type_fmi2_import_get_string_res: {
        fmi2_import_get_string_res r;
        r.ParseFromArray(data, size);
        handle_get_value_res(this, &Client::on_fmi2_import_get_string_res, r);
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
    GOOGLE_PROTOBUF_VERIFY_VERSION;
}

Client::~Client(){
    google::protobuf::ShutdownProtobufLibrary();
}

void Client::sendMessage(std::string s){
    messages++;
    m_pendingRequests++;
#ifdef USE_MPI
    MPI_Send((void*)s.c_str(), s.length(), MPI_CHAR, world_rank, 0, MPI_COMM_WORLD);
#else
    //ZMQ_DEALERs must send two-part messages with the first part being zero-length
    zmq::message_t zero(0);
    m_socket.send(zero, ZMQ_SNDMORE);

    zmq::message_t msg(s.size());
    memcpy(msg.data(), s.data(), s.size());
    m_socket.send(msg);
#endif
}

void Client::sendMessageBlocking(std::string s) {
    sendMessage(s);
    receiveAndHandleMessage();
}

void Client::receiveAndHandleMessage() {
#ifdef USE_MPI
    std::string str = mpi_recv_string(world_rank, NULL, NULL);
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
    clientData(static_cast<char*>(msg.data()), msg.size());
#endif
}

size_t Client::getNumPendingRequests() const {
    return m_pendingRequests;
}

