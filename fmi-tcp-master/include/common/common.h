#ifndef MASTER_COMMON_H_
#define MASTER_COMMON_H_

#define FMITCPMASTER_VERSION "0.0.1"

#include <string>
#include <vector>

#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include "fmitcp/fmitcp.pb.h"

namespace fmitcp_master {

    using namespace std;

    vector<string> &split(const string &s, char delim, vector<string> &elems);
    vector<string> split(const string &s, char delim);
    int string_to_int(const string& s);
    string int_to_string(int i);

    jm_log_level_enu_t protoJMLogLevelToFmiJMLogLevel(fmitcp_proto::jm_log_level_enu_t logLevel);
}

#endif
