#include "fmitcp.pb.h"
#include "serialize.h"
#include <stdint.h>

using namespace fmitcp_proto;
using namespace std;

template<typename T> std::string pack(fmitcp_message_Type type, T &req) {
  uint16_t t = type;
  uint8_t bytes[2] = {(uint8_t)t, (uint8_t)(t>>8)};
  return string(reinterpret_cast<char*>(bytes), 2) + req.SerializeAsString();
}

std::string fmitcp::serialize::fmi2_import_instantiate(int message_id) {
    return fmi2_import_instantiate2(message_id, false, 0);
}
#define SERIALIZE_NORMAL_MESSAGE_(type, extra)                          \
    /* Contruct message */                                              \
    type##_req req;                                                     \
    req.set_message_id(message_id);                                     \
    req.set_fmuid(fmuId);                                               \
    req.set_##extra(extra);                                             \
    return pack(type_##type##_req, req);

#define SERIALIZE_NORMAL_MESSAGE(type)                                  \
    /* Contruct message */                                              \
    type##_req req;                                                     \
    req.set_message_id(message_id);                                     \
    req.set_fmuid(fmuId);                                               \
    return pack(type_##type##_req, req);

std::string fmitcp::serialize::fmi2_import_instantiate2(int message_id, bool visible, int type) {
  fmi2_import_instantiate_req req;
  req.set_message_id(message_id);
  req.set_visible(visible);
  req.set_fmutype(type);

  return pack(type_fmi2_import_instantiate_req, req);
}

//implementation for all messages to FMI function that take no arguments (void)
#define FMU_VOID_REQ_IMPL(name)\
std::string fmitcp::serialize::name(int message_id, int fmuId){\
    name##_req req;\
    req.set_message_id(message_id);\
    req.set_fmuid(fmuId);\
\
    return pack(type_##name##_req, req);\
}

FMU_VOID_REQ_IMPL(fmi2_import_free_instance)

std::string fmitcp::serialize::fmi2_import_setup_experiment(int message_id, int fmuId, bool toleranceDefined, double tolerance, double startTime,
    bool stopTimeDefined, double stopTime) {
    fmi2_import_setup_experiment_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    req.set_tolerancedefined(toleranceDefined);
    req.set_tolerance(tolerance);
    req.set_starttime(startTime);
    req.set_stoptimedefined(stopTimeDefined);
    req.set_stoptime(stopTime);

    return pack(type_fmi2_import_setup_experiment_req, req);
}

FMU_VOID_REQ_IMPL(fmi2_import_enter_initialization_mode)
FMU_VOID_REQ_IMPL(fmi2_import_exit_initialization_mode)
FMU_VOID_REQ_IMPL(fmi2_import_terminate)
FMU_VOID_REQ_IMPL(fmi2_import_reset)

std::string fmitcp::serialize::fmi2_import_set_real_input_derivatives(int message_id, int fmuId,
                                                    std::vector<int> valueRefs,
                                                    std::vector<int> orders,
                                                    std::vector<double> values){
    fmi2_import_set_real_input_derivatives_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);

    // TODO: SET the derivatives in message

    return pack(type_fmi2_import_set_real_input_derivatives_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_real_output_derivatives(int message_id, int fmuId, std::vector<int> valueRefs, std::vector<int> orders){
    fmi2_import_get_real_output_derivatives_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);

    return pack(type_fmi2_import_get_real_output_derivatives_req, req);
}

FMU_VOID_REQ_IMPL(fmi2_import_cancel_step)

std::string fmitcp::serialize::fmi2_import_do_step(int message_id,
                                 int fmuId,
                                 double currentCommunicationPoint,
                                 double communicationStepSize,
                                 bool newStep){
    fmi2_import_do_step_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    req.set_currentcommunicationpoint(currentCommunicationPoint);
    req.set_communicationstepsize(communicationStepSize);
    req.set_newstep(newStep);

    return pack(type_fmi2_import_do_step_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    fmi2_import_get_status_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    req.set_status(s);

    return pack(type_fmi2_import_get_status_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_real_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    fmi2_import_get_real_status_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    req.set_kind(s);

    return pack(type_fmi2_import_get_real_status_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_integer_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    fmi2_import_get_integer_status_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    req.set_kind(s);

    return pack(type_fmi2_import_get_integer_status_req, req);
}


std::string fmitcp::serialize::fmi2_import_get_boolean_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    fmi2_import_get_boolean_status_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    req.set_kind(s);

    return pack(type_fmi2_import_get_boolean_status_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_string_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s){
    fmi2_import_get_string_status_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    req.set_kind(s);

    return pack(type_fmi2_import_get_string_status_req, req);
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
    SERIALIZE_NORMAL_MESSAGE_(fmi2_import_set_time, time);
}

std::string fmitcp::serialize::fmi2_import_set_continuous_states(int message_id, int fmuId, const double* x, int nx){
    fmi2_import_set_continuous_states_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    for(int i = 0; i < nx; i++)
      req.set_x(i, x[i]);

    return pack(type_fmi2_import_set_continuous_states_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_event_indicators(int message_id, int fmuId, int nz){
    SERIALIZE_NORMAL_MESSAGE_(fmi2_import_get_event_indicators,nz);
}

std::string fmitcp::serialize::fmi2_import_get_continuous_states(int message_id, int fmuId, int nx){
    SERIALIZE_NORMAL_MESSAGE_(fmi2_import_get_continuous_states,nx);
}

std::string fmitcp::serialize::fmi2_import_get_derivatives(int message_id, int fmuId, int nderivatives){
    SERIALIZE_NORMAL_MESSAGE_(fmi2_import_get_derivatives,nderivatives);
}

std::string fmitcp::serialize::fmi2_import_get_nominal_continuous_states(int message_id, int fmuId, int nx){
    SERIALIZE_NORMAL_MESSAGE_(fmi2_import_get_nominal_continuous_states,nx);
}

FMU_VOID_REQ_IMPL(fmi2_import_get_version)


std::string fmitcp::serialize::fmi2_import_set_debug_logging(int message_id, int fmuId, bool loggingOn, const std::vector<string> categories){
    fmi2_import_set_debug_logging_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    req.set_loggingon(loggingOn);
    for(int i=0; i<categories.size(); i++)
        req.add_categories(categories[i]);

    return pack(type_fmi2_import_set_debug_logging_req, req);
}

std::string fmitcp::serialize::fmi2_import_set_real(int message_id, int fmuId, const vector<int>& valueRefs, const vector<double>& values){
    fmi2_import_set_real_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req.add_values(values[i]);

    return pack(type_fmi2_import_set_real_req, req);
}

std::string fmitcp::serialize::fmi2_import_set_integer(int message_id, int fmuId, const vector<int>& valueRefs, const vector<int>& values){
    fmi2_import_set_integer_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req.add_values(values[i]);

    return pack(type_fmi2_import_set_integer_req, req);
}

std::string fmitcp::serialize::fmi2_import_set_boolean(int message_id, int fmuId, const vector<int>& valueRefs, const vector<bool>& values){
    fmi2_import_set_boolean_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req.add_values(values[i]);

    return pack(type_fmi2_import_set_boolean_req, req);
}


std::string fmitcp::serialize::fmi2_import_set_string(int message_id, int fmuId, const vector<int>& valueRefs, const vector<string>& values){
    fmi2_import_set_string_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);
    for(int i=0; i<values.size(); i++)
        req.add_values(values[i]);

    return pack(type_fmi2_import_set_string_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_real(int message_id, int fmuId, const vector<int>& valueRefs){
    fmi2_import_get_real_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);

    return pack(type_fmi2_import_get_real_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_integer(int message_id, int fmuId, const vector<int>& valueRefs){
    fmi2_import_get_integer_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);

    return pack(type_fmi2_import_get_integer_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_boolean(int message_id, int fmuId, const vector<int>& valueRefs){
    fmi2_import_get_boolean_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);

    return pack(type_fmi2_import_get_boolean_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_string (int message_id, int fmuId, const vector<int>& valueRefs){
    fmi2_import_get_string_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    for(int i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);

    return pack(type_fmi2_import_get_string_req, req);
}

FMU_VOID_REQ_IMPL(fmi2_import_get_fmu_state)

std::string fmitcp::serialize::fmi2_import_set_fmu_state(int message_id, int fmuId, int stateId){
    fmi2_import_set_fmu_state_req req;
    req.set_message_id(message_id);
    req.set_stateid(stateId);
    req.set_fmuid(fmuId);

    return pack(type_fmi2_import_set_fmu_state_req, req);
}

std::string fmitcp::serialize::fmi2_import_free_fmu_state(int message_id, int fmuId, int stateId) {
    fmi2_import_free_fmu_state_req req;
    req.set_message_id(message_id);
    req.set_stateid(stateId);
    req.set_fmuid(fmuId);

    return pack(type_fmi2_import_free_fmu_state_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_directional_derivative(int message_id, int fmuId,
                                                    const vector<int>& z_ref,
                                                    const vector<int>& v_ref,
                                                    const vector<double>& dv){
    fmi2_import_get_directional_derivative_req req;
    req.set_message_id(message_id);
    req.set_fmuid(fmuId);
    for(int i=0; i<v_ref.size(); i++)
        req.add_v_ref(v_ref[i]);
    for(int i=0; i<z_ref.size(); i++)
        req.add_z_ref(z_ref[i]);
    for(int i=0; i<dv.size(); i++)
        req.add_dv(dv[i]);

    return pack(type_fmi2_import_get_directional_derivative_req, req);
}

FMU_VOID_REQ_IMPL(get_xml)
