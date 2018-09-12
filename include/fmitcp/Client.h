#ifndef CLIENT_H_
#define CLIENT_H_

#ifndef USE_MPI
#include <zmq.hpp>
#endif
#include "fmitcp.pb.h"
#include <string>
#include <set>
#include <unordered_map>
#include <sstream>
#include "fmitcp/fmitcp-common.h"

using namespace std;

namespace fmitcp_master {
    class BaseMaster;
}

namespace fmitcp {

    /**
     * @brief FMI Client that can do requests to a server, similar to the FMI API.
     * The idea is that this class should be extended by a subclass that implements its methods. In this way the subclass can fetch events such as "onConnect" and "onError".
     */
    class Client {

    private:
        //only used during init, before BaseMaster has been created
        int m_pendingRequests;

#ifdef USE_MPI
        int world_rank;
        std::string m_mpi_str;
#else
    public:
        zmq::socket_t m_socket;
#endif

        std::ostringstream m_messageQueue;

        void clientDataInner(const char* data, size_t size);

#ifdef GATHER_SIZES
        //packet sizes
        map<size_t,size_t> sizes_out, sizes_in;
#endif

    public:
        int messages;
        fmitcp_master::BaseMaster * m_master;

        fmitcp_proto::fmi2_kinematic_res last_kinematic;

        //value cache
        std::unordered_map<int, double>      m_reals;
        std::unordered_map<int, int>         m_ints;
        std::unordered_map<int, bool>        m_bools;
        std::unordered_map<int, std::string> m_strings;

        //set of VRs currently being requested
        fmitcp::int_set              m_outgoing_reals;
        fmitcp::int_set              m_outgoing_ints;
        fmitcp::int_set              m_outgoing_bools;
        fmitcp::int_set              m_outgoing_strings;

        //delete cached values
        void deleteCachedValues(void) {
#ifdef DEBUG
            //only bother with this in debug mode
            //alternatively we could keep a "staleness" map around,
            //marking all entries "true" in here and "false" when parsing
            //type_fmi2_import_get_real_res and friends
            m_reals.clear();
            m_ints.clear();
            m_bools.clear();
            m_strings.clear();
#endif
        }

#if DONT_FILTER_OUTGOING_VRS == 1
        void queueReals(const vector<int>& vrs) {
          m_outgoing_reals.insert(vrs.begin(), vrs.end());
        }
        void queueInts(const vector<int>& vrs) {
          m_outgoing_ints.insert(vrs.begin(), vrs.end());
        }
        void queueBools(const vector<int>& vrs) {
          m_outgoing_bools.insert(vrs.begin(), vrs.end());
        }
        void queueStrings(const vector<int>& vrs) {
          m_outgoing_strings.insert(vrs.begin(), vrs.end());
        }
#else
        template<typename T> void queueFoo(const vector<int>& vrs,
                                           const unordered_map<int,T>& values,
                                           fmitcp::int_set& outgoing) {
          //only queue values which we haven't seen yet
          for (int vr : vrs) {
            auto it = values.find(vr);
            if (it == values.end()) {
              outgoing.insert(vr);
            }
          }
        }

        void queueReals(const vector<int>& vrs) {
          queueFoo(vrs, m_reals, m_outgoing_reals);
        }
        void queueInts(const vector<int>& vrs) {
          queueFoo(vrs, m_ints, m_outgoing_ints);
        }
        void queueBools(const vector<int>& vrs) {
          queueFoo(vrs, m_bools, m_outgoing_bools);
        }
        void queueStrings(const vector<int>& vrs) {
          queueFoo(vrs, m_strings, m_outgoing_strings);
        }
#endif

        void queueValueRequests();

        std::vector<double>       getReals(const std::vector<int>& vrs) const;
        std::vector<int>          getInts(const std::vector<int>& vrs) const;
        std::vector<bool>         getBools(const std::vector<int>& vrs) const;
        std::vector<std::string>  getStrings(const std::vector<int>& vrs) const;

