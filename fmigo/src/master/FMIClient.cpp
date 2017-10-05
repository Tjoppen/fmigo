#include <fstream>
#include <sstream>
#include <fmitcp/Client.h>
#include <fmitcp/serialize.h>

#include "master/BaseMaster.h"
#include "common/common.h"
#include "master/FMIClient.h"

#ifdef USE_GPL
#include "common/fmigo_storage.h"
#endif

using namespace fmitcp_master;
using namespace fmitcp::serialize;
using namespace common;

/*!
 * Callback function for FMILibrary. Logs the FMILibrary operations.
 */
void jmCallbacksLoggerClient(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message) {
    info("[module = %s][log level = %s] %s\n", module, jm_log_level_to_string(log_level), message);fflush(NULL);
}

#ifdef USE_MPI
FMIClient::FMIClient(int world_rank, int id) : fmitcp::Client(world_rank), sc::Slave() {
#else
FMIClient::FMIClient(zmq::context_t &context, int id, string uri) : fmitcp::Client(context, uri), sc::Slave() {
#endif
    m_id = id;
    m_fmi2Instance = NULL;
    m_context = NULL;
    m_fmi2Outputs = NULL;
    m_stateId = 0;
    m_fmuState = control_proto::fmu_state_State_instantiating;
    m_hasComputedStrongConnectorValueReferences = false;
};

void FMIClient::terminate() {
  m_fmuState = control_proto::fmu_state_State_terminating;

  //tell remove FMU to free itself
  sendMessageBlocking(fmi2_import_terminate());
  sendMessageBlocking(fmi2_import_free_instance());

  m_fmuState = control_proto::fmu_state_State_terminated;
}

FMIClient::~FMIClient() {
  // free the FMIL instances used for parsing the xml file.
  if(m_fmi2Instance!=NULL)  fmi2_import_free(m_fmi2Instance);
  if(m_context!=NULL)       fmi_import_free_context(m_context);
  fmi_import_rmdir(&m_jmCallbacks, m_workingDir.c_str());
  if(m_fmi2Outputs!=NULL)   fmi2_import_free_variable_list(m_fmi2Outputs);
};

int FMIClient::getId(){
    return m_id;
};

void FMIClient::on_get_xml_res(fmitcp_proto::jm_log_level_enu_t logLevel, string xml) {
  m_xml = xml;
  // parse the xml.
  // JM callbacks
  m_jmCallbacks.malloc = malloc;
  m_jmCallbacks.calloc = calloc;
  m_jmCallbacks.realloc = realloc;
  m_jmCallbacks.free = free;
  m_jmCallbacks.logger = jmCallbacksLoggerClient;
  m_jmCallbacks.log_level = fmigo_loglevel;
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
    setVariables();
  } else {
    error("Error parsing the modelDescription.xml file contained in %s\n", m_workingDir.c_str());
  }
};

std::string FMIClient::getModelName() const {
    return fmi2_import_get_model_name(m_fmi2Instance);
}

const variable_map& FMIClient::getVariables() const {
  return m_variables;
}

const variable_vr_map& FMIClient::getVRVariables() const {
  return m_vr_variables;
}

fmi2_fmu_kind_enu_t FMIClient::getFmuKind(){
  return fmi2_import_get_fmu_kind(m_fmi2Instance);
}

