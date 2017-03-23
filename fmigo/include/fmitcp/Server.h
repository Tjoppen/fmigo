#ifndef SERVER_H_
#define SERVER_H_

#include <string>
#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include "fmitcp.pb.h"
#include "common.h"
#include <map>
#include <list>
#include "common/common.h"

using namespace std;

namespace fmitcp {

  /// Serves an FMU to a port via FMI/TCP.
  class Server {

  private:
    bool m_sendDummyResponses;
    bool m_fmuParsed;
    ::google::protobuf::int32 nextStateId;
    std::map<::google::protobuf::int32, fmi2_FMU_state_t> stateMap;

    //for computeNumericalJacobian
    double currentCommunicationPoint, communicationStepSize;

    void setStartValues();
  protected:
    string m_fmuPath;

    /// FMU logging level
    jm_log_level_enu_t m_logLevel;
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
    std::string m_fmuLocation;
    std::string m_resourcePath;

    //HDF5 output
    std::string hdf5Filename;
    std::vector<size_t> field_offset;
    std::vector<hid_t> field_types;
    std::vector<const char*> field_names;
    std::vector<char> hdf5data; //this should be a map<int,vector<char>> once we start getting into having multiple FMU instances
    size_t rowsz, nrecords;

  public:

    Server(string fmuPath, jm_log_level_enu_t logLevel, std::string hdf5Filename = "");
    virtual ~Server();

    /// To be implemented in subclass
    virtual void onClientConnect(){};

    /// To be implemented in subclass
    virtual void onClientDisconnect(){};

    /// To be implemented in subclass
    virtual void onError(string message){};

    //returns reply as std::string
    //if the string is empty then there was some kind of problem
    std::string clientData(const char *data, size_t size);

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

    std::vector<fmi2_real_t> computeNumericalJacobian(
        const std::vector<fmi2_value_reference_t>& z_ref,
        const std::vector<fmi2_value_reference_t>& v_ref,
        const std::vector<fmi2_real_t>& dv);
  };

};

#endif
