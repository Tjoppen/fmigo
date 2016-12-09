#include <fstream>

#include "Server.h"
#include "Logger.h"
#include "common.h"
#include "fmitcp.pb.h"
#include <FMI2/fmi2_xml_variable.h>
#ifndef WIN32
//no unistd.h on Windows IIRC
#include <unistd.h>
#endif

using namespace fmitcp;

/*!
 * Callback function for FMILibrary. Logs the FMILibrary operations.
 */
void jmCallbacksLogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
  fprintf(stderr, "[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}

Server::Server(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, std::string hdf5Filename, int filter_depth, const Logger &logger) {
  m_fmi2Outputs = NULL;
  m_fmuParsed = true;
  m_fmuPath = fmuPath;
  m_debugLogging = debugLogging;
  m_logLevel = logLevel;
  m_logger = logger;
  this->hdf5Filename = hdf5Filename;
  nextStateId = 0;
  m_sendDummyResponses = false;
  this->filter_depth = filter_depth;

  if(m_fmuPath == "dummy"){
    m_sendDummyResponses = true;
    m_fmuParsed = true;
    return;
  }

#ifndef WIN32
  //better message for non-existing FMUs
  if (access(m_fmuPath.c_str(), F_OK) == -1) {
    m_logger.log(Logger::LOG_ERROR, "FMU does not exist: %s\n", m_fmuPath.c_str());
    exit(1);
  }
#endif

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
  char* dir;
  if (!(dir = fmi_import_mk_temp_dir(&m_jmCallbacks, NULL, "fmitcp_"))) {
    fprintf(stderr, "fmi_import_mk_temp_dir() failed\n");
    exit(1);
  }
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
    m_logger.log(Logger::LOG_ERROR, "Unknown FMI version or FMU does not exist: working directory=%s, path=%s\n", m_workingDir.c_str(), m_fmuPath.c_str());
    m_fmuParsed = false;
    exit(1);
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
    {
        char *temp = fmi_import_create_URL_from_abs_path(&m_jmCallbacks, m_fmuPath.c_str());
        m_fmuLocation = temp;
        m_jmCallbacks.free(temp);
    }

    {
        char *temp = fmi_import_create_URL_from_abs_path(&m_jmCallbacks, m_workingDir.c_str());
        m_resourcePath = temp;
        m_resourcePath = m_resourcePath + "/resources";
        m_jmCallbacks.free(temp);
    }

    /* 0 - original order as found in the XML file;
     * 1 - sorted alphabetically by variable name;
     * 2 sorted by types/value references.
     */
    int sortOrder = 0;
    m_fmi2Variables = fmi2_import_get_variable_list(m_fmi2Instance, sortOrder);
    m_fmi2Outputs = fmi2_import_get_outputs_list(m_fmi2Instance);

#ifndef WIN32
    //prepare HDF5
    getHDF5Info();
#endif
  } else {
    // todo add FMI 1.0 later on.
    fmi_import_free_context(m_context);
    fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
    m_logger.log(Logger::LOG_ERROR, "Only FMI Co-Simulation 2.0 is supported.\n");
    m_fmuParsed = false;
    return;
  }
}

