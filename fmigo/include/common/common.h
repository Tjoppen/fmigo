#ifndef COMMON_COMMON_H_
#define COMMON_COMMON_H_

#define FMITCPMASTER_VERSION "0.0.1"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
//Microsoft has implemented snprintf() starting with Visual Studio 2015
//for older versions we need to use sprintf_s()
#if _MSC_VER < 1900
#define snprintf sprintf_s
#endif
#endif

#include <vector>
#include <deque>

#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include "fmitcp.pb.h"

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

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

extern jm_log_level_enu_t fmigo_loglevel ;

void info(const char* fmt, ...);
void error(const char* fmt, ...);
void warning(const char* fmt, ...);

#ifdef WIN32
__declspec(noreturn) void fatal(const char* fmt, ...);
#else //assume GCC or Clang
void fatal(const char* fmt, ...) __attribute__ ((__noreturn__));
#endif

#ifdef DEBUG
void debug(const char* fmt, ...);
#else
#define debug(fmt, ...)
#endif

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

namespace common {
    std::deque<std::string> split(const std::string &s, char delim);
    int string_to_int(const std::string& s);
    std::string int_to_string(int i);

    jm_log_level_enu_t protoJMLogLevelToFmiJMLogLevel(fmitcp_proto::jm_log_level_enu_t logLevel);

    //converts -l option to JM compaible log level
    jm_log_level_enu_t logOptionToJMLogLevel(const char* option);

bool isNumeric(const std::string& input);

}

#endif
