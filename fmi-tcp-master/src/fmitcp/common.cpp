#include "common.h"
#include <vector>
#include <string>

#ifdef USE_LACEWING
void fmitcp::sendProtoBuffer(lw_client c, std::string s){
    char sz[4] = {
        //big endian network order is customary
        s.size() >> 24,
        s.size() >> 16,
        s.size() >> 8,
        s.size(),
    };

    //fprintf(stderr, "> sz: %02x,%02x,%02x,%02x (%zu)\n", (unsigned char)sz[0], (unsigned char)sz[1], (unsigned char)sz[2], (unsigned char)sz[3], s.size());

    //send everything in one call to lessen impact of lack of Nagle's
    string total = string(sz, 4) + s;
    lw_stream_write(c, total.c_str(), total.size());

    //printf("sendProtoBuffer(%s)\n", message->DebugString().c_str());
}

vector<string> fmitcp::unpackBuffer(const char* indata, long insize, string *tail) {
    vector<string> messages;

    string temp = *tail + string(indata, insize);
    const char *data = temp.c_str();
    long size = temp.size();
    long ofs = 0;

    for (; ofs < size - 4;) {
        //big endian (network order)
        size_t sz = (((unsigned char)data[ofs+0]) << 24) +
                    (((unsigned char)data[ofs+1]) << 16) +
                    (((unsigned char)data[ofs+2]) << 8) +
                     ((unsigned char)data[ofs+3]);

        //fprintf(stderr, "< sz: %02x,%02x,%02x,%02x (%zu) %s\n", (unsigned char)data[ofs+0], (unsigned char)data[ofs+1], (unsigned char)data[ofs+2], (unsigned char)data[ofs+3], sz, data+ofs);

        if (ofs + 4 + sz > size) {
            break;
        }

        ofs += 4;
        messages.push_back(string(data + ofs, sz));
        ofs += sz;
    }

    //give remaining bytes, if any
    *tail = string(data + ofs, size - ofs);

    return messages;
}
#endif

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
#ifndef WIN32
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
#else
    if (hdf5Filename.length()) {
        fprintf(stderr, "WARNING: HDF5 output not enabled on Windows\n");
    }
#endif
}
