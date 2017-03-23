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
#include <fmitcp/common.h>
#include <zmq.hpp>

// Define own server
class FMIServer : public fmitcp::Server {
public:
  FMIServer(string fmuPath, jm_log_level_enu_t logLevel, string hdf5Filename)
   : Server(fmuPath, logLevel, hdf5Filename) {}
  ~FMIServer() {};
  void onClientConnect() {
    printf("MyFMIServer::onConnect\n");
    //m_pump->exitEventLoop();
  };

  void onClientDisconnect() {
    printf("MyFMIServer::onDisconnect\n");
  };

  void onError(string message) {
    printf("MyFMIServer::onError\n");fflush(NULL);
  };
};

#endif	/* FMI_SERVER_H */
