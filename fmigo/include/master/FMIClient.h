#ifndef MASTER_FMICLIENT_H_
#define MASTER_FMICLIENT_H_

#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include <fmitcp/Client.h>
#include <sc/Slave.h>
#include <string>
#include "master/StrongConnector.h"
#include "WeakConnection.h"
#include "common/common.h"
#include <deque>

namespace fmitcp_master {
    struct variable {
        int vr;
        fmi2_base_type_enu_t type;
        fmi2_causality_enu_t causality;
    };
    typedef std::map<std::string, variable> variable_map;

    class BaseMaster;

    /// Adds high-level FMI methods to the Client, similar to FMILibrary functions.
    class FMIClient : public fmitcp::Client, public sc::Slave {

    private:
        int m_id;
        std::string m_xml;
        bool m_initialized;

        // variables for modelDescription.xml
        jm_callbacks m_jmCallbacks;
        string m_workingDir;
        fmi_import_context_t* m_context;
        // FMI 2.0
        fmi2_import_t* m_fmi2Instance;
        fmi2_import_variable_list_t* m_fmi2Outputs;
        variable_map m_variables;
        vector<variable> m_outputs;
        void setVariables();

    public:
        int m_stateId;

        BaseMaster * m_master;

        /// Last fetched result from getX
        std::deque<double>      m_getRealValues;
        std::deque<int>         m_getIntegerValues;
        std::deque<bool>        m_getBooleanValues;
        std::deque<std::string> m_getStringValues;

        /// Values returned from calls to fmiGetDirectionalDerivative()
        std::deque<std::vector<double> > m_getDirectionalDerivativeValues;

        void clearGetValues();

        std::string getModelName() const;
        const variable_map& getVariables() const;
        const std::vector<variable>& getOutputs() const;   //outputs in the same order as specified in the modelDescription

#ifdef USE_MPI
        FMIClient(int world_rank, int id);
#else
        FMIClient(zmq::context_t &context, int id, string uri);
#endif
        virtual ~FMIClient();

        int getId();

        bool isInitialized();

        /// Create a strong coupling connector for this client
        StrongConnector* createConnector();

        /// Get a connector. See getNumConnectors().
        StrongConnector* getConnector(int i);

        /// Accumulate a get_directional_derivative request
        void pushDirectionalDerivativeRequest(int fmiId, std::vector<int> v_ref, std::vector<int> z_ref, std::vector<double> dv);

        /// Execute the next directional derivative request in the queue
        void shiftExecuteDirectionalDerivativeRequest();

        /// Get the total number of directional derivative requests queued
        int numDirectionalDerivativeRequests();

        /// Returns value references of positions and velocities of all connectors
        std::vector<int> getStrongConnectorValueReferences();

        /// Get seed value references, this is equivalent to forces
        std::vector<int> getStrongSeedInputValueReferences();

        /// Get "result" value references, this is velocities
        std::vector<int> getStrongSeedOutputValueReferences();

        std::vector<int> getRealOutputValueReferences();

        void setConnectorValues          (std::vector<int> valueRefs, std::vector<double> values);
        void setConnectorFutureVelocities(std::vector<int> valueRefs, std::vector<double> values);

        bool hasCapability(fmi2_capabilities_enu_t cap) const;

        // --- These methods overrides Client methods. Most of them just passes the result to the Master ---

        void on_fmi2_import_instantiate_res                     (fmitcp_proto::jm_status_enu_t status);
        void on_fmi2_import_exit_initialization_mode_res        (fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_terminate_res                       (fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_reset_res                           (fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_free_instance_res                   ();
        //void on_fmi2_import_set_real_input_derivatives_res      (fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_real_output_derivatives_res     (fmitcp_proto::fmi2_status_t status, const vector<double>& values);
        //void on_fmi2_import_cancel_step_res                     (fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_do_step_res                         (fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_status_res                      (fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_real_status_res                 (double value);
        //void on_fmi2_import_get_integer_status_res              (int value);
        //void on_fmi2_import_get_boolean_status_res              (bool value);
        //void on_fmi2_import_get_string_status_res               (string value);
        //void on_fmi2_import_set_time_res                        (fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_continuous_states_res           (fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_completed_integrator_step_res       (bool callEventUpdate, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_initialize_model_res                (bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_derivatives_res                 (const vector<double>& derivatives, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_event_indicators_res            (const vector<double>& eventIndicators, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_eventUpdate_res                     (bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_completed_event_iteration_res       (fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_continuous_states_res           (const vector<double>& states, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_nominal_continuous_states_res   (const vector<double>& nominal, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_terminate_res                       (fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_version_res                     (string version);
        //void on_fmi2_import_set_debug_logging_res               (fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_set_real_res                        (fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_integer_res                     (fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_boolean_res                     (fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_string_res                      (fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_real_res                        (const deque<double>& values, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_integer_res                     (const deque<int>& values, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_boolean_res                     (const deque<bool>& values, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_string_res                      (const deque<string>& values, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_fmu_state_res                   (int stateId, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_set_fmu_state_res                   (fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_free_fmu_state_res                  (fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_directional_derivative_res      (const vector<double>& dz, fmitcp_proto::fmi2_status_t status);
        void on_get_xml_res                                     (fmitcp_proto::jm_log_level_enu_t logLevel, string xml);

        //returns string of field names, all of which prepended with prefix
        std::string getSpaceSeparatedFieldNames(std::string prefix) const;

        void sendGetX(const SendGetXType& typeRefs);
        void sendSetX(const SendSetXType& typeRefsValues);
    };
};

#endif
