#include "common/common.h"
#include "stdlib.h"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

void info(const char* fmt, ...){
    if (fmigo_loglevel >= ::jm_log_level_info) {
        va_list argptr;
        va_start(argptr, fmt);
        vfprintf(stderr, fmt, argptr);
        va_end(argptr);
    }
}

void fatal(const char* fmt, ...){
    if (fmigo_loglevel >= ::jm_log_level_fatal) {
        va_list argptr;
        va_start(argptr, fmt);
        fprintf(stderr,ANSI_COLOR_RED "FATAL: ");
        vfprintf(stderr, fmt, argptr);
        fprintf(stderr,ANSI_COLOR_RESET);
        va_end(argptr);
    }
    exit(1);
}

void error(const char*fmt,...){
    if (fmigo_loglevel >= ::jm_log_level_error) {
        va_list argptr;
        va_start(argptr, fmt);
        fprintf(stderr,ANSI_COLOR_RED "ERROR: ");
        vfprintf(stderr, fmt, argptr);
        fprintf(stderr,ANSI_COLOR_RESET);
        va_end(argptr);
    }
}

void warning(const char*fmt,...){
    if (fmigo_loglevel >= ::jm_log_level_warning) {
        va_list argptr;
        va_start(argptr, fmt);
        fprintf(stderr,ANSI_COLOR_YELLOW "WARNING: ");
        vfprintf(stderr, fmt, argptr);
        fprintf(stderr,ANSI_COLOR_RESET);
        va_end(argptr);
    }
}

#ifdef DEBUG
void debug(const char*fmt,...){
    if (fmigo_loglevel >= ::jm_log_level_debug) {
        va_list argptr;
        va_start(argptr, fmt);
        fprintf(stderr,ANSI_COLOR_MAGENTA);
        vfprintf(stderr, fmt, argptr);
        fprintf(stderr,ANSI_COLOR_RESET);
        va_end(argptr);
    }
}
#endif

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
        fatal("Invalid logging (%s). Possible options are from 0 to 7.\n", option);
    }
}

bool isVR(const std::string& input) {
    return std::all_of(input.begin(), input.end(), ::isdigit);
}

bool isReal(const std::string& input) {
    return input.find_first_not_of("+-0123456789.eE") == std::string::npos;
}

bool isInteger(const std::string& input) {
    return input.find_first_not_of("+-0123456789") == std::string::npos;
}

bool isBoolean(const std::string& input) {
    return input == "true" || input == "false";
}

}