void Server::setStartValues() {
    //step through all variables, setReal/Integer/whatever() their start values, when present
    int nstart = 0;
    for (size_t x = 0; x < fmi2_import_get_variable_list_size(m_fmi2Variables); x++) {
        fmi2_import_variable_t *var = fmi2_import_get_variable(m_fmi2Variables, x);
        fmi2_value_reference_t vr = fmi2_import_get_variable_vr(var);
        fmi2_base_type_enu_t type = fmi2_import_get_variable_base_type(var);

        if (fmi2_xml_get_variable_has_start(var)) {
            nstart++;

            switch(type) {
            case fmi2_base_type_real: {
                fmi2_real_t r = fmi2_xml_get_real_variable_start(fmi2_xml_get_variable_as_real(var));
                fmi2_import_set_real(m_fmi2Instance, &vr, 1, &r);
                //fprintf(stderr, "Setting start=%f for VR=%i\n", r, vr);
                break;
            }
            case fmi2_base_type_int: {
                fmi2_integer_t i = fmi2_xml_get_integer_variable_start(fmi2_xml_get_variable_as_integer(var));
                fmi2_import_set_integer(m_fmi2Instance, &vr, 1, &i);
                //fprintf(stderr, "Setting start=%i for VR=%i\n", i, vr);
                break;
            }
            case fmi2_base_type_bool: {
                fmi2_boolean_t b = fmi2_xml_get_boolean_variable_start(fmi2_xml_get_variable_as_boolean(var));
                fmi2_import_set_boolean(m_fmi2Instance, &vr, 1, &b);
                //fprintf(stderr, "Setting start=%i for VR=%i\n", b, vr);
                break;
            }
            case fmi2_base_type_str: {
                fmi2_string_t s = fmi2_xml_get_string_variable_start(fmi2_xml_get_variable_as_string(var));
                fmi2_import_set_string(m_fmi2Instance, &vr, 1, &s);
                //fprintf(stderr, "Setting start=%s for VR=%i\n", s, vr);
                break;
            }
            case fmi2_base_type_enum: {
                fmi2_integer_t i = fmi2_xml_get_enum_variable_start(fmi2_xml_get_variable_as_enum(var));
                fmi2_import_set_integer(m_fmi2Instance, &vr, 1, &i);
                //fprintf(stderr, "Setting start=%i for VR=%i\n", i, vr);
                break;
            }
            }
        }
    }

    if (nstart) {
        fprintf(stderr, "%s: Initialized %i variables with values from modelDescription.xml\n", m_instanceName, nstart);
    }
}

Server::~Server() {
  if(m_fmi2Outputs!=NULL)   fmi2_import_free_variable_list(m_fmi2Outputs);
}

