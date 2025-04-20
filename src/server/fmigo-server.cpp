#include "common/common.h"
#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <fmitcp/Server.h>
#include <fmitcp/fmitcp-common.h>
#include <zmq.hpp>
#include "server/FMIServer.h"
#include <thread>

using namespace std;
using namespace fmitcp;

#include "parse_server_args.cpp"

jm_log_level_enu_t fmigo_loglevel = jm_log_level_warning;
bool alwaysComputeNumericalDirectionalDerivatives = false;

static void handleMessage(zmq::socket_t& socket, FMIServer& server, int port) {
  zmq::message_t msg;
  if (!socket.recv(&msg)) {
      fatal("Port %i: !socket.recv(&msg)\n", port);
  }
  server.m_timer.rotate("recv");
#if CLIENTDATA_NEW == 0
  string str = server.clientData(static_cast<char*>(msg.data()), msg.size());

  if (str.length() > 0) {
    zmq::message_t rep(str.length());
    memcpy(rep.data(), str.data(), str.length());
    server.m_timer.rotate("pre_send");
    socket.send(rep, ZMQ_DONTWAIT);
    server.m_timer.rotate("send");
  }
#else
  const vector<char>& str = server.clientData(static_cast<char*>(msg.data()), msg.size());

  if (str.size() > 0) {
    zmq::message_t rep(str.size());
    memcpy(rep.data(), &str[0], str.size());
    server.m_timer.rotate("pre_send");
    socket.send(rep, ZMQ_DONTWAIT);
    server.m_timer.rotate("send");
  }
#endif
}

int main(int argc, char *argv[]) {
 try {
  zmq::context_t context(1);

  int port = 3000;
  bool debugLogging = false;
  string fmuPath = "";
  string hdf5Filename;

  parse_server_args(argc, argv, &fmuPath, &hdf5Filename, &debugLogging, &fmigo_loglevel, &port);

  FMIServer server(fmuPath, port, hdf5Filename);
  //HACKHACK: count waiting for the master to start toward "instantiate"
  server.m_timer.dont_rotate = true;
  if (!server.isFmuParsed())
    return EXIT_FAILURE;

  ostringstream oss;
  oss << "tcp://*:" << port;

  info("FMI Server %s - %s <-- %s\n",FMITCP_VERSION, oss.str().c_str(), fmuPath.c_str());

  zmq::socket_t socket(context, ZMQ_REP);
  socket.bind(oss.str().c_str());

  for (;;) {
    zmq::pollitem_t items[2];
    items[0].socket = (void*)socket;
    items[0].events = ZMQ_POLLIN;

    server.m_timer.rotate("pre_poll");
    //wait indefinitely, for now
    int n = zmq::poll(items, 1, -1);
    server.m_timer.rotate("poll");

    if (items[0].revents & ZMQ_POLLIN) {
      handleMessage(socket, server, port);
      if (server.m_freed) {
        break;
      }
    }
  }

  return EXIT_SUCCESS;
 } catch (zmq::error_t& e) {
      //catch any stray ZMQ exceptions
      //this should prevent "program stopped working" messages on Windows when fmigo-servers are taskkill'd
     error("zmq::error_t in %s: %s\n", argv[0], e.what());
     return 1;
 }
}
