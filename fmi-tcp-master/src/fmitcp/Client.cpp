#include "Client.h"
#include "Logger.h"
#include "common.h"
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#ifdef USE_MPI
#include "common/mpi_tools.h"
#endif

using namespace std;
using namespace fmitcp;
using namespace fmitcp_proto;

template<typename T, typename R> vector<T> repeated_to_vector(R *repeated_vector) {
    vector<T> values;
    for(int i=0; i < repeated_vector().size(); i++)
        values.push_back(repeated_vector(i));
    return values;
}

template<typename T, typename R> vector<T> values_to_vector(R *r) {
    vector<T> values;
    for(int i=0; i<r->values_size(); i++)
        values.push_back(r->values(i));
    return values;
}

//fmi_status_t and jm_status_enu_t have different "ok" values
static bool statusIsOK(fmitcp_proto::jm_status_enu_t jm) {
    return jm == fmitcp_proto::jm_status_success;
}

static bool statusIsOK(fmitcp_proto::fmi2_status_t fmi2) {
    return fmi2 == fmitcp_proto::fmi2_status_ok;
}

template<typename T, typename R> void handle_get_value_res(Client *c, Logger logger, void (Client::*callback)(int,const vector<T>&,fmitcp_proto::fmi2_status_t), R *r) {
    std::vector<T> values = values_to_vector<T>(r);
    logger.log(Logger::LOG_NETWORK,"< %s(mid=%d,values=...,status=%d)\n",r->GetTypeName().c_str(), r->message_id(), r->status());
    if (!statusIsOK(r->status())) {
        fprintf(stderr, "FMI call %s failed with status=%d\nMaybe a connection or <Output> was specified incorrectly?",
            r->GetTypeName().c_str(), r->status());
        exit(1);
    }
    (c->*callback)(r->message_id(),values,r->status());
}

