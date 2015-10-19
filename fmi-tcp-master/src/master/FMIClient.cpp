#include <fstream>
#include <fmitcp/Client.h>
#include <fmitcp/Logger.h>

#include "master/BaseMaster.h"
#include "common/common.h"
#include "master/FMIClient.h"

using namespace fmitcp_master;

/*!
 * Callback function for FMILibrary. Logs the FMILibrary operations.
 */
void jmCallbacksLoggerClient(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
  printf("[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}

FMIClient::FMIClient(fmitcp::EventPump* pump, int id, string host, long port) : fmitcp::Client(pump), sc::Slave() {
    m_id = id;
    m_host = host;
    m_port = port;
    m_master = NULL;
    m_initialized = false;
    m_numDirectionalDerivativesLeft = 0;
    m_fmi2Instance = NULL;
    m_context = NULL;
    m_fmi2Outputs = NULL;
};

FMIClient::~FMIClient() {
  // free the FMIL instances used for parsing the xml file.
  if(m_fmi2Instance!=NULL)  fmi2_import_free(m_fmi2Instance);
  if(m_context!=NULL)       fmi_import_free_context(m_context);
  fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
  if(m_fmi2Outputs!=NULL)   fmi2_import_free_variable_list(m_fmi2Outputs);
};

void FMIClient::connect(void) {
    Client::connect(m_host, m_port);
}

/// Accumulate a get_directional_derivative request
void FMIClient::pushDirectionalDerivativeRequest(int fmiId, std::vector<int> v_ref, std::vector<int> z_ref, std::vector<double> dv){
    m_dd_v_refs.push_back(v_ref);
    m_dd_z_refs.push_back(z_ref);
    m_dd_dvs.push_back(dv);
};

/// Execute the next directional derivative request in the queue
void FMIClient::shiftExecuteDirectionalDerivativeRequest(){
    std::vector<int> v_ref = m_dd_v_refs.back();
    std::vector<int> z_ref = m_dd_z_refs.back();
    std::vector<double> dv = m_dd_dvs.back();

    m_dd_v_refs.pop_back();
    m_dd_z_refs.pop_back();
    m_dd_dvs.pop_back();
    fmi2_import_get_directional_derivative(0, 0, v_ref, z_ref, dv);
};

/// Get the total number of directional derivative requests queued
int FMIClient::numDirectionalDerivativeRequests(){
    return m_dd_v_refs.size();
};

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
  m_jmCallbacks.log_level = protoJMLogLevelToFmiJMLogLevel(logLevel);
  m_jmCallbacks.context = 0;
  // working directory
  char* dir = fmi_import_mk_temp_dir(&m_jmCallbacks, NULL, "fmitcp_master_");
  m_workingDir = dir; // convert to std::string
  // save the xml as a file i.e modelDescription.xml
  ofstream xmlFile (m_workingDir.append("/modelDescription.xml").c_str());
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
  // Inform the master now.
  m_master->onSlaveGetXML(this);
};

void FMIClient::on_fmi2_import_instantiate_res(int mid, fmitcp_proto::jm_status_enu_t status){
    m_master->onSlaveInstantiated(this);
};

void FMIClient::on_fmi2_import_initialize_slave_res(int mid, fmitcp_proto::fmi2_status_t status){
    m_initialized = true; // Todo check the status
    m_master->onSlaveInitialized(this);
};

void FMIClient::on_fmi2_import_terminate_slave_res(int mid, fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveTerminated(this);
};

void FMIClient::on_fmi2_import_free_slave_instance_res(int mid){
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

void FMIClient::on_fmi2_import_get_real_res(int mid, const vector<double>& values, fmitcp_proto::fmi2_status_t status){
    // Store result
    m_getRealValues = values;

    // Notify master
    m_master->onSlaveGotReal(this);
};

void FMIClient::on_fmi2_import_get_fmu_state_res(int mid, int stateId, fmitcp_proto::fmi2_status_t status){
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

/*
//void on_fmi2_import_reset_slave_res                     (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_real_input_derivatives_res      (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_real_output_derivatives_res     (int mid, fmitcp_proto::fmi2_status_t status, const vector<double>& values);
//void on_fmi2_import_cancel_step_res                     (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_status_res                      (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_real_status_res                 (int mid, double value);
//void on_fmi2_import_get_integer_status_res              (int mid, int value);
//void on_fmi2_import_get_boolean_status_res              (int mid, bool value);
//void on_fmi2_import_get_string_status_res               (int mid, string value);
//void on_fmi2_import_instantiate_model_res               (int mid, fmitcp_proto::jm_status_enu_t status);
//void on_fmi2_import_free_model_instance_res             (int mid);
//void on_fmi2_import_set_time_res                        (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_continuous_states_res           (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_completed_integrator_step_res       (int mid, bool callEventUpdate, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_initialize_model_res                (int mid, bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_derivatives_res                 (int mid, const vector<double>& derivatives, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_event_indicators_res            (int mid, const vector<double>& eventIndicators, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_eventUpdate_res                     (int mid, bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_completed_event_iteration_res       (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_continuous_states_res           (int mid, const vector<double>& states, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_nominal_continuous_states_res   (int mid, const vector<double>& nominal, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_terminate_res                       (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_debug_logging_res               (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_integer_res                     (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_boolean_res                     (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_set_string_res                      (int mid, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_integer_res                     (int mid, const vector<int>& values, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_boolean_res                     (int mid, const vector<bool>& values, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_get_string_res                      (int mid, const vector<string>& values, fmitcp_proto::fmi2_status_t status);
*/

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
