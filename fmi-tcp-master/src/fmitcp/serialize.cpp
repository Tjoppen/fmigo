#include "fmitcp.pb.h"
#include "serialize.h"

using namespace fmitcp_proto;
using namespace std;

std::string fmitcp::serialize::fmi2_import_instantiate(int message_id) {
    return fmi2_import_instantiate2(message_id, false);
}

std::string fmitcp::serialize::fmi2_import_instantiate2(int message_id, bool visible) {
  // Construct message
  fmitcp_message m;
  m.set_type(fmitcp_message_Type_type_fmi2_import_instantiate_req);

  fmi2_import_instantiate_req * req = m.mutable_fmi2_import_instantiate_req();
  req->set_message_id(message_id);
  req->set_visible(visible);

  return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_initialize_slave(int message_id, int fmuId, bool toleranceDefined, double tolerance, double startTime,
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

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_terminate_slave(int message_id, int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_terminate_slave_req);

    fmi2_import_terminate_slave_req * req = m.mutable_fmi2_import_terminate_slave_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_reset_slave(int message_id, int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_reset_slave_req);

    fmi2_import_reset_slave_req * req = m.mutable_fmi2_import_reset_slave_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);


    return m.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_free_slave_instance(int message_id,int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_free_slave_instance_req);

    fmi2_import_free_slave_instance_req * req = m.mutable_fmi2_import_free_slave_instance_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    return m.SerializeAsString();
}

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

std::string fmitcp::serialize::fmi2_import_cancel_step(int message_id, int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_cancel_step_req);

    fmi2_import_cancel_step_req * req = m.mutable_fmi2_import_cancel_step_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    return m.SerializeAsString();
}

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

std::string fmitcp::serialize::fmi2_import_get_version(int message_id, int fmuId){
    // Construct message
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_version_req);

    fmi2_import_get_version_req * req = m.mutable_fmi2_import_get_version_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    return m.SerializeAsString();
}

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

std::string fmitcp::serialize::fmi2_import_get_fmu_state(int message_id, int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_fmi2_import_get_fmu_state_req);

    fmi2_import_get_fmu_state_req * req = m.mutable_fmi2_import_get_fmu_state_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    return m.SerializeAsString();
}

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


std::string fmitcp::serialize::get_xml(int message_id, int fmuId){
    fmitcp_message m;
    m.set_type(fmitcp_message_Type_type_get_xml_req);

    get_xml_req * req = m.mutable_get_xml_req();
    req->set_message_id(message_id);
    req->set_fmuid(fmuId);

    return m.SerializeAsString();
}