string Server::clientData(const char *data, size_t size) {
  fmitcp_proto::fmitcp_message req;
  bool parseStatus = req.ParseFromArray(data, size);
  fmitcp_proto::fmitcp_message_Type type = req.type();

  m_logger.log(Logger::LOG_DEBUG,"Parse status: %d\n", parseStatus);

  fmitcp_proto::fmitcp_message res;
  bool sendResponse = true;

#define SERVER_NORMAL_RESPONSE(type)                                    \
    {                                                                   \
      /* Create response */                                             \
      fmitcp_proto::fmi2_import_##type##_res * response = res.mutable_fmi2_import_##type##_res(); \
      res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_##type##_res); \
      response->set_message_id(r->message_id());                        \
      response->set_status(fmi2StatusToProtofmi2Status(status));        \
      m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_"#type"_res(mid=%d,status=%s)\n",response->message_id(),response->status()); \
    } 

  if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_instantiate_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_instantiate_req * r = req.mutable_fmi2_import_instantiate_req();
    int messageId = r->message_id();
    fmi2_boolean_t visible = r->visible();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_instantiate_req(mid=%d,visible=%d)\n",messageId, visible);

    jm_status_enu_t status = jm_status_success;
    if (!m_sendDummyResponses) {
      // instantiate FMU
      status = fmi2_import_instantiate(m_fmi2Instance, m_instanceName, fmi2_cosimulation, m_resourcePath.c_str(), visible);
    }

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_instantiate_res);
    fmitcp_proto::fmi2_import_instantiate_res * instantiateRes = res.mutable_fmi2_import_instantiate_res();
    instantiateRes->set_message_id(messageId);
    instantiateRes->set_status(fmiJMStatusToProtoJMStatus(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_instantiate_res(mid=%d,status=%d)\n",messageId,instantiateRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_setup_experiment_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_setup_experiment_req * r = req.mutable_fmi2_import_setup_experiment_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();
    bool toleranceDefined = r->has_tolerancedefined();
    double tolerance = r->tolerance();
    double starttime = r->starttime();
    bool stopTimeDefined = r->has_stoptimedefined();
    double stoptime = r->stoptime();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_setup_experiment_req(mid=%d)\n",messageId);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      status =  fmi2_import_setup_experiment(m_fmi2Instance, toleranceDefined, tolerance, starttime, stopTimeDefined, stoptime);
    }

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_setup_experiment_res);
    fmitcp_proto::fmi2_import_setup_experiment_res * initializeRes = res.mutable_fmi2_import_setup_experiment_res();
    initializeRes->set_message_id(messageId);
    initializeRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_setup_experiment_res(mid=%d,status=%d)\n",messageId,initializeRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_enter_initialization_mode_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_enter_initialization_mode_req * r = req.mutable_fmi2_import_enter_initialization_mode_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_enter_initialization_mode_req(mid=%d)\n",messageId);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      status = fmi2_import_enter_initialization_mode(m_fmi2Instance);
      setStartValues();
    }

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_enter_initialization_mode_res);
    fmitcp_proto::fmi2_import_enter_initialization_mode_res * initializeRes = res.mutable_fmi2_import_enter_initialization_mode_res();
    initializeRes->set_message_id(messageId);
    initializeRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_enter_initialization_mode_res(mid=%d,status=%d)\n",messageId,initializeRes->status());

} else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_exit_initialization_mode_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_exit_initialization_mode_req * r = req.mutable_fmi2_import_exit_initialization_mode_req();
    int messageId = r->message_id();
    int fmuId = r->fmuid();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_exit_initialization_mode_req(mid=%d)\n",messageId);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      status = fmi2_import_exit_initialization_mode(m_fmi2Instance);
    }

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_exit_initialization_mode_res);
    fmitcp_proto::fmi2_import_exit_initialization_mode_res * initializeRes = res.mutable_fmi2_import_exit_initialization_mode_res();
    initializeRes->set_message_id(messageId);
    initializeRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_exit_initialization_mode_res(mid=%d,status=%d)\n",messageId,initializeRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_terminate_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_terminate_req * r = req.mutable_fmi2_import_terminate_req();
    int fmuId = r->fmuid();
    int messageId = r->message_id();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_terminate_req(mid=%d,fmuId=%d)\n",messageId,fmuId);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // terminate FMU
      status = fmi2_import_terminate(m_fmi2Instance);
    }

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_terminate_res);
    fmitcp_proto::fmi2_import_terminate_res * terminateRes = res.mutable_fmi2_import_terminate_res();
    terminateRes->set_message_id(messageId);
    terminateRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_terminate_res(mid=%d,status=%d)\n",messageId,terminateRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_reset_req) {

    // Unpack message
    fmitcp_proto::fmi2_import_reset_req * r = req.mutable_fmi2_import_reset_req();
    int fmuId = r->fmuid();
    int messageId = r->message_id();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_reset_req(fmuId=%d)\n",fmuId);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // reset FMU
      status = fmi2_import_reset(m_fmi2Instance);
    }

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_reset_res);
    fmitcp_proto::fmi2_import_reset_res * resetRes = res.mutable_fmi2_import_reset_res();
    resetRes->set_message_id(messageId);
    resetRes->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_reset_res(mid=%d,status=%d)\n",messageId,resetRes->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_free_instance_req) {
    if (hdf5Filename.length()) {
        //dump hdf5 data
        //NOTE: this will only work if we have exactly one FMU instance on this server
        fprintf(stderr, "Gathered %li B HDF5 data\n", nrecords*rowsz);
        writeHDF5File(hdf5Filename, field_offset, field_types, field_names, "FMU", "fmu", nrecords, rowsz, hdf5data.data());
        nrecords = 0;
        hdf5data.clear();
    }

    // Unpack message
    fmitcp_proto::fmi2_import_free_instance_req * r = req.mutable_fmi2_import_free_instance_req();
    int fmuId = r->fmuid(),
        messageId = r->message_id();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_free_instance_req(fmuId=%d)\n",fmuId);

    // Create response message
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_free_instance_res);
    fmitcp_proto::fmi2_import_free_instance_res * resetRes = res.mutable_fmi2_import_free_instance_res();
    resetRes->set_message_id(messageId);

    if (!m_sendDummyResponses) {
      // Interact with FMU
      fmi2_import_free_instance(m_fmi2Instance);
      fmi2_import_destroy_dllfmu(m_fmi2Instance);
      fmi2_import_free(m_fmi2Instance);
      fmi_import_free_context(m_context);
      fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
    }

    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_free_instance_res(mid=%d)\n",messageId);

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
    //remember values
    currentCommunicationPoint = r->currentcommunicationpoint();
    communicationStepSize = r->communicationstepsize();
    bool newStep = r->newstep();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_do_step_req(fmuId=%d,commPoint=%g,stepSize=%g,newStep=%d)\n",fmuId,currentCommunicationPoint,communicationStepSize,newStep?1:0);

    if (hdf5Filename.length()) {
        //log outputs before doing anything
        hdf5data.insert(hdf5data.begin()+nrecords*rowsz, rowsz, 0);
        fillHDF5Row(&hdf5data[nrecords*rowsz], currentCommunicationPoint);
        nrecords++;
    }

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // Step the FMU
      status = fmi2_import_do_step(m_fmi2Instance, currentCommunicationPoint, communicationStepSize, newStep);
    }

    if (filter_depth) {
        //advance filter
        for (auto it : next_filter_map) {
            filter_log[it.first].push_back(it.second);

            if (filter_log[it.first].size() > filter_depth) {
                filter_log[it.first].pop_front();
            }
        }

        for (auto it : filter_log) {
            next_filter_map[it.first] = it.second.back();
        }
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

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_time_req){
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_set_time_req * r = req.mutable_fmi2_import_set_time_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_time_req(mid=%d,fmuId=%d,time=%f)\n",r->message_id(), r->fmuid(), r->time());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      status = fmi2_import_set_time(m_fmi2Instance, r->time());
    }

    // Create message
    SERVER_NORMAL_RESPONSE(set_time);
} else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_continuous_states_req){
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_set_continuous_states_req * r = req.mutable_fmi2_import_set_continuous_states_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_continuous_states_req(mid=%d,fmuId=%d,x=%f)\n",r->message_id(), r->fmuid(), r->x());

    fmi2_status_t status = fmi2_status_ok;
    std::vector<fmi2_real_t> x;
    x.reserve(r->nx()*sizeof(fmi2_real_t));
    for(int i; i<r->nx();i++)
      x[i] = r->x(i);

    if (!m_sendDummyResponses) {
      status = fmi2_import_set_continuous_states(m_fmi2Instance, x.data(), r->nx());
    }

    // Create response
    SERVER_NORMAL_RESPONSE(set_continuous_states);
    
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_completed_integrator_step_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_initialize_model_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_derivatives_req){
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_get_derivatives_req * r = req.mutable_fmi2_import_get_derivatives_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_derivatives_req(mid=%d,fmuId=%d,nderivatives=%d)\n",r->message_id(), r->fmuid(), r->nderivatives());

    fmi2_status_t status = fmi2_status_ok;
    std::vector<fmi2_real_t> derivatives;
    derivatives.reserve(r->nderivatives()*sizeof(fmi2_real_t));

    if (!m_sendDummyResponses) {
      status = fmi2_import_get_derivatives(m_fmi2Instance, derivatives.data(), r->nderivatives());
    }

    //Create response
    fmitcp_proto::fmi2_import_get_derivatives_res * response = res.mutable_fmi2_import_get_derivatives_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_derivatives_res);
    response->set_message_id(r->message_id());
    response->set_status(fmi2StatusToProtofmi2Status(status));
    for(int i = 0; i< r->nderivatives();i++)
      response->add_derivatives(derivatives[i]);

    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_derivatives_res(mid=%d,derivatives=%s)\n",response->message_id(),response->derivatives());
    
     sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_event_indicators_req){
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_get_event_indicators_req * r = req.mutable_fmi2_import_get_event_indicators_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_event_indicators_req(mid=%d,fmuId=%d,nz=%d)\n",r->message_id(), r->fmuid(), r->nz());

    fmi2_status_t status = fmi2_status_ok;
    std::vector<fmi2_real_t> z;
    z.reserve(r->nz()*sizeof(fmi2_real_t));

    if (!m_sendDummyResponses) {
      status = fmi2_import_get_event_indicators(m_fmi2Instance, z.data(), r->nz());
    }

    //Create response
    fmitcp_proto::fmi2_import_get_event_indicators_res * response = res.mutable_fmi2_import_get_event_indicators_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_event_indicators_res);
    response->set_message_id(r->message_id());
    response->set_status(fmi2StatusToProtofmi2Status(status));
    for(int i = 0; i< r->nz();i++)
      response->add_z(z[i]);

    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_event_indicators_res(mid=%d,z=%s)\n",response->message_id(),response->z());
                                                                                                                                                            
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_eventUpdate_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_completed_event_iteration_req){
    // TODO
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_continuous_states_req){
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_get_continuous_states_req * r = req.mutable_fmi2_import_get_continuous_states_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_continuous_states_req(mid=%d,fmuId=%d,nx=%d)\n",r->message_id(), r->fmuid(), r->nx());

    fmi2_status_t status = fmi2_status_ok;
    std::vector<fmi2_real_t> x;
    x.reserve(r->nx()*sizeof(fmi2_real_t));

    if (!m_sendDummyResponses) {
      status = fmi2_import_get_continuous_states(m_fmi2Instance, x.data(), r->nx());
    }

    //Create response
    fmitcp_proto::fmi2_import_get_continuous_states_res * response = res.mutable_fmi2_import_get_continuous_states_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_continuous_states_res);
    response->set_message_id(r->message_id());
    response->set_status(fmi2StatusToProtofmi2Status(status));
    for(int i = 0; i< r->nx();i++)
      response->add_x(x[i]);

    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_continuous_states_res(mid=%d,x=%s)\n",response->message_id(),response->x());
    sendResponse = false;
  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_nominal_continuous_states_req){
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_get_nominal_continuous_states_req * r = req.mutable_fmi2_import_get_nominal_continuous_states_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_nominal_continuous_states_req(mid=%d,fmuId=%d,nx=%d)\n",r->message_id(), r->fmuid(), r->nx());

    fmi2_status_t status = fmi2_status_ok;
    std::vector<fmi2_real_t> nominal;
    nominal.reserve(r->nx()*sizeof(fmi2_real_t));

    if (!m_sendDummyResponses) {
      status = fmi2_import_get_nominals_of_continuous_states(m_fmi2Instance, nominal.data(), r->nx());
    }

    //Create response
    fmitcp_proto::fmi2_import_get_nominal_continuous_states_res * response = res.mutable_fmi2_import_get_nominal_continuous_states_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_nominal_continuous_states_res);
    response->set_message_id(r->message_id());
    response->set_status(fmi2StatusToProtofmi2Status(status));
    for(int i = 0; i< r->nx();i++)
      response->add_nominal(nominal[i]);

    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_nominal_continuous_states_res(mid=%d,nominals=%s)\n",response->message_id(),response->nominal());
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
            //use filter if we should
            if (filter_depth) {
                //remember the value for do_step() to push it into the log
                next_filter_map[vr[i]] = value[i];

                //check that there's a corresponding entry and that it has values
                auto it = filter_log.find(vr[i]);
                if (it != filter_log.end() && it->second.size() > 0) {
                    fmi2_real_t sum = value[i];
                    for (fmi2_real_t val : it->second) {
                        sum += val;
                    }
                    getRealRes->add_values(sum / (1 + it->second.size()));
                } else {
                    getRealRes->add_values(value[i]);
                }
            } else {
                getRealRes->add_values(value[i]);
            }
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

    fmi2_status_t status = fmi2_status_ok;
    ::google::protobuf::int32 stateId = nextStateId++;
    if(!m_sendDummyResponses){
        fmi2_FMU_state_t state;
        status = fmi2_import_get_fmu_state(m_fmi2Instance, &state);
        stateMap[stateId] = state;
    }

    // Create response
    fmitcp_proto::fmi2_import_get_fmu_state_res * res2 = res.mutable_fmi2_import_get_fmu_state_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_get_fmu_state_res);
    res2->set_message_id(r->message_id());
    res2->set_status(fmi2StatusToProtofmi2Status(status));
    res2->set_stateid(stateId);
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_fmu_state_res(mid=%d,stateId=%d,status=%d)\n",res2->message_id(),res2->stateid(),res2->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_fmu_state_req){

    // Unpack message
    fmitcp_proto::fmi2_import_set_fmu_state_req * r = req.mutable_fmi2_import_set_fmu_state_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_fmu_state_req(mid=%d,fmuId=%d,stateId=%d)\n",r->message_id(),r->fmuid(),r->stateid());

    fmi2_status_t status = fmi2_status_ok;
    if(!m_sendDummyResponses){
        status = fmi2_import_set_fmu_state(m_fmi2Instance, stateMap[r->stateid()]);
    }

    // Create response
    fmitcp_proto::fmi2_import_set_fmu_state_res * res2 = res.mutable_fmi2_import_set_fmu_state_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_set_fmu_state_res);
    res2->set_message_id(r->message_id());
    res2->set_status(fmi2StatusToProtofmi2Status(status));
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_set_fmu_state_res(mid=%d,status=%d)\n",res2->message_id(),res2->status());

  } else if(type == fmitcp_proto::fmitcp_message_Type_type_fmi2_import_free_fmu_state_req){

    // Unpack message
    fmitcp_proto::fmi2_import_free_fmu_state_req * r = req.mutable_fmi2_import_free_fmu_state_req();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_free_fmu_state_req(mid=%d,stateId=%d)\n",r->message_id(),r->stateid());

    fmi2_status_t status = fmi2_status_ok;
    if(!m_sendDummyResponses){
        auto it = stateMap.find(r->stateid());
        status = fmi2_import_free_fmu_state(m_fmi2Instance, &it->second);
        stateMap.erase(it);
    }

    // Create response
    fmitcp_proto::fmi2_import_free_fmu_state_res * res2 = res.mutable_fmi2_import_free_fmu_state_res();
    res.set_type(fmitcp_proto::fmitcp_message_Type_type_fmi2_import_free_fmu_state_res);
    res2->set_message_id(r->message_id());
    res2->set_status(fmitcp_proto::fmi2_status_ok);
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_free_fmu_state_res(mid=%d,status=%d)\n",res2->message_id(),res2->status());

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
     if (hasCapability(fmi2_cs_providesDirectionalDerivatives)) {
      // interact with FMU
      status = fmi2_import_get_directional_derivative(m_fmi2Instance, v_ref.data(), r->v_ref_size(), z_ref.data(), r->z_ref_size(), dv.data(), dz.data());
     } else if (hasCapability(fmi2_cs_canGetAndSetFMUstate)) {
      dz = computeNumericalJacobian(z_ref, v_ref, dv);
     } else {
      m_logger.log(Logger::LOG_ERROR, "Tried to fmi2_import_get_directional_derivative() on FMU without directional derivatives or ability to save/load FMU state\n");
      status = fmi2_status_error;
     }
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

    if (sendResponse) {
        return res.SerializeAsString();
    } else {
        return "";
    }
}

