#include "fmitcp.pb.h"
#include "serialize.h"

using namespace fmitcp_proto;
using namespace std;

std::string fmitcp::serialize::fmi2_import_instantiate(int message_id) {
    return fmi2_import_instantiate2(message_id, false);
}
#define SERIALIZE_NORMAL_MESSAGE(type)                                  \
    /* Contruct message */                                              \
    fmitcp_message m;                                                   \
    m.set_type(fmitcp_message_Type_type_##type##_req);                  \
    type##_req * req = m.mutable_##type##_req();                        \
    req->set_message_id(message_id);                                    \
    req->set_fmuid(fmuId);                                              \
    return m.SerializeAsString();

std::string fmitcp::serialize::fmi2_import_instantiate2(int message_id, bool visible) {
  // Construct message
  fmitcp_message m;
  m.set_type(fmitcp_message_Type_type_fmi2_import_instantiate_req);

  fmi2_import_instantiate_req * req = m.mutable_fmi2_import_instantiate_req();
  req->set_message_id(message_id);
  req->set_visible(visible);

  return m.SerializeAsString();
}

//implementation for all messages to FMI function that take no arguments (void)
#define FMU_VOID_REQ_IMPL(name)\
std::string fmitcp::serialize::name(int message_id, int fmuId){\
    fmitcp_message m;\
    m.set_type(fmitcp_message_Type_type_##name## _req);\
\
    name##_req * req = m.mutable_##name##_req();\
    req->set_message_id(message_id);\
    req->set_fmuid(fmuId);\
\
    return m.SerializeAsString();\
}

FMU_VOID_REQ_IMPL(fmi2_import_free_instance)

std::string fmitcp::serialize::fmi2_import_setup_experiment(int message_id, int fmuId, bool toleranceDefined, double tolerance, double startTime,
    bool stopTimeDefined, double stopTime) {
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_setup_experiment_req);

    fmi2_import_setup_experiment_req * req = m.mutable_fmi2_import_setup_experiment_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_tolerancedefined(toleranceDefined);
    req->set_tolerance(tolerance);
    req->set_starttime(startTime);
    req->set_stoptimedefined(stopTimeDefined);
    req->set_stoptime(stopTime);

    return m.SerializeAsString();
}

FMU_VOID_REQ_IMPL(fmi2_import_enter_initialization_mode)
FMU_VOID_REQ_IMPL(fmi2_import_exit_initialization_mode)
FMU_VOID_REQ_IMPL(fmi2_import_terminate)
FMU_VOID_REQ_IMPL(fmi2_import_reset)

std::string fmitcp::serialize::fmi2_import_set_real_input_derivatives(int message_id, int fmuId,
                                                    std::vector<int> valueRefs,
                                                    std::vector<int> orders,
                                                    std::vector<double> values){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_real_input_derivatives_req);

    fmi2_import_set_real_input_derivatives_req * req = m.mutable_fmi2_import_set_real_input_derivatives_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    // TODO: SET the derivatives in message

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_get_real_output_derivatives(int message_id, int fmuId, std::vector<int> valueRefs, std::vector<int> orders){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_real_output_derivatives_req);

    fmi2_import_get_real_output_derivatives_req * req = m.mutable_fmi2_import_get_real_output_derivatives_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    return m.SerializeAsString();
}

FMU_VOID_REQ_IMPL(fmi2_import_cancel_step)

std::string fmitcp::serialize::fmi2_import_do_step(int message_id,
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

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_get_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_status_req);

    fmi2_import_get_status_req * req = m.mutable_fmi2_import_get_status_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_status(s);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_get_real_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_real_status_req);

    fmi2_import_get_real_status_req * req = m.mutable_fmi2_import_get_real_status_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_kind(s);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_get_integer_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_integer_status_req);

    fmi2_import_get_integer_status_req * req = m.mutable_fmi2_import_get_integer_status_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_kind(s);

    return m.SerializeAsString();
}


std::string fmitcp::serialize::fmi2_import_get_boolean_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_boolean_status_req);

    fmi2_import_get_boolean_status_req * req = m.mutable_fmi2_import_get_boolean_status_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_kind(s);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_get_string_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_string_status_req);

    fmi2_import_get_string_status_req * req = m.mutable_fmi2_import_get_string_status_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_kind(s);

    return m.SerializeAsString();
}

        // =========== FMI 2.0 (ME) Model Exchange functions ===========

std::string fmitcp::serialize::fmi2_import_enter_event_mode(int message_id, int fmuId){
    SERIALIZE_NORMAL_MESSAGE(fmi2_import_enter_event_mode);
}

