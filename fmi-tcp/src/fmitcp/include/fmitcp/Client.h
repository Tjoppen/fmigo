#ifndef CLIENT_H_
#define CLIENT_H_

#include "EventPump.h"
#include "Logger.h"
#include "fmitcp.pb.h"
#include <string>
#include <vector>
#define lw_import
#include <lacewing.h>

using namespace std;

namespace fmitcp {

    /**
     * @brief FMI Client that can do requests to a server, similar to the FMI API.
     * The idea is that this class should be extended by a subclass that implements its methods. In this way the subclass can fetch events such as "onConnect" and "onError".
     */
    class Client {

    protected:

        /// Event pump that will push the communication forward
        EventPump * m_pump;

        /// For logging
        Logger m_logger;

    private:
        lw_client m_client;

    public:
        Client(EventPump * pump);
        ~Client();

        /// Connect the client to a server
        void connect(string host, long port);
        void disconnect();
        Logger * getLogger();

        /// Send a binary message
        void sendMessage(fmitcp_proto::fmitcp_message * message);

        bool isConnected();

        /// To be implemented in subclass
        virtual void onConnect(){}

        /// To be implemented in subclass
        virtual void onDisconnect(){}

        /// To be implemented in subclass
        virtual void onError(string message){}

        /// Called when a client connects
        void clientConnected(lw_client c);
        void clientData(lw_client c, const char* data, long size);
        void clientDisconnected(lw_client c);
        void clientError(lw_client c, lw_error error);