void Server::sendDummyResponses(bool sendDummyResponses) {
  m_sendDummyResponses = sendDummyResponses;
}

static size_t fmi2_type_size(fmi2_base_type_enu_t type) {
    switch (type) {
    case fmi2_base_type_real: return sizeof(fmi2_real_t);
    case fmi2_base_type_int:  return sizeof(fmi2_integer_t);
    case fmi2_base_type_bool: return sizeof(fmi2_boolean_t);
    default:
        fprintf(stderr, "Type not supported for HDF5 output\n");
        exit(1);
    }
}

static hid_t fmi2_type_to_hdf5(fmi2_base_type_enu_t type) {
    switch (type) {
    case fmi2_base_type_real: return H5T_NATIVE_DOUBLE;
    case fmi2_base_type_int:  return H5T_NATIVE_INT;
    case fmi2_base_type_bool: return H5T_NATIVE_INT;
    default:
        fprintf(stderr, "Type not supported for HDF5 output\n");
        exit(1);
    }
}

void Server::getHDF5Info() {
    if (!hdf5Filename.length()) {
        return;
    }

    //first entry is time
    field_offset.push_back(0);
    field_names.push_back("currentCommunicationPoint");
    field_types.push_back(H5T_NATIVE_DOUBLE);
    size_t ofs = sizeof(double);

    for (size_t x = 0; x < fmi2_import_get_variable_list_size(m_fmi2Outputs); x++) {
        fmi2_import_variable_t *var = fmi2_import_get_variable(m_fmi2Outputs, x);
        fmi2_base_type_enu_t type = fmi2_import_get_variable_base_type(var);

        field_offset.push_back(ofs);
        field_names.push_back(fmi2_import_get_variable_name(var));
        field_types.push_back(fmi2_type_to_hdf5(type));

        ofs += fmi2_type_size(type);
    }

    rowsz = ofs;

    //preallocate some space for HDF5 data
    nrecords = 0;
    size_t res = rowsz*1000;
    hdf5data.reserve(res);
}