void FMIClient::setVariables() {
    if (!m_fmi2Instance) {
        fatal("!m_fmi2Instance in FMIClient::getVariables() - get_xml() failed?\n");
    }

    fmi2_import_variable_list_t *vl = fmi2_import_get_variable_list(m_fmi2Instance, 0);
    size_t sz = fmi2_import_get_variable_list_size(vl);
    for (size_t x = 0; x < sz; x++) {
        fmi2_import_variable_t *var = fmi2_import_get_variable(vl, x);
        string name = fmi2_import_get_variable_name(var);

        variable var2;
        var2.vr         = fmi2_import_get_variable_vr(var);
        var2.type       = fmi2_import_get_variable_base_type(var);
        var2.causality  = fmi2_import_get_causality(var);
        var2.initial    = fmi2_import_get_initial(var);

        //debug("VR %i, type %i, causality %i: %s \"%s\"\n", var2.vr, var2.type, var2.causality, name.c_str(), fmi2_import_get_variable_description(var));

        if (m_variables.find(name) != m_variables.end()) {
            warning("Two or variables named \"%s\"\n", name.c_str());
        }

        //make it possible to look up variables by name or by (vr,type)
        m_variables[name] = var2;
        m_vr_variables[make_pair(var2.vr, var2.type)] = var2;
    }
    fmi2_import_free_variable_list(vl);

    m_fmi2Outputs = fmi2_import_get_outputs_list(m_fmi2Instance);

    sz = fmi2_import_get_variable_list_size(m_fmi2Outputs);
    for (size_t x = 0; x < sz; x++) {
        fmi2_import_variable_t *var = fmi2_import_get_variable(m_fmi2Outputs, x);

        variable var2;
        var2.vr         = fmi2_import_get_variable_vr(var);
        var2.type       = fmi2_import_get_variable_base_type(var);
        var2.causality  = fmi2_import_get_causality(var);
        var2.initial    = fmi2_import_get_initial(var);

        m_outputs.push_back(var2);
    }
}

