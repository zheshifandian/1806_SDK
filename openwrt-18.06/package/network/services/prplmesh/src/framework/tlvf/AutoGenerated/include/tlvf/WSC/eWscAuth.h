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

#ifndef _TLVF_WSC_EWSCAUTH_H_
#define _TLVF_WSC_EWSCAUTH_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscAuth: uint16_t {
    WSC_AUTH_OPEN = 0x1,
    WSC_AUTH_WPAPSK = 0x2,
    WSC_AUTH_SHARED = 0x4,
    WSC_AUTH_WPA = 0x8,
    WSC_AUTH_WPA2 = 0x10,
    WSC_AUTH_WPA2PSK = 0x20,
    WSC_AUTH_SAE = 0x40,
    WSC_AUTH_INVALID = 0xffff,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscAuth_str(eWscAuth enum_value) {
    switch (enum_value) {
    case WSC_AUTH_OPEN:    return "WSC_AUTH_OPEN";
    case WSC_AUTH_WPAPSK:  return "WSC_AUTH_WPAPSK";
    case WSC_AUTH_SHARED:  return "WSC_AUTH_SHARED";
    case WSC_AUTH_WPA:     return "WSC_AUTH_WPA";
    case WSC_AUTH_WPA2:    return "WSC_AUTH_WPA2";
    case WSC_AUTH_WPA2PSK: return "WSC_AUTH_WPA2PSK";
    case WSC_AUTH_SAE:     return "WSC_AUTH_SAE";
    case WSC_AUTH_INVALID: return "WSC_AUTH_INVALID";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscAuth value) { return out << eWscAuth_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCAUTH_H_