void Server::fillHDF5Row(char *dest, double t) {
    *reinterpret_cast<double*>(dest + field_offset[0]) = t;

    for (size_t x = 0; x < fmi2_import_get_variable_list_size(m_fmi2Outputs); x++) {
        fmi2_import_variable_t *var = fmi2_import_get_variable(m_fmi2Outputs, x);
        fmi2_base_type_enu_t type = fmi2_import_get_variable_base_type(var);
        fmi2_value_reference_t vr = fmi2_import_get_variable_vr(var);

        switch (type) {
        case fmi2_base_type_real: fmi2_import_get_real(m_fmi2Instance,    &vr, 1, reinterpret_cast<fmi2_real_t*>(   dest + field_offset[x+1])); break;
        case fmi2_base_type_int:  fmi2_import_get_integer(m_fmi2Instance, &vr, 1, reinterpret_cast<fmi2_integer_t*>(dest + field_offset[x+1])); break;
        case fmi2_base_type_bool: fmi2_import_get_boolean(m_fmi2Instance, &vr, 1, reinterpret_cast<fmi2_boolean_t*>(dest + field_offset[x+1])); break;
        default:
            fprintf(stderr, "Type not supported for HDF5 output\n");
            exit(1);
        }
    }
}

bool Server::hasCapability(fmi2_capabilities_enu_t cap) const {
    return fmi2_import_get_capability(m_fmi2Instance, cap) != 0;
}