void Client::clientData(const char* data, long size){
    // Parse message
    fmitcp_message res;
    bool status = res.ParseFromArray(data, size);
    fmitcp_message_Type type = res.type();

    if (m_pendingRequests == 0) {
        fprintf(stderr, "Got response while m_pendingRequests = 0\n");
        exit(1);
    }
    m_pendingRequests--;

    m_logger.log(Logger::LOG_DEBUG,"Client parse status: %d (%i byte in)\n", status, size);

//useful for providing hints in case of failure
#define CHECK_WITH_STR(type, str) {\
        type##_res * r = res.mutable_##type##_res();\
        m_logger.log(Logger::LOG_NETWORK,"< "#type"_res(mid=%d,status=%d)\n",r->message_id(),r->status());\
        if (!statusIsOK(r->status())) {\
            fprintf(stderr, "FMI call "#type"() failed with status=%d\n" str, r->status());\
            exit(1);\
        }\
        on_##type##_res(r->message_id(),r->status());\
    }

#define NORMAL_CASE(type) CHECK_WITH_STR(type, "")
#define NOSTAT_CASE(type) {\
        type##_res * r = res.mutable_##type##_res();\
        m_logger.log(Logger::LOG_NETWORK,"< "#type"_res(mid=%d)\n",r->message_id());\
        on_##type##_res(r->message_id());\
    }

    // Check type and run the corresponding event handler
    switch (type) {
    case fmitcp_message_Type_type_fmi2_import_instantiate_res:                  NORMAL_CASE(fmi2_import_instantiate); break;
    case fmitcp_message_Type_type_fmi2_import_free_instance_res:                NOSTAT_CASE(fmi2_import_free_instance); break;
    case fmitcp_message_Type_type_fmi2_import_setup_experiment_res:             NORMAL_CASE(fmi2_import_setup_experiment); break;
    case fmitcp_message_Type_type_fmi2_import_enter_initialization_mode_res:    NORMAL_CASE(fmi2_import_enter_initialization_mode); break;
    case fmitcp_message_Type_type_fmi2_import_exit_initialization_mode_res:     NORMAL_CASE(fmi2_import_exit_initialization_mode); break;
    case fmitcp_message_Type_type_fmi2_import_terminate_res:                    NORMAL_CASE(fmi2_import_terminate); break;
    case fmitcp_message_Type_type_fmi2_import_reset_res:                        NORMAL_CASE(fmi2_import_reset); break;
    case fmitcp_message_Type_type_fmi2_import_set_real_input_derivatives_res:   NORMAL_CASE(fmi2_import_set_real_input_derivatives); break;
    case fmitcp_message_Type_type_fmi2_import_get_real_output_derivatives_res: {
        fmi2_import_get_real_output_derivatives_res * r = res.mutable_fmi2_import_get_real_output_derivatives_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_output_derivatives_res(mid=%d,status=%d,values=...)\n",r->message_id(),r->status());
        on_fmi2_import_get_real_output_derivatives_res(r->message_id(),r->status(),values_to_vector<double>(r));

        break;
    }
    case fmitcp_message_Type_type_fmi2_import_cancel_step_res:                  NORMAL_CASE(fmi2_import_cancel_step); break;
    case fmitcp_message_Type_type_fmi2_import_do_step_res:                      NORMAL_CASE(fmi2_import_do_step); break;
    case fmitcp_message_Type_type_fmi2_import_get_status_res: {
        fmi2_import_get_status_res * r = res.mutable_fmi2_import_get_status_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_status_res(value=%d)\n",r->value());
        on_fmi2_import_get_status_res(r->message_id(), r->value());

        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_real_status_res: {
        fmi2_import_get_real_status_res * r = res.mutable_fmi2_import_get_real_status_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_status_res(value=%g)\n",r->value());
        on_fmi2_import_get_real_status_res(r->message_id(), r->value());

        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_integer_status_res: {
        fmi2_import_get_integer_status_res * r = res.mutable_fmi2_import_get_integer_status_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_integer_status_res(mid=%d,value=%d)\n",r->message_id(),r->value());
        on_fmi2_import_get_integer_status_res(r->message_id(), r->value());

        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_boolean_status_res: {
        fmi2_import_get_boolean_status_res * r = res.mutable_fmi2_import_get_boolean_status_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_boolean_status_res(value=%d)\n",r->value());
        on_fmi2_import_get_boolean_status_res(r->message_id(), r->value());

        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_string_status_res: {
        fmi2_import_get_string_status_res * r = res.mutable_fmi2_import_get_string_status_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_string_status_res(value=%s)\n",r->value().c_str());
        on_fmi2_import_get_string_status_res(r->message_id(), r->value());

        break;
    }
    case fmitcp_message_Type_type_fmi2_import_set_time_res:                     NORMAL_CASE(fmi2_import_set_time); break;
    case fmitcp_message_Type_type_fmi2_import_set_continuous_states_res:        NORMAL_CASE(fmi2_import_set_continuous_states); break;
    case fmitcp_message_Type_type_fmi2_import_completed_integrator_step_res: {
        fmi2_import_completed_integrator_step_res * r = res.mutable_fmi2_import_completed_integrator_step_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_completed_integrator_step_res(mid=%d,callEventUpdate=%d,status=%d)\n",r->message_id(), r->calleventupdate(), r->status());
        on_fmi2_import_completed_integrator_step_res(r->message_id(),r->calleventupdate(),r->status());

        break;
    }
    case fmitcp_message_Type_type_fmi2_import_initialize_model_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_derivatives_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        fmi2_import_get_derivatives_res * r = res.mutable_fmi2_import_get_derivatives_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_derivatives_res(mid=%d,derivatives=%d,status=%d)\n",r->message_id(), r->derivatives(), r->status());
        //        on_fmi2_import_get_derivatives_res(r->message_id(),repeated_to_vector<double>(r->derivatives()),r->status());
        
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_event_indicators_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        fmi2_import_get_event_indicators_res * r = res.mutable_fmi2_import_get_event_indicators_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_event_indicators_res(mid=%d,event_indicators=%d,status=%d)\n",r->message_id(), r->z(), r->status());
        
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_eventUpdate_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_completed_event_iteration_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_continuous_states_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        fmi2_import_get_continuous_states_res * r = res.mutable_fmi2_import_get_continuous_states_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_continuous_states_res(mid=%d,continuous_states=%d,states=%d)\n",r->message_id(), r->x(), r->status());
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_nominal_continuous_states_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_version_res: {
        fmi2_import_get_version_res * r = res.mutable_fmi2_import_get_version_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_version_res(mid=%d,version=%s)\n",r->message_id(), r->version().c_str());
        on_fmi2_import_get_version_res(r->message_id(),r->version());

        break;
    }
    case fmitcp_message_Type_type_fmi2_import_set_debug_logging_res:            NORMAL_CASE(fmi2_import_set_debug_logging); break;
#define SETX_HINT "Maybe a parameter or a connection was specified incorrectly?\n"
    case fmitcp_message_Type_type_fmi2_import_set_real_res:                     CHECK_WITH_STR(fmi2_import_set_real, SETX_HINT); break;
    case fmitcp_message_Type_type_fmi2_import_set_integer_res:                  CHECK_WITH_STR(fmi2_import_set_integer, SETX_HINT); break;
    case fmitcp_message_Type_type_fmi2_import_set_boolean_res:                  CHECK_WITH_STR(fmi2_import_set_boolean, SETX_HINT); break;
    case fmitcp_message_Type_type_fmi2_import_set_string_res:                   CHECK_WITH_STR(fmi2_import_set_string, SETX_HINT); break;
    case fmitcp_message_Type_type_fmi2_import_get_real_res:
        handle_get_value_res(this, m_logger, &Client::on_fmi2_import_get_real_res, res.mutable_fmi2_import_get_real_res());
        break;
    case fmitcp_message_Type_type_fmi2_import_get_integer_res:
        handle_get_value_res(this, m_logger, &Client::on_fmi2_import_get_integer_res, res.mutable_fmi2_import_get_integer_res());
        break;
    case fmitcp_message_Type_type_fmi2_import_get_boolean_res:
        handle_get_value_res(this, m_logger, &Client::on_fmi2_import_get_boolean_res, res.mutable_fmi2_import_get_boolean_res());
        break;
    case fmitcp_message_Type_type_fmi2_import_get_string_res:
        handle_get_value_res(this, m_logger, &Client::on_fmi2_import_get_string_res, res.mutable_fmi2_import_get_string_res());
        break;
    case fmitcp_message_Type_type_fmi2_import_get_fmu_state_res: {
        fmi2_import_get_fmu_state_res * r = res.mutable_fmi2_import_get_fmu_state_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_fmu_state_res(mid=%d,stateId=%d,status=%d)\n",r->message_id(), r->stateid(), r->status());
        on_fmi2_import_get_fmu_state_res(r->message_id(),r->stateid(),r->status());

        break;
    }
    case fmitcp_message_Type_type_fmi2_import_set_fmu_state_res:                NORMAL_CASE(fmi2_import_set_fmu_state); break;
    case fmitcp_message_Type_type_fmi2_import_free_fmu_state_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_serialized_fmu_state_size_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_serialize_fmu_state_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_de_serialize_fmu_state_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_directional_derivative_res: {
        fmi2_import_get_directional_derivative_res * r = res.mutable_fmi2_import_get_directional_derivative_res();
        std::vector<double> dz;
        for(int i=0; i<r->dz_size(); i++)
            dz.push_back(r->dz(i));
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_directional_derivative_res(mid=%d,dz=...,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_get_directional_derivative_res(r->message_id(),dz,r->status());

        break;
    }
    case fmitcp_message_Type_type_get_xml_res: {

        get_xml_res * r = res.mutable_get_xml_res();
        m_logger.log(Logger::LOG_NETWORK,"< get_xml_res(mid=%d,xml=...)\n",r->message_id());
        on_get_xml_res(r->message_id(), r->loglevel(), r->xml());

        break;
    }
    default:
        m_logger.log(Logger::LOG_ERROR,"Message type not recognized: %d!\n",type);
        break;
    }
}

#ifdef USE_MPI
Client::Client(int world_rank) : world_rank(world_rank) {
#else
Client::Client(zmq::context_t &context) : m_socket(context, ZMQ_PAIR) {
#endif
    m_pendingRequests = 0;
    GOOGLE_PROTOBUF_VERIFY_VERSION;
}

Client::~Client(){
    google::protobuf::ShutdownProtobufLibrary();
}

Logger * Client::getLogger() {
    return &m_logger;
}

void Client::sendMessage(std::string s){
    m_pendingRequests++;
#ifdef USE_MPI
    MPI_Send((void*)s.c_str(), s.length(), MPI_CHAR, world_rank, 0, MPI_COMM_WORLD);
#else
    zmq::message_t msg(s.size());
    memcpy(msg.data(), s.data(), s.size());
    m_socket.send(msg);
#endif
}

void Client::sendMessageBlocking(std::string s) {
    sendMessage(s);

#ifdef USE_MPI
    std::string str = mpi_recv_string(world_rank, NULL, NULL);
    clientData(str.c_str(), str.length());
#else
    zmq::message_t msg;
    m_socket.recv(&msg);
    clientData(static_cast<char*>(msg.data()), msg.size());
#endif
}

size_t Client::getNumPendingRequests() const {
    return m_pendingRequests;
}

#ifndef USE_MPI
void Client::connect(string host, long port){
    ostringstream oss;
    oss << "tcp://" << host << ":" << port;
    string str = oss.str();
    m_logger.log(fmitcp::Logger::LOG_DEBUG,"connecting to %s\n", str.c_str());
    m_socket.connect(str.c_str());
    m_logger.log(fmitcp::Logger::LOG_DEBUG,"connected\n");
}
#endif
