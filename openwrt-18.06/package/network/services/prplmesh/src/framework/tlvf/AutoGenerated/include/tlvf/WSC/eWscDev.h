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

#ifndef _TLVF_WSC_EWSCDEV_H_
#define _TLVF_WSC_EWSCDEV_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscDev: uint16_t {
    WSC_DEV_NETWORK_INFRA_AP = 0x1,
    WSC_DEV_NETWORK_INFRA_ROUTER = 0x2,
    WSC_DEV_NETWORK_INFRA_SWITCH = 0x3,
    WSC_DEV_NETWORK_INFRA_GATEWAY = 0x4,
    WSC_DEV_NETWORK_INFRA_BRIDGE = 0x5,
    WSC_DEV_NETWORK_INFRA = 0x6,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscDev_str(eWscDev enum_value) {
    switch (enum_value) {
    case WSC_DEV_NETWORK_INFRA_AP:      return "WSC_DEV_NETWORK_INFRA_AP";
    case WSC_DEV_NETWORK_INFRA_ROUTER:  return "WSC_DEV_NETWORK_INFRA_ROUTER";
    case WSC_DEV_NETWORK_INFRA_SWITCH:  return "WSC_DEV_NETWORK_INFRA_SWITCH";
    case WSC_DEV_NETWORK_INFRA_GATEWAY: return "WSC_DEV_NETWORK_INFRA_GATEWAY";
    case WSC_DEV_NETWORK_INFRA_BRIDGE:  return "WSC_DEV_NETWORK_INFRA_BRIDGE";
    case WSC_DEV_NETWORK_INFRA:         return "WSC_DEV_NETWORK_INFRA";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscDev value) { return out << eWscDev_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCDEV_H_
