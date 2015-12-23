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
#ifdef USE_LACEWING
  FMIServer(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, EventPump* pump)
   : Server(fmuPath, debugLogging, logLevel, pump) {}
#else
  FMIServer(string fmuPath, bool debugLogging, jm_log_level_enu_t logLevel, string hdf5Filename)
   : Server(fmuPath, debugLogging, logLevel, hdf5Filename) {}
#endif
  ~FMIServer() {};
  void onClientConnect() {
    printf("MyFMIServer::onConnect\n");
    //m_pump->exitEventLoop();
  };

  void onClientDisconnect() {
    printf("MyFMIServer::onDisconnect\n");
#ifdef USE_LACEWING
    m_pump->exitEventLoop();
#endif
  };

  void onError(string message) {
    printf("MyFMIServer::onError\n");fflush(NULL);
#ifdef USE_LACEWING
    m_pump->exitEventLoop();
#endif
  };
};

#endif	/* FMI_SERVER_H */

