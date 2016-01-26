#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <fmitcp/Server.h>
#include <fmitcp/common.h>
#include <zmq.hpp>
#include "server/FMIServer.h"

using namespace std;
using namespace fmitcp;

void printHelp() {
  printf("Usage\n\
\n\
fmi-tcp-server [OPTIONS] FMUPATH\n\
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
    fmi-tcp-server --host localhost --port 3000 mySim.fmu\n\n");fflush(NULL);
}

int main(int argc, char *argv[]) {
 try {
  zmq::context_t context(1);

  if (argc == 1) {
    // No args given, print help
    printHelp();
    return EXIT_SUCCESS;
  }

  int j;
  int port = 3000;
  bool debugLogging = false;
  //there does not appear to be any way to actually set the log level in an FMU
  //default to logging nothing, else every little thing gets logged
  jm_log_level_enu_t log_level = jm_log_level_nothing;
  string hostName = "localhost",
      fmuPath = "";
  string hdf5Filename;

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
      int logging;
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
    } else if (arg == "-5" && !last) {
      hdf5Filename = argv[++j];
    } else {
      fmuPath = argv[j];
    }
  }

  if(fmuPath == ""){
    printHelp();
    return EXIT_FAILURE;
  }

  FMIServer server(fmuPath, debugLogging, log_level, hdf5Filename);
  if (!server.isFmuParsed())
    return EXIT_FAILURE;

  printf("FMI Server %s - tcp://%s:%i <-- %s\n",FMITCP_VERSION, hostName.c_str(), port, fmuPath.c_str());
  server.getLogger()->setPrefix("Server: ");

  zmq::socket_t socket(context, ZMQ_PAIR);
  ostringstream oss;
  oss << "tcp://*:" << port;
  socket.bind(oss.str().c_str());
  
  for (;;) {
      zmq::message_t msg;
      if (!socket.recv(&msg)) {
          fprintf(stderr, "Port %i: !socket.recv(&msg)\n", port);
          exit(1);
      }
      string str = server.clientData(static_cast<char*>(msg.data()), msg.size());
      if (str.length() == 0) {
          fprintf(stderr, "Zero-length reply implies error - quitting\n");
          exit(1);
      }
      zmq::message_t rep(str.length());
      memcpy(rep.data(), str.data(), str.length());
      socket.send(rep);
  }

  return EXIT_SUCCESS;
 } catch (zmq::error_t e) {
      //catch any stray ZMQ exceptions
      //this should prevent "program stopped working" messages on Windows when fmi-tcp-servers are taskkill'd
      fprintf(stderr, "zmq::error_t: %s\n", e.what());
      return 1;
 }
}
