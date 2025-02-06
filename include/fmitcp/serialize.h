#ifndef FMITCP_SERIALIZE_H
#define FMITCP_SERIALIZE_H

#include <string>
#include <sstream>
#include "fmitcp.pb.h"

namespace fmitcp {
    namespace serialize {
        template<typename T> std::string pack(fmitcp_proto::fmitcp_message_Type type, T &req) {
          uint16_t t = type;
#if GOOGLE_PROTOBUF_VERSION >= 3021000
          // ByteSize() is deprecated on newer versions of protobuf
          std::string ret(2 + req.ByteSizeLong(), 0);
#else
          std::string ret(2 + req.ByteSize(), 0);
#endif
          ret[0] = (uint8_t)t;
          ret[1] = (uint8_t)(t>>8);
          req.SerializeWithCachedSizesToArray((uint8_t*)&ret[2]);
          return ret;
        }

#if CLIENTDATA_NEW == 0
        static void packIntoOstringstream(std::ostringstream& oss, const std::string& s) {
          size_t sz = s.size();

          if (sz > 0xFFFFFFFF) {
            fprintf(stderr, "sz = %lu\n", sz);
            exit(1);
          }

          //size of packet, including type
          std::string szStr(4, 0);
          szStr[0] = (uint8_t)sz;
          szStr[1] = (uint8_t)(sz>>8);
          szStr[2] = (uint8_t)(sz>>16);
          szStr[3] = (uint8_t)(sz>>24);
          oss << szStr << s;
        }
#endif

        static void packIntoCharVector(std::vector<char>& vec, const std::string& s) {
          size_t sz = s.size();

          if (sz > 0xFFFFFFFF) {
            fprintf(stderr, "sz = %lu\n", sz);
            exit(1);
          }

          vec.reserve(vec.size() + 4 + s.length());

          //size of packet, including type
          char szStr[4];
          szStr[0] = (uint8_t)sz;
          szStr[1] = (uint8_t)(sz>>8);
          szStr[2] = (uint8_t)(sz>>16);
          szStr[3] = (uint8_t)(sz>>24);
          vec.insert(vec.end(), szStr, &szStr[4]);
          vec.insert(vec.end(), s.c_str(), s.c_str() + s.length());
        }

        static size_t parseSize(const char *data, size_t size) {
          if (size < 4) {
            fprintf(stderr, "parseSize(): not enough data\n");
            exit(1);
          }

          size_t packetSize = 0;
          packetSize += (uint8_t)data[0];
          packetSize += ((uint8_t)data[1]) << 8;
          packetSize += ((uint8_t)data[2]) << 16;
          packetSize += ((uint8_t)data[3]) << 24;
          return packetSize;
        }

        // FMI functions follow. These should correspond to FMILibrary functions.

