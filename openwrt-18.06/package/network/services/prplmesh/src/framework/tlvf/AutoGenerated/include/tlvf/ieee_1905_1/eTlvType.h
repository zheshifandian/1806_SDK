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

#ifndef _TLVF_IEEE_1905_1_ETLVTYPE_H_
#define _TLVF_IEEE_1905_1_ETLVTYPE_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace ieee1905_1 {

enum class eTlvType : uint8_t {
    TLV_END_OF_MESSAGE = 0x0,
    TLV_AL_MAC_ADDRESS = 0x1,
    TLV_MAC_ADDRESS = 0x2,
    TLV_DEVICE_INFORMATION = 0x3,
    TLV_DEVICE_BRIDGING_CAPABILITY = 0x4,
    TLV_NON_1905_NEIGHBOR_DEVICE_LIST = 0x6,
    TLV_1905_NEIGHBOR_DEVICE = 0x7,
    TLV_LINK_METRIC_QUERY = 0x8,
    TLV_TRANSMITTER_LINK_METRIC = 0x9,
    TLV_RECEIVER_LINK_METRIC = 0xa,
    TLV_VENDOR_SPECIFIC = 0xb,
    TLV_LINK_METRIC_RESULT_CODE = 0xc,
    TLV_SEARCHED_ROLE = 0xd,
    TLV_AUTOCONFIG_FREQ_BAND = 0xe,
    TLV_SUPPORTED_ROLE = 0xf,
    TLV_SUPPORTED_FREQ_BAND = 0x10,
    TLV_WSC = 0x11,
    TLV_PUSH_BUTTON_EVENT_NOTIFICATION = 0x12,
    TLV_PUSH_BUTTON_JOIN_NOTIFICATION = 0x13,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eTlvType_str(eTlvType enum_value) {
    switch (enum_value) {
    case eTlvType::TLV_END_OF_MESSAGE:                 return "eTlvType::TLV_END_OF_MESSAGE";
    case eTlvType::TLV_AL_MAC_ADDRESS:                 return "eTlvType::TLV_AL_MAC_ADDRESS";
    case eTlvType::TLV_MAC_ADDRESS:                    return "eTlvType::TLV_MAC_ADDRESS";
    case eTlvType::TLV_DEVICE_INFORMATION:             return "eTlvType::TLV_DEVICE_INFORMATION";
    case eTlvType::TLV_DEVICE_BRIDGING_CAPABILITY:     return "eTlvType::TLV_DEVICE_BRIDGING_CAPABILITY";
    case eTlvType::TLV_NON_1905_NEIGHBOR_DEVICE_LIST:  return "eTlvType::TLV_NON_1905_NEIGHBOR_DEVICE_LIST";
    case eTlvType::TLV_1905_NEIGHBOR_DEVICE:           return "eTlvType::TLV_1905_NEIGHBOR_DEVICE";
    case eTlvType::TLV_LINK_METRIC_QUERY:              return "eTlvType::TLV_LINK_METRIC_QUERY";
    case eTlvType::TLV_TRANSMITTER_LINK_METRIC:        return "eTlvType::TLV_TRANSMITTER_LINK_METRIC";
    case eTlvType::TLV_RECEIVER_LINK_METRIC:           return "eTlvType::TLV_RECEIVER_LINK_METRIC";
    case eTlvType::TLV_VENDOR_SPECIFIC:                return "eTlvType::TLV_VENDOR_SPECIFIC";
    case eTlvType::TLV_LINK_METRIC_RESULT_CODE:        return "eTlvType::TLV_LINK_METRIC_RESULT_CODE";
    case eTlvType::TLV_SEARCHED_ROLE:                  return "eTlvType::TLV_SEARCHED_ROLE";
    case eTlvType::TLV_AUTOCONFIG_FREQ_BAND:           return "eTlvType::TLV_AUTOCONFIG_FREQ_BAND";
    case eTlvType::TLV_SUPPORTED_ROLE:                 return "eTlvType::TLV_SUPPORTED_ROLE";
    case eTlvType::TLV_SUPPORTED_FREQ_BAND:            return "eTlvType::TLV_SUPPORTED_FREQ_BAND";
    case eTlvType::TLV_WSC:                            return "eTlvType::TLV_WSC";
    case eTlvType::TLV_PUSH_BUTTON_EVENT_NOTIFICATION: return "eTlvType::TLV_PUSH_BUTTON_EVENT_NOTIFICATION";
    case eTlvType::TLV_PUSH_BUTTON_JOIN_NOTIFICATION:  return "eTlvType::TLV_PUSH_BUTTON_JOIN_NOTIFICATION";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eTlvType value) { return out << eTlvType_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end
class eTlvTypeValidate {
public:
    static bool check(uint8_t value) {
        bool ret = false;
        switch (value) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x6:
        case 0x7:
        case 0x8:
        case 0x9:
        case 0xa:
        case 0xb:
        case 0xc:
        case 0xd:
        case 0xe:
        case 0xf:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
                ret = true;
                break;
            default:
                ret = false;
                break;
        }
        return ret;
    }
};


}; // close namespace: ieee1905_1

#endif //_TLVF/IEEE_1905_1_ETLVTYPE_H_
