#include "serialize.h"
#include "fmitcp/fmitcp-common.h"
#include <stdint.h>
using namespace fmitcp_proto;
using namespace std;

#define SERIALIZE_NORMAL_MESSAGE_(type, extra)                          \
    /* Contruct message */                                              \
    type##_req req;                                                     \
    req.set_##extra(extra);                                             \
    return pack(type_##type##_req, req);

#define SERIALIZE_NORMAL_MESSAGE(type)                                  \
    /* Contruct message */                                              \
    type##_req req;                                                     \
    return pack(type_##type##_req, req);

std::string fmitcp::serialize::fmi2_import_instantiate2(bool visible) {
  fmi2_import_instantiate_req req;
  req.set_visible(visible);

  return pack(type_fmi2_import_instantiate_req, req);
}

//implementation for all messages to FMI function that take no arguments (void)
#define FMU_VOID_REQ_IMPL(name)\
std::string fmitcp::serialize::name(){\
    name##_req req;\
\
    return pack(type_##name##_req, req);\
}

FMU_VOID_REQ_IMPL(fmi2_import_free_instance)

std::string fmitcp::serialize::fmi2_import_setup_experiment(bool toleranceDefined, double tolerance, double startTime,
    bool stopTimeDefined, double stopTime) {
    fmi2_import_setup_experiment_req req;
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

std::string fmitcp::serialize::fmi2_import_set_real_input_derivatives(std::vector<int> valueRefs,
                                                                      std::vector<int> orders,
                                                                      std::vector<double> values){
    fmi2_import_set_real_input_derivatives_req req;

    // TODO: SET the derivatives in message

    return pack(type_fmi2_import_set_real_input_derivatives_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_real_output_derivatives(std::vector<int> valueRefs, std::vector<int> orders){
    fmi2_import_get_real_output_derivatives_req req;

    return pack(type_fmi2_import_get_real_output_derivatives_req, req);
}

FMU_VOID_REQ_IMPL(fmi2_import_cancel_step)

std::string fmitcp::serialize::fmi2_import_do_step(double currentCommunicationPoint,
                                                   double communicationStepSize,
                                                   bool newStep){
#if USE_DO_STEP_S == 1
    std::string str(2 + sizeof(do_step_s), 0);
    str[0] = type_fmi2_import_do_step_req;
    str[1] = type_fmi2_import_do_step_req >> 8;
    do_step_s *s = (do_step_s*)&str[2];
    s->currentcommunicationpoint = currentCommunicationPoint;
    s->communicationstepsize = communicationStepSize;
    s->newStep = newStep;

    return str;
#else
    fmi2_import_do_step_req req;
    req.set_currentcommunicationpoint(currentCommunicationPoint);
    req.set_communicationstepsize(communicationStepSize);
    req.set_newstep(newStep);

    return pack(type_fmi2_import_do_step_req, req);
#endif
}

std::string fmitcp::serialize::fmi2_import_get_status(fmitcp_proto::fmi2_status_kind_t s){
    fmi2_import_get_status_req req;
    req.set_status(s);

    return pack(type_fmi2_import_get_status_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_real_status(fmitcp_proto::fmi2_status_kind_t s){
    fmi2_import_get_real_status_req req;
    req.set_kind(s);

    return pack(type_fmi2_import_get_real_status_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_integer_status(fmitcp_proto::fmi2_status_kind_t s){
    fmi2_import_get_integer_status_req req;
    req.set_kind(s);

    return pack(type_fmi2_import_get_integer_status_req, req);
}


std::string fmitcp::serialize::fmi2_import_get_boolean_status(fmitcp_proto::fmi2_status_kind_t s){
    fmi2_import_get_boolean_status_req req;
    req.set_kind(s);

    return pack(type_fmi2_import_get_boolean_status_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_string_status(fmitcp_proto::fmi2_status_kind_t s){
    fmi2_import_get_string_status_req req;
    req.set_kind(s);

    return pack(type_fmi2_import_get_string_status_req, req);
}

        // =========== FMI 2.0 (ME) Model Exchange functions ===========

std::string fmitcp::serialize::fmi2_import_enter_event_mode(){
    SERIALIZE_NORMAL_MESSAGE(fmi2_import_enter_event_mode);
}

std::string fmitcp::serialize::fmi2_import_new_discrete_states(){
    SERIALIZE_NORMAL_MESSAGE(fmi2_import_new_discrete_states);
}

std::string fmitcp::serialize::fmi2_import_enter_continuous_time_mode(){
    SERIALIZE_NORMAL_MESSAGE(fmi2_import_enter_continuous_time_mode);
}

std::string fmitcp::serialize::fmi2_import_completed_integrator_step(){
    SERIALIZE_NORMAL_MESSAGE(fmi2_import_completed_integrator_step);
}

std::string fmitcp::serialize::fmi2_import_set_time(double time){
    SERIALIZE_NORMAL_MESSAGE_(fmi2_import_set_time, time);
}

std::string fmitcp::serialize::fmi2_import_set_continuous_states(const double* x, int nx){
    fmi2_import_set_continuous_states_req req;
    req.set_nx(nx);

    for(int i = 0; i < nx; i++)
        req.add_x(x[i]);

    return pack(type_fmi2_import_set_continuous_states_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_event_indicators(int nz){
    SERIALIZE_NORMAL_MESSAGE_(fmi2_import_get_event_indicators,nz);
}

std::string fmitcp::serialize::fmi2_import_get_continuous_states(int nx){
    SERIALIZE_NORMAL_MESSAGE_(fmi2_import_get_continuous_states,nx);
}

std::string fmitcp::serialize::fmi2_import_get_derivatives(int nderivatives){
    SERIALIZE_NORMAL_MESSAGE_(fmi2_import_get_derivatives,nderivatives);
}

std::string fmitcp::serialize::fmi2_import_get_nominal_continuous_states(int nx){
    SERIALIZE_NORMAL_MESSAGE_(fmi2_import_get_nominal_continuous_states,nx);
}

FMU_VOID_REQ_IMPL(fmi2_import_get_version)

std::string fmitcp::serialize::fmi2_import_set_debug_logging(bool loggingOn, const std::vector<string> categories){
    fmi2_import_set_debug_logging_req req;
    req.set_loggingon(loggingOn);
    for(size_t i=0; i<categories.size(); i++)
        req.add_categories(categories[i]);

    return pack(type_fmi2_import_set_debug_logging_req, req);
}

std::string fmitcp::serialize::fmi2_import_set_real(const vector<int>& valueRefs, const vector<double>& values){
#if USE_SET_REAL_S == 1
    if (sizeof(int) != sizeof(fmi2_value_reference_t) ||
        sizeof(double) != sizeof(fmi2_real_t)) {
        fatal("int/double must be same size as fmi2_value_reference_t/fmi2_real_t\n");
    }
    if (valueRefs.size() != values.size()) {
        fatal("valueRefs.size() != values.size()\n");
    }

    size_t n = valueRefs.size();
    std::string str(2 + n * (sizeof(int) + sizeof(double)), 0);
    int *vr =  (int*)&str[2];
    double *value = (double*)&vr[n];

    str[0] = type_fmi2_import_set_real_req & 0xFF;
    str[1] = type_fmi2_import_set_real_req >> 8;
    memcpy(vr, valueRefs.data(), n*sizeof(int));
    memcpy(value, values.data(), n*sizeof(double));

    return str;
#else
    fmi2_import_set_real_req req;
    for(size_t i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);
    for(size_t i=0; i<values.size(); i++)
        req.add_values(values[i]);

    return pack(type_fmi2_import_set_real_req, req);
#endif
}

std::string fmitcp::serialize::fmi2_import_set_integer(const vector<int>& valueRefs, const vector<int>& values){
    fmi2_import_set_integer_req req;
    for(size_t i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);
    for(size_t i=0; i<values.size(); i++)
        req.add_values(values[i]);

    return pack(type_fmi2_import_set_integer_req, req);
}

std::string fmitcp::serialize::fmi2_import_set_boolean(const vector<int>& valueRefs, const vector<bool>& values){
    fmi2_import_set_boolean_req req;
    for(size_t i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);
    for(size_t i=0; i<values.size(); i++)
        req.add_values(values[i]);

    return pack(type_fmi2_import_set_boolean_req, req);
}


std::string fmitcp::serialize::fmi2_import_set_string(const vector<int>& valueRefs, const vector<string>& values){
    fmi2_import_set_string_req req;
    for(size_t i=0; i<valueRefs.size(); i++)
        req.add_valuereferences(valueRefs[i]);
    for(size_t i=0; i<values.size(); i++)
        req.add_values(values[i]);

    return pack(type_fmi2_import_set_string_req, req);
}

FMU_VOID_REQ_IMPL(fmi2_import_get_fmu_state)

std::string fmitcp::serialize::fmi2_import_set_fmu_state(int stateId){
    fmi2_import_set_fmu_state_req req;
    req.set_stateid(stateId);

    return pack(type_fmi2_import_set_fmu_state_req, req);
}

std::string fmitcp::serialize::fmi2_import_free_fmu_state(int stateId) {
    fmi2_import_free_fmu_state_req req;
    req.set_stateid(stateId);

    return pack(type_fmi2_import_free_fmu_state_req, req);
}

std::string fmitcp::serialize::fmi2_import_set_free_last_fmu_state() {
    fmi2_import_set_free_last_fmu_state_req req;
    return pack(type_fmi2_import_set_free_last_fmu_state_req, req);
}

std::string fmitcp::serialize::fmi2_import_get_directional_derivative(
                                                    const vector<int>& z_ref,
                                                    const vector<int>& v_ref,
                                                    const vector<double>& dv){
    fmi2_import_get_directional_derivative_req req;
    for(size_t i=0; i<v_ref.size(); i++)
        req.add_v_ref(v_ref[i]);
    for(size_t i=0; i<z_ref.size(); i++)
        req.add_z_ref(z_ref[i]);
    for(size_t i=0; i<dv.size(); i++)
        req.add_dv(dv[i]);

    return pack(type_fmi2_import_get_directional_derivative_req, req);
}

FMU_VOID_REQ_IMPL(get_xml)
