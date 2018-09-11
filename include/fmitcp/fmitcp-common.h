#ifndef COMMON_H_
#define COMMON_H_

#define FMITCP_VERSION "0.0.1"

#include "common/common.h"
#include "fmitcp.pb.h"
#define FMILIB_BUILDING_LIBRARY
#include <fmilib.h>
#include <string>
#include <sstream>
//for HDF5 output
#include <hdf5.h>
#include <hdf5_hl.h>

using namespace std;
namespace fmitcp {
  /*!
   * Makes a comma separated string of an array. The array could be if type int, double etc.
   */
  template <typename TArray>
  string arrayToString(std::vector<TArray>& arr) {
    ostringstream res;
    res << "{";
    for (size_t i = 0 ; i < arr.size() ; i++) {
      if (i != 0) res << ",";
      res << arr[i];
    }
    res << "}";
    return res.str();
  }

  fmitcp_proto::jm_status_enu_t fmiJMStatusToProtoJMStatus(jm_status_enu_t status);
  fmitcp_proto::fmi2_status_t fmi2StatusToProtofmi2Status(fmi2_status_t status);
  fmitcp_proto::fmi2_event_info_t* fmi2EventInfoToProtoEventInfo(const fmi2_event_info_t &eventInfo);
  fmitcp_proto::jm_log_level_enu_t fmiJMLogLevelToProtoJMLogLevel(jm_log_level_enu_t logLevel);
  fmi2_event_info_t protoEventInfoToFmi2EventInfo(fmitcp_proto::fmi2_event_info_t eventInfo);
  fmi2_status_kind_t protoStatusKindToFmiStatusKind(fmitcp_proto::fmi2_status_kind_t statusKind);

    void writeHDF5File(
        std::string hdf5Filename,
        std::vector<size_t>& field_offset,
        std::vector<hid_t>& field_types,
        std::vector<const char*>& field_names,
        const char *table_title,
        const char *dset_name,
        hsize_t nrecords,
        hsize_t type_size,
        const void *buf);

  static fmitcp_proto::fmitcp_message_Type parseType(const char* data, long size) {
      if (size < 2) {
          fatal("Client::clientData() needs at least 2 bytes\n");
      }
      int t = (uint8_t)data[0] + 256*(uint8_t)data[1];
      if (!fmitcp_proto::fmitcp_message_Type_IsValid(t)) {
          fatal("Message type %i invalid\n", t);
      }
      return (fmitcp_proto::fmitcp_message_Type)t;
  }

#define CLIENTDATA_NEW 1
#define USE_DO_STEP_S 1
#define USE_SET_REAL_S 1
#define USE_GET_REAL_S 1
#define USE_3BYTE_STATUS_RES 1
#define USE_GET_REAL_RES_S 1
#define SERVER_CLIENTDATA_NO_STRING_RET 1

//don't bother checking if a value was already requested?
//this is of dubious utility
//tests show it's sometimes faster, sometimes slower
#define DONT_FILTER_OUTGOING_VRS 0

    struct do_step_s {
        double currentcommunicationpoint;
        double communicationstepsize;
        bool newStep;
    };
}

#endif
