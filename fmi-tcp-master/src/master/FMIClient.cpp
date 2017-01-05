#include <fstream>
#include <sstream>
#include <fmitcp/Client.h>
#include <fmitcp/Logger.h>
#include <fmitcp/serialize.h>

#include "master/BaseMaster.h"
#include "common/common.h"
#include "master/FMIClient.h"

using namespace fmitcp_master;
using namespace fmitcp::serialize;
using namespace common;

/*!
 * Callback function for FMILibrary. Logs the FMILibrary operations.
 */
void jmCallbacksLoggerClient(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
  fprintf(stderr, "[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}

#ifdef USE_MPI
FMIClient::FMIClient(int world_rank, int id) : fmitcp::Client(world_rank), sc::Slave() {
#else
FMIClient::FMIClient(zmq::context_t &context, int id, string host, long port) : fmitcp::Client(context), sc::Slave() {
    m_host = host;
    m_port = port;
#endif
    m_id = id;
    m_master = NULL;
    m_initialized = false;
    m_fmi2Instance = NULL;
    m_context = NULL;
    m_fmi2Outputs = NULL;
    m_stateId = 0;
    m_loglevel = jm_log_level_nothing;
};

FMIClient::~FMIClient() {
  //tell remove FMU to free itself
  sendMessageBlocking(fmi2_import_terminate(0,0));
  sendMessageBlocking(fmi2_import_free_instance(0,0));

  // free the FMIL instances used for parsing the xml file.
  if(m_fmi2Instance!=NULL)  fmi2_import_free(m_fmi2Instance);
  if(m_context!=NULL)       fmi_import_free_context(m_context);
  fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
  if(m_fmi2Outputs!=NULL)   fmi2_import_free_variable_list(m_fmi2Outputs);
};

void FMIClient::connect(void) {
#ifndef USE_MPI
    Client::connect(m_host, m_port);
#endif
    //request modelDescription XML, don't return until we have it
    sendMessageBlocking(get_xml(0,0));
}

void FMIClient::onConnect(){
    m_master->slaveConnected(this);
};

void FMIClient::onDisconnect(){
    m_logger.log(fmitcp::Logger::LOG_DEBUG,"onDisconnect\n");
    m_master->slaveDisconnected(this);
};

void FMIClient::onError(string err){
    m_logger.log(fmitcp::Logger::LOG_DEBUG,"onError\n");
    m_master->slaveError(this);
};

int FMIClient::getId(){
    return m_id;
};

bool FMIClient::isInitialized(){
    return m_initialized;
};

void FMIClient::on_get_xml_res(int mid, fmitcp_proto::jm_log_level_enu_t logLevel, string xml) {
  m_xml = xml;
  // parse the xml.
  // JM callbacks
  m_jmCallbacks.malloc = malloc;
  m_jmCallbacks.calloc = calloc;
  m_jmCallbacks.realloc = realloc;
  m_jmCallbacks.free = free;
  m_jmCallbacks.logger = jmCallbacksLoggerClient;
  m_jmCallbacks.log_level = m_loglevel;
  m_jmCallbacks.context = 0;
  // working directory
  char* dir = fmi_import_mk_temp_dir(&m_jmCallbacks, NULL, "fmitcp_master_");
  m_workingDir = dir; // convert to std::string
  // save the xml as a file i.e modelDescription.xml
  string xmlPath = m_workingDir + "/modelDescription.xml";
  ofstream xmlFile (xmlPath.c_str());
  xmlFile << m_xml;
  xmlFile.close();
  // import allocate context
  m_context = fmi_import_allocate_context(&m_jmCallbacks);
  // parse the xml file
  m_fmi2Instance = fmi2_import_parse_xml(m_context, dir, 0);
  free(dir);
  if (m_fmi2Instance) {
    m_fmi2Outputs = fmi2_import_get_outputs_list(m_fmi2Instance);
  } else {
    m_logger.log(fmitcp::Logger::LOG_ERROR, "Error parsing the modelDescription.xml file contained in %s\n", m_workingDir.c_str());
  }
};

std::string FMIClient::getModelName() const {
    return fmi2_import_get_model_name(m_fmi2Instance);
}

variable_map FMIClient::getVariables() const {
    variable_map ret;

    if (!m_fmi2Instance) {
        fprintf(stderr, "!m_fmi2Instance in FMIClient::getVariables() - get_xml() failed?\n");
        exit(1);
    }

    fmi2_import_variable_list_t *vl = fmi2_import_get_variable_list(m_fmi2Instance, 0);
    size_t sz = fmi2_import_get_variable_list_size(vl);
    for (size_t x = 0; x < sz; x++) {
        fmi2_import_variable_t *var = fmi2_import_get_variable(vl, x);
        string name = fmi2_import_get_variable_name(var);

        variable var2;
        var2.vr = fmi2_import_get_variable_vr(var);
        var2.type = fmi2_import_get_variable_base_type(var);
        var2.causality = fmi2_import_get_causality(var);

        //fprintf(stderr, "VR %i, type %i, causality %i: %s \"%s\"\n", var2.vr, var2.type, var2.causality, name.c_str(), fmi2_import_get_variable_description(var));

        if (ret.find(name) != ret.end()) {
            fprintf(stderr, "WARNING: Two or variables named \"%s\"\n", name.c_str());
        }
        ret[name] = var2;
    }
    fmi2_import_free_variable_list(vl);

    return ret;
}

vector<variable> FMIClient::getOutputs() const {
    vector<variable> ret;

    size_t sz = fmi2_import_get_variable_list_size(m_fmi2Outputs);
    for (size_t x = 0; x < sz; x++) {
        fmi2_import_variable_t *var = fmi2_import_get_variable(m_fmi2Outputs, x);
        string name = fmi2_import_get_variable_name(var);

        variable var2;
        var2.vr = fmi2_import_get_variable_vr(var);
        var2.type = fmi2_import_get_variable_base_type(var);
        var2.causality = fmi2_import_get_causality(var);

        ret.push_back(var2);
    }

    return ret;
}

bool FMIClient::hasCapability(fmi2_capabilities_enu_t cap) const {
    return fmi2_import_get_capability(m_fmi2Instance, cap) != 0;
}

size_t FMIClient::getNumEventIndicators(void){
    return fmi2_import_get_number_of_event_indicators(m_fmi2Instance);
}
 
size_t FMIClient::getNumContinuousStates(void){
    return fmi2_import_get_number_of_continuous_states(m_fmi2Instance);
}

void FMIClient::on_fmi2_import_instantiate_res(int mid, fmitcp_proto::jm_status_enu_t status){
    m_master->onSlaveInstantiated(this);
};

void FMIClient::on_fmi2_import_exit_initialization_mode_res(int mid, fmitcp_proto::fmi2_status_t status){
    m_initialized = true; // Todo check the status
    m_master->onSlaveInitialized(this);
};

void FMIClient::on_fmi2_import_terminate_res(int mid, fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveTerminated(this);
};

void FMIClient::on_fmi2_import_free_instance_res(int mid){
    m_master->onSlaveFreed(this);
};

void FMIClient::on_fmi2_import_do_step_res(int mid, fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveStepped(this);
};

void FMIClient::on_fmi2_import_get_version_res(int mid, string version){
    m_master->onSlaveGotVersion(this);
};


void FMIClient::on_fmi2_import_set_real_res(int mid, fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveSetReal(this);
};

void FMIClient::on_fmi2_import_get_real_res(int mid, const deque<double>& values, fmitcp_proto::fmi2_status_t status){
    // Store result
    m_getRealValues = values;
};

void FMIClient::on_fmi2_import_get_integer_res(int mid, const deque<int>& values, fmitcp_proto::fmi2_status_t status) {
    m_getIntegerValues = values;
}

void FMIClient::on_fmi2_import_get_boolean_res(int mid, const deque<bool>& values, fmitcp_proto::fmi2_status_t status) {
    m_getBooleanValues = values;
}

void FMIClient::on_fmi2_import_get_string_res(int mid, const deque<string>& values, fmitcp_proto::fmi2_status_t status) {
    m_getStringValues = values;
}

void FMIClient::on_fmi2_import_get_fmu_state_res(int mid, int stateId, fmitcp_proto::fmi2_status_t status){
    //remember stateId
    m_stateId = stateId;
    m_master->onSlaveGotState(this);
};

void FMIClient::on_fmi2_import_set_fmu_state_res(int mid, fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveSetState(this);
};

void FMIClient::on_fmi2_import_free_fmu_state_res(int mid, fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveFreedState(this);
};

void FMIClient::on_fmi2_import_get_directional_derivative_res(int mid, const vector<double>& dz, fmitcp_proto::fmi2_status_t status){
    /*for (size_t x = 0; x < dz.size(); x++) {
        fprintf(stderr, "%f ", dz[x]);
    }
    fprintf(stderr, "\n");*/

    m_getDirectionalDerivativeValues.push_back(dz);
    m_master->onSlaveDirectionalDerivative(this);
}

// TODO:

//void on_fmi2_import_reset_slave_res                     (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_real_input_derivatives_res      (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_real_output_derivatives_res     (int mid, fmitcp_proto::fmi2_status_t status, const vector<double>& values);
//void on_fmi2_import_cancel_step_res                     (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_status_res                      (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_real_status_res                 (int mid, double value);
//void on_fmi2_import_get_integer_status_res              (int mid, int value);
//void on_fmi2_import_get_boolean_status_res              (int mid, bool value);
//void on_fmi2_import_get_string_status_res               (int mid, string value);
//void on_fmi2_import_set_time_res                        (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_continuous_states_res           (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_completed_integrator_step_res       (int mid, bool callEventUpdate, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_initialize_model_res                (int mid, bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status);
void FMIClient::on_fmi2_import_get_derivatives_res                 (int mid, const vector<double>& derivatives, fmitcp_proto::fmi2_status_t status){
    m_getDerivatives.push_back(derivatives);
}
void FMIClient::on_fmi2_import_get_event_indicators_res            (int mid, const vector<double>& eventIndicators, fmitcp_proto::fmi2_status_t status){
    m_getEventIndicators.push_back(eventIndicators);
}
//void on_fmi2_import_eventUpdate_res                     (int mid, bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_completed_event_iteration_res       (int mid, fmitcp_proto::fmi2_status_t status);
void FMIClient::on_fmi2_import_get_continuous_states_res           (int mid, const vector<double>& states, fmitcp_proto::fmi2_status_t status){
    m_getContinuousStates.push_back(states);
}
void FMIClient::on_fmi2_import_get_nominal_continuous_states_res   (int mid, const vector<double>& nominal, fmitcp_proto::fmi2_status_t status){
    m_getNominalContinuousStates.push_back(nominal);
}
//void on_fmi2_import_terminate_res                       (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_debug_logging_res               (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_integer_res                     (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_boolean_res                     (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_string_res                      (int mid, fmitcp_proto::fmi2_status_t status);


StrongConnector * FMIClient::createConnector(){
    StrongConnector * conn = new StrongConnector(this);
    addConnector(conn);
    return conn;
};

StrongConnector* FMIClient::getConnector(int i){
    return (StrongConnector*) Slave::getConnector(i);
};

void FMIClient::setConnectorValues(std::vector<int> valueRefs, std::vector<double> values){
    for(int i=0; i<numConnectors(); i++)
        getConnector(i)->setValues(valueRefs,values);
};

void FMIClient::setConnectorFutureVelocities(std::vector<int> valueRefs, std::vector<double> values){
    for(int i=0; i<numConnectors(); i++)
        getConnector(i)->setFutureValues(valueRefs,values);
};

std::vector<int> FMIClient::getStrongConnectorValueReferences(){
    std::vector<int> valueRefs;

    for(int i=0; i<numConnectors(); i++){
        StrongConnector* c = getConnector(i);

        // Do we need position?
        if(c->hasPosition()){
            std::vector<int> refs = c->getPositionValueRefs();
            valueRefs.insert(valueRefs.end(), refs.begin(), refs.end());
        }

        // Do we need quaternion?
        if(c->hasQuaternion()){
            std::vector<int> refs = c->getQuaternionValueRefs();
            valueRefs.insert(valueRefs.end(), refs.begin(), refs.end());
        }

        if (c->hasShaftAngle()) {
            std::vector<int> refs = c->getShaftAngleValueRefs();
            valueRefs.insert(valueRefs.end(), refs.begin(), refs.end());
        }

        // Do we need velocity?
        if(c->hasVelocity()){
            std::vector<int> refs = c->getVelocityValueRefs();
            valueRefs.insert(valueRefs.end(), refs.begin(), refs.end());
        }

        // Do we need angular velocity?
        if(c->hasAngularVelocity()){
            std::vector<int> refs = c->getAngularVelocityValueRefs();
            valueRefs.insert(valueRefs.end(), refs.begin(), refs.end());
        }
    }

    return valueRefs;
};

std::vector<int> FMIClient::getStrongSeedInputValueReferences(){
    //this used to be just a copy-paste of getStrongConnectorValueReferences() - better to just call the function itself directly
    return getStrongConnectorValueReferences();
};

std::vector<int> FMIClient::getStrongSeedOutputValueReferences(){
    //same here - just a copy-paste job
    return getStrongConnectorValueReferences();
};

std::vector<int> FMIClient::getRealOutputValueReferences() {
    const fmi2_value_reference_t *vrs = fmi2_import_get_value_referece_list(m_fmi2Outputs);
    size_t n = fmi2_import_get_variable_list_size(m_fmi2Outputs);

    return std::vector<int>(vrs, vrs + n);
}

string FMIClient::getSpaceSeparatedFieldNames(string prefix) const {
    ostringstream oss;
    for (size_t x = 0; x < fmi2_import_get_variable_list_size(m_fmi2Outputs); x++) {
        fmi2_import_variable_t *var = fmi2_import_get_variable(m_fmi2Outputs, x);
        oss << prefix << fmi2_import_get_variable_name(var);
    }
    return oss.str();
}

void FMIClient::sendGetX(const SendGetXType& typeRefs) {
    clearGetValues();

    for (auto it = typeRefs.begin(); it != typeRefs.end(); it++) {
        if (it->second.size() > 0) {
            switch (it->first) {
            case fmi2_base_type_real:
                sendMessage(fmi2_import_get_real(0, 0, it->second));
                break;
            case fmi2_base_type_int:
                sendMessage(fmi2_import_get_integer(0, 0, it->second));
                break;
            case fmi2_base_type_bool:
                sendMessage(fmi2_import_get_boolean(0, 0, it->second));
                break;
            case fmi2_base_type_str:
                sendMessage(fmi2_import_get_string(0, 0, it->second));
                break;
            }
        }
    }
}

//converts a vector<MultiValue> to vector<T>, with the help of a member pointer of type T
template<typename T> vector<T> vectorToBaseType(const vector<MultiValue>& in, T MultiValue::*member) {
    vector<T> ret;
    for (auto it = in.begin(); it != in.end(); it++) {
        ret.push_back((*it).*member);
    }
    return ret;
}

void FMIClient::sendSetX(const SendSetXType& typeRefsValues) {
    for (auto it = typeRefsValues.begin(); it != typeRefsValues.end(); it++) {
        if (it->second.first.size() != it->second.second.size()) {
            fprintf(stderr, "VR-values count mismatch - something is wrong\n");
            exit(1);
        }

        if (it->second.first.size() > 0) {
            switch (it->first) {
            case fmi2_base_type_real:
                sendMessage(fmi2_import_set_real   (0, 0, it->second.first, vectorToBaseType(it->second.second, &MultiValue::r)));
                break;
            case fmi2_base_type_int:
                sendMessage(fmi2_import_set_integer(0, 0, it->second.first, vectorToBaseType(it->second.second, &MultiValue::i)));
                break;
            case fmi2_base_type_bool:
                sendMessage(fmi2_import_set_boolean(0, 0, it->second.first, vectorToBaseType(it->second.second, &MultiValue::b)));
                break;
            case fmi2_base_type_str:
                sendMessage(fmi2_import_set_string (0, 0, it->second.first, vectorToBaseType(it->second.second, &MultiValue::s)));
                break;
            }
        }
    }
}
//send(it->first, fmi2_import_set_real(0, 0, it->second.first, it->second.second));

void FMIClient::clearGetValues() {
    m_getRealValues.clear();
    m_getIntegerValues.clear();
    m_getBooleanValues.clear();
    m_getStringValues.clear();
    m_getDerivatives.clear();
    m_getContinuousStates.clear();
    m_getNominalContinuousStates.clear();
    m_getEventIndicators.clear();
    m_getDirectionalDerivativeValues.clear();
}
