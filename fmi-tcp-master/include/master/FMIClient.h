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
#ifndef USE_MPI
        string m_host;
        long m_port;
#endif
        std::string m_xml;
        bool m_initialized;

        // variables for modelDescription.xml
        jm_callbacks m_jmCallbacks;
        string m_workingDir;
        fmi_import_context_t* m_context;
        // FMI 2.0
        fmi2_import_t* m_fmi2Instance;
        fmi2_import_variable_list_t* m_fmi2Outputs;
        fmi2_event_info_t* m_fmi2EventInfo;

    public:
        int m_stateId;

        BaseMaster * m_master;

        /// Last fetched result from getX
        std::deque<double>      m_getRealValues;
        std::deque<int>         m_getIntegerValues;
        std::deque<bool>        m_getBooleanValues;
        std::deque<std::string> m_getStringValues;

        std::deque<std::vector<double>> m_getDerivatives;
        std::deque<std::vector<double>> m_getContinuousStates;
        std::deque<std::vector<double>> m_getNominalContinuousStates;
        std::deque<std::vector<double>> m_getEventIndicators;
        /// Values returned from calls to fmiGetDirectionalDerivative()
        std::deque<std::vector<double> > m_getDirectionalDerivativeValues;

        void clearGetValues();

        jm_log_level_enu_t m_loglevel;

        std::string getModelName() const;
        variable_map getVariables() const;
        std::vector<variable> getOutputs() const;   //outputs in the same order as specified in the modelDescription

#ifdef USE_MPI
        FMIClient(int world_rank, int id);
#else
        FMIClient(zmq::context_t &context, int id, string host, long port);
#endif
        virtual ~FMIClient();

        int getId();

        //connects to remote host and gets modelDescription XML
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

        bool hasCapability(fmi2_capabilities_enu_t cap) const;

        size_t getNumEventIndicators(void);
        size_t getNumContinuousStates(void);

        // --- These methods overrides Client methods. Most of them just passes the result to the Master ---

        void on_fmi2_import_instantiate_res                     (int mid, fmitcp_proto::jm_status_enu_t status);
        void on_fmi2_import_exit_initialization_mode_res        (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_terminate_res                       (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_reset_res                           (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_free_instance_res                   (int mid);
        //void on_fmi2_import_set_real_input_derivatives_res      (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_real_output_derivatives_res     (int mid, fmitcp_proto::fmi2_status_t status, const vector<double>& values);
        //void on_fmi2_import_cancel_step_res                     (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_do_step_res                         (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_status_res                      (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_get_real_status_res                 (int mid, double value);
        //void on_fmi2_import_get_integer_status_res              (int mid, int value);
        //void on_fmi2_import_get_boolean_status_res              (int mid, bool value);
        //void on_fmi2_import_get_string_status_res               (int mid, string value);
        //void on_fmi2_import_set_time_res                        (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_continuous_states_res           (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_completed_integrator_step_res       (int mid, bool callEventUpdate, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_new_discrete_states_res             (int mid, fmitcp_proto::fmi2_event_info_t eventInfo);
        //void on_fmi2_import_initialize_model_res                (int mid, bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_derivatives_res                 (int mid, const vector<double>& derivatives, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_event_indicators_res            (int mid, const vector<double>& eventIndicators, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_eventUpdate_res                     (int mid, bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_completed_event_iteration_res       (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_continuous_states_res           (int mid, const vector<double>& states, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_nominal_continuous_states_res   (int mid, const vector<double>& nominal, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_terminate_res                       (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_version_res                     (int mid, string version);
        //void on_fmi2_import_set_debug_logging_res               (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_set_real_res                        (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_integer_res                     (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_boolean_res                     (int mid, fmitcp_proto::fmi2_status_t status);
        //void on_fmi2_import_set_string_res                      (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_real_res                        (int mid, const deque<double>& values, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_integer_res                     (int mid, const deque<int>& values, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_boolean_res                     (int mid, const deque<bool>& values, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_string_res                      (int mid, const deque<string>& values, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_fmu_state_res                   (int mid, int stateId, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_set_fmu_state_res                   (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_free_fmu_state_res                  (int mid, fmitcp_proto::fmi2_status_t status);
        void on_fmi2_import_get_directional_derivative_res      (int mid, const vector<double>& dz, fmitcp_proto::fmi2_status_t status);
        void on_get_xml_res                                     (int mid, fmitcp_proto::jm_log_level_enu_t logLevel, string xml);

        //returns string of field names, all of which prepended with prefix
        std::string getSpaceSeparatedFieldNames(std::string prefix) const;

        void sendGetX(const SendGetXType& typeRefs);
        void sendSetX(const SendSetXType& typeRefsValues);
    };
};

#endif
