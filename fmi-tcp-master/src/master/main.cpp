#include <string>
#include <fmitcp/Client.h>
#include <fmitcp/common.h>
#include <fmitcp/Logger.h>
#include <fmitcp/EventPump.h>
#include <stdlib.h>
#include <signal.h>
#include <sstream>
#include "master/Master.h"
#include "master/FMIClient.h"
#include "common/common.h"

using namespace fmitcp_master;

void printHelp(){
    printf("Usage\n\
\n\
master [OPTIONS] [FMU_URLS...]\n\
\n\
OPTIONS\n\
\n\
    --timeStep [NUMBER]\n\
            Timestep size. Default is 0.1.\n\
\n\
    --stopAfter [NUMBER]\n\
        End simulation time in seconds. Default is 1.0.\n\
\n\
    --weakMethod [STRING]\n\
        Stepping  method for weak coupling connections. Must be \"parallel\" or \"serial\". Default is \"parallel\".\n\
\n\
    --weakConnections [STRING]\n\
        Connection  specification. No connections by default. Quadruples of\n\
        positive integers, representing which FMU and value reference to connect\n\
        from and what to connect to. Syntax is\n\
\n\
            CONN1:CONN2:CONN3...\n\
\n\
        where CONNX is four comma-separated integers FMUFROM,VRFROM,FMUTO,VRTO.\n\
        An example connection string is\n\
\n\
            0,0,1,0:0,1,1,1\n\
\n\
        which means: connect FMU0 (value reference 0) to FMU1 (vr 0) and FMU0\n\
        (vr 1) to FMU1 (vr 1).  Default is no  connections.\n\
\n\
    --strongConnections [STRING]\n\
        TODO\n\
\n\
    --help\n\
        You're looking at it.\n\
\n\
FMU_URLS\n\
\n\
    Urls to FMU servers, separated by spaces. For example \"tcp://fmiserver.com:1234\".\n\
\n\
EXAMPLES\n\
\n\
    master --weakConnections 0,0,0,0:0,0,0,0 tcp://localhost:3000\n\n");fflush(NULL);
}

int main(int argc, char *argv[] ) {

    printf("FMI Master %s\n",FMITCPMASTER_VERSION);fflush(NULL);

    fmitcp::Logger logger;
    fmitcp::EventPump pump;
    Master master(logger,&pump);
    master.setTimeStep(0.1);
    master.setEnableEndTime(false);
    master.setWeakMethod(PARALLEL);

    const char* connectionsArg;
    int i, j;

    // Connections
    vector<int> strong_slaveA;
    vector<int> strong_slaveB;
    vector<int> strong_connA;
    vector<int> strong_connB;

    vector<int> weak_slaveA;
    vector<int> weak_slaveB;
    vector<int> weak_connA;
    vector<int> weak_connB;

    vector<FMIClient*> slaves;

    for (j = 1; j < argc; j++) {
        std::string arg = argv[j];
        bool last = (j == argc-1);

        if (arg == "-h" || arg == "--help") {
            printHelp();
            return EXIT_SUCCESS;

        } else if (arg == "--timeStep" && !last) {
            std::string nextArg = argv[j+1];
            double timeStepSize = ::atof(nextArg.c_str());
            j++;

            if(timeStepSize <= 0){
                fprintf(stderr,"Invalid timeStepSize.");
                return EXIT_FAILURE;
            }

            master.setTimeStep(timeStepSize);

        } else if (arg == "--version") {
            printf("%s\n",FMITCPMASTER_VERSION);
            return EXIT_SUCCESS;

        } else if ((arg == "-t" || arg == "--stopAfter") && !last) {
            std::string nextArg = argv[j+1];
            double endTime = ::atof(nextArg.c_str());
            j++;

            if (endTime <= 0) {
                fprintf(stderr,"Invalid end time.");
                return EXIT_FAILURE;
            }

            master.setEnableEndTime(true);
            master.setEndTime(endTime);

        } else if ((arg == "-wm" || arg == "--weakMethod") && !last) {
            std::string nextArg = argv[j+1];
            j++;

            if (nextArg == "parallel") {
                master.setWeakMethod(PARALLEL);

            } else if (nextArg == "serial") {
                master.setWeakMethod(SERIAL);

            } else {
                fprintf(stderr,"Weak coupling method not recognized.\n");
                return EXIT_FAILURE;

            }

        } else if ((arg == "-wc" || arg == "--weakConnections") && !last) {
            std::string nextArg = argv[j+1];
            j++;

            // Get connections
            vector<string> conns = split(nextArg,':');
            for(i=0; i<conns.size(); i++){
                vector<string> quad = split(conns[i],',');
                weak_slaveA.push_back(string_to_int(quad[0]));
                weak_slaveB.push_back(string_to_int(quad[1]));
                weak_connA .push_back(string_to_int(quad[2]));
                weak_connB .push_back(string_to_int(quad[3]));
            }

        } else if ((arg == "-sc" || arg == "--strongConnections") && !last) {
            std::string nextArg = argv[j+1];
            j++;

            // Get connections
            vector<string> conns = split(nextArg,':');
            for(i=0; i<conns.size(); i++){
                vector<string> quad = split(conns[i],',');
                strong_slaveA.push_back(string_to_int(quad[0]));
                strong_slaveB.push_back(string_to_int(quad[1]));
                strong_connA .push_back(string_to_int(quad[2]));
                strong_connB .push_back(string_to_int(quad[3]));
            }

        } else if (arg ==  "--debug") {
            // debugFlag = 1;
            // Todo: set flag in logger

        } else {
            // Assume URI to slave
            slaves.push_back(master.connectSlave(arg));
        }
    }

    // Set connections
    // TODO : How should these connections be specified via command line???
    //for(i=0; i<strong_slaveA.size(); i++)
    //    master.createStrongConnection(slaves[strong_slaveA[i]], slaves[strong_slaveB[i]], strong_connA[i], strong_connB[i]);

    // Weak coupling
    for(i=0; i<weak_slaveA.size(); i++){
        printf("Creating weak connection %d %d %d %d\n",weak_slaveA[i],weak_slaveB[i],weak_connA[i],weak_connB[i]); fflush(NULL);
        master.createWeakConnection(slaves[weak_slaveA[i]], slaves[weak_slaveB[i]], weak_connA[i], weak_connB[i]);
    }

    master.simulate();

    return 0;
}
