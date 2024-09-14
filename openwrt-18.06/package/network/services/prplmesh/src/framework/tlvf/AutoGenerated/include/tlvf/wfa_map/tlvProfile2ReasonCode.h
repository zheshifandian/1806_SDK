///////////////////////////////////////
// AUTO GENERATED FILE - DO NOT EDIT //
///////////////////////////////////////

/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _TLVF_WFA_MAP_TLVPROFILE2REASONCODE_H_
#define _TLVF_WFA_MAP_TLVPROFILE2REASONCODE_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"
#include <ostream>

namespace wfa_map {


class tlvProfile2ReasonCode : public BaseClass
{
    public:
        tlvProfile2ReasonCode(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2ReasonCode(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2ReasonCode();

        enum eReasonCode: uint16_t {
            RESERVED = 0x0,
            UNSPECIFIED_REASON = 0x1,
            INVALID_AUTHENTICATION = 0x2,
            LEAVING_NETWORK_DEAUTH = 0x3,
            REASON_INACTIVITY = 0x4,
            NO_MORE_STAS = 0x5,
            INVALID_CLASS2_FRAME = 0x6,
            INVALID_CLASS3_FRAME = 0x7,
            LEAVING_NETWORK_DISASSOC = 0x8,
            NOT_AUTHENTICATED = 0x9,
            UNACCEPTABLE_POWER_CAPABILITY = 0xa,
            UNACCEPTABLE_SUPPORTED_CHANNELS = 0xb,
            BSS_TRANSITION_DISASSOC = 0xc,
            REASON_INVALID_ELEMENT = 0xd,
            MIC_FAILURE = 0xe,
            FOUR_WAY_HANDSHAKE_TIMEOUT = 0xf,
            GK_HANDSHAKE_TIMEOUT = 0x10,
            HANDSHAKE_ELEMENT_MISMATCH = 0x11,
            REASON_INVALID_GROUP_CIPHER = 0x12,
            REASON_INVALID_PAIRWISE_CIPHER = 0x13,
            REASON_INVALID_AKMP = 0x14,
            UNSUPPORTED_RSNE_VERSION = 0x15,
            INVALID_RSNE_CAPABILITIES = 0x16,
            X_AUTH_FAILED_802_1 = 0x17,
            REASON_CIPHER_OUT_OF_POLICY = 0x18,
            TDLS_PEER_UNREACHABLE = 0x19,
            TDLS_UNSPECIFIED_REASON = 0x1a,
            SSP_REQUESTED_DISASSOC = 0x1b,
            NO_SSP_ROAMING_AGREEMENT = 0x1c,
            BAD_CIPHER_OR_AKM = 0x1d,
            NOT_AUTHORIZED_THIS_LOCATION = 0x1e,
            SERVICE_CHANGE_PRECLUDES_TS = 0x1f,
            UNSPECIFIED_QOS_REASON = 0x20,
            NOT_ENOUGH_BANDWIDTH = 0x21,
            MISSING_ACKS = 0x22,
            EXCEEDED_TXOP = 0x23,
            STA_LEAVING = 0x24,
            END_TSEND_BA = 0x25,
            UNKNOWN_TSUNKNOWN_BA = 0x26,
            TIMEOUT = 0x27,
            PEERKEY_MISMATCH = 0x2d,
            PEER_INITIATED = 0x2e,
            AP_INITIATED = 0x2f,
            REASON_INVALID_FT_ACTION_FRAME_COUNT = 0x30,
            REASON_INVALID_PMKID = 0x31,
            REASON_INVALID_MDE = 0x32,
            REASON_INVALID_FTE = 0x33,
            MESH_PEERING_CANCELED = 0x34,
            MESH_MAX_PEERS = 0x35,
            MESH_CONFIGURATIONPOLICY_VIOLATION = 0x36,
            MESH_CLOSE_RCVD = 0x37,
            MESH_MAX_RETRIES = 0x38,
            MESH_CONFIRM_TIMEOUT = 0x39,
            MESH_INVALID_GTK = 0x3a,
            MESH_INCONSISTENTPARAMETERS = 0x3b,
            MESH_INVALID_SECURITYCAPABILITY = 0x3c,
            MESH_PATH_ERROR_NOPROXY_INFORMATION = 0x3d,
            MESH_PATH_ERROR_NOFORWARDING_INFORMATION = 0x3e,
            MESH_PATH_ERRORDESTINATIONUNREACHABLE = 0x3f,
            MAC_ADDRESS_ALREADYEXISTS_IN_MBSS = 0x40,
            MESH_CHANNEL_SWITCHREGULATORYREQUIREMENTS = 0x41,
            MESH_CHANNEL_SWITCHUNSPECIFIED = 0x42,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eReasonCode_str(eReasonCode enum_value) {
            switch (enum_value) {
            case RESERVED:                                  return "RESERVED";
            case UNSPECIFIED_REASON:                        return "UNSPECIFIED_REASON";
            case INVALID_AUTHENTICATION:                    return "INVALID_AUTHENTICATION";
            case LEAVING_NETWORK_DEAUTH:                    return "LEAVING_NETWORK_DEAUTH";
            case REASON_INACTIVITY:                         return "REASON_INACTIVITY";
            case NO_MORE_STAS:                              return "NO_MORE_STAS";
            case INVALID_CLASS2_FRAME:                      return "INVALID_CLASS2_FRAME";
            case INVALID_CLASS3_FRAME:                      return "INVALID_CLASS3_FRAME";
            case LEAVING_NETWORK_DISASSOC:                  return "LEAVING_NETWORK_DISASSOC";
            case NOT_AUTHENTICATED:                         return "NOT_AUTHENTICATED";
            case UNACCEPTABLE_POWER_CAPABILITY:             return "UNACCEPTABLE_POWER_CAPABILITY";
            case UNACCEPTABLE_SUPPORTED_CHANNELS:           return "UNACCEPTABLE_SUPPORTED_CHANNELS";
            case BSS_TRANSITION_DISASSOC:                   return "BSS_TRANSITION_DISASSOC";
            case REASON_INVALID_ELEMENT:                    return "REASON_INVALID_ELEMENT";
            case MIC_FAILURE:                               return "MIC_FAILURE";
            case FOUR_WAY_HANDSHAKE_TIMEOUT:                return "FOUR_WAY_HANDSHAKE_TIMEOUT";
            case GK_HANDSHAKE_TIMEOUT:                      return "GK_HANDSHAKE_TIMEOUT";
            case HANDSHAKE_ELEMENT_MISMATCH:                return "HANDSHAKE_ELEMENT_MISMATCH";
            case REASON_INVALID_GROUP_CIPHER:               return "REASON_INVALID_GROUP_CIPHER";
            case REASON_INVALID_PAIRWISE_CIPHER:            return "REASON_INVALID_PAIRWISE_CIPHER";
            case REASON_INVALID_AKMP:                       return "REASON_INVALID_AKMP";
            case UNSUPPORTED_RSNE_VERSION:                  return "UNSUPPORTED_RSNE_VERSION";
            case INVALID_RSNE_CAPABILITIES:                 return "INVALID_RSNE_CAPABILITIES";
            case X_AUTH_FAILED_802_1:                       return "X_AUTH_FAILED_802_1";
            case REASON_CIPHER_OUT_OF_POLICY:               return "REASON_CIPHER_OUT_OF_POLICY";
            case TDLS_PEER_UNREACHABLE:                     return "TDLS_PEER_UNREACHABLE";
            case TDLS_UNSPECIFIED_REASON:                   return "TDLS_UNSPECIFIED_REASON";
            case SSP_REQUESTED_DISASSOC:                    return "SSP_REQUESTED_DISASSOC";
            case NO_SSP_ROAMING_AGREEMENT:                  return "NO_SSP_ROAMING_AGREEMENT";
            case BAD_CIPHER_OR_AKM:                         return "BAD_CIPHER_OR_AKM";
            case NOT_AUTHORIZED_THIS_LOCATION:              return "NOT_AUTHORIZED_THIS_LOCATION";
            case SERVICE_CHANGE_PRECLUDES_TS:               return "SERVICE_CHANGE_PRECLUDES_TS";
            case UNSPECIFIED_QOS_REASON:                    return "UNSPECIFIED_QOS_REASON";
            case NOT_ENOUGH_BANDWIDTH:                      return "NOT_ENOUGH_BANDWIDTH";
            case MISSING_ACKS:                              return "MISSING_ACKS";
            case EXCEEDED_TXOP:                             return "EXCEEDED_TXOP";
            case STA_LEAVING:                               return "STA_LEAVING";
            case END_TSEND_BA:                              return "END_TSEND_BA";
            case UNKNOWN_TSUNKNOWN_BA:                      return "UNKNOWN_TSUNKNOWN_BA";
            case TIMEOUT:                                   return "TIMEOUT";
            case PEERKEY_MISMATCH:                          return "PEERKEY_MISMATCH";
            case PEER_INITIATED:                            return "PEER_INITIATED";
            case AP_INITIATED:                              return "AP_INITIATED";
            case REASON_INVALID_FT_ACTION_FRAME_COUNT:      return "REASON_INVALID_FT_ACTION_FRAME_COUNT";
            case REASON_INVALID_PMKID:                      return "REASON_INVALID_PMKID";
            case REASON_INVALID_MDE:                        return "REASON_INVALID_MDE";
            case REASON_INVALID_FTE:                        return "REASON_INVALID_FTE";
            case MESH_PEERING_CANCELED:                     return "MESH_PEERING_CANCELED";
            case MESH_MAX_PEERS:                            return "MESH_MAX_PEERS";
            case MESH_CONFIGURATIONPOLICY_VIOLATION:        return "MESH_CONFIGURATIONPOLICY_VIOLATION";
            case MESH_CLOSE_RCVD:                           return "MESH_CLOSE_RCVD";
            case MESH_MAX_RETRIES:                          return "MESH_MAX_RETRIES";
            case MESH_CONFIRM_TIMEOUT:                      return "MESH_CONFIRM_TIMEOUT";
            case MESH_INVALID_GTK:                          return "MESH_INVALID_GTK";
            case MESH_INCONSISTENTPARAMETERS:               return "MESH_INCONSISTENTPARAMETERS";
            case MESH_INVALID_SECURITYCAPABILITY:           return "MESH_INVALID_SECURITYCAPABILITY";
            case MESH_PATH_ERROR_NOPROXY_INFORMATION:       return "MESH_PATH_ERROR_NOPROXY_INFORMATION";
            case MESH_PATH_ERROR_NOFORWARDING_INFORMATION:  return "MESH_PATH_ERROR_NOFORWARDING_INFORMATION";
            case MESH_PATH_ERRORDESTINATIONUNREACHABLE:     return "MESH_PATH_ERRORDESTINATIONUNREACHABLE";
            case MAC_ADDRESS_ALREADYEXISTS_IN_MBSS:         return "MAC_ADDRESS_ALREADYEXISTS_IN_MBSS";
            case MESH_CHANNEL_SWITCHREGULATORYREQUIREMENTS: return "MESH_CHANNEL_SWITCHREGULATORYREQUIREMENTS";
            case MESH_CHANNEL_SWITCHUNSPECIFIED:            return "MESH_CHANNEL_SWITCHUNSPECIFIED";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eReasonCode value) { return out << eReasonCode_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        eReasonCode& reason_code();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        eReasonCode* m_reason_code = nullptr;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2REASONCODE_H_
