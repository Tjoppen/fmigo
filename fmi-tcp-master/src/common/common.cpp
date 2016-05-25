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

OutputRefsType fmitcp_master::getOutputWeakRefs(vector<WeakConnection> weakConnections) {
    OutputRefsType weakRefs;

    for (size_t x = 0; x < weakConnections.size(); x++) {
        WeakConnection wc = weakConnections[x];
        weakRefs[wc.from][wc.conn.fromType].push_back(wc.conn.fromOutputVR);
    }

    return weakRefs;
}

template<typename T> void doit(
        InputRefsValuesType& refValues,
        WeakConnection& wc,
        map<FMIClient*, size_t>& valueOfs,
        const vector<T>& values,
        MultiValue (WeakConnection::*convert)(T) const)
{
    size_t ofs = valueOfs[wc.from];

    if (ofs >= values.size()) {
        //shouldn't happen
        fprintf(stderr, "Number of setX() doesn't match number of getX() (%zu vs %zu) '%s'\n", ofs, values.size(), typeid(T).name());
        exit(1);
    }

    MultiValue value = (wc.*convert)(values[ofs]);

    refValues[wc.to][wc.conn.toType].first.push_back(wc.conn.toInputVR);
    refValues[wc.to][wc.conn.toType].second.push_back(value);
    valueOfs[wc.from]++;

}

static InputRefsValuesType getInputWeakRefsAndValues_internal(vector<WeakConnection> weakConnections, FMIClient *toClient) {
    InputRefsValuesType refValues; //VRs and corresponding values for each client

    //for keeping track of where we are in each FMIClient->m_getXValues
    map<FMIClient*, size_t> realValueOfs;
    map<FMIClient*, size_t> integerValueOfs;
    map<FMIClient*, size_t> booleanValueOfs;
    map<FMIClient*, size_t> stringValueOfs;

    for (size_t x = 0; x < weakConnections.size(); x++) {
        WeakConnection& wc = weakConnections[x];

        if (toClient && wc.to != toClient) {
            //skip if we only want for a specific client
            continue;
        }

        switch (wc.conn.fromType) {
        case fmi2_base_type_real:
            doit(refValues, wc, realValueOfs,    wc.from->m_getRealValues,    &WeakConnection::setFromReal);
            break;
        case fmi2_base_type_int:
            doit(refValues, wc, integerValueOfs, wc.from->m_getIntegerValues, &WeakConnection::setFromInteger);
            break;
        case fmi2_base_type_bool:
            doit(refValues, wc, booleanValueOfs, wc.from->m_getBooleanValues, &WeakConnection::setFromBoolean);
            break;
        case fmi2_base_type_str:
            doit(refValues, wc, stringValueOfs,  wc.from->m_getStringValues,  &WeakConnection::setFromString);
            break;
        }
    }

    return refValues;
}

InputRefsValuesType fmitcp_master::getInputWeakRefsAndValues(vector<WeakConnection> weakConnections) {
    return getInputWeakRefsAndValues_internal(weakConnections, NULL);
}

SendSetXType fmitcp_master::getInputWeakRefsAndValues(vector<WeakConnection> weakConnections, FMIClient *client) {
    InputRefsValuesType temp = getInputWeakRefsAndValues_internal(weakConnections, client);
    auto it = temp.find(client);
    if (it == temp.end()) {
        return SendSetXType();
    } else {
        return it->second;
    }
}
