#ifndef COMMON_H_
#define COMMON_H_

#define FMITCP_VERSION "0.0.1"

#include "fmitcp.pb.h"
#define lw_import
#include <lacewing.h>
#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include <string>
#include <sstream>

using namespace std;
namespace fmitcp {

  /*!
   * Converts the type e.g int, double etc. to string.
   */
  template <typename T>
  string typeToString(T type) {
    ostringstream ss;
    ss << type;
    return ss.str();
  }

  /*!
   * Makes a comma separated string of an array. The array could be if type int, double etc.
   */
  template <typename TArray>
  string arrayToString(TArray arr[], int size) {
    string res;
    res.append("{");
    for (int i = 0 ; i < size ; i++) {
      if (i != 0) res.append(",");
      res.append(typeToString(arr[i]));
    }
    res.append("}");
    return res;
  }

  /// Send a binary protobuf to a client
  void sendProtoBuffer(lw_client c, fmitcp_proto::fmitcp_message * message);

  /// Convert incoming data to a C++ string
  string dataToString(const char* data, long size);

  fmitcp_proto::jm_status_enu_t fmiJMStatusToProtoJMStatus(jm_status_enu_t status);
  fmitcp_proto::fmi2_status_t fmi2StatusToProtofmi2Status(fmi2_status_t status);
  fmitcp_proto::jm_log_level_enu_t fmiJMLogLevelToProtoJMLogLevel(jm_log_level_enu_t logLevel);
  fmi2_status_kind_t protoStatusKindToFmiStatusKind(fmitcp_proto::fmi2_status_kind_t statusKind);

}

#endif
