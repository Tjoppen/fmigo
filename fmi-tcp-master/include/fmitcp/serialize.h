#ifndef FMITCP_SERIALIZE_H
#define FMITCP_SERIALIZE_H

#include <string>

namespace fmitcp {
    namespace serialize {
        // FMI functions follow. These should correspond to FMILibrary functions.

        // =========== FMI 2.0 (CS) Co-Simulation functions ===========
        std::string fmi2_import_set_real_input_derivatives(int mid, int fmuId, std::vector<int> valueRefs, std::vector<int> orders, std::vector<double> values);
        std::string fmi2_import_get_real_output_derivatives(int mid, int fmuId, std::vector<int> valueRefs, std::vector<int> orders);
        std::string fmi2_import_cancel_step(int mid, int fmuId);
        std::string fmi2_import_do_step(int message_id,
                                 int fmuId,
                                 double currentCommunicationPoint,
                                 double communicationStepSize,
                                 bool newStep);
        std::string fmi2_import_get_status        (int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_real_status   (int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_integer_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_boolean_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_string_status (int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s);

        // =========== FMI 2.0 (ME) Model Exchange functions ===========
        std::string fmi2_import_enter_event_mode(int message_id, int fmuId);
        std::string fmi2_import_new_discrete_states(int message_id, int fmuId);
        std::string fmi2_import_enter_continuous_time_mode(int message_id, int fmuId);
        std::string fmi2_import_completed_integrator_step(int message_id, int fmuId);
        std::string fmi2_import_set_time(int message_id, int fmuId, double time);
        std::string fmi2_import_set_continuous_states(int message_id, int fmuId, const double* x, int nx);
        std::string fmi2_import_get_event_indicators(int message_id, int fmuId, int nz);
        std::string fmi2_import_get_continuous_states(int message_id, int fmuId, int nx);
        std::string fmi2_import_get_derivatives(int message_id, int fmuId, int nDerivatives);
        std::string fmi2_import_get_nominal_continuous_states(int message_id, int fmuId, int nx);

        // ========= FMI 2.0 CS & ME COMMON FUNCTIONS ============
        std::string fmi2_import_get_version(int message_id, int fmuId);
        std::string fmi2_import_set_debug_logging(int message_id, int fmuId, bool loggingOn, std::vector<std::string> categories);
        std::string fmi2_import_instantiate(int message_id);               //calls fmi2_import_instantiate2() with visible=false (backward compatibility)
        std::string fmi2_import_instantiate2(int message_id, bool visible);
        std::string fmi2_import_setup_experiment(int message_id, int fmuId, bool toleranceDefined, double tolerance, double startTime,
            bool stopTimeDefined, double stopTime);
        std::string fmi2_import_enter_initialization_mode(int message_id, int fmuId);
        std::string fmi2_import_exit_initialization_mode(int message_id, int fmuId);
        std::string fmi2_import_terminate(int mid, int fmuId);
        std::string fmi2_import_reset(int mid, int fmuId);
        std::string fmi2_import_free_instance(int mid, int fmuId);
        std::string fmi2_import_set_real   (int message_id, int fmuId, const std::vector<int>& valueRefs, const std::vector<double>& values);
        std::string fmi2_import_set_integer(int message_id, int fmuId, const std::vector<int>& valueRefs, const std::vector<int>& values);
        std::string fmi2_import_set_boolean(int message_id, int fmuId, const std::vector<int>& valueRefs, const std::vector<bool>& values);
        std::string fmi2_import_set_string (int message_id, int fmuId, const std::vector<int>& valueRefs, const std::vector<std::string>& values);
        std::string fmi2_import_get_real(int message_id, int fmuId, const std::vector<int>& valueRefs);
        std::string fmi2_import_get_integer(int message_id, int fmuId, const std::vector<int>& valueRefs);
        std::string fmi2_import_get_boolean(int message_id, int fmuId, const std::vector<int>& valueRefs);
        std::string fmi2_import_get_string (int message_id, int fmuId, const std::vector<int>& valueRefs);
        std::string fmi2_import_get_fmu_state(int message_id, int fmuId);
        std::string fmi2_import_set_fmu_state(int message_id, int fmuId, int stateId);
        std::string fmi2_import_free_fmu_state(int messageId, int fmuId, int stateId);
        std::string fmi2_import_serialized_fmu_state_size();
        std::string fmi2_import_serialize_fmu_state();
        std::string fmi2_import_de_serialize_fmu_state();
        std::string fmi2_import_get_directional_derivative(int message_id, int fmuId, const std::vector<int>& v_ref, const std::vector<int>& z_ref, const std::vector<double>& dv);

        // ========= NETWORK SPECIFIC FUNCTIONS ============
        std::string get_xml(int message_id, int fmuId);
    }
}

#endif //FMITCP_SERIALIZE_H
