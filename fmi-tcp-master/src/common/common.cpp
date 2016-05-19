#include "common/common.h"
#include "stdlib.h"
#include <vector>
#include <string>
#include <sstream>
#include "master/FMIClient.h"

using namespace fmitcp_master;
using namespace std;

deque<string> fmitcp_master::split(const string &s, char delim) {
    deque<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
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

map<FMIClient*, vector<int> > fmitcp_master::getOutputWeakRefs(vector<WeakConnection> weakConnections) {
    map<FMIClient*, vector<int> > weakRefs;

    for (size_t x = 0; x < weakConnections.size(); x++) {
        WeakConnection wc = weakConnections[x];
        weakRefs[wc.from].push_back(wc.conn.fromOutputVR);
    }

    return weakRefs;
}

map<FMIClient*, pair<vector<int>, vector<double> > > fmitcp_master::getInputWeakRefsAndValues(vector<WeakConnection> weakConnections) {
    map<FMIClient*, pair<vector<int>, vector<double> > > refValues; //VRs and corresponding values for each client
    map<FMIClient*, size_t> realValueOfs;                           //for keeping track of where we are in each FMIClient->m_getRealValues

    for (size_t x = 0; x < weakConnections.size(); x++) {
        WeakConnection wc = weakConnections[x];
        size_t ofs = realValueOfs[wc.from];

        if (ofs >= wc.from->m_getRealValues.size()) {
            //probably didn't call get_real() on this client yet - skip value and trust that the user knows what they're doing
            continue;
        }

        double value = wc.from->m_getRealValues[ofs];

        refValues[wc.to].first.push_back(wc.conn.toInputVR);
        refValues[wc.to].second.push_back(value);
        realValueOfs[wc.from]++;
    }

    return refValues;
}
