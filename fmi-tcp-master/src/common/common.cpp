#include "common/common.h"
#include "stdlib.h"
#include <vector>
#include <string>
#include <sstream>

using namespace std;

namespace common {

deque<string> split(const string &s, char delim) {
    deque<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

int string_to_int(const string& s){
    int result;
    std::istringstream ss(s);
    ss >> result;
    return result;
}

string int_to_string(int i){
    ostringstream ss;
    ss << i;
    return ss.str();
}

void extract_vector(double *outVec, std::deque<std::vector<double>> *inQue){
    if(inQue->size() == 0) return;
    int i = 0;
    for(auto v:(*inQue)[0]) 
      outVec[i] = v;
    inQue->pop_back();
} 
  
void extract_vector(std::vector<double> *outVec, std::deque<std::vector<double>> *inQue){
    if(inQue->size() == 0) return;
    outVec->clear();
    for(auto v:(*inQue)[0]) 
      outVec->push_back( v );
    inQue->pop_back();
} 

jm_log_level_enu_t protoJMLogLevelToFmiJMLogLevel(fmitcp_proto::jm_log_level_enu_t logLevel) {
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

jm_log_level_enu_t logOptionToJMLogLevel(const char* option) {
    string str(option);
    istringstream ss(str);
    int logging;
    ss >> logging;

    switch (logging) {
    case 0: return jm_log_level_nothing;
    case 1: return jm_log_level_fatal;
    case 2: return jm_log_level_error;
    case 3: return jm_log_level_warning;
    case 4: return jm_log_level_info;
    case 5: return jm_log_level_verbose;
    case 6: return jm_log_level_debug;
    case 7: return jm_log_level_all;
    default:
        fprintf(stderr, "Invalid logging (%s). Possible options are from 0 to 7.\n", option);
        exit(EXIT_FAILURE);
    }
}

}
