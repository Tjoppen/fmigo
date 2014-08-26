#include "Client.h"
#include "Logger.h"
#include "common.h"

using namespace std;
using namespace fmitcp;
using namespace fmitcp_proto;

void clientOnConnect(lw_client c) {
    Client * client = (Client*)lw_stream_tag(c);
    client->clientConnected(c);
}
void clientOnData(lw_client c, const char* data, long size) {
    Client * client = (Client*)lw_stream_tag(c);
    client->clientData(c,data,size);
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
}

void Client::clientData(lw_client c, const char* data, long size){
    string data2 = fmitcp::dataToString(data,size);

    if(data2 ==  "\n"){
        m_logger.log(Logger::LOG_NETWORK,"Recieved empty message from server.\n");
        return;
    } else if(data2 == "connected\n"){
        m_logger.log(Logger::LOG_NETWORK,"Recieved connected message from server.\n");
        return onConnect();
    }

    // Parse message
    fmitcp_message res;
    bool status = res.ParseFromString(data2);
    fmitcp_message_Type type = res.type();

    m_logger.log(Logger::LOG_DEBUG,"Client parse status: %d\n", status);

    // Check type and run the corresponding event handler
    if(type == fmitcp_message_Type_type_fmi2_import_instantiate_res){
        fmi2_import_instantiate_res * r = res.mutable_fmi2_import_instantiate_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_instantiate_slave_res(mid=%d,status=%d)\n",r->message_id(),r->status());
        on_fmi2_import_instantiate_res(r->message_id(), r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_initialize_slave_res){
        fmi2_import_initialize_slave_res * r = res.mutable_fmi2_import_initialize_slave_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_initialize_slave_res(status=%d)\n",r->status());
        on_fmi2_import_initialize_slave_res(r->message_id(), r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_terminate_slave_res){
        fmi2_import_terminate_slave_res * r = res.mutable_fmi2_import_terminate_slave_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_terminate_slave_res(status=%d)\n",r->status());
        on_fmi2_import_terminate_slave_res(r->message_id(), r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_reset_slave_res){
        fmi2_import_reset_slave_res * r = res.mutable_fmi2_import_reset_slave_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_reset_slave_res(status=%d)\n",r->status());
        on_fmi2_import_reset_slave_res(r->message_id(), r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_free_slave_instance_res){
        fmi2_import_free_slave_instance_res * r = res.mutable_fmi2_import_free_slave_instance_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_free_slave_instance_res(mid=%d)\n",r->message_id());
        on_fmi2_import_free_slave_instance_res(r->message_id());

    } else if(type == fmitcp_message_Type_type_fmi2_import_set_real_input_derivatives_res){
        fmi2_import_set_real_input_derivatives_res * r = res.mutable_fmi2_import_set_real_input_derivatives_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_real_input_derivatives_res(mid=%d,status=%d)\n",r->message_id(),r->status());
        on_fmi2_import_set_real_input_derivatives_res(r->message_id(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_real_output_derivatives_res){
        fmi2_import_get_real_output_derivatives_res * r = res.mutable_fmi2_import_get_real_output_derivatives_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_output_derivatives_res(mid=%d,status=%d,values=...)\n",r->message_id(),r->status());
        on_fmi2_import_get_real_output_derivatives_res(r->message_id(),r->status(),vector<double>());

    } else if(type == fmitcp_message_Type_type_fmi2_import_cancel_step_res){
        fmi2_import_cancel_step_res * r = res.mutable_fmi2_import_cancel_step_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_cancel_step_res(mid=%d,status=%d)\n",r->message_id(),r->status());
        on_fmi2_import_cancel_step_res(r->message_id(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_do_step_res){
        fmi2_import_do_step_res * r = res.mutable_fmi2_import_do_step_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_do_step_res(status=%d)\n",r->status());
        on_fmi2_import_do_step_res(r->message_id(), r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_status_res){
        fmi2_import_get_status_res * r = res.mutable_fmi2_import_get_status_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_status_res(value=%d)\n",r->value());
        on_fmi2_import_get_status_res(r->message_id(), r->value());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_real_status_res){
        fmi2_import_get_real_status_res * r = res.mutable_fmi2_import_get_real_status_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_status_res(value=%g)\n",r->value());
        on_fmi2_import_get_real_status_res(r->message_id(), r->value());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_integer_status_res){
        fmi2_import_get_integer_status_res * r = res.mutable_fmi2_import_get_integer_status_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_integer_status_res(mid=%d,value=%d)\n",r->message_id(),r->value());
        on_fmi2_import_get_integer_status_res(r->message_id(), r->value());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_boolean_status_res){
        fmi2_import_get_boolean_status_res * r = res.mutable_fmi2_import_get_boolean_status_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_boolean_status_res(value=%d)\n",r->value());
        on_fmi2_import_get_boolean_status_res(r->message_id(), r->value());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_string_status_res){
        fmi2_import_get_string_status_res * r = res.mutable_fmi2_import_get_string_status_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_string_status_res(value=%s)\n",r->value().c_str());
        on_fmi2_import_get_string_status_res(r->message_id(), r->value());

    } else if(type == fmitcp_message_Type_type_fmi2_import_instantiate_model_res){
        fmi2_import_instantiate_model_res * r = res.mutable_fmi2_import_instantiate_model_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_instantiate_model_res(mid=%d,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_instantiate_model_res(r->message_id(), r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_free_model_instance_res){
        fmi2_import_free_model_instance_res * r = res.mutable_fmi2_import_free_model_instance_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_free_model_instance_res(mid=%d)\n",r->message_id());
        on_fmi2_import_free_model_instance_res(r->message_id());

    } else if(type == fmitcp_message_Type_type_fmi2_import_set_time_res){
        fmi2_import_set_time_res * r = res.mutable_fmi2_import_set_time_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_time_res(mid=%d,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_set_time_res(r->message_id(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_set_continuous_states_res){
        fmi2_import_set_continuous_states_res * r = res.mutable_fmi2_import_set_continuous_states_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_continuous_states_res(mid=%d,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_set_continuous_states_res(r->message_id(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_completed_integrator_step_res){
        fmi2_import_completed_integrator_step_res * r = res.mutable_fmi2_import_completed_integrator_step_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_completed_integrator_step_res(mid=%d,callEventUpdate=%d,status=%d)\n",r->message_id(), r->calleventupdate(), r->status());
        on_fmi2_import_completed_integrator_step_res(r->message_id(),r->calleventupdate(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_initialize_model_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_get_derivatives_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_get_event_indicators_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_eventUpdate_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_completed_event_iteration_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_get_continuous_states_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_get_nominal_continuous_states_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_terminate_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_get_version_res){
        fmi2_import_get_version_res * r = res.mutable_fmi2_import_get_version_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_version_res(mid=%d,version=%s)\n",r->message_id(), r->version().c_str());
        on_fmi2_import_get_version_res(r->message_id(),r->version());

    } else if(type == fmitcp_message_Type_type_fmi2_import_set_debug_logging_res){
        fmi2_import_set_debug_logging_res * r = res.mutable_fmi2_import_set_debug_logging_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_debug_logging_res(mid=%d,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_set_debug_logging_res(r->message_id(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_set_real_res){
        fmi2_import_set_real_res * r = res.mutable_fmi2_import_set_real_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_real_res(mid=%d,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_set_real_res(r->message_id(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_set_integer_res){
        fmi2_import_set_integer_res * r = res.mutable_fmi2_import_set_integer_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_integer_res(mid=%d,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_set_integer_res(r->message_id(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_set_boolean_res){
        fmi2_import_set_boolean_res * r = res.mutable_fmi2_import_set_boolean_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_boolean_res(mid=%d,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_set_boolean_res(r->message_id(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_set_string_res){
        fmi2_import_set_string_res * r = res.mutable_fmi2_import_set_string_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_string_res(mid=%d,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_set_string_res(r->message_id(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_real_res){
        fmi2_import_get_real_res * r = res.mutable_fmi2_import_get_real_res();
        std::vector<double> values;
        for(int i=0; i<r->values_size(); i++)
            values.push_back(r->values(i));
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_res(mid=%d,values=...,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_get_real_res(r->message_id(),values,r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_integer_res){
        fmi2_import_get_integer_res * r = res.mutable_fmi2_import_get_integer_res();
        std::vector<int> values;
        for(int i=0; i<r->values_size(); i++)
            values.push_back(r->values(i));
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_integer_res(mid=%d,values=...,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_get_integer_res(r->message_id(),values,r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_boolean_res){
        fmi2_import_get_boolean_res * r = res.mutable_fmi2_import_get_boolean_res();
        std::vector<bool> values;
        for(int i=0; i<r->values_size(); i++)
            values.push_back(r->values(i));
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_boolean_res(mid=%d,values=...,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_get_boolean_res(r->message_id(),values,r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_string_res){
        fmi2_import_get_string_res * r = res.mutable_fmi2_import_get_string_res();
        std::vector<string> values;
        for(int i=0; i<r->values_size(); i++)
            values.push_back(r->values(i));
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_string_res(mid=%d,values=...,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_get_string_res(r->message_id(),values,r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_get_fmu_state_res){
        fmi2_import_get_fmu_state_res * r = res.mutable_fmi2_import_get_fmu_state_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_fmu_state_res(mid=%d,stateId=%d,status=%d)\n",r->message_id(), r->stateid(), r->status());
        on_fmi2_import_get_fmu_state_res(r->message_id(),r->stateid(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_set_fmu_state_res){
        fmi2_import_set_fmu_state_res * r = res.mutable_fmi2_import_set_fmu_state_res();
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_fmu_state_res(mid=%d,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_set_fmu_state_res(r->message_id(),r->status());

    } else if(type == fmitcp_message_Type_type_fmi2_import_free_fmu_state_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_serialized_fmu_state_size_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_serialize_fmu_state_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_de_serialize_fmu_state_res){
        m_logger.log(Logger::LOG_NETWORK,"This command is TODO\n");
    } else if(type == fmitcp_message_Type_type_fmi2_import_get_directional_derivative_res){
        fmi2_import_get_directional_derivative_res * r = res.mutable_fmi2_import_get_directional_derivative_res();
        std::vector<double> dz;
        for(int i=0; i<r->dz_size(); i++)
            dz.push_back(r->dz(i));
        m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_directional_derivative_res(mid=%d,dz=...,status=%d)\n",r->message_id(), r->status());
        on_fmi2_import_get_directional_derivative_res(r->message_id(),dz,r->status());

    } else if(type == fmitcp_message_Type_type_get_xml_res){

        get_xml_res * r = res.mutable_get_xml_res();
        m_logger.log(Logger::LOG_NETWORK,"< get_xml_res(mid=%d,xml=...)\n",r->message_id());
        onGetXmlRes(r->message_id(), r->loglevel(), r->xml());

    } else {
        m_logger.log(Logger::LOG_ERROR,"Message type not recognized: %d!\n",type);
    }
}

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

Client::Client(EventPump * pump){
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    m_pump = pump;
    m_client = lw_client_new(m_pump->getPump());
    //lw_fdstream_nagle(m_client,lw_false);
}

Client::~Client(){
    m_logger.log(Logger::LOG_DEBUG,"Closing stream.\n");

    bool status = lw_stream_close(m_client,false);
    m_logger.log(Logger::LOG_DEBUG,"Closed stream with status %d.\n",status);

    //lw_stream_delete(m_client);
    google::protobuf::ShutdownProtobufLibrary();
}

bool Client::isConnected(){
    return lw_client_connected(m_client);
}

Logger * Client::getLogger() {
    return &m_logger;
}

void Client::sendMessage(fmitcp_proto::fmitcp_message * message){
    fmitcp::sendProtoBuffer(m_client,message);
}

void Client::connect(string host, long port){

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
}

void Client::disconnect(){
    //lw_eventpump_post_eventloop_exit(m_pump->getPump());
    //lw_stream_close(m_client,lw_true);
    //lw_stream_delete(m_client);
    //lw_pump_delete(m_pump->getPump());
}

void Client::getXml(int message_id, int fmuId) {
  // Construct message
  fmitcp_message m;
  m.set_type(fmitcp_message_Type_type_get_xml_req);

  get_xml_req * req = m.mutable_get_xml_req();
  req->set_message_id(message_id);
  req->set_fmuid(fmuId);

  m_logger.log(Logger::LOG_NETWORK, "> get_xml_req(mid=%d,fmuId=%d)\n", message_id, fmuId);

  sendMessage(&m);
}

void Client::fmi2_import_instantiate(int message_id) {
  // Construct message
  fmitcp_message m;
  m.set_type(fmitcp_message_Type_type_fmi2_import_instantiate_req);

  fmi2_import_instantiate_req * req = m.mutable_fmi2_import_instantiate_req();
  req->set_message_id(message_id);

  m_logger.log(Logger::LOG_NETWORK,
      "> fmi2_import_instantiate_slave_req(mid=%d)\n",
      message_id);

  sendMessage(&m);
  /*string msg = "INSTANTIATEEEEE\n"; // TEST !? :)
  lw_stream_write(m_client,msg.c_str(), msg.size());*/
}

void Client::fmi2_import_initialize_slave(int message_id, int fmuId, bool toleranceDefined, double tolerance, double startTime,
    bool stopTimeDefined, double stopTime) {
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_initialize_slave_req);

    fmi2_import_initialize_slave_req * req = m.mutable_fmi2_import_initialize_slave_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_tolerancedefined(toleranceDefined);
    req->set_tolerance(tolerance);
    req->set_starttime(startTime);
    req->set_stoptimedefined(stopTimeDefined);
    req->set_stoptime(stopTime);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_initialize_slave_req(mid=%d,fmu=%d,toleranceDefined=%d,tolerance=%g,"
        "startTime=%g,stopTimeDefined=%d,stopTime=%g)\n", message_id, fmuId, toleranceDefined, tolerance, startTime,
        stopTimeDefined, stopTime);

    sendMessage(&m);
}

void Client::fmi2_import_terminate_slave(int message_id, int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_terminate_slave_req);

    fmi2_import_terminate_slave_req * req = m.mutable_fmi2_import_terminate_slave_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    m_logger.log(Logger::LOG_NETWORK,
        "> fmi2_import_terminate_slave_req(mid=%d,fmu=%d)\n",
        message_id,
        fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_reset_slave(int message_id, int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_reset_slave_req);

    fmi2_import_reset_slave_req * req = m.mutable_fmi2_import_reset_slave_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    m_logger.log(Logger::LOG_NETWORK,
        "> fmi2_import_reset_slave_req(mid=%d,fmu=%d)\n",
        message_id,
        fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_free_slave_instance(int message_id,int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_free_slave_instance_req);

    fmi2_import_free_slave_instance_req * req = m.mutable_fmi2_import_free_slave_instance_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_free_slave_instance_req(mid=%d,fmu=%d)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_set_real_input_derivatives(int message_id, int fmuId,
                                                    std::vector<int> valueRefs,
                                                    std::vector<int> orders,
                                                    std::vector<double> values){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_real_input_derivatives_req);

    fmi2_import_set_real_input_derivatives_req * req = m.mutable_fmi2_import_set_real_input_derivatives_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    // TODO: SET the derivatives in message

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_set_real_input_derivatives_req(mid=%d,fmu=%d)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_get_real_output_derivatives(int message_id, int fmuId, std::vector<int> valueRefs, std::vector<int> orders){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_real_output_derivatives_req);

    fmi2_import_get_real_output_derivatives_req * req = m.mutable_fmi2_import_get_real_output_derivatives_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_real_output_derivatives_req(mid=%d,fmu=%d)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_cancel_step(int message_id, int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_cancel_step_req);

    fmi2_import_cancel_step_req * req = m.mutable_fmi2_import_cancel_step_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_cancel_step_req(mid=%d,fmu=%d)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_do_step(int message_id,
                                 int fmuId,
                                 double currentCommunicationPoint,
                                 double communicationStepSize,
                                 bool newStep){

    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_do_step_req);

    fmi2_import_do_step_req * req = m.mutable_fmi2_import_do_step_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_currentcommunicationpoint(currentCommunicationPoint);
    req->set_communicationstepsize(communicationStepSize);
    req->set_newstep(newStep);

    m_logger.log(Logger::LOG_NETWORK,
        "> fmi2_import_do_step_req(mid=%d,fmu=%d,commPoint=%g,stepSize=%g,newStep=%d)\n",
        message_id,
        fmuId,
        currentCommunicationPoint,
        communicationStepSize,
        newStep);

    sendMessage(&m);
}

void Client::fmi2_import_get_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_status_req);

    fmi2_import_get_status_req * req = m.mutable_fmi2_import_get_status_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_status(s);

    m_logger.log(Logger::LOG_NETWORK,
        "> fmi2_import_get_status_req(mid=%d,fmu=%d,status=%d)\n",
        message_id,
        fmuId,
        s);

    sendMessage(&m);
}

void Client::fmi2_import_get_real_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_real_status_req);

    fmi2_import_get_real_status_req * req = m.mutable_fmi2_import_get_real_status_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_kind(s);

    m_logger.log(Logger::LOG_NETWORK,
        "> fmi2_import_get_real_status_req(mid=%d,fmu=%d,kind=%d)\n",
        message_id,
        fmuId,
        s);

    sendMessage(&m);
}

void Client::fmi2_import_get_integer_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_integer_status_req);

    fmi2_import_get_integer_status_req * req = m.mutable_fmi2_import_get_integer_status_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_kind(s);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_integer_status_req(mid=%d,fmu=%d,kind=%d)\n", message_id, fmuId, s);

    sendMessage(&m);
}


void Client::fmi2_import_get_boolean_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_boolean_status_req);

    fmi2_import_get_boolean_status_req * req = m.mutable_fmi2_import_get_boolean_status_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_kind(s);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_boolean_status_req(mid=%d,fmu=%d,kind=%d)\n", message_id, fmuId, s);

    sendMessage(&m);
}

void Client::fmi2_import_get_string_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_string_status_req);

    fmi2_import_get_string_status_req * req = m.mutable_fmi2_import_get_string_status_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_kind(s);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_string_status_req(mid=%d,fmu=%d,kind=%d)\n", message_id, fmuId, s);

    sendMessage(&m);
}

void Client::fmi2_import_get_version(int message_id, int fmuId){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_version_req);

    fmi2_import_get_version_req * req = m.mutable_fmi2_import_get_version_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_version_req(mid=%d,fmu=%d)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_set_debug_logging(int message_id, int fmuId, bool loggingOn, const std::vector<string> categories){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_debug_logging_req);

    fmi2_import_set_debug_logging_req * req = m.mutable_fmi2_import_set_debug_logging_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_loggingon(loggingOn);
    for(int i=0; i<categories.size(); i++)
        req->add_categories(categories[i]);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_set_debug_logging_req(mid=%d,fmu=%d,categories=...)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_set_real(int message_id, int fmuId, const vector<int>& valueRefs, const vector<double>& values){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_real_req);

    fmi2_import_set_real_req * req = m.mutable_fmi2_import_set_real_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req->add_values(values[i]);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_set_real_req(mid=%d,fmu=%d,vrs=...,values=...)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_set_integer(int message_id, int fmuId, const vector<int>& valueRefs, const vector<int>& values){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_integer_req);

    fmi2_import_set_integer_req * req = m.mutable_fmi2_import_set_integer_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req->add_values(values[i]);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_set_integer_req(mid=%d,fmu=%d,vrs=...,values=...)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_set_boolean(int message_id, int fmuId, const vector<int>& valueRefs, const vector<bool>& values){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_boolean_req);

    fmi2_import_set_boolean_req * req = m.mutable_fmi2_import_set_boolean_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req->add_values(values[i]);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_set_boolean_req(mid=%d,fmu=%d,vrs=...,values=...)\n", message_id, fmuId);

    sendMessage(&m);
}


void Client::fmi2_import_set_string(int message_id, int fmuId, const vector<int>& valueRefs, const vector<string>& values){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_string_req);

    fmi2_import_set_string_req * req = m.mutable_fmi2_import_set_string_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req->add_values(values[i]);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_set_string_req(mid=%d,fmu=%d,vrs=...,values=...)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_get_real(int message_id, int fmuId, const vector<int>& valueRefs){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_real_req);

    fmi2_import_get_real_req * req = m.mutable_fmi2_import_get_real_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_real_req(mid=%d,fmu=%d,vrs=...)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_get_integer(int message_id, int fmuId, const vector<int>& valueRefs){

    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_integer_req);

    fmi2_import_get_integer_req * req = m.mutable_fmi2_import_get_integer_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_integer_req(mid=%d,fmu=%d,vrs=...)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_get_boolean(int message_id, int fmuId, const vector<int>& valueRefs){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_boolean_req);

    fmi2_import_get_boolean_req * req = m.mutable_fmi2_import_get_boolean_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_boolean_req(mid=%d,fmu=%d,vrs=...)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_get_string (int message_id, int fmuId, const vector<int>& valueRefs){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_string_req);

    fmi2_import_get_string_req * req = m.mutable_fmi2_import_get_string_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_string_req(mid=%d,fmu=%d,vrs=...)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_get_fmu_state(int message_id, int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_fmu_state_req);

    fmi2_import_get_fmu_state_req * req = m.mutable_fmi2_import_get_fmu_state_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_fmu_state_req(mid=%d,fmu=%d)\n", message_id, fmuId);

    sendMessage(&m);
}

void Client::fmi2_import_set_fmu_state(int message_id, int fmuId, int stateId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_fmu_state_req);

    fmi2_import_set_fmu_state_req * req = m.mutable_fmi2_import_set_fmu_state_req();
    req->set_message_id(message_id);
    req->set_stateid(stateId);
    req->set_fmuid(fmuId);
    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_set_fmu_state_req(mid=%d,fmu=%d,stateId=%d)\n", message_id, fmuId, stateId);

    sendMessage(&m);
}

void Client::fmi2_import_get_directional_derivative(int message_id, int fmuId,
                                                    const vector<int>& v_ref,
                                                    const vector<int>& z_ref,
                                                    const vector<double>& dv){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_directional_derivative_req);

    fmi2_import_get_directional_derivative_req * req = m.mutable_fmi2_import_get_directional_derivative_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<v_ref.size(); i++)
        req->add_v_ref(v_ref[i]);
    for(int i=0; i<z_ref.size(); i++)
        req->add_z_ref(z_ref[i]);
    for(int i=0; i<dv.size(); i++)
        req->add_dv(dv[i]);

    m_logger.log(Logger::LOG_NETWORK, "> fmi2_import_get_directional_derivative_req(mid=%d,fmu=%d,vref=...,zref=...,dv=...)\n", message_id, fmuId);

    sendMessage(&m);
}


void Client::get_xml(int message_id, int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_get_xml_req);

    get_xml_req * req = m.mutable_get_xml_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    m_logger.log(Logger::LOG_NETWORK, "> get_xml_req(mid=%d,fmu=%d)\n", message_id, fmuId);

    sendMessage(&m);
}
