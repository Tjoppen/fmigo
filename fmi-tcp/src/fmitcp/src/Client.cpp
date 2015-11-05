#include "Client.h"
#include "Logger.h"
#include "common.h"

using namespace std;
using namespace fmitcp;
using namespace fmitcp_proto;

#ifdef USE_LACEWING
void clientOnConnect(lw_client c) {
    Client * client = (Client*)lw_stream_tag(c);
    client->clientConnected(c);
}
void clientOnData(lw_client c, const char* data, long size) {
    Client * client = (Client*)lw_stream_tag(c);
    client->clientData(data,size);
}
void clientOnDisconnect(lw_client c) {
    Client * client = (Client*)lw_stream_tag(c);
    client->clientDisconnected(c);
}
void clientOnError(lw_client c, lw_error error) {
    Client * client = (Client*)lw_stream_tag(c);
    client->clientError(c,error);
}

void Client::clientConnected(lw_client c){
    m_logger.log(Logger::LOG_NETWORK,"+ Connected to FMU server.\n");
    onConnect();
}
#endif

template<typename T, typename R> vector<T> values_to_vector(R *r) {
    vector<T> values;
    for(int i=0; i<r->values_size(); i++)
        values.push_back(r->values(i));
    return values;
}

template<typename T, typename R> void handle_get_value_res(Client *c, Logger logger, void (Client::*callback)(int,const vector<T>&,fmitcp_proto::fmi2_status_t), R *r) {
    std::vector<T> values = values_to_vector<T>(r);
    logger.log(Logger::LOG_NETWORK,"< %s(mid=%d,values=...,status=%d)\n",r->GetTypeName().c_str(), r->message_id(), r->status());
    (c->*callback)(r->message_id(),values,r->status());
}

