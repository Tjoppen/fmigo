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
#include <stdint.h>

using namespace fmitcp;

/*!
 * Callback function for FMILibrary. Logs the FMILibrary operations.
 */
void jmCallbacksLogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
  fprintf(stderr, "[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}

Server::Server(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, std::string hdf5Filename, const Logger &logger) {
  m_fmi2Outputs = NULL;
  m_fmuParsed = true;
  m_fmuPath = fmuPath;
  m_debugLogging = debugLogging;
  m_logLevel = logLevel;
  m_logger = logger;
  this->hdf5Filename = hdf5Filename;
  nextStateId = 0;
  m_sendDummyResponses = false;

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
    if(fmuType != fmi2_fmu_kind_cs && fmuType != fmi2_fmu_kind_me_and_cs && fmuType != fmi2_fmu_kind_me) {
      fmi2_import_free(m_fmi2Instance);
      fmi_import_free_context(m_context);
      fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
      m_logger.log(Logger::LOG_ERROR, "Unknown fmuType %i\n", fmuType);
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
    m_logger.log(Logger::LOG_ERROR, "Only FMI 2.0 is supported.\n");
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
  std::pair<fmitcp_proto::fmitcp_message_Type,std::string> ret;
  ret.second = "error"; //to detect if we forgot to set ret.second somewhere below

  fmitcp_proto::fmitcp_message_Type type = parseType(data, size);

  data += 2;
  size -= 2;

  bool sendResponse = true;

#define SERVER_NORMAL_MESSAGE(type)                                     \
  /* Unpack message */                                                  \
    fmitcp_proto::fmi2_import_##type##_req r; r.ParseFromArray(data, size); \
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_"#type"_req()\n"); \
                                                                        \
    fmi2_status_t status = fmi2_status_ok;                              \
    if (!m_sendDummyResponses) {                                        \
      status = fmi2_import_##type(m_fmi2Instance);                      \
    }

#define SERVER_NORMAL_RESPONSE(type)                                    \
      /* Create response */                                             \
      fmitcp_proto::fmi2_import_##type##_res response; \
      response.set_status(fmi2StatusToProtofmi2Status(status));        \
      ret.first = fmitcp_proto::type_fmi2_import_##type##_res; \
      ret.second = response.SerializeAsString(); \
      m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_"#type"_res(status=%s)\n",response.status());

  switch (type) {
  case fmitcp_proto::type_fmi2_import_get_version_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_version_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_version_req()\n");

    const char* version = "VeRsIoN";
    if (!m_sendDummyResponses) {
      // get FMU version
      version = fmi2_import_get_version(m_fmi2Instance);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_version_res getVersionRes;
    getVersionRes.set_version(version);
    ret.first = fmitcp_proto::type_fmi2_import_get_version_res;
    ret.second = getVersionRes.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_version_res()\n");

  break; } case fmitcp_proto::type_fmi2_import_set_debug_logging_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_set_debug_logging_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_debug_logging_req(loggingOn=%d,categories=...)\n",r.loggingon());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // set the debug logging for FMU
      // fetch the logging categories from the FMU
      size_t nCategories = fmi2_import_get_log_categories_num(m_fmi2Instance);
      vector<fmi2_string_t> categories(nCategories);
      size_t i;
      for (i = 0 ; i < nCategories ; i++) {
        categories[i] = fmi2_import_get_log_category(m_fmi2Instance, i);
      }
      // set debug logging. We don't care about its result.
      status = fmi2_import_set_debug_logging(m_fmi2Instance, m_debugLogging, nCategories, categories.data());
    }

    SERVER_NORMAL_RESPONSE(set_debug_logging);

  break; } case fmitcp_proto::type_fmi2_import_instantiate_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_instantiate_req r; r.ParseFromArray(data, size);
    fmi2_boolean_t visible = r.visible();

    fmi2_type_t simType;
    simType = fmi2_cosimulation;
    if (r.has_fmutype() && r.fmutype() == 2)
      simType = fmi2_model_exchange;

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_instantiate_req(visible=%d)\n", visible);

    jm_status_enu_t status = jm_status_success;
    if (!m_sendDummyResponses) {
      // instantiate FMU
      status = fmi2_import_instantiate(m_fmi2Instance, m_instanceName, simType, m_resourcePath.c_str(), visible);
    }

    // Create response message
    fmitcp_proto::fmi2_import_instantiate_res instantiateRes;
    instantiateRes.set_status(fmiJMStatusToProtoJMStatus(status));
    ret.first = fmitcp_proto::type_fmi2_import_instantiate_res;
    ret.second = instantiateRes.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_instantiate_res(status=%d)\n",instantiateRes.status());

  break; } case fmitcp_proto::type_fmi2_import_free_instance_req: {
    if (hdf5Filename.length()) {
        //dump hdf5 data
        //NOTE: this will only work if we have exactly one FMU instance on this server
        fprintf(stderr, "Gathered %li B HDF5 data\n", nrecords*rowsz);
        writeHDF5File(hdf5Filename, field_offset, field_types, field_names, "FMU", "fmu", nrecords, rowsz, hdf5data.data());
        nrecords = 0;
        hdf5data.clear();
    }

    // Unpack message
    fmitcp_proto::fmi2_import_free_instance_req r; r.ParseFromArray(data, size);

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_free_instance_req()\n");

    if (!m_sendDummyResponses) {
      // Interact with FMU
      fmi2_import_free_instance(m_fmi2Instance);
      fmi2_import_destroy_dllfmu(m_fmi2Instance);
      fmi2_import_free(m_fmi2Instance);
      fmi_import_free_context(m_context);
      fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
    }

    // Create response message
    fmitcp_proto::fmi2_import_free_instance_res resetRes;
    ret.first = fmitcp_proto::type_fmi2_import_free_instance_res;
    ret.second = resetRes.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_free_instance_res()\n");

  break; } case fmitcp_proto::type_fmi2_import_setup_experiment_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_setup_experiment_req r; r.ParseFromArray(data, size);
    bool toleranceDefined = r.has_tolerancedefined();
    double tolerance = r.tolerance();
    double starttime = r.starttime();
    bool stopTimeDefined = r.has_stoptimedefined();
    double stoptime = r.stoptime();

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_setup_experiment_req()\n");

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      status =  fmi2_import_setup_experiment(m_fmi2Instance, toleranceDefined, tolerance, starttime, stopTimeDefined, stoptime);
    }

    SERVER_NORMAL_RESPONSE(setup_experiment);

  break; } case fmitcp_proto::type_fmi2_import_enter_initialization_mode_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_enter_initialization_mode_req r; r.ParseFromArray(data, size);

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_enter_initialization_mode_req()\n");

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      status = fmi2_import_enter_initialization_mode(m_fmi2Instance);
      setStartValues();
    }

    SERVER_NORMAL_RESPONSE(enter_initialization_mode);

  break; } case fmitcp_proto::type_fmi2_import_exit_initialization_mode_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_exit_initialization_mode_req r; r.ParseFromArray(data, size);

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_exit_initialization_mode_req()\n");

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      status = fmi2_import_exit_initialization_mode(m_fmi2Instance);
    }

    SERVER_NORMAL_RESPONSE(exit_initialization_mode);

  break; } case fmitcp_proto::type_fmi2_import_terminate_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_terminate_req r; r.ParseFromArray(data, size);

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_terminate_req()\n");

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // terminate FMU
      status = fmi2_import_terminate(m_fmi2Instance);
    }

    SERVER_NORMAL_RESPONSE(terminate);

  break; } case fmitcp_proto::type_fmi2_import_reset_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_reset_req r; r.ParseFromArray(data, size);

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_reset_req()\n");

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // reset FMU
      status = fmi2_import_reset(m_fmi2Instance);
    }

    SERVER_NORMAL_RESPONSE(reset);

  break; } case fmitcp_proto::type_fmi2_import_get_real_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_real_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> vr(r.valuereferences_size());
    vector<fmi2_real_t> value(r.valuereferences_size());

    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      vr[i] = r.valuereferences(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_req(vrs=%s)\n",arrayToString(vr, r.valuereferences_size()).c_str());

    // Create response
    fmitcp_proto::fmi2_import_get_real_res response;

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
        // interact with FMU
        status = fmi2_import_get_real(m_fmi2Instance, vr.data(), r.valuereferences_size(), value.data());
        response.set_status(fmi2StatusToProtofmi2Status(status));
        for (int i = 0 ; i < r.valuereferences_size() ; i++) {
            response.add_values(value[i]);
        }
    } else {
        // Set dummy values
        for (int i = 0 ; i < r.valuereferences_size() ; i++) {
            response.add_values(0.0);
        }
        response.set_status(fmi2StatusToProtofmi2Status(fmi2_status_ok));
    }

    ret.first = fmitcp_proto::type_fmi2_import_get_real_res;
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_real_res(status=%d,values=%s)\n",response.status(),arrayToString(value, r.valuereferences_size()).c_str());

  break; } case fmitcp_proto::type_fmi2_import_get_integer_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_integer_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> vr(r.valuereferences_size());
    vector<fmi2_integer_t> value(r.valuereferences_size());

    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      vr[i] = r.valuereferences(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_integer_req(vrs=%s)\n",arrayToString(vr, r.valuereferences_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_get_integer(m_fmi2Instance, vr.data(), r.valuereferences_size(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_get_integer_res response;
    response.set_status(fmi2StatusToProtofmi2Status(status));
    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      response.add_values(value[i]);
    }
    ret.first = fmitcp_proto::type_fmi2_import_get_integer_res;
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_integer_res(status=%d,values=%s)\n",response.status(),arrayToString(value, r.valuereferences_size()).c_str());

  break; } case fmitcp_proto::type_fmi2_import_get_boolean_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_boolean_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> vr(r.valuereferences_size());
    vector<fmi2_boolean_t> value(r.valuereferences_size());

    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      vr[i] = r.valuereferences(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_boolean_req(vrs=%s)\n",arrayToString(vr, r.valuereferences_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_get_boolean(m_fmi2Instance, vr.data(), r.valuereferences_size(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_get_boolean_res response;
    response.set_status(fmi2StatusToProtofmi2Status(status));
    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      response.add_values(value[i]);
    }
    ret.first = fmitcp_proto::type_fmi2_import_get_boolean_res;
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_boolean_res(status=%d,values=%s)\n",response.status(),arrayToString(value, r.valuereferences_size()).c_str());

  break; } case fmitcp_proto::type_fmi2_import_get_string_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_string_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> vr(r.valuereferences_size());
    vector<fmi2_string_t> value(r.valuereferences_size());

    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      vr[i] = r.valuereferences(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_string_req(vrs=%s)\n",arrayToString(vr, r.valuereferences_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_get_string(m_fmi2Instance, vr.data(), r.valuereferences_size(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_get_string_res response;
    response.set_status(fmi2StatusToProtofmi2Status(status));
    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      response.add_values(value[i]);
    }
    ret.first = fmitcp_proto::type_fmi2_import_get_string_res;
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_string_res(status=%d,values=%s)\n",response.status(),arrayToString(value, r.valuereferences_size()).c_str());

  break; } case fmitcp_proto::type_fmi2_import_set_real_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_set_real_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> vr(r.valuereferences_size());
    vector<fmi2_real_t> value(r.values_size());

    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      vr[i] = r.valuereferences(i);
      value[i] = r.values(i);
    }

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_real_req(vrs=%s,values=%s)\n",
        arrayToString(vr, r.valuereferences_size()).c_str(), arrayToString(value, r.values_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
       status = fmi2_import_set_real(m_fmi2Instance, vr.data(), r.valuereferences_size(), value.data());
    }

    SERVER_NORMAL_RESPONSE(set_real);

  break; } case fmitcp_proto::type_fmi2_import_set_integer_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_set_integer_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> vr(r.valuereferences_size());
    vector<fmi2_integer_t> value(r.values_size());

    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      vr[i] = r.valuereferences(i);
      value[i] = r.values(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_integer_req(vrs=%s,values=%s)\n",
        arrayToString(vr, r.valuereferences_size()).c_str(), arrayToString(value, r.values_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_set_integer(m_fmi2Instance, vr.data(), r.valuereferences_size(), value.data());
    }

    SERVER_NORMAL_RESPONSE(set_integer);

  break; } case fmitcp_proto::type_fmi2_import_set_boolean_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_set_boolean_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> vr(r.valuereferences_size());
    vector<fmi2_boolean_t> value(r.values_size());

    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      vr[i] = r.valuereferences(i);
      value[i] = r.values(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_boolean_req(vrs=%s,values=%s)\n",
        arrayToString(vr, r.valuereferences_size()).c_str(), arrayToString(value, r.values_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_set_boolean(m_fmi2Instance, vr.data(), r.valuereferences_size(), value.data());
    }

    SERVER_NORMAL_RESPONSE(set_boolean);

  break; } case fmitcp_proto::type_fmi2_import_set_string_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_set_string_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> vr(r.valuereferences_size());
    vector<fmi2_string_t> value(r.values_size());

    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      vr[i] = r.valuereferences(i);
      value[i] = r.values(i).c_str();
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_string_req(vrs=%s,values=%s)\n",
        arrayToString(vr, r.valuereferences_size()).c_str(), arrayToString(value, r.values_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_set_string(m_fmi2Instance, vr.data(), r.valuereferences_size(), value.data());
    }

    SERVER_NORMAL_RESPONSE(set_string);

  break; } case fmitcp_proto::type_fmi2_import_get_fmu_state_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_fmu_state_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_fmu_state_req()\n");

    fmi2_status_t status = fmi2_status_ok;
    ::google::protobuf::int32 stateId = nextStateId++;
    if(!m_sendDummyResponses){
        fmi2_FMU_state_t state;
        status = fmi2_import_get_fmu_state(m_fmi2Instance, &state);
        stateMap[stateId] = state;
    }

    // Create response
    fmitcp_proto::fmi2_import_get_fmu_state_res response;
    response.set_status(fmi2StatusToProtofmi2Status(status));
    response.set_stateid(stateId);
    ret.first = fmitcp_proto::type_fmi2_import_get_fmu_state_res;
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_fmu_state_res(stateId=%d,status=%d)\n",response.stateid(),response.status());

  break; } case fmitcp_proto::type_fmi2_import_set_fmu_state_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_set_fmu_state_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_fmu_state_req(stateId=%d)\n",r.stateid());

    fmi2_status_t status = fmi2_status_ok;
    if(!m_sendDummyResponses){
        status = fmi2_import_set_fmu_state(m_fmi2Instance, stateMap[r.stateid()]);
    }

    SERVER_NORMAL_RESPONSE(set_fmu_state);

  break; } case fmitcp_proto::type_fmi2_import_free_fmu_state_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_free_fmu_state_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_free_fmu_state_req(stateId=%d)\n",r.stateid());

    fmi2_status_t status = fmi2_status_ok;
    if(!m_sendDummyResponses){
        auto it = stateMap.find(r.stateid());
        status = fmi2_import_free_fmu_state(m_fmi2Instance, &it->second);
        stateMap.erase(it);
    }

    // Create response
    fmitcp_proto::fmi2_import_free_fmu_state_res response;
    response.set_status(fmitcp::fmi2StatusToProtofmi2Status(status));
    ret.first = fmitcp_proto::type_fmi2_import_free_fmu_state_res;
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_free_fmu_state_res(status=%d)\n",response.status());

  // break; } case fmitcp_proto::type_fmi2_import_serialized_fmu_state_size_req: {
  //   // TODO
  //   sendResponse = false;
  // break; } case fmitcp_proto::type_fmi2_import_serialize_fmu_state_req: {
  //   // TODO
  //   sendResponse = false;
  // break; } case fmitcp_proto::type_fmi2_import_de_serialize_fmu_state_req: {
  //   // TODO
  //   sendResponse = false;
  break; } case fmitcp_proto::type_fmi2_import_get_directional_derivative_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_directional_derivative_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> v_ref(r.v_ref_size());
    vector<fmi2_value_reference_t> z_ref(r.z_ref_size());
    vector<fmi2_real_t> dv(r.dv_size()), dz(r.z_ref_size());

    for (int i = 0 ; i < r.v_ref_size() ; i++) {
      v_ref[i] = r.v_ref(i);
    }
    for (int i = 0 ; i < r.z_ref_size() ; i++) {
      z_ref[i] = r.z_ref(i);
    }
    for (int i = 0 ; i < r.dv_size() ; i++) {
      dv[i] = r.dv(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_directional_derivative_req(vref=%s,zref=%s,dv=%s)\n",
        arrayToString(v_ref, r.v_ref_size()).c_str(), arrayToString(z_ref, r.z_ref_size()).c_str(), arrayToString(dv, r.dv_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
     if (hasCapability(fmi2_cs_providesDirectionalDerivatives)) {
      // interact with FMU
      status = fmi2_import_get_directional_derivative(m_fmi2Instance, v_ref.data(), r.v_ref_size(), z_ref.data(), r.z_ref_size(), dv.data(), dz.data());
     } else if (hasCapability(fmi2_cs_canGetAndSetFMUstate)) {
      dz = computeNumericalJacobian(z_ref, v_ref, dv);
     } else {
      m_logger.log(Logger::LOG_ERROR, "Tried to fmi2_import_get_directional_derivative() on FMU without directional derivatives or ability to save/load FMU state\n");
      status = fmi2_status_error;
     }
    }

    // Create response
    fmitcp_proto::fmi2_import_get_directional_derivative_res response;
    response.set_status(fmi2StatusToProtofmi2Status(status));
    for (int i = 0 ; i < r.z_ref_size() ; i++) {
      response.add_dz(dz[i]);
    }
    ret.first = fmitcp_proto::type_fmi2_import_get_directional_derivative_res;
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_directional_derivative_res(status=%d,dz=%s)\n",response.status(),arrayToString(dz, r.z_ref_size()).c_str());

  break; } case fmitcp_proto::type_fmi2_import_enter_event_mode_req: {
    // TODO
    SERVER_NORMAL_MESSAGE(enter_event_mode);
    SERVER_NORMAL_RESPONSE(enter_event_mode);
  break; } case fmitcp_proto::type_fmi2_import_new_discrete_states_req: {
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_new_discrete_states_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_new_discrete_states_req()\n");

    fmi2_event_info_t eventInfo;
    if (!m_sendDummyResponses) {
      fmi2_import_new_discrete_states(m_fmi2Instance, &eventInfo);
    }

    //Create response
    fmitcp_proto::fmi2_import_new_discrete_states_res response;
    fmitcp_proto::fmi2_event_info_t* eventin = fmi2EventInfoToProtoEventInfo(eventInfo);
    response.set_allocated_eventinfo(eventin);
    fprintf(stderr,"Server.cpp: %d %d %d %d %d %f \n",
            response.eventinfo().newdiscretestatesneeded(),
            response.eventinfo().terminatesimulation(),
            response.eventinfo().nominalsofcontinuousstateschanged(),
            response.eventinfo().valuesofcontinuousstateschanged(),
            response.eventinfo().nexteventtimedefined(),
            response.eventinfo().nexteventtime());
    //response.set_allocated_eventinfo(fmi2EventInfoToProtoEventInfo(eventInfo));
    //fmitcp_proto::fmi2_event_info_t * ei = response.mutable_eventinfo();
    //response.mutable_eventinfo(eventInfo);

    ret.first = fmitcp_proto::type_fmi2_import_new_discrete_states_res;
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_new_discrete_states_res()\n");
  break; } case fmitcp_proto::type_fmi2_import_enter_continuous_time_mode_req: {
    // TODO
    SERVER_NORMAL_MESSAGE(enter_continuous_time_mode);
    SERVER_NORMAL_RESPONSE(enter_continuous_time_mode);
  break; } case fmitcp_proto::type_fmi2_import_completed_integrator_step_req: {
    // TODO
    sendResponse = false;
  break; } case fmitcp_proto::type_fmi2_import_set_time_req: {
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_set_time_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_time_req(time=%f)\n", r.time());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      status = fmi2_import_set_time(m_fmi2Instance, r.time());
    }

    // Create message
    SERVER_NORMAL_RESPONSE(set_time);
  break; } case fmitcp_proto::type_fmi2_import_set_continuous_states_req: {
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_set_continuous_states_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_continuous_states_req()\n");

    fmi2_status_t status = fmi2_status_ok;
    std::vector<fmi2_real_t> x;
    x.reserve(r.nx()*sizeof(fmi2_real_t));
    int i;
    for(i=0; i<r.nx();i++)
      x[i] = r.x(i);

    if (!m_sendDummyResponses) {
      status = fmi2_import_set_continuous_states(m_fmi2Instance, x.data(), r.nx());
    }

    // Create response
    SERVER_NORMAL_RESPONSE(set_continuous_states);

  break; } case fmitcp_proto::type_fmi2_import_get_event_indicators_req: {
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_get_event_indicators_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_event_indicators_req(nz=%d)\n", r.nz());

    fmi2_status_t status = fmi2_status_ok;
    std::vector<fmi2_real_t> z;
    z.reserve(r.nz()*sizeof(fmi2_real_t));

    if (!m_sendDummyResponses) {
      status = fmi2_import_get_event_indicators(m_fmi2Instance, z.data(), r.nz());
    }

    //Create response
    fmitcp_proto::fmi2_import_get_event_indicators_res response;
    ret.first = fmitcp_proto::type_fmi2_import_get_event_indicators_res;
    response.set_status(fmi2StatusToProtofmi2Status(status));
    for(int i = 0; i< r.nz();i++)
      response.add_z(z[i]);

    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_event_indicators_res()\n");
  break; } case fmitcp_proto::type_fmi2_import_get_continuous_states_req: {
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_get_continuous_states_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_continuous_states_req(nx=%d)\n", r.nx());

    fmi2_status_t status = fmi2_status_ok;
    std::vector<fmi2_real_t> x(r.nx());

    if (!m_sendDummyResponses) {
      status = fmi2_import_get_continuous_states(m_fmi2Instance, x.data(), r.nx());
    }

    //Create response
    fmitcp_proto::fmi2_import_get_continuous_states_res response;
    ret.first = fmitcp_proto::type_fmi2_import_get_continuous_states_res;
    response.set_status(fmi2StatusToProtofmi2Status(status));
    for(int i = 0; i< r.nx();i++)
      response.add_x(x[i]);

    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_continuous_states_res()\n");
  break; } case fmitcp_proto::type_fmi2_import_get_derivatives_req: {
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_get_derivatives_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_derivatives_req(nderivatives=%d)\n", r.nderivatives());

    fmi2_status_t status = fmi2_status_ok;
    std::vector<fmi2_real_t> derivatives(r.nderivatives());

    if (!m_sendDummyResponses) {
      status = fmi2_import_get_derivatives(m_fmi2Instance, derivatives.data(), r.nderivatives());
    }

    //Create response
    fmitcp_proto::fmi2_import_get_derivatives_res response;
    ret.first = fmitcp_proto::type_fmi2_import_get_derivatives_res;
    response.set_status(fmi2StatusToProtofmi2Status(status));
    for(int i = 0; i< r.nderivatives();i++)
      response.add_derivatives(derivatives[i]);

    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_derivatives_res()\n");
  break; } case fmitcp_proto::type_fmi2_import_get_nominal_continuous_states_req: {
    // TODO
    // Unpack message
    fmitcp_proto::fmi2_import_get_nominal_continuous_states_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_nominal_continuous_states_req(nx=%d)\n", r.nx());

    fmi2_status_t status = fmi2_status_ok;
    std::vector<fmi2_real_t> nominal;
    nominal.reserve(r.nx()*sizeof(fmi2_real_t));

    if (!m_sendDummyResponses) {
      status = fmi2_import_get_nominals_of_continuous_states(m_fmi2Instance, nominal.data(), r.nx());
    }

    //Create response
    fmitcp_proto::fmi2_import_get_nominal_continuous_states_res response;
    ret.first = fmitcp_proto::type_fmi2_import_get_nominal_continuous_states_res;
    response.set_status(fmi2StatusToProtofmi2Status(status));
    for(int i = 0; i< r.nx();i++)
      response.add_nominal(nominal[i]);

    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_nominal_continuous_states_res()\n");
  break; } case fmitcp_proto::type_fmi2_import_set_real_input_derivatives_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_set_real_input_derivatives_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> vr(r.valuereferences_size());
    vector<fmi2_integer_t> order(r.orders_size());
    vector<fmi2_real_t> value(r.values_size());

    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      vr[i] = r.valuereferences(i);
      order[i] = r.orders(i);
      value[i] = r.values(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_set_real_input_derivatives_req(vrs=%s,orders=%s,values=%s)\n",
        arrayToString(vr, r.valuereferences_size()).c_str(), arrayToString(order, r.orders_size()).c_str(), arrayToString(value, r.values_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_set_real_input_derivatives(m_fmi2Instance, vr.data(), r.valuereferences_size(), order.data(), value.data());
    }

    SERVER_NORMAL_RESPONSE(set_real_input_derivatives);

  break; } case fmitcp_proto::type_fmi2_import_get_real_output_derivatives_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_real_output_derivatives_req r; r.ParseFromArray(data, size);
    vector<fmi2_value_reference_t> vr(r.valuereferences_size());
    vector<fmi2_integer_t> order(r.orders_size());
    vector<fmi2_real_t> value(r.valuereferences_size());

    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      vr[i] = r.valuereferences(i);
      order[i] = r.orders(i);
    }
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_output_derivatives_req(vrs=%s,orders=%s)\n",
        arrayToString(vr, r.valuereferences_size()).c_str(), arrayToString(order, r.orders_size()).c_str());

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // interact with FMU
      status = fmi2_import_get_real_output_derivatives(m_fmi2Instance, vr.data(), r.valuereferences_size(), order.data(), value.data());
    }

    // Create response
    fmitcp_proto::fmi2_import_get_real_output_derivatives_res response;
    ret.first = fmitcp_proto::type_fmi2_import_get_real_output_derivatives_res;
    response.set_status(fmi2StatusToProtofmi2Status(status));
    for (int i = 0 ; i < r.valuereferences_size() ; i++) {
      response.add_values(value[i]);
    }
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_real_output_derivatives_res(status=%d,values=%s)\n",response.status(),arrayToString(value, r.valuereferences_size()).c_str());

  break; } case fmitcp_proto::type_fmi2_import_do_step_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_do_step_req r; r.ParseFromArray(data, size);
    //remember values
    currentCommunicationPoint = r.currentcommunicationpoint();
    communicationStepSize = r.communicationstepsize();
    bool newStep = r.newstep();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_do_step_req(commPoint=%g,stepSize=%g,newStep=%d)\n",currentCommunicationPoint,communicationStepSize,newStep?1:0);

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

    SERVER_NORMAL_RESPONSE(do_step);

  break; } case fmitcp_proto::type_fmi2_import_cancel_step_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_cancel_step_req r; r.ParseFromArray(data, size);

    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_cancel_step_req()\n");

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // Interact with FMU
      status = fmi2_import_cancel_step(m_fmi2Instance);
    }

    SERVER_NORMAL_RESPONSE(cancel_step);

  break; } case fmitcp_proto::type_fmi2_import_get_status_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_status_req r; r.ParseFromArray(data, size);
    fmitcp_proto::fmi2_status_kind_t statusKind = r.status();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_status_req(status=%d)\n", statusKind);

    fmi2_status_t status = fmi2_status_ok;
    if (!m_sendDummyResponses) {
      // get the FMU status
      fmi2_import_get_status(m_fmi2Instance, protoStatusKindToFmiStatusKind(statusKind), &status);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_status_res response;
    ret.first = fmitcp_proto::type_fmi2_import_get_status_res;
    response.set_value(fmi2StatusToProtofmi2Status(status));
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_status_res(value=%d)\n",response.value());

  break; } case fmitcp_proto::type_fmi2_import_get_real_status_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_real_status_req r; r.ParseFromArray(data, size);
    fmitcp_proto::fmi2_status_kind_t statusKind = r.kind();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_real_status_req(status=%d)\n", statusKind);

    fmi2_real_t value = 0.0;
    if (!m_sendDummyResponses) {
      // get the FMU real status
      fmi2_import_get_real_status(m_fmi2Instance, protoStatusKindToFmiStatusKind(statusKind), &value);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_real_status_res response;
    ret.first = fmitcp_proto::type_fmi2_import_get_real_status_res;
    response.set_value(value);
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_real_status_res(value=%g)\n",response.value());

  break; } case fmitcp_proto::type_fmi2_import_get_integer_status_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_integer_status_req r; r.ParseFromArray(data, size);
    fmitcp_proto::fmi2_status_kind_t statusKind = r.kind();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_integer_status_req(status=%d)\n", statusKind);

    fmi2_integer_t value = 0;
    if (!m_sendDummyResponses) {
      // get the FMU integer status
      fmi2_import_get_integer_status(m_fmi2Instance, protoStatusKindToFmiStatusKind(statusKind), &value);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_integer_status_res response;
    ret.first = fmitcp_proto::type_fmi2_import_get_integer_status_res;
    response.set_value(value);
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_integer_status_res(value=%d)\n",response.value());

  break; } case fmitcp_proto::type_fmi2_import_get_boolean_status_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_boolean_status_req r; r.ParseFromArray(data, size);
    fmitcp_proto::fmi2_status_kind_t statusKind = r.kind();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_boolean_status_req(status=%d)\n", statusKind);

    fmi2_boolean_t value = 0;
    if (!m_sendDummyResponses) {
      // get the FMU boolean status
      fmi2_import_get_boolean_status(m_fmi2Instance, protoStatusKindToFmiStatusKind(statusKind), &value);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_boolean_status_res response;
    ret.first = fmitcp_proto::type_fmi2_import_get_boolean_status_res;
    response.set_value(value);
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_boolean_status_res(value=%d)\n",response.value());

  break; } case fmitcp_proto::type_fmi2_import_get_string_status_req: {

    // Unpack message
    fmitcp_proto::fmi2_import_get_string_status_req r; r.ParseFromArray(data, size);
    fmitcp_proto::fmi2_status_kind_t statusKind = r.kind();
    m_logger.log(Logger::LOG_NETWORK,"< fmi2_import_get_string_status_req(status=%d)\n", statusKind);

    fmi2_string_t value = "";
    if (!m_sendDummyResponses) {
      // TODO: Step the FMU
      fmi2_import_get_string_status(m_fmi2Instance, protoStatusKindToFmiStatusKind(statusKind), &value);
    }

    // Create response
    fmitcp_proto::fmi2_import_get_string_status_res response;
    ret.first = fmitcp_proto::type_fmi2_import_get_string_status_res;
    response.set_value(value);
    ret.second = response.SerializeAsString();
    m_logger.log(Logger::LOG_NETWORK,"> fmi2_import_get_string_status_res(value=%s)\n",response.value().c_str());

  break; } case fmitcp_proto::type_get_xml_req: {

    // Unpack message
    fmitcp_proto::get_xml_req r; r.ParseFromArray(data, size);
    m_logger.log(Logger::LOG_NETWORK,"< get_xml_req()\n");

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
    fmitcp_proto::get_xml_res response;
    ret.first = fmitcp_proto::type_get_xml_res;
    response.set_loglevel(fmiJMLogLevelToProtoJMLogLevel(m_logLevel));
    response.set_xml(xml);
    ret.second = response.SerializeAsString();
    // only printing the first 38 characters of xml.
    m_logger.log(Logger::LOG_NETWORK,"> get_xml_res(logLevel=%d,xml=%.*s)\n", response.loglevel(), 38, response.xml().c_str());

  break; } default: {
    // Something is wrong.
    sendResponse = false;
    m_logger.log(Logger::LOG_ERROR,"Message type not recognized: %d.\n",type);
    break; }
  }

  if (ret.second == "error") {
    fprintf(stderr, "error!: %i\n", ret.first);
    exit(1);
  }

  if (sendResponse) {
    uint16_t t = ret.first;
    uint8_t bytes[2] = {(uint8_t)t, (uint8_t)(t>>8)};
    return string(reinterpret_cast<char*>(bytes), 2) + ret.second;
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