        // =========== FMI 2.0 (CS) Co-Simulation functions ===========
        std::string fmi2_import_set_real_input_derivatives(std::vector<int> valueRefs, std::vector<int> orders, std::vector<double> values);
        std::string fmi2_import_get_real_output_derivatives(std::vector<int> valueRefs, std::vector<int> orders);
        std::string fmi2_import_cancel_step();
        std::string fmi2_import_do_step(double currentCommunicationPoint,
                                        double communicationStepSize,
                                        bool newStep);
        std::string fmi2_import_get_status        (fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_real_status   (fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_integer_status(fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_boolean_status(fmitcp_proto::fmi2_status_kind_t s);
        std::string fmi2_import_get_string_status (fmitcp_proto::fmi2_status_kind_t s);

        // =========== FMI 2.0 (ME) Model Exchange functions ===========
        std::string fmi2_import_enter_event_mode();
        std::string fmi2_import_new_discrete_states();
        std::string fmi2_import_enter_continuous_time_mode();
        std::string fmi2_import_completed_integrator_step();
        std::string fmi2_import_set_time(double time);
        std::string fmi2_import_set_continuous_states(const double* x, int nx);
        std::string fmi2_import_get_event_indicators(int nz);
        std::string fmi2_import_get_continuous_states(int nx);
        std::string fmi2_import_get_derivatives(int nDerivatives);
        std::string fmi2_import_get_nominal_continuous_states(int nx);

        // ========= FMI 2.0 CS & ME COMMON FUNCTIONS ============
        std::string fmi2_import_get_version();
        std::string fmi2_import_set_debug_logging(bool loggingOn, std::vector<std::string> categories);
        std::string fmi2_import_instantiate();               //calls fmi2_import_instantiate2() with visible=false (backward compatibility)
        std::string fmi2_import_instantiate2(bool visible);
        std::string fmi2_import_setup_experiment(bool toleranceDefined, double tolerance, double startTime,
            bool stopTimeDefined, double stopTime);
        std::string fmi2_import_enter_initialization_mode();
        std::string fmi2_import_exit_initialization_mode();
        std::string fmi2_import_terminate();
        std::string fmi2_import_reset();
        std::string fmi2_import_free_instance();
        std::string fmi2_import_set_real   (const std::vector<int>& valueRefs, const std::vector<double>& values);
        std::string fmi2_import_set_integer(const std::vector<int>& valueRefs, const std::vector<int>& values);
        std::string fmi2_import_set_boolean(const std::vector<int>& valueRefs, const std::vector<bool>& values);
        std::string fmi2_import_set_string (const std::vector<int>& valueRefs, const std::vector<std::string>& values);

        //C is vector or set
        template<typename R, typename C> std::string collection_to_req(fmitcp_proto::fmitcp_message_Type type, const C& refs) {
          R req;
          for (int vr : refs) {
            req.add_valuereferences(vr);
          }
          return pack(type, req);
        }

        template<typename C> void fmi2_import_get_real_fast(std::vector<char>& buffer, const C& valueRefs) {
            size_t bofs = buffer.size();
            size_t sz = 2 + valueRefs.size() * sizeof(int);
            buffer.resize(bofs + 4 + sz);

            buffer[bofs+0] = sz;
            buffer[bofs+1] = sz >> 8;
            buffer[bofs+2] = sz >> 16;
            buffer[bofs+3] = sz >> 24;
            buffer[bofs+4] = fmitcp_proto::type_fmi2_import_get_real_req & 0xFF;
            buffer[bofs+5] = fmitcp_proto::type_fmi2_import_get_real_req >> 8;

            int *vrs =  (int*)&buffer[bofs+6];
            int ofs = 0;
            for (int vr : valueRefs) {
                vrs[ofs++] = vr;
            }
        }

        template<typename C> std::string fmi2_import_get_real(const C& valueRefs) {
#if USE_SET_REAL_S == 1
            std::string str(2 + valueRefs.size() * (sizeof(int)), 0);
            int *vrs =  (int*)&str[2];

            str[0] = fmitcp_proto::type_fmi2_import_get_real_req & 0xFF;
            str[1] = fmitcp_proto::type_fmi2_import_get_real_req >> 8;

            int ofs = 0;
            for (int vr : valueRefs) {
                vrs[ofs++] = vr;
            }

            return str;
#else
          return collection_to_req<fmitcp_proto::fmi2_import_get_real_req>(fmitcp_proto::type_fmi2_import_get_real_req, valueRefs);
#endif
        }

        template<typename C> std::string fmi2_import_get_integer(const C& valueRefs) {
          return collection_to_req<fmitcp_proto::fmi2_import_get_integer_req>(fmitcp_proto::type_fmi2_import_get_integer_req, valueRefs);
        }

        template<typename C> std::string fmi2_import_get_boolean(const C& valueRefs) {
          return collection_to_req<fmitcp_proto::fmi2_import_get_boolean_req>(fmitcp_proto::type_fmi2_import_get_boolean_req, valueRefs);
        }

        template<typename C> std::string fmi2_import_get_string(const C& valueRefs) {
          return collection_to_req<fmitcp_proto::fmi2_import_get_string_req> (fmitcp_proto::type_fmi2_import_get_string_req,  valueRefs);
        }

        std::string fmi2_import_get_fmu_state();
        std::string fmi2_import_set_fmu_state(int stateId);
        std::string fmi2_import_free_fmu_state(int stateId);
        std::string fmi2_import_set_free_last_fmu_state();
        std::string fmi2_import_serialized_fmu_state_size();
        std::string fmi2_import_serialize_fmu_state();
        std::string fmi2_import_de_serialize_fmu_state();
        std::string fmi2_import_get_directional_derivative(const std::vector<int>& v_ref, const std::vector<int>& z_ref, const std::vector<double>& dv);

        // ========= NETWORK SPECIFIC FUNCTIONS ============
        std::string get_xml();
    }
}

#endif //FMITCP_SERIALIZE_H