const vector<variable>& FMIClient::getOutputs() const {
    return m_outputs;
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

void FMIClient::on_fmi2_import_instantiate_res(fmitcp_proto::jm_status_enu_t status){
    m_fmuState = control_proto::fmu_state_State_instantiated;
    m_master->onSlaveInstantiated(this);
};

void FMIClient::on_fmi2_import_exit_initialization_mode_res(fmitcp_proto::fmi2_status_t status){
    m_fmuState = control_proto::fmu_state_State_initialized;
    m_master->onSlaveInitialized(this);
};

void FMIClient::on_fmi2_import_terminate_res(fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveTerminated(this);
};

void FMIClient::on_fmi2_import_free_instance_res(){
    m_master->onSlaveFreed(this);
};

void FMIClient::on_fmi2_import_do_step_res(fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveStepped(this);
};

void FMIClient::on_fmi2_import_get_version_res(string version){
    m_master->onSlaveGotVersion(this);
};


void FMIClient::on_fmi2_import_set_real_res(fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveSetReal(this);
};

template<typename T> void cache_values(const deque<T>& values, set<int>& outgoing, unordered_map<int,T>& dest) {
  size_t x = 0;
  for (int vr : outgoing) {
    dest.insert(make_pair(vr, values[x]));
    x++;
  }
  outgoing.clear();
}

void FMIClient::on_fmi2_import_get_real_res(const deque<double>& values, fmitcp_proto::fmi2_status_t status){
  cache_values(values, m_outgoing_reals, m_reals);
}

void FMIClient::on_fmi2_import_get_integer_res(const deque<int>& values, fmitcp_proto::fmi2_status_t status) {
  cache_values(values, m_outgoing_ints, m_ints);
}

void FMIClient::on_fmi2_import_get_boolean_res(const deque<bool>& values, fmitcp_proto::fmi2_status_t status) {
  cache_values(values, m_outgoing_bools, m_bools);
}

void FMIClient::on_fmi2_import_get_string_res(const deque<string>& values, fmitcp_proto::fmi2_status_t status) {
  cache_values(values, m_outgoing_strings, m_strings);
}

void FMIClient::on_fmi2_import_get_fmu_state_res(int stateId, fmitcp_proto::fmi2_status_t status){
    //remember stateId
    m_stateId = stateId;
    m_master->onSlaveGotState(this);
};

void FMIClient::on_fmi2_import_set_fmu_state_res(fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveSetState(this);
};

void FMIClient::on_fmi2_import_free_fmu_state_res(fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveFreedState(this);
};

void FMIClient::on_fmi2_import_get_directional_derivative_res(const vector<double>& dz, fmitcp_proto::fmi2_status_t status){
    m_master->onSlaveDirectionalDerivative(this);
}
void FMIClient::on_fmi2_import_new_discrete_states_res             (fmitcp_proto::fmi2_event_info_t event_info){
    m_event_info.newDiscreteStatesNeeded           = event_info.newdiscretestatesneeded();
    m_event_info.terminateSimulation               = event_info.terminatesimulation();
    m_event_info.nominalsOfContinuousStatesChanged = event_info.nominalsofcontinuousstateschanged();
    m_event_info.valuesOfContinuousStatesChanged   = event_info.valuesofcontinuousstateschanged();
    m_event_info.nextEventTimeDefined              = event_info.nexteventtimedefined();
    m_event_info.nextEventTime                     = event_info.nexteventtime();
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
#define debugprint(name)\
    fprintf(stderr, " FMIClient "#name"");\
    for(auto x : name)\
        fprintf(stderr, " %f ",x);\
    fprintf(stderr, " \n");

void FMIClient::on_fmi2_import_get_derivatives_res                 (const vector<double>& derivatives, fmitcp_proto::fmi2_status_t status){
#ifdef USE_GPL
    m_master->get_storage().push_to(getId(),STORAGE::derivatives, derivatives);
#endif
}
void FMIClient::on_fmi2_import_get_event_indicators_res            (const vector<double>& eventIndicators, fmitcp_proto::fmi2_status_t status){
#ifdef USE_GPL
    m_master->get_storage().push_to(getId(),STORAGE::indicators,eventIndicators);
#endif
}
//void on_fmi2_import_eventUpdate_res                     (int mid, bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status);
//void on_fmi2_import_completed_event_iteration_res       (int mid, fmitcp_proto::fmi2_status_t status);
void FMIClient::on_fmi2_import_get_continuous_states_res           (const vector<double>& states, fmitcp_proto::fmi2_status_t status){
#ifdef USE_GPL
    m_master->get_storage().push_to(getId(),STORAGE::states,states);
#endif
}
void FMIClient::on_fmi2_import_get_nominal_continuous_states_res   (const vector<double>& nominals, fmitcp_proto::fmi2_status_t status){
#ifdef USE_GPL
    m_master->get_storage().push_to(getId(),STORAGE::nominals,nominals);
#endif
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

StrongConnector* FMIClient::getConnector(int i) const{
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

const std::vector<int>& FMIClient::getStrongConnectorValueReferences() const {
    if (m_hasComputedStrongConnectorValueReferences) {
        return m_strongConnectorValueReferences;
    }

    for(int i=0; i<numConnectors(); i++){
        StrongConnector* c = getConnector(i);

        // Do we need position?
        if(c->hasPosition()){
            std::vector<int> refs = c->getPositionValueRefs();
            m_strongConnectorValueReferences.insert(m_strongConnectorValueReferences.end(), refs.begin(), refs.end());
        }

        // Do we need quaternion?
        if(c->hasQuaternion()){
            std::vector<int> refs = c->getQuaternionValueRefs();
            m_strongConnectorValueReferences.insert(m_strongConnectorValueReferences.end(), refs.begin(), refs.end());
        }

        if (c->hasShaftAngle()) {
            std::vector<int> refs = c->getShaftAngleValueRefs();
            m_strongConnectorValueReferences.insert(m_strongConnectorValueReferences.end(), refs.begin(), refs.end());
        }

        // Do we need velocity?
        if(c->hasVelocity()){
            std::vector<int> refs = c->getVelocityValueRefs();
            m_strongConnectorValueReferences.insert(m_strongConnectorValueReferences.end(), refs.begin(), refs.end());
        }

        // Do we need angular velocity?
        if(c->hasAngularVelocity()){
            std::vector<int> refs = c->getAngularVelocityValueRefs();
            m_strongConnectorValueReferences.insert(m_strongConnectorValueReferences.end(), refs.begin(), refs.end());
        }
    }

    m_hasComputedStrongConnectorValueReferences = true;
    return m_strongConnectorValueReferences;
};

const std::vector<int>& FMIClient::getStrongSeedInputValueReferences() const {
    //this used to be just a copy-paste of getStrongConnectorValueReferences() - better to just call the function itself directly
    return getStrongConnectorValueReferences();
};

const std::vector<int>& FMIClient::getStrongSeedOutputValueReferences() const {
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

void FMIClient::sendSetX(const SendSetXType& typeRefsValues) {
    for (auto it = typeRefsValues.begin(); it != typeRefsValues.end(); it++) {
        if (it->second.first.size() != it->second.second.size()) {
            fatal("VR-values count mismatch - something is wrong\n");
        }

        if (it->second.first.size() > 0) {
            switch (it->first) {
            case fmi2_base_type_real:
                sendMessage(fmi2_import_set_real   (it->second.first, vectorToBaseType(it->second.second, &MultiValue::r)));
                break;
            case fmi2_base_type_int:
                sendMessage(fmi2_import_set_integer(it->second.first, vectorToBaseType(it->second.second, &MultiValue::i)));
                break;
            case fmi2_base_type_bool:
                sendMessage(fmi2_import_set_boolean(it->second.first, vectorToBaseType(it->second.second, &MultiValue::b)));
                break;
            case fmi2_base_type_str:
                sendMessage(fmi2_import_set_string (it->second.first, vectorToBaseType(it->second.second, &MultiValue::s)));
                break;
            case fmi2_base_type_enum:
                fatal("fmi2_base_type_enum snuck its way into FMIClient::sendSetX() somehow\n");
            }
        }
    }
}
//send(it->first, fmi2_import_set_real(0, 0, it->second.first, it->second.second));

void FMIClient::deleteCachedValues() {
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

void FMIClient::queueReals(const vector<int>& vrs) {
  queueFoo(vrs, m_reals, m_outgoing_reals);
}
void FMIClient::queueInts(const vector<int>& vrs) {
  queueFoo(vrs, m_ints, m_outgoing_ints);
}
void FMIClient::queueBools(const vector<int>& vrs) {
  queueFoo(vrs, m_bools, m_outgoing_bools);
}
void FMIClient::queueStrings(const vector<int>& vrs) {
  queueFoo(vrs, m_strings, m_outgoing_strings);
}
void FMIClient::queueX(const SendGetXType& typeRefs) {
  for (const auto& it : typeRefs) {
    switch (it.first) {
    case fmi2_base_type_real:
      queueReals(it.second);
      break;
    case fmi2_base_type_int:
      queueInts(it.second);
      break;
    case fmi2_base_type_bool:
      queueBools(it.second);
      break;
    case fmi2_base_type_str:
      queueStrings(it.second);
      break;
    case fmi2_base_type_enum:
      fatal("fmi2_base_type_enum snuck its way into FMIClient::queueX() somehow\n");
    }
  }
}

static vector<int> setToVector(const set<int> &s) {
  vector<int> ret;
  for (int v : s) {
    ret.push_back(v);
  }
  return ret;
}

void FMIClient::sendValueRequests() {
  if (m_outgoing_reals.size()) {
    sendMessage(fmi2_import_get_real(setToVector(m_outgoing_reals)));
  }
  if (m_outgoing_ints.size()) {
    sendMessage(fmi2_import_get_integer(setToVector(m_outgoing_ints)));
  }
  if (m_outgoing_bools.size()) {
    sendMessage(fmi2_import_get_boolean(setToVector(m_outgoing_bools)));
  }
  if (m_outgoing_strings.size()) {
    sendMessage(fmi2_import_get_string(setToVector(m_outgoing_strings)));
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

vector<double> FMIClient::getReals(const vector<int>& vrs) const {
  return getFoo(vrs, m_reals);
}
vector<int> FMIClient::getInts(const vector<int>& vrs) const {
  return getFoo(vrs, m_ints);
}
vector<bool> FMIClient::getBools(const vector<int>& vrs) const {
  return getFoo(vrs, m_bools);
}
vector<string> FMIClient::getStrings(const vector<int>& vrs) const {
  return getFoo(vrs, m_strings);
}

double FMIClient::getReal(int vr) const {
  return getReals(std::vector<int>(1, vr))[0];
}
int FMIClient::getInt(int vr) const {
  return getInts(std::vector<int>(1, vr))[0];
}
bool FMIClient::getBool(int vr) const {
  return getBools(std::vector<int>(1, vr))[0];
}
string FMIClient::getString(int vr) const {
  return getStrings(std::vector<int>(1, vr))[0];
}