        // Response functions - to be implemented by subclass
        virtual void onGetXmlRes(int mid, fmitcp_proto::jm_log_level_enu_t logLevel, string xml){}
        virtual void on_fmi2_import_instantiate_res                     (int mid, fmitcp_proto::jm_status_enu_t status){}
        virtual void on_fmi2_import_initialize_slave_res                (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_terminate_slave_res                 (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_reset_slave_res                     (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_free_slave_instance_res             (int mid){}
        virtual void on_fmi2_import_set_real_input_derivatives_res      (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_real_output_derivatives_res     (int mid, fmitcp_proto::fmi2_status_t status, const vector<double>& values){}
        virtual void on_fmi2_import_cancel_step_res                     (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_do_step_res                         (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_status_res                      (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_real_status_res                 (int mid, double value){}
        virtual void on_fmi2_import_get_integer_status_res              (int mid, int value){}
        virtual void on_fmi2_import_get_boolean_status_res              (int mid, bool value){}
        virtual void on_fmi2_import_get_string_status_res               (int mid, string value){}
        virtual void on_fmi2_import_instantiate_model_res               (int mid, fmitcp_proto::jm_status_enu_t status){}
        virtual void on_fmi2_import_free_model_instance_res             (int mid){}
        virtual void on_fmi2_import_set_time_res                        (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_continuous_states_res           (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_completed_integrator_step_res       (int mid, bool callEventUpdate, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_initialize_model_res                (int mid, bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_derivatives_res                 (int mid, const vector<double>& derivatives, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_event_indicators_res            (int mid, const vector<double>& eventIndicators, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_eventUpdate_res                     (int mid, bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_completed_event_iteration_res       (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_continuous_states_res           (int mid, const vector<double>& states, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_nominal_continuous_states_res   (int mid, const vector<double>& nominal, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_terminate_res                       (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_version_res                     (int mid, string version){}
        virtual void on_fmi2_import_set_debug_logging_res               (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_real_res                        (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_integer_res                     (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_boolean_res                     (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_string_res                      (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_real_res                        (int mid, const vector<double>& values, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_integer_res                     (int mid, const vector<int>& values, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_boolean_res                     (int mid, const vector<bool>& values, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_string_res                      (int mid, const vector<string>& values, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_fmu_state_res                   (int mid, int stateId, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_fmu_state_res                   (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_free_fmu_state_res                  (int mid, fmitcp_proto::fmi2_status_t status){}
        /*virtual void on_fmi2_import_serialized_fmu_state_size_res(){}
        virtual void on_fmi2_import_serialize_fmu_state_res(){}
        virtual void on_fmi2_import_de_serialize_fmu_state_res(){}
        */
        virtual void on_fmi2_import_get_directional_derivative_res(int mid, const vector<double>& dz, fmitcp_proto::fmi2_status_t status){}

        void getXml(int message_id, int fmuId);

        // FMI functions follow. These should correspond to FMILibrary functions.

        // =========== FMI 2.0 (CS) Co-Simulation functions ===========
        void fmi2_import_instantiate(int message_id);
        void fmi2_import_initialize_slave(int message_id, int fmuId, bool toleranceDefined, double tolerance, double startTime,
            bool stopTimeDefined, double stopTime);
        void fmi2_import_terminate_slave(int mid, int fmuId);
        void fmi2_import_reset_slave(int mid, int fmuId);
        void fmi2_import_free_slave_instance(int mid, int fmuId);
        void fmi2_import_set_real_input_derivatives(int mid, int fmuId, std::vector<int> valueRefs, std::vector<int> orders, std::vector<double> values);
        void fmi2_import_get_real_output_derivatives(int mid, int fmuId, std::vector<int> valueRefs, std::vector<int> orders);
        void fmi2_import_cancel_step(int mid, int fmuId);
        void fmi2_import_do_step(int message_id,
                                 int fmuId,
                                 double currentCommunicationPoint,
                                 double communicationStepSize,
                                 bool newStep);
        void fmi2_import_get_status        (int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s);
        void fmi2_import_get_real_status   (int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s);
        void fmi2_import_get_integer_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s);
        void fmi2_import_get_boolean_status(int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s);
        void fmi2_import_get_string_status (int message_id, int fmuId, fmitcp_proto::fmi2_status_kind_t s);

        // =========== FMI 2.0 (ME) Model Exchange functions ===========
        void fmi2_import_instantiate_model();
        void fmi2_import_free_model_instance();
        void fmi2_import_set_time();
        void fmi2_import_set_continuous_states();
        void fmi2_import_completed_integrator_step();
        void fmi2_import_initialize_model();
        void fmi2_import_get_derivatives();
        void fmi2_import_get_event_indicators();
        void fmi2_import_eventUpdate();
        void fmi2_import_completed_event_iteration();
        void fmi2_import_get_continuous_states();
        void fmi2_import_get_nominal_continuous_states();
        void fmi2_import_terminate();

        // ========= FMI 2.0 CS & ME COMMON FUNCTIONS ============
        void fmi2_import_get_version(int message_id, int fmuId);
        void fmi2_import_set_debug_logging(int message_id, int fmuId, bool loggingOn, std::vector<string> categories);
        void fmi2_import_set_real   (int message_id, int fmuId, const vector<int>& valueRefs, const vector<double>& values);
        void fmi2_import_set_integer(int message_id, int fmuId, const vector<int>& valueRefs, const vector<int>& values);
        void fmi2_import_set_boolean(int message_id, int fmuId, const vector<int>& valueRefs, const vector<bool>& values);
        void fmi2_import_set_string (int message_id, int fmuId, const vector<int>& valueRefs, const vector<string>& values);
        void fmi2_import_get_real(int message_id, int fmuId, const vector<int>& valueRefs);
        void fmi2_import_get_integer(int message_id, int fmuId, const vector<int>& valueRefs);
        void fmi2_import_get_boolean(int message_id, int fmuId, const vector<int>& valueRefs);
        void fmi2_import_get_string (int message_id, int fmuId, const vector<int>& valueRefs);
        void fmi2_import_get_fmu_state(int message_id, int fmuId);
        void fmi2_import_set_fmu_state(int message_id, int fmuId, int stateId);
        void fmi2_import_free_fmu_state(int messageId, int stateId);
        void fmi2_import_serialized_fmu_state_size();
        void fmi2_import_serialize_fmu_state();
        void fmi2_import_de_serialize_fmu_state();
        void fmi2_import_get_directional_derivative(int message_id, int fmuId, const vector<int>& v_ref, const vector<int>& z_ref, const vector<double>& dv);

        // ========= NETWORK SPECIFIC FUNCTIONS ============
        void get_xml(int message_id, int fmuId);
    };

};

#endif