void Client::clientData(const char* data, long size){
#ifdef USE_LACEWING
  //undo the framing - we might have gotten more than one packet
  vector<string> messages = unpackBuffer(data, size, &tail);

  for (size_t x = 0; x < messages.size(); x++) {
    // Parse message
    fmitcp_message res;
    bool status = res.ParseFromString(messages[x]);
    fmitcp_message_Type type = res.type();

    m_logger.log(Logger::LOG_DEBUG,"Client parse status: %d (%i byte in)\n", status, messages[x].length());
#else
    // Parse message
    fmitcp_message res;
    bool status = res.ParseFromArray(data, size);
    fmitcp_message_Type type = res.type();

    m_logger.log(Logger::LOG_DEBUG,"Client parse status: %d (%i byte in)\n", status, size);
#endif

#define NORMAL_CASE(type) {\
        type##_res * r = res.mutable_##type##_res();\
        m_logger.log(Logger::LOG_NETWORK,"< "#type"_res(mid=%d,status=%d)\n",r->message_id(),r->status());\
        on_##type##_res(r->message_id(),r->status());\
    }
#define NOSTAT_CASE(type) {\
        type##_res * r = res.mutable_##type##_res();\
        m_logger.log(Logger::LOG_NETWORK,"< "#type"_res(mid=%d)\n",r->message_id());\
        on_##type##_res(r->message_id());\
    }

    // Check type and run the corresponding event handler
    switch (type) {
    case fmitcp_message_Type_type_fmi2_import_instantiate_res:                  NORMAL_CASE(fmi2_import_instantiate); break;
    case fmitcp_message_Type_type_fmi2_import_initialize_slave_res:             NORMAL_CASE(fmi2_import_initialize_slave); break;
    case fmitcp_message_Type_type_fmi2_import_terminate_slave_res:              NORMAL_CASE(fmi2_import_terminate_slave); break;
    case fmitcp_message_Type_type_fmi2_import_reset_slave_res:                  NORMAL_CASE(fmi2_import_reset_slave); break;
    case fmitcp_message_Type_type_fmi2_import_free_slave_instance_res:          NOSTAT_CASE(fmi2_import_free_slave_instance); break;
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
    case fmitcp_message_Type_type_fmi2_import_instantiate_model_res:            NORMAL_CASE(fmi2_import_instantiate_model); break;
    case fmitcp_message_Type_type_fmi2_import_free_model_instance_res:          NOSTAT_CASE(fmi2_import_free_model_instance); break;
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
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_event_indicators_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
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
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_get_nominal_continuous_states_res: {
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
        break;
    }
    case fmitcp_message_Type_type_fmi2_import_terminate_res: {
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
    case fmitcp_message_Type_type_fmi2_import_set_real_res:                     NORMAL_CASE(fmi2_import_set_real); break;
    case fmitcp_message_Type_type_fmi2_import_set_integer_res:                  NORMAL_CASE(fmi2_import_set_integer); break;
    case fmitcp_message_Type_type_fmi2_import_set_boolean_res:                  NORMAL_CASE(fmi2_import_set_boolean); break;
    case fmitcp_message_Type_type_fmi2_import_set_string_res:                   NORMAL_CASE(fmi2_import_set_string); break;
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
#ifdef USE_LACEWING
  }
#endif
}

#ifdef USE_LACEWING
void Client::clientDisconnected(lw_client c){
    m_logger.log(Logger::LOG_NETWORK,"- Disconnected from server.\n");
    lw_stream_close(c,true);
    lw_stream_delete(c);
    onDisconnect();
}

void Client::clientError(lw_client c, lw_error error){
    string err = lw_error_tostring(error);
    m_logger.log(Logger::LOG_ERROR,"Error: %s\n",err.c_str());
    onError(err);
}
#endif

#ifdef USE_LACEWING
Client::Client(EventPump * pump){
    m_pump = pump;
    m_client = lw_client_new(m_pump->getPump());
    //lw_fdstream_nagle(m_client,lw_false);
#else
Client::Client(zmq::context_t &context) : m_socket(context, ZMQ_PAIR) {
#endif
    GOOGLE_PROTOBUF_VERIFY_VERSION;
}

Client::~Client(){
#ifdef USE_LACEWING
    m_logger.log(Logger::LOG_DEBUG,"Closing stream.\n");

    bool status = lw_stream_close(m_client,false);
    m_logger.log(Logger::LOG_DEBUG,"Closed stream with status %d.\n",status);

    //lw_stream_delete(m_client);
#endif

    google::protobuf::ShutdownProtobufLibrary();
}

#ifdef USE_LACEWING
bool Client::isConnected(){
    return lw_client_connected(m_client);
}
#endif

Logger * Client::getLogger() {
    return &m_logger;
}

void Client::sendMessage(std::string s){
#ifdef USE_LACEWING
    fmitcp::sendProtoBuffer(m_client, s);
#else
    zmq::message_t msg(s.size());
    memcpy(msg.data(), s.data(), s.size());
    m_socket.send(msg);
#endif
}

void Client::connect(string host, long port){
#ifdef USE_LACEWING
    // Set the master object as tag
    lw_stream_set_tag(m_client, (void*)this);

    // connect the event handlers
    lw_client_on_connect(   m_client, clientOnConnect);
    lw_client_on_data(      m_client, clientOnData);
    lw_client_on_disconnect(m_client, clientOnDisconnect);
    lw_client_on_error(     m_client, clientOnError);

    // connect the client to the server
    lw_client_connect(m_client, host.c_str(), port);

    m_logger.log(Logger::LOG_DEBUG,"Connecting to %s:%ld...\n",host.c_str(),port);
#else
    ostringstream oss;
    oss << "tcp://" << host << ":" << port;
    string str = oss.str();
    m_logger.log(fmitcp::Logger::LOG_DEBUG,"connecting to %s\n", str.c_str());
    m_socket.connect(str.c_str());
    m_logger.log(fmitcp::Logger::LOG_DEBUG,"connected\n");
#endif
}

#ifdef USE_LACEWING
void Client::disconnect(){
    //lw_eventpump_post_eventloop_exit(m_pump->getPump());
    //lw_stream_close(m_client,lw_true);
    //lw_stream_delete(m_client);
    //lw_pump_delete(m_pump->getPump());
}
#endif
