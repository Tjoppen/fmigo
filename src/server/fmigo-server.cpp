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

class mymonitor : public zmq::monitor_t {
public:
  virtual void on_event_disconnected(const zmq_event_t &event_, const char* addr_) {
    //stop monitor when client disconnects, fall back into start_monitor()
    abort();
  }
};

//std::thread ctor can't pass socket_t&/context_t& into start_monitor() for some reason, resort to pointers
static void start_monitor(zmq::socket_t *socket, zmq::context_t *context) {
  mymonitor mon;

  //monitor until client disconnects
  mon.monitor(*socket, "inproc://monitor");

  //send stop message to main thread
  zmq::socket_t stopper2(*context, ZMQ_PAIR);
  stopper2.connect("inproc://stopper");
  zmq::message_t msg(0);    //zero-length message works well enough
  stopper2.send(msg);

  //at this point this thraed is going to be waiting for join()
}

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

  FMIServer server(fmuPath, hdf5Filename);
  //HACKHACK: count waiting for the master to start toward "instantiate"
  server.m_timer.dont_rotate = true;
  if (!server.isFmuParsed())
    return EXIT_FAILURE;

  ostringstream oss;
  oss << "tcp://*:" << port;

  info("FMI Server %s - %s <-- %s\n",FMITCP_VERSION, oss.str().c_str(), fmuPath.c_str());

  zmq::socket_t socket(context, ZMQ_REP);
  socket.bind(oss.str().c_str());

  //use monitor + inproc PAIR to stop when the client disconnects
  //maybe there's an easier way? the ZMQ documentation has this to say (http://zeromq.org/area:faq):
  //
  // How can I be notified that a peer has connected/disconnected from my socket?
  //
  // ZeroMQ sockets can bind and/or connect to multiple peers simultaneously.
  // The sockets also transparently provide asynchronous connection and
  // reconnection facilities. At this time, none of the sockets will provide
  // notification of peer connect/disconnect. This feature is being
  // investigated for a future release.
  //
  zmq::socket_t stopper(context, ZMQ_PAIR);
  stopper.bind("inproc://stopper");

  std::thread monitor_thread(start_monitor, &socket, &context);

  for (;;) {
    zmq::pollitem_t items[2];
    items[0].socket = (void*)socket;
    items[0].events = ZMQ_POLLIN;
    items[1].socket = (void*)stopper;
    items[1].events = ZMQ_POLLIN;

    server.m_timer.rotate("pre_poll");
    //wait indefinitely, for now
    int n = zmq::poll(items, 2, -1);
    server.m_timer.rotate("poll");

    if (items[0].revents & ZMQ_POLLIN) {
      handleMessage(socket, server, port);
    }

    if (items[1].revents & ZMQ_POLLIN) {
      //client disconnected - join monitor thread and stop
      monitor_thread.join();
      break;
    }
  }

  return EXIT_SUCCESS;
 } catch (zmq::error_t e) {
      //catch any stray ZMQ exceptions
      //this should prevent "program stopped working" messages on Windows when fmigo-servers are taskkill'd
     error("zmq::error_t in %s: %s\n", argv[0], e.what());
     return 1;
 }
}
