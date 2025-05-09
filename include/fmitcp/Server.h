#ifndef SERVER_H_
#define SERVER_H_

#include <string>
#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include "fmitcp.pb.h"
#include "fmitcp-common.h"
#include <map>
#include <list>
#include "common/common.h"
#include "common/timer.h"

using namespace std;

namespace fmitcp {

  /// Serves an FMU to a port via FMI/TCP.
  class Server {

  public:
    //timing stuff
    fmigo::timer m_timer;

  private:
    bool m_sendDummyResponses;
    bool m_fmuParsed;
    ::google::protobuf::int32 lastStateId, nextStateId;
    std::map<::google::protobuf::int32, fmi2_FMU_state_t> stateMap;

    //future value of currentCommunicationPoint, and a guess for future communicationStepSize
    //for computeNumericalDirectionalDerivative
    double currentCommunicationPoint, communicationStepSize;

    void setStartValues();

    fmi2_status_t getDirectionalDerivatives(
        const fmitcp_proto::fmi2_import_get_directional_derivative_req& r,
        fmitcp_proto::fmi2_import_get_directional_derivative_res& response);

    //the purpose of this vector is to minimize the amount of allocations that need to happen
    //during every call to clientData()
    vector<char> responseBuffer;

  protected:
    string m_fmuPath;

    /// FMU logging level
    jm_callbacks m_jmCallbacks;

    /// Directory for the unpacked FMU
    string m_workingDir;
    fmi_import_context_t* m_context;
    fmi_version_enu_t m_version;

    /// FMI 2.0 instance
    fmi2_import_t* m_fmi2Instance;
    fmi2_callback_functions_t m_fmi2CallbackFunctions;
    fmi2_import_variable_list_t *m_fmi2Variables, *m_fmi2Outputs;

    const char* m_instanceName;
    std::string m_resourcePath;

    //HDF5 output
    std::string hdf5Filename;
    std::vector<size_t> field_offset;
    std::vector<hid_t> field_types;
    std::vector<const char*> field_names;
    std::vector<char> hdf5data; //this should be a map<int,vector<char>> once we start getting into having multiple FMU instances
    size_t rowsz, nrecords;

private:
    //avoid allocation
    std::pair<fmitcp_proto::fmitcp_message_Type,std::string> ret;

#ifdef ENABLE_HDF5_HACK
    std::vector<std::string> columnnames;
#endif

public:
#if SERVER_CLIENTDATA_NO_STRING_RET == 1
    //packs data into responseBuffer
    void clientDataInner(const char *data, size_t size);
#else
    std::string clientDataInner(const char *data, size_t size);
#endif

    // set to true when fmi2_import_free_instance() is called
    // this serves as the signal to stop the server
    bool m_freed;

  public:

    explicit Server(string fmuPath, int rank_or_port, std::string hdf5Filename = "");
    virtual ~Server();

    /// To be implemented in subclass
    virtual void onClientConnect(){};

    /// To be implemented in subclass
    virtual void onClientDisconnect(){};

    /// To be implemented in subclass
    virtual void onError(string message){};

#if CLIENTDATA_NEW == 0
    //returns reply as std::string
    //if the string is empty then there was some kind of problem
    std::string clientData(const char *data, size_t size);
#else
    //if the char vector is empty then there was some kind of problem
    const vector<char>& clientData(const char *data, size_t size);
#endif

    /// Set to true to start ignoring the local FMU and just send back dummy responses. Good for debugging the protocol.
    void sendDummyResponses(bool);

    bool isFmuParsed() {return m_fmuParsed;}

    /// Check if the fmi2 status is ok or warning
    bool fmi2StatusOkOrWarning(fmi2_status_t fmistatus) {
      return (fmistatus == fmi2_status_ok) || (fmistatus == fmi2_status_warning);
    }

    void getHDF5Info();
    void fillHDF5Row(char *dest, double t);

    bool hasCapability(fmi2_capabilities_enu_t cap) const;

    std::vector<fmi2_real_t> computeNumericalDirectionalDerivative(
        const std::vector<fmi2_value_reference_t>& z_ref,
        const std::vector<fmi2_value_reference_t>& v_ref,
        const std::vector<fmi2_real_t>& dv);
  };

};

#endif
