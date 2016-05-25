#ifndef MASTER_COMMON_H_
#define MASTER_COMMON_H_

#define FMITCPMASTER_VERSION "0.0.1"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <string>
#include <vector>
#include <deque>

#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include "fmitcp.pb.h"
#include "master/WeakConnection.h"

#ifndef WIN32
//for HDF5 output
#include <hdf5.h>
#include <hdf5_hl.h>
#include <sys/time.h>
extern timeval tl1, tl2;
extern std::vector<int> timelog;
extern int columnofs;
extern std::map<int, const char*> columnnames;
#define MAX_TIME_COLS 20    //for estimating the size of timelog

//measures and logs elapsed time in microseconds
#define PRINT_HDF5_DELTA(label) do {\
        gettimeofday(&tl2, NULL);\
        int dt = 1000000*(tl2.tv_sec-tl1.tv_sec) + tl2.tv_usec-tl1.tv_usec;\
        timelog.push_back(dt);\
        columnnames[columnofs++] = label;\
        gettimeofday(&tl1, NULL);\
    } while (0)
#else
#define PRINT_HDF5_DELTA(label)
#endif

namespace fmitcp_master {

    using namespace std;

    std::deque<string> split(const std::string &s, char delim);
    int string_to_int(const string& s);
    string int_to_string(int i);

    jm_log_level_enu_t protoJMLogLevelToFmiJMLogLevel(fmitcp_proto::jm_log_level_enu_t logLevel);

    class FMIClient;

    //this type holds value references grouped by data type and client
    typedef map<fmi2_base_type_enu_t, vector<int> > SendGetXType;
    typedef map<FMIClient*, SendGetXType> OutputRefsType;
    OutputRefsType getOutputWeakRefs(std::vector<WeakConnection> weakConnections);

    //OK, this type is getting a bit complicated
    //it collects value references and their associated type converted values, grouped by data type and client
    typedef std::map<fmi2_base_type_enu_t, std::pair<std::vector<int>, std::vector<MultiValue> > > SendSetXType;
    typedef std::map<FMIClient*, SendSetXType> InputRefsValuesType;
    InputRefsValuesType getInputWeakRefsAndValues(std::vector<WeakConnection> weakConnections);
    SendSetXType        getInputWeakRefsAndValues(std::vector<WeakConnection> weakConnections, FMIClient *client);
}

#endif
