#include <string>
#include <sstream>
#include <stdlib.h>
#include <fmitcp/Server.h>
#include <fmitcp/EventPump.h>
#include "master/Master.h"
#include "common/common.h"

using namespace fmitcp_master;

void printHelp(){
    printf("Test app. Will start a master and a number of slaves and try doing all kinds of coupling in between them.\n\
\n\
Usage: ./test [OPTIONS] [FMUPATH]\n\
\n\
[OPTIONS]\n\
    --host [HOST]   The host. Defaults to 'localhost'.\n\
    --port [PORT]   A free port to use. Default is 3000.\n\
\n\
FMUPATH\n\
    Path to an FMU to run. If \"dummy\" is given, the servers will always respond with dummy data and won't load any FMU.\n\n");
}

int main(int argc, char *argv[] ) {

    printf("FMI Master tester. Will start a TCP client and connect to an active slave server.\n");

    long port = 3000;
    int j;
    bool debugLogging = false;
    jm_log_level_enu_t log_level = jm_log_level_fatal;
    int logging = jm_log_level_fatal;
    string hostName = "localhost",
        fmuPath = "";

    fmitcp::EventPump pump;
    fmitcp::Logger logger;
    Master master(logger,&pump);
    master.setTimeStep(0.1);
    master.setEnableEndTime(true);
    master.setEndTime(0.1);
    master.setWeakMethod(PARALLEL);

    for (j = 1; j < argc; j++) {
        std::string arg = argv[j];
        bool last = (j == argc-1);

        if (arg == "-h" || arg == "--help") {
            printHelp();
            return EXIT_SUCCESS;

        } else if (arg == "-d" || arg == "--debugLogging") {
          debugLogging = true;

        } else if ((arg == "-l" || arg == "--logging") && !last) {
          std::string nextArg = argv[j+1];
          j++;

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
            j++;

            port = string_to_int(nextArg);

            if (port <= 0) {
                printf("Invalid port.\n");
                return EXIT_FAILURE;
            }

        } else if (arg == "--host" && !last) {
            hostName = argv[j+1];
            j++;

        } else {
          fmuPath = argv[j];
        }
    }

    if(fmuPath == "") {
        // Use dummy
        fmuPath = "dummy";
    }

    // Create a slave
    fmitcp::Server serverA(fmuPath, debugLogging, log_level, master.getEventPump());
    fmitcp::Server serverB(fmuPath, debugLogging, log_level, master.getEventPump());
    if (!serverA.isFmuParsed() || !serverB.isFmuParsed())
        return EXIT_FAILURE;
    serverA.getLogger()->setPrefix("slaveA: ");
    serverB.getLogger()->setPrefix("SlaveB: ");
    serverA.host(hostName,port);
    serverB.host(hostName,port+1);
    master.getLogger()->setPrefix("Master:   ");

    // Create ports
    ostringstream portStringStream1; portStringStream1 << port;
    ostringstream portStringStream2; portStringStream2 << (port+1);

    // Connect
    FMIClient* slaveA = master.connectSlave("tcp://"+hostName+":"+portStringStream1.str());
    FMIClient* slaveB = master.connectSlave("tcp://"+hostName+":"+portStringStream2.str());

    slaveA->getLogger()->setPrefix("Master "+int_to_string(slaveA->getId())+": ");
    slaveB->getLogger()->setPrefix("Master "+int_to_string(slaveB->getId())+": ");

    // Create connectors
    StrongConnector* connA = slaveA->createConnector();
    StrongConnector* connB = slaveB->createConnector();

    // We must specify what value refs that correspond to each connector
    connA->setPositionValueRefs(0,1,2);
    connA->setQuaternionValueRefs(3,4,5,6);
    connA->setVelocityValueRefs(7,8,9);
    connA->setAngularVelocityValueRefs(10,11,12);
    connA->setForceValueRefs(13,14,15);
    connA->setTorqueValueRefs(16,17,18);

    connB->setPositionValueRefs(0,1,2);
    connB->setQuaternionValueRefs(3,4,5,6);
    connB->setVelocityValueRefs(7,8,9);
    connB->setAngularVelocityValueRefs(10,11,12);
    connB->setForceValueRefs(13,14,15);
    connB->setTorqueValueRefs(16,17,18);

    // Create strong connections
    StrongConnection sconn(connA,connB,StrongConnection::CONNECTION_LOCK);
    master.addStrongConnection(&sconn);

    // Test weak connection
    master.createWeakConnection(slaveA, slaveB, 0, 0);

    // Needed to fix lacewing bug on unix. Try commenting/uncommenting if communication get stuck.
    fflush(NULL); fflush(NULL);

    // Start simulation
    master.simulate();

    return 0;
}