vector<fmi2_real_t> Server::computeNumericalJacobian(
        const vector<fmi2_value_reference_t>& z_ref,
        const vector<fmi2_value_reference_t>& v_ref,
        const vector<fmi2_real_t>& dv) {
    vector<fmi2_real_t> dz;
    fmi2_FMU_state_t state;
    double t = currentCommunicationPoint + communicationStepSize;
    double dt = 1e-3 * communicationStepSize;

    fmi2_import_get_fmu_state(m_fmi2Instance, &state);

    //this assumes the system is linear
    //conveniently this allows us to simplify things to just two do_step() calls
    vector<fmi2_real_t> v0(v_ref.size()), v1;
    vector<fmi2_real_t> z0(z_ref.size());
    vector<fmi2_real_t> z1(z_ref.size());

    fmi2_import_get_real(m_fmi2Instance, v_ref.data(), v_ref.size(), v0.data());
    fmi2_import_do_step(m_fmi2Instance, t, dt, false);
    fmi2_import_get_real(m_fmi2Instance, z_ref.data(), z_ref.size(), z0.data());
    fmi2_import_set_fmu_state(m_fmi2Instance, state);

    for (size_t x = 0; x < v_ref.size(); x++) {
        v1.push_back(v0[x] + dv[x]);
    }

    fmi2_import_set_real(m_fmi2Instance, v_ref.data(), v_ref.size(), v1.data());
    fmi2_import_do_step(m_fmi2Instance, t, dt, false);
    fmi2_import_get_real(m_fmi2Instance, z_ref.data(), z_ref.size(), z1.data());
    fmi2_import_set_fmu_state(m_fmi2Instance, state);
    fmi2_import_free_fmu_state(m_fmi2Instance, &state);

    for (size_t x = 0; x < z_ref.size(); x++) {
        /**
         * NOTE: This works because the master only cares about computing mobilities,
         * which can be defined as:
         *
         *  m^-1 = a/f = da/df = (a1 - a0) / (f1 - f0)
         *
         * So since we only ever get partial derivatives on acceleration outputs,
         * and f1 - f0 (aka dv) has magnitude one, this simplifies to the
         * z1 - z0 computation below.
         *
         * A more proper solution would at least divide the difference by the
         * magnitude of dv.
         */
        dz.push_back(z1[x] - z0[x]);
    }

    return dz;
}
