#ifndef CLIENT_H_
#define CLIENT_H_

#ifndef USE_MPI
#include <zmq.hpp>
#endif
#include "Logger.h"
#include "fmitcp.pb.h"
#include <string>
#include <vector>

using namespace std;

namespace fmitcp {

    /**
     * @brief FMI Client that can do requests to a server, similar to the FMI API.
     * The idea is that this class should be extended by a subclass that implements its methods. In this way the subclass can fetch events such as "onConnect" and "onError".
     */
    class Client {

    protected:

        /// For logging
        Logger m_logger;

    private:
        size_t m_pendingRequests;

#ifdef USE_MPI
        int world_rank;
#else
    public:
        zmq::socket_t m_socket;
#endif

    public:
#ifdef USE_MPI
        Client(int world_rank);
#else
        Client(zmq::context_t &context);
#endif
        virtual ~Client();

        Logger * getLogger();

#ifndef USE_MPI
        /// Connect the client to a server
        void connect(string host, long port);
#endif

        /// Send a binary message
        void sendMessage(std::string s);

        //like sendMessage() but also receives the result message and calls clientData() on it
        void sendMessageBlocking(std::string s);

        /// To be implemented in subclass
        virtual void onConnect(){}

        /// To be implemented in subclass
        virtual void onDisconnect(){}

        /// To be implemented in subclass
        virtual void onError(string message){}

        size_t getNumPendingRequests() const;

        /**
         * clientData
         * Decode given protobuffer, call appropriate virtual callback function.
         * @param data Protobuf data buffer
         * @param size Size of data buffer
         */
        void clientData(const char* data, long size);

        // Response functions - to be implemented by subclass
        
        // =========== FMI 2.0 (CS) Co-Simulation functions ===========
        virtual void on_fmi2_import_set_real_input_derivatives_res      (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_real_output_derivatives_res     (int mid, fmitcp_proto::fmi2_status_t status, const vector<double>& values){}
        virtual void on_fmi2_import_do_step_res                         (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_cancel_step_res                     (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_status_res                      (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_real_status_res                 (int mid, double value){}
        virtual void on_fmi2_import_get_integer_status_res              (int mid, int value){}
        virtual void on_fmi2_import_get_boolean_status_res              (int mid, bool value){}
        virtual void on_fmi2_import_get_string_status_res               (int mid, string value){}

        
        // =========== FMI 2.0 (ME) Model Exchange functions ===========
        virtual void on_fmi2_import_enter_event_mode_res                (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_new_discrete_states_res             (int mid, fmitcp_proto::fmi2_event_info_t eventInfo){}
        virtual void on_fmi2_import_enter_continuous_time_mode_res      (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_instantiate_res                     (int mid, fmitcp_proto::jm_status_enu_t status){}
        virtual void on_fmi2_import_free_instance_res                   (int mid){}
        virtual void on_fmi2_import_setup_experiment_res                (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_enter_initialization_mode_res       (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_exit_initialization_mode_res        (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_terminate_res                       (int mid, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_reset_res                           (int mid, fmitcp_proto::fmi2_status_t status){}
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


        // =========== FMI 2.0 Common functions ===========
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
        virtual void on_get_xml_res                                     (int mid, fmitcp_proto::jm_log_level_enu_t logLevel, string xml){}
    };

};

#endif
