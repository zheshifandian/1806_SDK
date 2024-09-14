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

#ifndef _TLVF_WSC_EWSCWFAVENDOREXTSUBELEMENT_H_
#define _TLVF_WSC_EWSCWFAVENDOREXTSUBELEMENT_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscWfaVendorExtSubelement {
    VERSION2 = 0x0,
    AUTHORIZED_MACS = 0x1,
    NETWORK_KEY_SHAREABLE = 0x2,
    REQUEST_TO_ENROLL = 0x3,
    SETTINGS_DELAY_TIME = 0x4,
    REGISTRAR_CONFIGURATION_METHODS = 0x5,
    MULTI_AP_IDENTIFIER = 0x6,
    MULTI_AP_PROFILE = 0x7,
    MULTI_AP_DEFAULT_802_1Q_SETTING = 0x8,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscWfaVendorExtSubelement_str(eWscWfaVendorExtSubelement enum_value) {
    switch (enum_value) {
    case VERSION2:                        return "VERSION2";
    case AUTHORIZED_MACS:                 return "AUTHORIZED_MACS";
    case NETWORK_KEY_SHAREABLE:           return "NETWORK_KEY_SHAREABLE";
    case REQUEST_TO_ENROLL:               return "REQUEST_TO_ENROLL";
    case SETTINGS_DELAY_TIME:             return "SETTINGS_DELAY_TIME";
    case REGISTRAR_CONFIGURATION_METHODS: return "REGISTRAR_CONFIGURATION_METHODS";
    case MULTI_AP_IDENTIFIER:             return "MULTI_AP_IDENTIFIER";
    case MULTI_AP_PROFILE:                return "MULTI_AP_PROFILE";
    case MULTI_AP_DEFAULT_802_1Q_SETTING: return "MULTI_AP_DEFAULT_802_1Q_SETTING";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscWfaVendorExtSubelement value) { return out << eWscWfaVendorExtSubelement_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCWFAVENDOREXTSUBELEMENT_H_
