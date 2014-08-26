#include "common/common.h"
#include "stdlib.h"
#include <vector>
#include <string>
#include <sstream>

using namespace fmitcp_master;
using namespace std;

vector<string>& fmitcp_master::split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

vector<string> fmitcp_master::split(const string &s, char delim) {
    vector<string> elems;
    fmitcp_master::split(s, delim, elems);
    return elems;
}

int fmitcp_master::string_to_int(const string& s){
    int result;
    std::istringstream ss(s);
    ss >> result;
    return result;
}

string fmitcp_master::int_to_string(int i){
    ostringstream ss;
    ss << i;
    return ss.str();
}

jm_log_level_enu_t fmitcp_master::protoJMLogLevelToFmiJMLogLevel(fmitcp_proto::jm_log_level_enu_t logLevel) {
  switch (logLevel) {
  case fmitcp_proto::jm_log_level_nothing:
    return jm_log_level_nothing;
  case fmitcp_proto::jm_log_level_fatal:
    return jm_log_level_fatal;
  case fmitcp_proto::jm_log_level_error:
    return jm_log_level_error;
  case fmitcp_proto::jm_log_level_warning:
    return jm_log_level_warning;
  case fmitcp_proto::jm_log_level_info:
    return jm_log_level_info;
  case fmitcp_proto::jm_log_level_verbose:
    return jm_log_level_verbose;
  case fmitcp_proto::jm_log_level_debug:
    return jm_log_level_debug;
  case fmitcp_proto::jm_log_level_all:
    return jm_log_level_all;
  default: // should never be reached
    return jm_log_level_nothing;
  }
}
