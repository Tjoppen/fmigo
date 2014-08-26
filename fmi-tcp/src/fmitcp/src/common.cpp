#include "common.h"
#include <vector>
#include <string>

void fmitcp::sendProtoBuffer(lw_client c, fmitcp_proto::fmitcp_message * message){
    std::string s;
    bool status = message->SerializeToString(&s);
    //printf("serialize status=%d\n", status);
    lw_stream_write(c, s.c_str(), s.size());
    //std::string newline = "\n";
    //lw_stream_write(c, newline.c_str(),newline.size());
    //fflush(NULL);
    //
    printf("sendProtoBuffer(%s)\n", message->DebugString().c_str());
}

string fmitcp::dataToString(const char* data, long size) {
  std::string data2(data, size);
  return data2;
}

fmitcp_proto::jm_status_enu_t fmitcp::fmiJMStatusToProtoJMStatus(jm_status_enu_t status) {
  switch (status) {
  case jm_status_error:
    return fmitcp_proto::jm_status_error;
  case jm_status_success:
    return fmitcp_proto::jm_status_success;
  case jm_status_warning:
    return fmitcp_proto::jm_status_warning;
  default: // should never be reached
    return fmitcp_proto::jm_status_error;
  }
}

fmitcp_proto::fmi2_status_t fmitcp::fmi2StatusToProtofmi2Status(fmi2_status_t status) {
  switch (status) {
  case fmi2_status_ok:
    return fmitcp_proto::fmi2_status_ok;
  case fmi2_status_warning:
    return fmitcp_proto::fmi2_status_warning;
  case fmi2_status_discard:
    return fmitcp_proto::fmi2_status_discard;
  case fmi2_status_error:
      return fmitcp_proto::fmi2_status_error;
  case fmi2_status_fatal:
      return fmitcp_proto::fmi2_status_fatal;
  case fmi2_status_pending:
      return fmitcp_proto::fmi2_status_pending;
  default: // should never be reached
    return fmitcp_proto::fmi2_status_error;
  }
}

fmitcp_proto::jm_log_level_enu_t fmitcp::fmiJMLogLevelToProtoJMLogLevel(jm_log_level_enu_t logLevel) {
  switch (logLevel) {
  case jm_log_level_nothing:
    return fmitcp_proto::jm_log_level_nothing;
  case jm_log_level_fatal:
    return fmitcp_proto::jm_log_level_fatal;
  case jm_log_level_error:
    return fmitcp_proto::jm_log_level_error;
  case jm_log_level_warning:
    return fmitcp_proto::jm_log_level_warning;
  case jm_log_level_info:
    return fmitcp_proto::jm_log_level_info;
  case jm_log_level_verbose:
    return fmitcp_proto::jm_log_level_verbose;
  case jm_log_level_debug:
    return fmitcp_proto::jm_log_level_debug;
  case jm_log_level_all:
    return fmitcp_proto::jm_log_level_all;
  default: // should never be reached
    return fmitcp_proto::jm_log_level_nothing;
  }
}

fmi2_status_kind_t fmitcp::protoStatusKindToFmiStatusKind(fmitcp_proto::fmi2_status_kind_t statusKind) {
  switch (statusKind) {
  case fmitcp_proto::fmi2_do_step_status:
    return fmi2_do_step_status;
  case fmitcp_proto::fmi2_pending_status:
    return fmi2_pending_status;
  case fmitcp_proto::fmi2_last_successful_time:
    return fmi2_last_successful_time;
  case fmitcp_proto::fmi2_terminated:
    return fmi2_terminated;
  default: // should never be reached
    return fmi2_do_step_status;
  }
}
