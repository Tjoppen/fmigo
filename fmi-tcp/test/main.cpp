#include <fmilib.h>
#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <fmitcp/Server.h>
#include <fmitcp/Client.h>
#include <fmitcp/common.h>
#include <assert.h>

using namespace fmitcp;

/// Sends all possible network messages to see that everything is working OK
class TestClient : public Client {

private:
    int m_message_id;

    void assertMessageId(int message_id){
        assert(message_id == m_message_id-1);
    }
    int messageId(){
        return m_message_id++;
    }

public:
    TestClient(EventPump* pump) : Client(pump) {
        m_message_id = 1;
    };
    ~TestClient(){};

    void onConnect() {
      fmi2_import_instantiate(messageId());
    };

    void on_fmi2_import_instantiate_res(int message_id, fmitcp_proto::jm_status_enu_t status) {
      assertMessageId(message_id);
      double relTol = 0.0001,
          tStart = 0,
          tStop = 10;
      bool StopTimeDefined = true;
      fmi2_import_initialize_slave(messageId(), 0, true, relTol, tStart, StopTimeDefined, tStop);
    }

    void on_fmi2_import_initialize_slave_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        fmi2_import_terminate_slave(messageId(), 0);
    }

    void on_fmi2_import_terminate_slave_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        fmi2_import_reset_slave(messageId(), 0);
    }

    void on_fmi2_import_reset_slave_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        fmi2_import_free_slave_instance(messageId(),0);
    }

    void on_fmi2_import_free_slave_instance_res(int message_id){
        assertMessageId(message_id);
        std::vector<int> valueRefs;
        std::vector<int> orders;
        std::vector<double> values;
        fmi2_import_set_real_input_derivatives(messageId(),0,valueRefs,orders,values);
    }

    void on_fmi2_import_set_real_input_derivatives_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        std::vector<int> valueRefs;
        std::vector<int> orders;
        fmi2_import_get_real_output_derivatives(messageId(),0,valueRefs,orders);
    }

    void on_fmi2_import_get_real_output_derivatives_res(int message_id, fmitcp_proto::fmi2_status_t status, const vector<double>& values){
        assertMessageId(message_id);
        fmi2_import_cancel_step(messageId(),0);
    }

    void on_fmi2_import_cancel_step_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        fmi2_import_do_step(messageId(),0,0.0,0.1,true);
    }

    void on_fmi2_import_do_step_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        fmi2_import_get_status(messageId(), 0, fmitcp_proto::fmi2_do_step_status);
    }

    void on_fmi2_import_get_status_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        fmi2_import_get_real_status(messageId(), 0, fmitcp_proto::fmi2_do_step_status);
    }
    void on_fmi2_import_get_real_status_res(int message_id, double value){
        assertMessageId(message_id);
        fmi2_import_get_integer_status(messageId(), 0, fmitcp_proto::fmi2_do_step_status);
    }
    void on_fmi2_import_get_integer_status_res(int message_id, int value){
        assertMessageId(message_id);
        fmi2_import_get_boolean_status(messageId(), 0, fmitcp_proto::fmi2_do_step_status);
    }
    void on_fmi2_import_get_boolean_status_res(int message_id, bool value){
        assertMessageId(message_id);
        fmi2_import_get_string_status(messageId(), 0, fmitcp_proto::fmi2_do_step_status);
    }
    void on_fmi2_import_get_string_status_res(int message_id, string value){
        assertMessageId(message_id);
        fmi2_import_get_version(messageId(), 0);
    }

    /*

    TODOOOOO:

    // =========== FMI 2.0 (ME) Model Exchange functions ===========
    void on_fmi2_import_instantiate_model_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_free_model_instance_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_set_time_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_set_continuous_states_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_completed_integrator_step_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_initialize_model_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_get_derivatives_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_get_event_indicators_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_eventUpdate_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_completed_event_iteration_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_get_continuous_states_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_get_nominal_continuous_states_res(){
        m_pump->exitEventLoop();
    }
    void on_fmi2_import_terminate_res(){
        m_pump->exitEventLoop();
    }
    */

    // ========= FMI 2.0 CS & ME COMMON FUNCTIONS ============
    void on_fmi2_import_get_version_res(int message_id, string version){
        assertMessageId(message_id);
        std::vector<string> categories;
        fmi2_import_set_debug_logging(messageId(), 0, true, categories);
    }

    void on_fmi2_import_set_debug_logging_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        std::vector<int> valueRefs;
        std::vector<double> values;
        fmi2_import_set_real(messageId(), 0, valueRefs, values);
    }

    /*
        void fmi2_import_get_integer(int message_id, int fmuId, const vector<int>& valueRefs);
        void fmi2_import_get_boolean(int message_id, int fmuId, const vector<int>& valueRefs);
        void fmi2_import_get_string (int message_id, int fmuId, const vector<int>& valueRefs);
        void fmi2_import_get_fmu_state();
        void fmi2_import_set_fmu_state();
        void fmi2_import_free_fmu_state();
        void fmi2_import_serialized_fmu_state_size();
        void fmi2_import_serialize_fmu_state();
        void fmi2_import_de_serialize_fmu_state();
        void fmi2_import_get_directional_derivative(int message_id, int fmuId, const vector<int>& v_ref, const vector<int>& z_ref, const vector<double>& dv);
*/



    void on_fmi2_import_set_real_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        std::vector<int> valueRefs;
        std::vector<int> values;
        fmi2_import_set_integer(messageId(), 0, valueRefs, values);
    }

    void on_fmi2_import_set_integer_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        std::vector<int> valueRefs;
        std::vector<bool> values;
        fmi2_import_set_boolean(messageId(), 0, valueRefs, values);
    }

    void on_fmi2_import_set_boolean_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        std::vector<int> valueRefs;
        std::vector<string> values;
        fmi2_import_set_string(messageId(), 0, valueRefs, values);
    }

    void on_fmi2_import_set_string_res(int message_id, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        std::vector<int> valueRefs;
        fmi2_import_get_real(messageId(), 0, valueRefs);
    }

    void on_fmi2_import_get_real_res(int message_id, const vector<double>& values, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        std::vector<int> valueRefs;
        fmi2_import_get_integer(messageId(), 0, valueRefs);
    }

    void on_fmi2_import_get_integer_res(int message_id, const vector<int>& values, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        std::vector<int> valueRefs;
        fmi2_import_get_boolean(messageId(), 0, valueRefs);
    }

    void on_fmi2_import_get_boolean_res(int message_id, const vector<bool>& values, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        std::vector<int> valueRefs;
        fmi2_import_get_string(messageId(), 0, valueRefs);
    }

    void on_fmi2_import_get_string_res(int message_id, const vector<string>& values, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        std::vector<int> v_ref;
        std::vector<int> z_ref;
        std::vector<double> dv;
        fmi2_import_get_directional_derivative(messageId(), 0, v_ref, z_ref, dv);
    }

    /*
    void on_fmi2_import_get_fmu_state_res(int message_id, int stateId){
        m_pump->exitEventLoop();
    }

    void on_fmi2_import_set_fmu_state_res(int message_id, fmitcp_proto::fmi2_status_t status){
        m_pump->exitEventLoop();
    }

    void on_fmi2_import_free_fmu_state_res(int message_id, fmitcp_proto::fmi2_status_t status){
        m_pump->exitEventLoop();
    }
    */

    void on_fmi2_import_get_directional_derivative_res(int message_id, const vector<double>& dz, fmitcp_proto::fmi2_status_t status){
        assertMessageId(message_id);
        get_xml(messageId(),0);
    }

    void onGetXmlRes(int message_id, string xml){
        assertMessageId(message_id);
        m_pump->exitEventLoop();
    };

    void onDisconnect(){
        m_logger.log(Logger::LOG_DEBUG,"TestClient::onDisconnect\n");
        m_pump->exitEventLoop();
    };

    void onError(string err){
        m_logger.log(Logger::LOG_DEBUG,"TestClient::onError\n");
        m_pump->exitEventLoop();
    };

};

void printHelp(){
    printf("HELP PAGE: TODO\n");//fflush(NULL);
}

int main(int argc, char const *argv[]){

    // Flush all
    fflush(NULL);

    // Defaults
    string hostName = "localhost";
    long port = 3123;

    int j;
    for (j = 1; j < argc; j++) {
        std::string arg = argv[j];
        bool last = (j==argc-1);
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return EXIT_SUCCESS;

        } else if((arg == "--port" || arg == "-p") && !last) {
            std::string nextArg = argv[j+1];

            std::istringstream ss(nextArg);
            ss >> port;
            //fflush(NULL);
            if (port <= 0) {
                printf("Invalid port.\n");//fflush(NULL);
                return EXIT_FAILURE;
            }

        } else if (arg == "--host" && !last) {
            hostName = argv[j+1];

        }
    }

    printf("%s\n",lw_version());

    EventPump pump;

    Server server("", false, jm_log_level_all, &pump);
    server.sendDummyResponses(true);
    server.host(hostName,port);
    server.getLogger()->setPrefix("Server: ");

    TestClient client(&pump);
    client.getLogger()->setPrefix("Client:        ");
    client.connect(hostName,port);

    pump.startEventLoop();

    return 0;
}
