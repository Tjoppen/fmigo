/*
 * File:   fmi-server.h
 * Author: thardin
 *
 * Created on December 23, 2015, 12:01 PM
 */

#ifndef FMI_SERVER_H
#define	FMI_SERVER_H

#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <fmitcp/Server.h>
#include <fmitcp/fmitcp-common.h>
#include <zmq.hpp>

// Define own server
class FMIServer : public fmitcp::Server {
public:
  FMIServer(string fmuPath, int rank_or_port, string hdf5Filename)
   : Server(fmuPath, rank_or_port, hdf5Filename) {}
  ~FMIServer() {};
  void onClientConnect() {
    //m_pump->exitEventLoop();
  };

  void onClientDisconnect() {
  };

  void onError(string message) {
  };
};

#endif	/* FMI_SERVER_H */
