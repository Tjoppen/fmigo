#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <fmitcp/Server.h>
#include <fmitcp/common.h>

using namespace std;
using namespace fmitcp;

// Define own server
class FMIServer : public Server {
public:
  FMIServer(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, EventPump* pump)
   : Server(fmuPath, debugLogging, logLevel, pump) {}
  ~FMIServer() {};
  void onClientConnect() {
    printf("MyFMIServer::onConnect\n");
    //m_pump->exitEventLoop();
  };

  void onClientDisconnect() {
    printf("MyFMIServer::onDisconnect\n");
    m_pump->exitEventLoop();
  };

  void onError(string message) {
    printf("MyFMIServer::onError\n");fflush(NULL);
    m_pump->exitEventLoop();
  };
};

void printHelp() {
  printf("Usage\n\
\n\
slave [OPTIONS] FMUPATH\n\
\n\
OPTIONS\n\
\n\
    --port [INTEGER]\n\
        The port to run the server on. Default is 3000.\n\
\n\
    --host [STRING]\n\
        The host name to run the server on. Default is 'localhost'.\n\
\n\
    --help\n\
        You're looking at it.\n\
\n\
FMUPATH\n\
\n\
    The path to the FMU to serve. If FMUPATH is \"dummy\", then the server will always respond with dummy responses, which is nice for debugging.\n\
\n\
EXAMPLES\n\
\n\
    slave --host localhost --port 3000 mySim.fmu\n\n");fflush(NULL);
}

int main(int argc, char *argv[]) {

  printf("FMI Server %s\n",FMITCP_VERSION);fflush(NULL);

  if (argc == 1) {
    // No args given, print help
    printHelp();
    return EXIT_SUCCESS;
  }

  int j;
  long port = 3000;
  bool debugLogging = false;
  jm_log_level_enu_t log_level = jm_log_level_fatal;
  int logging = jm_log_level_fatal;
  string hostName = "localhost",
      fmuPath = "";

  for (j = 1; j < argc; j++) {
    string arg = argv[j];
    bool last = (j==argc-1);

    if (arg == "-h" || arg == "--help") {
      printHelp();
      return EXIT_SUCCESS;

    } else if (arg == "-d" || arg == "--debugLogging") {
      debugLogging = true;

    } else if (arg == "-v" || arg == "--version") {
      printf("%s\n",FMITCP_VERSION); // todo
      return EXIT_SUCCESS;

    } else if ((arg == "-l" || arg == "--logging") && !last) {
      std::string nextArg = argv[j+1];

      std::istringstream ss(nextArg);
      ss >> logging;

      if (logging < 0) {
        printf("Invalid logging. Possible options are from 0 to 7.\n");fflush(NULL);
        return EXIT_FAILURE;
      }

      switch (logging) {
      case 0:
        log_level = jm_log_level_nothing; break;
      case 1:
        log_level = jm_log_level_fatal; break;
      case 2:
        log_level = jm_log_level_error; break;
      case 3:
        log_level = jm_log_level_warning; break;
      case 4:
        log_level = jm_log_level_info; break;
      case 5:
        log_level = jm_log_level_verbose; break;
      case 6:
        log_level = jm_log_level_debug; break;
      case 7:
        log_level = jm_log_level_all; break;
      }

    } else if((arg == "--port" || arg == "-p") && !last) {
      std::string nextArg = argv[j+1];

      std::istringstream ss(nextArg);
      ss >> port;

      if (port <= 0) {
        printf("Invalid port.\n");fflush(NULL);
        return EXIT_FAILURE;
      }

    } else if (arg == "--host" && !last) {
      hostName = argv[j+1];

    } else {
      fmuPath = argv[j];
    }
  }

  if(fmuPath == ""){
    printHelp();
    return EXIT_FAILURE;
  }

  EventPump pump;
  FMIServer server(fmuPath, debugLogging, log_level, &pump);
  if (!server.isFmuParsed())
    return EXIT_FAILURE;

  server.getLogger()->setPrefix("Server: ");
  server.host(hostName, port);

  // If communication stops without reason, try removing one or both of these. This is due to a bug in the lacewing library?
  //fflush(NULL); fflush(NULL);

  //server.getLogger()->setFilter(Logger::LOG_NETWORK | Logger::LOG_DEBUG | Logger::LOG_ERROR);
  pump.startEventLoop();

  return EXIT_SUCCESS;
}
