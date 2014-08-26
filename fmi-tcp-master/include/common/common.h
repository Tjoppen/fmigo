#ifndef MASTER_COMMON_H_
#define MASTER_COMMON_H_

#define FMITCPMASTER_VERSION "0.0.1"

#include <string>
#include <vector>
#include <deque>

#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include "fmitcp/fmitcp.pb.h"

namespace fmitcp_master {

    using namespace std;

    std::deque<string> split(const std::string &s, char delim);
    int string_to_int(const string& s);
    string int_to_string(int i);

    jm_log_level_enu_t protoJMLogLevelToFmiJMLogLevel(fmitcp_proto::jm_log_level_enu_t logLevel);

    class WeakConnection;
    class FMIClient;
    std::map<FMIClient*, std::vector<int> > getOutputWeakRefs(std::vector<WeakConnection*> weakConnections);
    std::map<FMIClient*, std::pair<std::vector<int>, std::vector<double> > > getInputWeakRefsAndValues(std::vector<WeakConnection*> weakConnections);
}

#endif
