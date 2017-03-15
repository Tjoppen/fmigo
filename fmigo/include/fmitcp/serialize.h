#ifndef FMITCP_SERIALIZE_H
#define FMITCP_SERIALIZE_H

#include <string>

namespace fmitcp {
    namespace serialize {
        // FMI functions follow. These should correspond to FMILibrary functions.

        // =========== FMI 2.0 (CS) Co-Simulation functions ===========
        std::string fmi2_import_set_real_input_derivatives(std::vector<int> valueRefs, std::vector<int> orders, std::vector<double> values);
        std::string fmi2_import_get_real_output_derivatives(std::vector<int> valueRefs, std::vector<int> orders);
        std::string fmi2_import_cancel_step();
        std::string fmi2_import_do_step(double currentCommunicationPoint,
                                        double communicationStepSize,
                                        bool newStep);
        std::string fmi2_import_get_status        (fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_real_status   (fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_integer_status(fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_boolean_status(fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_string_status (fmitcp_proto::fmi2_status_kind_t s);

        // =========== FMI 2.0 (ME) Model Exchange functions ===========
        std::string fmi2_import_set_time(double time);
        std::string fmi2_import_set_time();
        std::string fmi2_import_set_continuous_states();
        std::string fmi2_import_completed_integrator_step();
        std::string fmi2_import_initialize_model();
        std::string fmi2_import_get_derivatives();
        std::string fmi2_import_get_event_indicators();
        std::string fmi2_import_eventUpdate();
        std::string fmi2_import_completed_event_iteration();
        std::string fmi2_import_get_continuous_states();
        std::string fmi2_import_get_nominal_continuous_states();

        // ========= FMI 2.0 CS & ME COMMON FUNCTIONS ============
        std::string fmi2_import_get_version();
        std::string fmi2_import_set_debug_logging(bool loggingOn, std::vector<std::string> categories);
        std::string fmi2_import_instantiate();               //calls fmi2_import_instantiate2() with visible=false (backward compatibility)
        std::string fmi2_import_instantiate2(bool visible);
        std::string fmi2_import_setup_experiment(bool toleranceDefined, double tolerance, double startTime,
            bool stopTimeDefined, double stopTime);
        std::string fmi2_import_enter_initialization_mode();
        std::string fmi2_import_exit_initialization_mode();
        std::string fmi2_import_terminate();
        std::string fmi2_import_reset();
        std::string fmi2_import_free_instance();
        std::string fmi2_import_set_real   (const std::vector<int>& valueRefs, const std::vector<double>& values);
        std::string fmi2_import_set_integer(const std::vector<int>& valueRefs, const std::vector<int>& values);
        std::string fmi2_import_set_boolean(const std::vector<int>& valueRefs, const std::vector<bool>& values);
        std::string fmi2_import_set_string (const std::vector<int>& valueRefs, const std::vector<std::string>& values);
        std::string fmi2_import_get_real(const std::vector<int>& valueRefs);
        std::string fmi2_import_get_integer(const std::vector<int>& valueRefs);
        std::string fmi2_import_get_boolean(const std::vector<int>& valueRefs);
        std::string fmi2_import_get_string (const std::vector<int>& valueRefs);
        std::string fmi2_import_get_fmu_state();
        std::string fmi2_import_set_fmu_state(int stateId);
        std::string fmi2_import_free_fmu_state(int stateId);
        std::string fmi2_import_serialized_fmu_state_size();
        std::string fmi2_import_serialize_fmu_state();
        std::string fmi2_import_de_serialize_fmu_state();
        std::string fmi2_import_get_directional_derivative(const std::vector<int>& v_ref, const std::vector<int>& z_ref, const std::vector<double>& dv);

        // ========= NETWORK SPECIFIC FUNCTIONS ============
        std::string get_xml();
    }
}

#endif //FMITCP_SERIALIZE_H
