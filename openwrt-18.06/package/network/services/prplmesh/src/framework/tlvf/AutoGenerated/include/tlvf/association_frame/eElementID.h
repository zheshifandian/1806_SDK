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

#ifndef _TLVF_ASSOCIATION_FRAME_EELEMENTID_H_
#define _TLVF_ASSOCIATION_FRAME_EELEMENTID_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>
enum eElementID: uint8_t {
    ID_SSID = 0x0,
    ID_SUPPORT_RATES = 0x1,
    ID_EDCA_PARAM_SET = 0xc,
    ID_POWER_CAPABILITY = 0x21,
    ID_SUP_CHANNELS = 0x24,
    ID_HT_CAPABILITY = 0x2d,
    ID_QOS_CAPABILITY = 0x2e,
    ID_RSN = 0x30,
    ID_EXTENDED_SUP_RATES = 0x32,
    ID_MOBILITY_DOMAIN = 0x36,
    ID_FAST_BSS_TRANS = 0x37,
    ID_SUP_OP_CLASSES = 0x3b,
    ID_RM_ENABLED_CAPS = 0x46,
    ID_BSS_COEXISTENCE20_40 = 0x48,
    ID_RIC = 0x4b,
    ID_FMS_REQUEST = 0x57,
    ID_QOS_TRAFFIC_CAP = 0x59,
    ID_TIM_BROADCAST_REQUEST = 0x5e,
    ID_DMS_REQUEST = 0x63,
    ID_INTERWORKING = 0x6b,
    ID_EXTENDED_CAPABILITY = 0x7f,
    ID_MULTI_BAND = 0x9e,
    ID_MMS = 0xaa,
    ID_VHT_CAPS = 0xbf,
    ID_DMG_CAPS = 0xc2,
    ID_OP_MODE_NOTIFICATION = 0xc7,
    ID_VENDOR_SPECIFIC = 0xdd,
    ID_EID_EXTENSION = 0xff,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eElementID_str(eElementID enum_value) {
    switch (enum_value) {
    case ID_SSID:                  return "ID_SSID";
    case ID_SUPPORT_RATES:         return "ID_SUPPORT_RATES";
    case ID_EDCA_PARAM_SET:        return "ID_EDCA_PARAM_SET";
    case ID_POWER_CAPABILITY:      return "ID_POWER_CAPABILITY";
    case ID_SUP_CHANNELS:          return "ID_SUP_CHANNELS";
    case ID_HT_CAPABILITY:         return "ID_HT_CAPABILITY";
    case ID_QOS_CAPABILITY:        return "ID_QOS_CAPABILITY";
    case ID_RSN:                   return "ID_RSN";
    case ID_EXTENDED_SUP_RATES:    return "ID_EXTENDED_SUP_RATES";
    case ID_MOBILITY_DOMAIN:       return "ID_MOBILITY_DOMAIN";
    case ID_FAST_BSS_TRANS:        return "ID_FAST_BSS_TRANS";
    case ID_SUP_OP_CLASSES:        return "ID_SUP_OP_CLASSES";
    case ID_RM_ENABLED_CAPS:       return "ID_RM_ENABLED_CAPS";
    case ID_BSS_COEXISTENCE20_40:  return "ID_BSS_COEXISTENCE20_40";
    case ID_RIC:                   return "ID_RIC";
    case ID_FMS_REQUEST:           return "ID_FMS_REQUEST";
    case ID_QOS_TRAFFIC_CAP:       return "ID_QOS_TRAFFIC_CAP";
    case ID_TIM_BROADCAST_REQUEST: return "ID_TIM_BROADCAST_REQUEST";
    case ID_DMS_REQUEST:           return "ID_DMS_REQUEST";
    case ID_INTERWORKING:          return "ID_INTERWORKING";
    case ID_EXTENDED_CAPABILITY:   return "ID_EXTENDED_CAPABILITY";
    case ID_MULTI_BAND:            return "ID_MULTI_BAND";
    case ID_MMS:                   return "ID_MMS";
    case ID_VHT_CAPS:              return "ID_VHT_CAPS";
    case ID_DMG_CAPS:              return "ID_DMG_CAPS";
    case ID_OP_MODE_NOTIFICATION:  return "ID_OP_MODE_NOTIFICATION";
    case ID_VENDOR_SPECIFIC:       return "ID_VENDOR_SPECIFIC";
    case ID_EID_EXTENSION:         return "ID_EID_EXTENSION";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eElementID value) { return out << eElementID_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


#endif //_TLVF/ASSOCIATION_FRAME_EELEMENTID_H_
