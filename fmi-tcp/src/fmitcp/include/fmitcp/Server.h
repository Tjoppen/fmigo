#ifndef SERVER_H_
#define SERVER_H_

#include <string>
#ifdef USE_LACEWING
#define lw_import
#include <lacewing.h>
#endif
#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#ifdef USE_LACEWING
#include "EventPump.h"
#endif
#include "Logger.h"
#include "fmitcp.pb.h"

using namespace std;

namespace fmitcp {

  /// Serves an FMU to a port via FMI/TCP.
  class Server {

  private:
    Logger m_logger;
#ifdef USE_LACEWING
    lw_server m_server;
#endif
    bool m_sendDummyResponses;
    bool m_fmuParsed;
#ifdef USE_LACEWING
    std::string tail;   //leftover data from last clientData() call
#endif

  protected:
#ifdef USE_LACEWING
    EventPump * m_pump;
#endif
    string m_fmuPath;

    /// FMU logging level
    jm_log_level_enu_t m_logLevel;
    bool m_debugLogging;
    jm_callbacks m_jmCallbacks;

    /// Directory for the unpacked FMU
    string m_workingDir;
    fmi_import_context_t* m_context;
    fmi_version_enu_t m_version;

    /// FMI 2.0 instance
    fmi2_import_t* m_fmi2Instance;
    fmi2_callback_functions_t m_fmi2CallbackFunctions;
    fmi2_import_variable_list_t* m_fmi2Variables;

    const char* m_instanceName;
    char* m_fmuLocation;
    char* m_resourcePath;

  public:

#ifdef USE_LACEWING
    /// Create a server for an FMU using an eventpump
    Server(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, EventPump *pump);
    Server(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, EventPump *pump, const Logger &logger);
#else
    Server(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel);
    Server(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, const Logger &logger);
#endif
    virtual ~Server();

#ifdef USE_LACEWING
    void init(EventPump *pump);
#else
    void init();
#endif

    /// To be implemented in subclass
    virtual void onClientConnect(){};

    /// To be implemented in subclass
    virtual void onClientDisconnect(){};

    /// To be implemented in subclass
    virtual void onError(string message){};

#ifdef USE_LACEWING
    void clientConnected(lw_client c);
    void clientDisconnected(lw_client c);
    void clientData(lw_client c, const char *data, size_t size);
    void error(lw_server s, lw_error err);
#else
    //returns reply as std::string
    //if the string is empty then there was some kind of problem
    std::string clientData(const char *data, size_t size);
#endif

#ifdef USE_LACEWING
    /// Start hosting on a port.
    void host(string host, long port);
#endif

    /// Set to true to start ignoring the local FMU and just send back dummy responses. Good for debugging the protocol.
    void sendDummyResponses(bool);

    Logger* getLogger() {return &m_logger;}
    void setLogger(const Logger &logger) {m_logger = logger;}

    bool isFmuParsed() {return m_fmuParsed;}

    /// Check if the fmi2 status is ok or warning
    bool fmi2StatusOkOrWarning(fmi2_status_t fmistatus) {
      return (fmistatus == fmi2_status_ok) || (fmistatus == fmi2_status_warning);
    }
  };

};

#endif
