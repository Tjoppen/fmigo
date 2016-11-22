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

#include "parse_server_args.cpp"

int main(int argc, char *argv[]) {
 try {
  zmq::context_t context(1);

  int port = 3000;
  bool debugLogging = false;
  jm_log_level_enu_t log_level = jm_log_level_error;
  string hostName = "localhost", fmuPath = "";
  string hdf5Filename;
  int filter_depth = 0;

  parse_server_args(argc, argv, &fmuPath, &filter_depth, &hdf5Filename, &debugLogging, &log_level, &hostName, &port);

  FMIServer server(fmuPath, debugLogging, log_level, hdf5Filename, filter_depth);
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