        double       getReal(int vr) const;
        int          getInt(int vr) const;
        bool         getBool(int vr) const;
        std::string  getString(int vr) const;

#ifdef USE_MPI
        Client(int world_rank);
#else
        Client(zmq::context_t &context, std::string uri);
#endif
        virtual ~Client();

        //queue a message to be sent by sendQueuedMessages()
        //helps reduce the number of MPI_Send()s performed
        void queueMessage(const std::string& s);

        //sends queued messages, unsurprisingly
        void sendQueuedMessages();

        //like sendMessage() but also calls receiveAndHandleMessage()
        void sendMessageBlocking(const std::string& s);

        //called by anyone that knows we have a message waiting or who wants us to do a blocking recv
        //calls clientData() on the recv'd data
        void receiveAndHandleMessage();

        /**
         * clientData
         * Decode given protobuffer, call appropriate virtual callback function.
         * @param data Protobuf data buffer
         * @param size Size of data buffer
         */
        void clientData(const char* data, size_t size);

        // Response functions - to be implemented by subclass

        // =========== FMI 2.0 (CS) Co-Simulation functions ===========
        virtual void on_fmi2_import_set_real_input_derivatives_res      (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_real_output_derivatives_res     (fmitcp_proto::fmi2_status_t status, const vector<double>& values){}
        virtual void on_fmi2_import_do_step_res                         (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_cancel_step_res                     (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_status_res                      (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_real_status_res                 (double value){}
        virtual void on_fmi2_import_get_integer_status_res              (int value){}
        virtual void on_fmi2_import_get_boolean_status_res              (bool value){}
        virtual void on_fmi2_import_get_string_status_res               (string value){}


        // =========== FMI 2.0 (ME) Model Exchange functions ===========
        virtual void on_fmi2_import_enter_event_mode_res                (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_new_discrete_states_res             (fmitcp_proto::fmi2_event_info_t eventInfo){}
        virtual void on_fmi2_import_enter_continuous_time_mode_res      (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_instantiate_res                     (fmitcp_proto::jm_status_enu_t status){}
        virtual void on_fmi2_import_free_instance_res                   (){}
        virtual void on_fmi2_import_setup_experiment_res                (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_enter_initialization_mode_res       (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_exit_initialization_mode_res        (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_terminate_res                       (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_reset_res                           (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_time_res                        (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_continuous_states_res           (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_completed_integrator_step_res       (bool callEventUpdate, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_initialize_model_res                (bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_derivatives_res                 (const vector<double>& derivatives, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_event_indicators_res            (const vector<double>& eventIndicators, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_eventUpdate_res                     (bool iterationConverged, bool stateValueReferencesChanged, bool stateValuesChanged, bool terminateSimulation, bool upcomingTimeEvent, double nextEventTime, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_completed_event_iteration_res       (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_continuous_states_res           (const vector<double>& states, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_nominal_continuous_states_res   (const vector<double>& nominal, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_version_res                     (string version){}
        virtual void on_fmi2_import_set_debug_logging_res               (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_real_res                        (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_integer_res                     (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_boolean_res                     (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_string_res                      (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_get_fmu_state_res                   (int stateId, fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_set_fmu_state_res                   (fmitcp_proto::fmi2_status_t status){}
        virtual void on_fmi2_import_free_fmu_state_res                  (fmitcp_proto::fmi2_status_t status){}
        /*virtual void on_fmi2_import_serialized_fmu_state_size_res(){}
        virtual void on_fmi2_import_serialize_fmu_state_res(){}
        virtual void on_fmi2_import_de_serialize_fmu_state_res(){}
        */
        virtual void on_fmi2_import_get_directional_derivative_res(const vector<double>& dz, fmitcp_proto::fmi2_status_t status){}
        virtual void on_get_xml_res                                     (fmitcp_proto::jm_log_level_enu_t logLevel, string xml){}
    };

};

#endif
