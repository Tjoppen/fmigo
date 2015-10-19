#include <fstream>

#include "Server.h"
#include "Logger.h"
#include "common.h"
#include "fmitcp.pb.h"

using namespace fmitcp;

#ifdef USE_LACEWING
void serverOnConnect(lw_server s, lw_client c) {
  Server * server = (Server*)lw_server_tag(s);
  server->clientConnected(c);
  //lw_fdstream_nagle(c,lw_false);
}
void serverOnData(lw_server s, lw_client client, const char* data, size_t size) {
  Server * server = (Server*)lw_server_tag(s);
  server->clientData(client,data,size);
}
void serverOnDisconnect(lw_server s, lw_client c) {
  Server * server = (Server*)lw_server_tag(s);
  server->clientDisconnected(c);
}
void serverOnError(lw_server s, lw_error error) {
  Server * server = (Server*)lw_server_tag(s);
  server->error(s,error);
}
#endif

/*!
 * Callback function for FMILibrary. Logs the FMILibrary operations.
 */
void jmCallbacksLogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
  printf("[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}

#ifdef USE_LACEWING
Server::Server(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, EventPump *pump) {
  m_fmuParsed = true;
  m_fmuPath = fmuPath;
  m_debugLogging = debugLogging;
  m_logLevel = logLevel;
  init(pump);
}

Server::Server(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, EventPump *pump, const Logger &logger) {
  m_fmuParsed = true;
  m_fmuPath = fmuPath;
  m_debugLogging = debugLogging;
  m_logLevel = logLevel;
  m_logger = logger;
  init(pump);
}
#else
Server::Server(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel) {
  m_fmuParsed = true;
  m_fmuPath = fmuPath;
  m_debugLogging = debugLogging;
  m_logLevel = logLevel;
  init();
}

Server::Server(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, const Logger &logger) {
  m_fmuParsed = true;
  m_fmuPath = fmuPath;
  m_debugLogging = debugLogging;
  m_logLevel = logLevel;
  m_logger = logger;
  init();
}
#endif

Server::~Server() {
#ifdef USE_LACEWING
  lw_server_delete(m_server);
#endif
}

#ifdef USE_LACEWING
void Server::init(EventPump * pump) {
  m_pump = pump;
  m_server = lw_server_new(pump->getPump());
#else
void Server::init() {
#endif
  m_sendDummyResponses = false;

  if(m_fmuPath == "dummy"){
    m_sendDummyResponses = true;
    m_fmuParsed = true;
    return;
  }

  // Parse FMU
  // JM callbacks
  m_jmCallbacks.malloc = malloc;
  m_jmCallbacks.calloc = calloc;
  m_jmCallbacks.realloc = realloc;
  m_jmCallbacks.free = free;
  m_jmCallbacks.logger = jmCallbacksLogger;
  m_jmCallbacks.log_level = m_logLevel;
  m_jmCallbacks.context = 0;
  // working directory
  char* dir = fmi_import_mk_temp_dir(&m_jmCallbacks, NULL, "fmitcp_");
  m_workingDir = dir; // convert to std::string
  free(dir);
  // import allocate context
  m_context = fmi_import_allocate_context(&m_jmCallbacks);
  // get FMU version
  m_version = fmi_import_get_fmi_version(m_context, m_fmuPath.c_str(), m_workingDir.c_str());
  // Check version OK
  if ((m_version <= fmi_version_unknown_enu) || (m_version >= fmi_version_unsupported_enu)) {
    fmi_import_free_context(m_context);
    fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
    m_logger.log(Logger::LOG_ERROR, "Unsupported/unknown FMU version: '%s'.\n", fmi_version_to_string(m_version));
    m_fmuParsed = false;
    return;
  }
  if (m_version == fmi_version_2_0_enu) { // FMI 2.0
    // parse the xml file
    m_fmi2Instance = fmi2_import_parse_xml(m_context, m_workingDir.c_str(), 0);
    if(!m_fmi2Instance) {
      fmi_import_free_context(m_context);
      fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
      m_logger.log(Logger::LOG_ERROR, "Error parsing the modelDescription.xml file contained in %s\n", m_workingDir.c_str());
      m_fmuParsed = false;
      return;
    }
    // check FMU kind
    fmi2_fmu_kind_enu_t fmuType = fmi2_import_get_fmu_kind(m_fmi2Instance);
    if(fmuType != fmi2_fmu_kind_cs && fmuType != fmi2_fmu_kind_me_and_cs) {
      fmi2_import_free(m_fmi2Instance);
      fmi_import_free_context(m_context);
      fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
      m_logger.log(Logger::LOG_ERROR, "Only FMI Co-Simulation 2.0 is supported.\n");
      m_fmuParsed = false;
      return;
    }
    // FMI callback functions
    m_fmi2CallbackFunctions.logger = fmi2_log_forwarding;
    m_fmi2CallbackFunctions.allocateMemory = calloc;
    m_fmi2CallbackFunctions.freeMemory = free;
    m_fmi2CallbackFunctions.stepFinished = 0;
    m_fmi2CallbackFunctions.componentEnvironment = 0;
    // Load the binary (dll/so)
    jm_status_enu_t status = fmi2_import_create_dllfmu(m_fmi2Instance, fmuType, &m_fmi2CallbackFunctions);
    if (status == jm_status_error) {
      fmi2_import_free(m_fmi2Instance);
      fmi_import_free_context(m_context);
      fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
      m_logger.log(Logger::LOG_ERROR, "There was an error loading the FMU binary. Turn on logging (-l) for more info.\n");
      m_fmuParsed = false;
      return;
    }
    m_instanceName = fmi2_import_get_model_name(m_fmi2Instance);
    m_fmuLocation = fmi_import_create_URL_from_abs_path(&m_jmCallbacks, m_fmuPath.c_str());
    m_resourcePath = fmi_import_create_URL_from_abs_path(&m_jmCallbacks, m_workingDir.c_str());

    strcat(m_resourcePath,"/resources");

    /* 0 - original order as found in the XML file;
     * 1 - sorted alphabetically by variable name;
     * 2 sorted by types/value references.
     */
    int sortOrder = 0;
    m_fmi2Variables = fmi2_import_get_variable_list(m_fmi2Instance, sortOrder);
  } else {
    // todo add FMI 1.0 later on.
    fmi_import_free_context(m_context);
    fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
    m_logger.log(Logger::LOG_ERROR, "Only FMI Co-Simulation 2.0 is supported.\n");
    m_fmuParsed = false;
    return;
  }
}

#ifdef USE_LACEWING
void Server::clientConnected(lw_client c) {
  m_logger.log(Logger::LOG_NETWORK,"+ Client connected.\n");
  onClientConnect();
}

void Server::clientDisconnected(lw_client c) {
  m_logger.log(Logger::LOG_NETWORK,"- Client disconnected.\n");
  /*
  lw_stream_close(c,true);
  lw_stream_delete(c);
  fflush(NULL);
  */

 /*
  lw_server_delete(m_server);
  lw_pump_remove_user(m_pump->getPump());
  init(m_pump);
  */
  onClientDisconnect();
}
#endif

#ifdef USE_LACEWING
void Server::clientData(lw_client c, const char *data, size_t size) {
 //undo the framing - we might have gotten more than one packet
 vector<string> messages = unpackBuffer(data, size, &tail);

 for (size_t x = 0; x < messages.size(); x++) {
  string data2 = messages[x];

  // Construct message
  fmitcp_proto::fmitcp_message req;
  bool parseStatus = req.ParseFromString(data2);
#else
string Server::clientData(const char *data, size_t size) {
  fmitcp_proto::fmitcp_message req;
  bool parseStatus = req.ParseFromArray(data, size);
#endif
  fmitcp_proto::fmitcp_message_Type type = req.type();

  m_logger.log(Logger::LOG_DEBUG,"Parse status: %d\n", parseStatus);

  fmitcp_proto::fmitcp_message res;
  bool sendResponse = true;

  if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_instantiate_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_instantiate_req * r = req.mutable_fmi2_import_instantiate_req();
    int messageId = r->message_id();
    fmi2_boolean_t visible = r->visible();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_instantiate_req(mid=%d,visible=%d)\n",messageId, visible);

    jm_status_enu_t status = jm_status_success;
    if (!m_sendDummyResponses) {
      // instantiate FMU
      status = fmi2_import_instantiate(m_fmi2Instance, m_instanceName, fmi2_cosimulation, m_resourcePath, visible);
    }

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_instantiate_res);
    fmitcp_proto::fmi2_import_instantiate_res * instantiateRes = res.mutable_fmi2_import_instantiate_res();
    instantiateRes->set_message_id(messageId);
    instantiateRes->set_status(fmiJMStatusToProtoJMStatus(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_instantiate_slave_res(mid=%d,status=%d)\n",messageId,instantiateRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_initialize_slave_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_initialize_slave_req * r = req.mutable_fmi2_import_initialize_slave_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    bool toleranceDefined = r->has_tolerancedefined();
    double tolerance = r->tolerance();
    double starttime = r->starttime();
    bool stopTimeDefined = r->has_stoptimedefined();
    double stoptime = r->stoptime();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_initialize_slave_req(mid=%d)\n",messageId);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // initialize FMU
      /*!
       * \todo
       * We should set all variable start values (of "ScalarVariable / <type> / start").
       * fmiSetReal/Integer/Boolean/String(s1, ...);
       */
      /*!
       * \todo
       * What about FMUs internal experiment values e.g tolerance ????
       */
//      fmi2_boolean_t toleranceControlled = fmi2_false;
//      fmi2_real_t relativeTolerance = fmi2_import_get_default_experiment_tolerance(m_fmi2Instance);

      /*!
       * \todo
       * We need to set the input values at time = startTime after fmiEnterInitializationMode and before fmiExitInitializationMode.
       * fmiSetReal/Integer/Boolean/String(s1, ...);
       */
      if (fmi2StatusOkOrWarning(status =  fmi2_import_setup_experiment(m_fmi2Instance, toleranceDefined, tolerance,
          starttime, stopTimeDefined, stoptime)) &&
          fmi2StatusOkOrWarning(status = fmi2_import_enter_initialization_mode(m_fmi2Instance)) &&
          fmi2StatusOkOrWarning(fmi2_import_exit_initialization_mode(m_fmi2Instance))) {
        // do nothing
      }
    }

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_initialize_slave_res);
    fmitcp_proto::fmi2_import_initialize_slave_res * initializeRes = res.mutable_fmi2_import_initialize_slave_res();
    initializeRes->set_message_id(messageId);
    initializeRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_initialize_slave_res(mid=%d,status=%d)\n",messageId,initializeRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_terminate_slave_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_terminate_slave_req * r = req.mutable_fmi2_import_terminate_slave_req();
    int fmuId = r->fmuid();
    int messageId = r->message_id();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_terminate_slave_req(mid=%d,fmuId=%d)\n",messageId,fmuId);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // terminate FMU
      status = fmi2_import_terminate(m_fmi2Instance);
    }

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_terminate_slave_res);
    fmitcp_proto::fmi2_import_terminate_slave_res * terminateRes = res.mutable_fmi2_import_terminate_slave_res();
    terminateRes->set_message_id(messageId);
    terminateRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_terminate_slave_res(mid=%d,status=%d)\n",messageId,terminateRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_reset_slave_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_reset_slave_req * r = req.mutable_fmi2_import_reset_slave_req();
    int fmuId = r->fmuid();
    int messageId = r->message_id();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_reset_slave_req(fmuId=%d)\n",fmuId);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // reset FMU
      status = fmi2_import_reset(m_fmi2Instance);
    }

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_reset_slave_res);
    fmitcp_proto::fmi2_import_reset_slave_res * resetRes = res.mutable_fmi2_import_reset_slave_res();
    resetRes->set_message_id(messageId);
    resetRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_reset_slave_res(mid=%d,status=%d)\n",messageId,resetRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_free_slave_instance_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_free_slave_instance_req * r = req.mutable_fmi2_import_free_slave_instance_req();
    int fmuId = r->fmuid(),
        messageId = r->message_id();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_free_slave_instance_req(fmuId=%d)\n",fmuId);

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_free_slave_instance_res);
    fmitcp_proto::fmi2_import_free_slave_instance_res * resetRes = res.mutable_fmi2_import_free_slave_instance_res();
    resetRes->set_message_id(messageId);

    if (!m_sendDummyResponses) {
      // Interact with FMU
      fmi2_import_free(m_fmi2Instance);
      fmi_import_free_context(m_context);
      fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
    }

    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_free_slave_instance_res(mid=%d)\n",messageId);

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_real_input_derivatives_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_set_real_input_derivatives_req * r = req.mutable_fmi2_import_set_real_input_derivatives_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> vr(r->valuereferences_size());
    vector<fmi2_integer_t> order(r->orders_size());
    vector<fmi2_real_t> value(r->values_size());

    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      vr[i] = r->valuereferences(i);
      order[i] = r->orders(i);
      value[i] = r->values(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_real_input_derivatives_req(mid=%d,fmuId=%d,vrs=%s,orders=%s,values=%s)\n",messageId,fmuId,
        arrayToString(vr, r->valuereferences_size()).c_str(), arrayToString(order, r->orders_size()).c_str(), arrayToString(value, r->values_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_set_real_input_derivatives(m_fmi2Instance, vr.data(), r->valuereferences_size(), order.data(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_set_real_input_derivatives_res * setRealInputDerivativesRes = res.mutable_fmi2_import_set_real_input_derivatives_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_real_input_derivatives_res);
    setRealInputDerivativesRes->set_message_id(messageId);
    setRealInputDerivativesRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_set_real_input_derivatives_res(mid=%d,status=%d)\n",messageId, setRealInputDerivativesRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_real_output_derivatives_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_real_output_derivatives_req * r = req.mutable_fmi2_import_get_real_output_derivatives_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> vr(r->valuereferences_size());
    vector<fmi2_integer_t> order(r->orders_size());
    vector<fmi2_real_t> value(r->valuereferences_size());

    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      vr[i] = r->valuereferences(i);
      order[i] = r->orders(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_output_derivatives_req(mid=%d,fmuId=%d,vrs=%s,orders=%s)\n",messageId,fmuId,
        arrayToString(vr, r->valuereferences_size()).c_str(), arrayToString(order, r->orders_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_get_real_output_derivatives(m_fmi2Instance, vr.data(), r->valuereferences_size(), order.data(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_get_real_output_derivatives_res * getRealOutputDerivativesRes = res.mutable_fmi2_import_get_real_output_derivatives_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_real_output_derivatives_res);
    getRealOutputDerivativesRes->set_message_id(messageId);
    getRealOutputDerivativesRes->set_status(fmi2StatusToProtofmi2Status(status));
    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      getRealOutputDerivativesRes->add_values(value[i]);
    }
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_real_output_derivatives_res(mid=%d,status=%d,values=%s)\n",getRealOutputDerivativesRes->message_id(),getRealOutputDerivativesRes->status(),arrayToString(value, r->valuereferences_size()).c_str());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_cancel_step_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_cancel_step_req * r = req.mutable_fmi2_import_cancel_step_req();
    int fmuId = r->fmuid(),
        messageId = r->message_id();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_cancel_step_req(mid=%d,fmuId=%d)\n",messageId,fmuId);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // Interact with FMU
      status = fmi2_import_cancel_step(m_fmi2Instance);
    }

    // Create response
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_cancel_step_res);
    fmitcp_proto::fmi2_import_cancel_step_res * cancelStepRes = res.mutable_fmi2_import_cancel_step_res();
    cancelStepRes->set_message_id(messageId);
    cancelStepRes->set_status(fmi2StatusToProtofmi2Status(status));

    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_cancel_step_res(mid=%d,status=%d)\n",messageId,cancelStepRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_do_step_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_do_step_req * r = req.mutable_fmi2_import_do_step_req();
    int fmuId = r->fmuid();
    double currentCommunicationPoint = r->currentcommunicationpoint(),
        communicationStepSize = r->communicationstepsize();
    bool newStep = r->newstep();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_do_step_req(fmuId=%d,commPoint=%g,stepSize=%g,newStep=%d)\n",fmuId,currentCommunicationPoint,communicationStepSize,newStep?1:0);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // Step the FMU
      status = fmi2_import_do_step(m_fmi2Instance, currentCommunicationPoint, communicationStepSize, newStep);
    }

    // Create response
    fmitcp_proto::fmi2_import_do_step_res * doStepRes = res.mutable_fmi2_import_do_step_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_do_step_res);
    doStepRes->set_message_id(r->message_id());
    doStepRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_do_step_res(status=%d)\n",doStepRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_status_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_status_req * r = req.mutable_fmi2_import_get_status_req();
    int fmuId = r->fmuid();
    fmitcp_proto::fmi2_status_kind_t statusKind = r->status();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_status_req(fmuId=%d,status=%d)\n",fmuId, statusKind);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // get the FMU status
      fmi2_import_get_status(m_fmi2Instance, protoStatusKindToFmiStatusKind(statusKind), &status);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_status_res * getStatusRes = res.mutable_fmi2_import_get_status_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_status_res);
    getStatusRes->set_message_id(r->message_id());
    getStatusRes->set_value(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_status_res(value=%d)\n",getStatusRes->value());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_real_status_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_real_status_req * r = req.mutable_fmi2_import_get_real_status_req();
    int fmuId = r->fmuid();
    fmitcp_proto::fmi2_status_kind_t statusKind = r->kind();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_status_req(fmuId=%d,status=%d)\n",fmuId, statusKind);

    fmi2_real_t value = 0.0;
    if (!m_sendDummyResponses) {
      // get the FMU real status
      fmi2_import_get_real_status(m_fmi2Instance, protoStatusKindToFmiStatusKind(statusKind), &value);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_real_status_res * getRealStatusRes = res.mutable_fmi2_import_get_real_status_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_real_status_res);
    getRealStatusRes->set_message_id(r->message_id());
    getRealStatusRes->set_value(value);
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_real_status_res(value=%g)\n",getRealStatusRes->value());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_integer_status_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_integer_status_req * r = req.mutable_fmi2_import_get_integer_status_req();
    int fmuId = r->fmuid();
    fmitcp_proto::fmi2_status_kind_t statusKind = r->kind();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_integer_status_req(fmuId=%d,status=%d)\n",fmuId, statusKind);

    fmi2_integer_t value = 0;
    if (!m_sendDummyResponses) {
      // get the FMU integer status
      fmi2_import_get_integer_status(m_fmi2Instance, protoStatusKindToFmiStatusKind(statusKind), &value);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_integer_status_res * getIntegerStatusRes = res.mutable_fmi2_import_get_integer_status_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_integer_status_res);
    getIntegerStatusRes->set_message_id(r->message_id());
    getIntegerStatusRes->set_value(value);
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_integer_status_res(value=%d)\n",getIntegerStatusRes->value());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_boolean_status_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_boolean_status_req * r = req.mutable_fmi2_import_get_boolean_status_req();
    int fmuId = r->fmuid();
    fmitcp_proto::fmi2_status_kind_t statusKind = r->kind();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_boolean_status_req(mid=%d,fmuId=%d,status=%d)\n",r->message_id(), fmuId, statusKind);

    fmi2_boolean_t value = 0;
    if (!m_sendDummyResponses) {
      // get the FMU boolean status
      fmi2_import_get_boolean_status(m_fmi2Instance, protoStatusKindToFmiStatusKind(statusKind), &value);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_boolean_status_res * getBooleanStatusRes = res.mutable_fmi2_import_get_boolean_status_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_boolean_status_res);
    getBooleanStatusRes->set_message_id(r->message_id());
    getBooleanStatusRes->set_value(value);
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_boolean_status_res(mid=%d,value=%d)\n",getBooleanStatusRes->message_id(),getBooleanStatusRes->value());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_string_status_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_string_status_req * r = req.mutable_fmi2_import_get_string_status_req();
    int fmuId = r->fmuid();
    fmitcp_proto::fmi2_status_kind_t statusKind = r->kind();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_string_status_req(mid=%d,fmuId=%d,status=%d)\n",r->message_id(), fmuId, statusKind);

    fmi2_string_t value = "";
    if (!m_sendDummyResponses) {
      // TODO: Step the FMU
      fmi2_import_get_string_status(m_fmi2Instance, protoStatusKindToFmiStatusKind(statusKind), &value);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_string_status_res * getStringStatusRes = res.mutable_fmi2_import_get_string_status_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_string_status_res);
    getStringStatusRes->set_message_id(r->message_id());
    getStringStatusRes->set_value(value);
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_string_status_res(mid=%d,value=%s)\n",getStringStatusRes->message_id(),getStringStatusRes->value().c_str());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_instantiate_model_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_free_model_instance_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_time_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_continuous_states_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_completed_integrator_step_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_initialize_model_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_derivatives_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_event_indicators_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_eventUpdate_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_completed_event_iteration_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_continuous_states_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_nominal_continuous_states_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_terminate_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_version_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_version_req * r = req.mutable_fmi2_import_get_version_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_version_req(mid=%d,fmuId=%d)\n",r->message_id(),r->fmuid());

    const char* version = "VeRsIoN";
    if (!m_sendDummyResponses) {
      // get FMU version
      version = fmi2_import_get_version(m_fmi2Instance);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_version_res * getVersionRes = res.mutable_fmi2_import_get_version_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_version_res);
    getVersionRes->set_message_id(r->message_id());
    getVersionRes->set_version(version);
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_version_res(mid=%d,version=%s)\n",getVersionRes->message_id(),getVersionRes->version().c_str());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_debug_logging_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_set_debug_logging_req * r = req.mutable_fmi2_import_set_debug_logging_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_debug_logging_req(mid=%d,fmuId=%d,loggingOn=%d,categories=...)\n",r->message_id(),r->fmuid(),r->loggingon());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // set the debug logging for FMU
      // fetch the logging categories from the FMU
      size_t nCategories = fmi2_import_get_log_categories_num(m_fmi2Instance);
      vector<fmi2_string_t> categories(nCategories);
      int i;
      for (i = 0 ; i < nCategories ; i++) {
        categories[i] = fmi2_import_get_log_category(m_fmi2Instance, i);
      }
      // set debug logging. We don't care about its result.
      status = fmi2_import_set_debug_logging(m_fmi2Instance, m_debugLogging, nCategories, categories.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_set_debug_logging_res * getStatusRes = res.mutable_fmi2_import_set_debug_logging_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_debug_logging_res);
    getStatusRes->set_message_id(r->message_id());
    getStatusRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_set_debug_logging_res(mid=%d,status=%d)\n",getStatusRes->message_id(),getStatusRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_real_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_set_real_req * r = req.mutable_fmi2_import_set_real_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> vr(r->valuereferences_size());
    vector<fmi2_real_t> value(r->values_size());

    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      vr[i] = r->valuereferences(i);
      value[i] = r->values(i);
    }

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_real_req(mid=%d,fmuId=%d,vrs=%s,values=%s)\n",r->message_id(),r->fmuid(),
        arrayToString(vr, r->valuereferences_size()).c_str(), arrayToString(value, r->values_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
       status = fmi2_import_set_real(m_fmi2Instance, vr.data(), r->valuereferences_size(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_set_real_res * setRealRes = res.mutable_fmi2_import_set_real_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_real_res);
    setRealRes->set_message_id(r->message_id());
    setRealRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_set_real_res(mid=%d,status=%d)\n",setRealRes->message_id(),setRealRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_integer_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_set_integer_req * r = req.mutable_fmi2_import_set_integer_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> vr(r->valuereferences_size());
    vector<fmi2_integer_t> value(r->values_size());

    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      vr[i] = r->valuereferences(i);
      value[i] = r->values(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_integer_req(mid=%d,fmuId=%d,vrs=%s,values=%s)\n",r->message_id(),r->fmuid(),
        arrayToString(vr, r->valuereferences_size()).c_str(), arrayToString(value, r->values_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_set_integer(m_fmi2Instance, vr.data(), r->valuereferences_size(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_set_integer_res * setIntegerRes = res.mutable_fmi2_import_set_integer_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_integer_res);
    setIntegerRes->set_message_id(r->message_id());
    setIntegerRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_set_integer_res(mid=%d,status=%d)\n",setIntegerRes->message_id(),setIntegerRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_boolean_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_set_boolean_req * r = req.mutable_fmi2_import_set_boolean_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> vr(r->valuereferences_size());
    vector<fmi2_boolean_t> value(r->values_size());

    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      vr[i] = r->valuereferences(i);
      value[i] = r->values(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_boolean_req(mid=%d,fmuId=%d,vrs=%s,values=%s)\n",r->message_id(),r->fmuid(),
        arrayToString(vr, r->valuereferences_size()).c_str(), arrayToString(value, r->values_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_set_boolean(m_fmi2Instance, vr.data(), r->valuereferences_size(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_set_boolean_res * setBooleanRes = res.mutable_fmi2_import_set_boolean_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_boolean_res);
    setBooleanRes->set_message_id(r->message_id());
    setBooleanRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_set_boolean_res(mid=%d,status=%d)\n",setBooleanRes->message_id(),setBooleanRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_string_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_set_string_req * r = req.mutable_fmi2_import_set_string_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> vr(r->valuereferences_size());
    vector<fmi2_string_t> value(r->values_size());

    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      vr[i] = r->valuereferences(i);
      value[i] = r->values(i).c_str();
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_string_req(mid=%d,fmuId=%d,vrs=%s,values=%s)\n",r->message_id(),r->fmuid(),
        arrayToString(vr, r->valuereferences_size()).c_str(), arrayToString(value, r->values_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_set_string(m_fmi2Instance, vr.data(), r->valuereferences_size(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_set_string_res * getStatusRes = res.mutable_fmi2_import_set_string_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_string_res);
    getStatusRes->set_message_id(r->message_id());
    getStatusRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_set_string_res(mid=%d,status=%d)\n",getStatusRes->message_id(),getStatusRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_real_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_real_req * r = req.mutable_fmi2_import_get_real_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> vr(r->valuereferences_size());
    vector<fmi2_real_t> value(r->valuereferences_size());

    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      vr[i] = r->valuereferences(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_req(mid=%d,fmuId=%d,vrs=%s)\n",r->message_id(),r->fmuid(),arrayToString(vr, r->valuereferences_size()).c_str());

    // Create response
    fmitcp_proto::fmi2_import_get_real_res * getRealRes = res.mutable_fmi2_import_get_real_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_real_res);
    getRealRes->set_message_id(r->message_id());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
        // interact with FMU
        status = fmi2_import_get_real(m_fmi2Instance, vr.data(), r->valuereferences_size(), value.data());
        getRealRes->set_status(fmi2StatusToProtofmi2Status(status));
        for (int i = 0 ; i < r->valuereferences_size() ; i++) {
          getRealRes->add_values(value[i]);
        }
    } else {
        // Set dummy values
        for (int i = 0 ; i < r->valuereferences_size() ; i++) {
            getRealRes->add_values(0.0);
        }
        getRealRes->set_status(fmi2StatusToProtofmi2Status(fmi2_status_ok));
    }

    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_real_res(mid=%d,status=%d,values=%s)\n",getRealRes->message_id(),getRealRes->status(),arrayToString(value, r->valuereferences_size()).c_str());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_integer_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_integer_req * r = req.mutable_fmi2_import_get_integer_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> vr(r->valuereferences_size());
    vector<fmi2_integer_t> value(r->valuereferences_size());

    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      vr[i] = r->valuereferences(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_integer_req(mid=%d,fmuId=%d,vrs=%s)\n",r->message_id(),r->fmuid(),arrayToString(vr, r->valuereferences_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_get_integer(m_fmi2Instance, vr.data(), r->valuereferences_size(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_get_integer_res * getIntegerRes = res.mutable_fmi2_import_get_integer_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_integer_res);
    getIntegerRes->set_message_id(r->message_id());
    getIntegerRes->set_status(fmi2StatusToProtofmi2Status(status));
    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      getIntegerRes->add_values(value[i]);
    }
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_integer_res(mid=%d,status=%d,values=%s)\n",getIntegerRes->message_id(),getIntegerRes->status(),arrayToString(value, r->valuereferences_size()).c_str());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_boolean_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_boolean_req * r = req.mutable_fmi2_import_get_boolean_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> vr(r->valuereferences_size());
    vector<fmi2_boolean_t> value(r->valuereferences_size());

    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      vr[i] = r->valuereferences(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_boolean_req(mid=%d,fmuId=%d,vrs=%s)\n",r->message_id(),r->fmuid(),arrayToString(vr, r->valuereferences_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_get_boolean(m_fmi2Instance, vr.data(), r->valuereferences_size(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_get_boolean_res * getBooleanRes = res.mutable_fmi2_import_get_boolean_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_boolean_res);
    getBooleanRes->set_message_id(r->message_id());
    getBooleanRes->set_status(fmi2StatusToProtofmi2Status(status));
    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      getBooleanRes->add_values(value[i]);
    }
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_boolean_res(mid=%d,status=%d,values=%s)\n",getBooleanRes->message_id(),getBooleanRes->status(),arrayToString(value, r->valuereferences_size()).c_str());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_string_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_string_req * r = req.mutable_fmi2_import_get_string_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> vr(r->valuereferences_size());
    vector<fmi2_string_t> value(r->valuereferences_size());

    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      vr[i] = r->valuereferences(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_string_req(mid=%d,fmuId=%d,vrs=%s)\n",r->message_id(),r->fmuid(),arrayToString(vr, r->valuereferences_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_get_string(m_fmi2Instance, vr.data(), r->valuereferences_size(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_get_string_res * getStringRes = res.mutable_fmi2_import_get_string_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_string_res);
    getStringRes->set_message_id(r->message_id());
    getStringRes->set_status(fmi2StatusToProtofmi2Status(status));
    for (int i = 0 ; i < r->valuereferences_size() ; i++) {
      getStringRes->add_values(value[i]);
    }
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_string_res(mid=%d,status=%d,values=%s)\n",getStringRes->message_id(),getStringRes->status(),arrayToString(value, r->valuereferences_size()).c_str());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_fmu_state_req){

    // Unpack message
    fmitcp_proto::fmi2_import_get_fmu_state_req * r = req.mutable_fmi2_import_get_fmu_state_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_fmu_state_req(mid=%d,fmuId=%d)\n",r->message_id(),r->fmuid());

    fmitcp_proto::fmi2_import_get_fmu_state_res * getStatusRes = res.mutable_fmi2_import_get_fmu_state_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_fmu_state_res);
    getStatusRes->set_message_id(r->message_id());
    getStatusRes->set_status(fmitcp_proto::fmi2_status_ok);
    getStatusRes->set_stateid(0); // TODO

    if(!m_sendDummyResponses){
      // TODO: interact with FMU
    }

    // Create response
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_fmu_state_res(mid=%d,stateId=%d,status=%d)\n",getStatusRes->message_id(),getStatusRes->stateid(),getStatusRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_fmu_state_req){

    // Unpack message
    fmitcp_proto::fmi2_import_set_fmu_state_req * r = req.mutable_fmi2_import_set_fmu_state_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_fmu_state_req(mid=%d,fmuId=%d,stateId=%d)\n",r->message_id(),r->fmuid(),r->stateid());

    fmitcp_proto::fmi2_import_set_fmu_state_res * getStatusRes = res.mutable_fmi2_import_set_fmu_state_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_fmu_state_res);
    getStatusRes->set_message_id(r->message_id());
    getStatusRes->set_status(fmitcp_proto::fmi2_status_ok);

    if(!m_sendDummyResponses){
      // TODO: interact with FMU
    }

    // Create response
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_set_fmu_state_res(mid=%d,status=%d)\n",getStatusRes->message_id(),getStatusRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_free_fmu_state_req){

    // Unpack message
    fmitcp_proto::fmi2_import_free_fmu_state_req * r = req.mutable_fmi2_import_free_fmu_state_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_free_fmu_state_req(mid=%d,stateId=%d)\n",r->message_id(),r->stateid());

    fmitcp_proto::fmi2_import_free_fmu_state_res * getStatusRes = res.mutable_fmi2_import_free_fmu_state_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_free_fmu_state_res);
    getStatusRes->set_message_id(r->message_id());
    getStatusRes->set_status(fmitcp_proto::fmi2_status_ok);

    if(!m_sendDummyResponses){
      // TODO: interact with FMU
    }

    // Create response
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_free_fmu_state_res(mid=%d,status=%d)\n",getStatusRes->message_id(),getStatusRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_serialized_fmu_state_size_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_serialize_fmu_state_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_de_serialize_fmu_state_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_directional_derivative_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_get_directional_derivative_req * r = req.mutable_fmi2_import_get_directional_derivative_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    vector<fmi2_value_reference_t> v_ref(r->v_ref_size());
    vector<fmi2_value_reference_t> z_ref(r->z_ref_size());
    vector<fmi2_real_t> dv(r->dv_size()), dz(r->z_ref_size());

    for (int i = 0 ; i < r->v_ref_size() ; i++) {
      v_ref[i] = r->v_ref(i);
    }
    for (int i = 0 ; i < r->z_ref_size() ; i++) {
      z_ref[i] = r->z_ref(i);
    }
    for (int i = 0 ; i < r->dv_size() ; i++) {
      dv[i] = r->dv(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_directional_derivative_req(mid=%d,fmuId=%d,vref=%s,zref=%s,dv=%s)\n",r->message_id(),r->fmuid(),
        arrayToString(v_ref, r->v_ref_size()).c_str(), arrayToString(z_ref, r->z_ref_size()).c_str(), arrayToString(dv, r->dv_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_get_directional_derivative(m_fmi2Instance, v_ref.data(), r->v_ref_size(), z_ref.data(), r->z_ref_size(), dv.data(), dz.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_get_directional_derivative_res * getDirectionalDerivativesRes = res.mutable_fmi2_import_get_directional_derivative_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_directional_derivative_res);
    getDirectionalDerivativesRes->set_message_id(r->message_id());
    getDirectionalDerivativesRes->set_status(fmi2StatusToProtofmi2Status(status));
    for (int i = 0 ; i < r->z_ref_size() ; i++) {
      getDirectionalDerivativesRes->add_dz(dz[i]);
    }
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_directional_derivative_res(mid=%d,status=%d,dz=%s)\n",getDirectionalDerivativesRes->message_id(),getDirectionalDerivativesRes->status(),arrayToString(dz, r->z_ref_size()).c_str());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_get_xml_req) {

    // Unpack message
    fmitcp_proto::get_xml_req * r = req.mutable_get_xml_req();
    m_logger.log(Logger::LOG_NETWORK,"< get_xml_req(mid=%d,fmuId=%d)\n",r->message_id(),r->fmuid());

    string xml = "";
    if (!m_sendDummyResponses) {
      // interact with FMU
      string line;
      char* xmlFilePath = fmi_import_get_model_description_path(m_workingDir.c_str(), &m_jmCallbacks);
      m_logger.log(Logger::LOG_DEBUG,"xmlFilePath=%s\n",xmlFilePath);
      ifstream xmlFile (xmlFilePath);
      if (xmlFile.is_open()) {
        while (getline(xmlFile, line)) {
          xml.append(line).append("\n");
        }
        xmlFile.close();
      } else {
        m_logger.log(Logger::LOG_ERROR, "Error opening the %s file.\n", xmlFilePath);
      }
      free(xmlFilePath);
    }

    // Create response
    fmitcp_proto::get_xml_res * getXmlRes = res.mutable_get_xml_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_get_xml_res);
    getXmlRes->set_message_id(r->message_id());
    getXmlRes->set_loglevel(fmiJMLogLevelToProtoJMLogLevel(m_logLevel));
    getXmlRes->set_xml(xml);
    // only printing the first 38 characters of xml.
    m_logger.log(Logger::LOG_NETWORK,"> get_xml_res(mid=%d,logLevel=%d,xml=%.*s)\n",getXmlRes->message_id(), getXmlRes->loglevel(), 38, getXmlRes->xml().c_str());

  } else {
    // Something is wrong.
    sendResponse = false;
    m_logger.log(Logger::LOG_ERROR,"Message type not recognized: %d.\n",type);
  }

#ifdef USE_LACEWING
  if (sendResponse) {
    fmitcp::sendProtoBuffer(c,res.SerializeAsString());
  }
 }
#else
    if (sendResponse) {
        return res.SerializeAsString();
    } else {
        return "";
    }
#endif
}

#ifdef USE_LACEWING
void Server::error(lw_server s, lw_error error) {
  string err = lw_error_tostring(error);
  onError(err);
}

void Server::host(string hostName, long port) {
  // save this object in the server tag so we can use it later on.
  lw_server_set_tag(m_server, (void*)this);
  // connect the hooks
  lw_server_on_connect(   m_server, serverOnConnect);
  lw_server_on_data(      m_server, serverOnData);
  lw_server_on_disconnect(m_server, serverOnDisconnect);
  lw_server_on_error(     m_server, serverOnError);
  // setup the server port
  lw_filter filter = lw_filter_new();
  lw_filter_set_local_port(filter, port);
  // host/start the server
  lw_server_host_filter(m_server, filter);
  lw_filter_delete(filter);

  m_logger.log(Logger::LOG_NETWORK,"Listening to %s:%ld\n",hostName.c_str(),port);
}
#endif

void Server::sendDummyResponses(bool sendDummyResponses) {
  m_sendDummyResponses = sendDummyResponses;
}
