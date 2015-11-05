#ifndef MASTER_FMICLIENT_H_
#define MASTER_FMICLIENT_H_

#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include <fmitcp/Client.h>
#include <sc/Slave.h>
#include <string>
#include "master/StrongConnector.h"
#include <deque>

namespace fmitcp_master {

    class BaseMaster;

    /// Adds high-level FMI methods to the Client, similar to FMILibrary functions.
    class FMIClient : public fmitcp::Client, public sc::Slave {

    private:
        int m_id;
        string m_host;
        long m_port;
        std::string m_xml;
        bool m_initialized;

        // variables for modelDescription.xml
        jm_callbacks m_jmCallbacks;
        string m_workingDir;
        fmi_import_context_t* m_context;
        // FMI 2.0
        fmi2_import_t* m_fmi2Instance;
        fmi2_import_variable_list_t* m_fmi2Outputs;

    public:

        BaseMaster * m_master;

        /// Last fetched result from getReal
        std::vector<double> m_getRealValues;

        /// Values returned from calls to fmiGetDirectionalDerivative()
        std::deque<std::vector<double> > m_getDirectionalDerivativeValues;

#ifdef USE_LACEWING
        /// Create a new client for the Master, driven by the given eventpump.
        FMIClient(fmitcp::EventPump* pump, int id, std::string host, long port);
#else
        FMIClient(zmq::context_t &context, int id, string host, long port);
#endif
        virtual ~FMIClient();

        int getId();

        void connect(void);

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

        /// Called when the client connects.
        void onConnect();

        /// Called when the client disconnects.
        void onDisconnect();

        /// Called if an error occurred.
        void onError(string err);


        // --- These methods overrides Client methods. Most of them just passes the result to the Master ---

        void on_fmi2_import_instantiate_res                     (int mid, fmitcp_proto::jm_status_enu_t status);
        void on_fmi2_import_initialize_slave_res                (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_terminate_slave_res                 (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_reset_slave_res                     (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_free_slave_instance_res             (int mid);
        //void on_fmi2_import_set_real_input_derivatives_res      (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_real_output_derivatives_res     (int mid, fmitcp_proto::fmi2_status_t status, const vector<double>& values);
        //void on_fmi2_import_cancel_step_res                     (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_do_step_res                         (int mid, fmitcp_proto::fmi2_status_t status);
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
        void on_fmi2_import_get_version_res                     (int mid, string version);
        //void on_fmi2_import_set_debug_logging_res               (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_set_real_res                        (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_integer_res                     (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_boolean_res                     (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_string_res                      (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_real_res                        (int mid, const vector<double>& values, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_integer_res                     (int mid, const vector<int>& values, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_boolean_res                     (int mid, const vector<bool>& values, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_string_res                      (int mid, const vector<string>& values, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_fmu_state_res                   (int mid, int stateId, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_set_fmu_state_res                   (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_free_fmu_state_res                  (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_directional_derivative_res      (int mid, const vector<double>& dz, fmitcp_proto::fmi2_status_t status);
        void on_get_xml_res                                     (int mid, fmitcp_proto::jm_log_level_enu_t logLevel, string xml);
    };
};

#endif