std::string fmitcp::serialize::fmi2_import_new_discrete_states(int message_id, int fmuId){
    SERIALIZE_NORMAL_MESSAGE(fmi2_import_new_discrete_states); 
}

std::string fmitcp::serialize::fmi2_import_enter_continuous_time_mode(int message_id, int fmuId){
    SERIALIZE_NORMAL_MESSAGE(fmi2_import_enter_continuous_time_mode);
}

std::string fmitcp::serialize::fmi2_import_completed_integrator_step(int message_id, int fmuId){
    SERIALIZE_NORMAL_MESSAGE(fmi2_import_completed_integrator_step); 
}

std::string fmitcp::serialize::fmi2_import_set_time(int message_id, int fmuId, double time){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_time_req);
    
    fmi2_import_set_time_req * req = m.mutable_fmi2_import_set_time_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_time(time);

    return m.SerializeAsString();
}   

std::string fmi2_import_set_continuous_states(int message_id, int fmuId, double* x, int nx){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_continuous_states_req);

    fmi2_import_set_continuous_states_req * req = m.mutable_fmi2_import_set_continuous_states_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i = 0; i < nx; i++)
      req->set_x(i, x[i]);

    return m.SerializeAsString();
}

FMU_VOID_REQ_IMPL(fmi2_import_get_version)
  

std::string fmitcp::serialize::fmi2_import_set_debug_logging(int message_id, int fmuId, bool loggingOn, const std::vector<string> categories){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_debug_logging_req);

    fmi2_import_set_debug_logging_req * req = m.mutable_fmi2_import_set_debug_logging_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    req->set_loggingon(loggingOn);
    for(int i=0; i<categories.size(); i++)
        req->add_categories(categories[i]);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_set_real(int message_id, int fmuId, const vector<int>& valueRefs, const vector<double>& values){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_real_req);

    fmi2_import_set_real_req * req = m.mutable_fmi2_import_set_real_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req->add_values(values[i]);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_set_integer(int message_id, int fmuId, const vector<int>& valueRefs, const vector<int>& values){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_integer_req);

    fmi2_import_set_integer_req * req = m.mutable_fmi2_import_set_integer_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req->add_values(values[i]);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_set_boolean(int message_id, int fmuId, const vector<int>& valueRefs, const vector<bool>& values){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_boolean_req);

    fmi2_import_set_boolean_req * req = m.mutable_fmi2_import_set_boolean_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req->add_values(values[i]);

    return m.SerializeAsString();
}


std::string fmitcp::serialize::fmi2_import_set_string(int message_id, int fmuId, const vector<int>& valueRefs, const vector<string>& values){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_string_req);

    fmi2_import_set_string_req * req = m.mutable_fmi2_import_set_string_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req->add_values(values[i]);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_get_real(int message_id, int fmuId, const vector<int>& valueRefs){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_real_req);

    fmi2_import_get_real_req * req = m.mutable_fmi2_import_get_real_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_get_integer(int message_id, int fmuId, const vector<int>& valueRefs){

    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_integer_req);

    fmi2_import_get_integer_req * req = m.mutable_fmi2_import_get_integer_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_get_boolean(int message_id, int fmuId, const vector<int>& valueRefs){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_boolean_req);

    fmi2_import_get_boolean_req * req = m.mutable_fmi2_import_get_boolean_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_get_string (int message_id, int fmuId, const vector<int>& valueRefs){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_string_req);

    fmi2_import_get_string_req * req = m.mutable_fmi2_import_get_string_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req->add_valuereferences(valueRefs[i]);

    return m.SerializeAsString();
}

FMU_VOID_REQ_IMPL(fmi2_import_get_fmu_state)

std::string fmitcp::serialize::fmi2_import_set_fmu_state(int message_id, int fmuId, int stateId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_set_fmu_state_req);

    fmi2_import_set_fmu_state_req * req = m.mutable_fmi2_import_set_fmu_state_req();
    req->set_message_id(message_id);
    req->set_stateid(stateId);
    req->set_fmuid(fmuId);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_free_fmu_state(int message_id, int fmuId, int stateId) {
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_free_fmu_state_req);

    fmi2_import_free_fmu_state_req * req = m.mutable_fmi2_import_free_fmu_state_req();
    req->set_message_id(message_id);
    req->set_stateid(stateId);
    req->set_fmuid(fmuId);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_get_directional_derivative(int message_id, int fmuId,
                                                    const vector<int>& z_ref,
                                                    const vector<int>& v_ref,
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

    return m.SerializeAsString();
}

FMU_VOID_REQ_IMPL(get_xml)
