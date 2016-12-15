#include "common.h"
#include <vector>
#include <string>

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

void fmitcp::writeHDF5File(
        std::string hdf5Filename,
        std::vector<size_t>& field_offset,
        std::vector<hid_t>& field_types,
        std::vector<const char*>& field_names,
        const char *table_title,
        const char *dset_name,
        hsize_t nrecords,
        hsize_t type_size,
        const void *buf) {
    //HDF5
    if (hdf5Filename.length()) {
        fprintf(stderr, "Writing HDF5 file \"%s\"\n", hdf5Filename.c_str());
        fprintf(stderr, "HDF5 column names:\n");

        for (size_t x = 0; x < field_names.size(); x++) {
            fprintf(stderr, "%2li: %s\n", x, field_names[x]);
        }

        hid_t file_id = H5Fcreate(hdf5Filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
        H5TBmake_table(table_title, file_id, dset_name, field_names.size(), nrecords,
                type_size, field_names.data(), field_offset.data(),
                field_types.data(), 10 /* chunk_size */, NULL, 0, buf);
        H5Fclose(file_id);
    }
}

fmitcp_proto::fmi2_event_info_t* fmitcp::fmi2EventInfoToProtoEventInfo(fmi2_event_info_t *eventInfo)
{
  fmitcp_proto::fmi2_event_info_t* info;
  info->set_newdiscretestatesneeded(eventInfo->newDiscreteStatesNeeded);
  info->set_terminatesimulation(eventInfo->terminateSimulation);
  info->set_nominalsofcontinuousstateschanged(eventInfo->nominalsOfContinuousStatesChanged);
  info->set_valuesofcontinuousstateschanged(eventInfo->valuesOfContinuousStatesChanged);
  info->set_nexteventtimedefined(eventInfo->nextEventTimeDefined);
  info->set_nexteventtime(eventInfo->nextEventTime);

  return info;
}
