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

#ifndef _TLVF_WSC_EWSCVENDOREXT_H_
#define _TLVF_WSC_EWSCVENDOREXT_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscVendorExtSubelementBssType {
    BACKHAUL_STA = 0x80,
    BACKHAUL_BSS = 0x40,
    FRONTHAUL_BSS = 0x20,
    TEARDOWN = 0x10,
    PROFILE1_BACKHAUL_STA_ASSOCIATION_DISALLOWED = 0x8,
    PROFILE2_BACKHAUL_STA_ASSOCIATION_DISALLOWED = 0x4,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscVendorExtSubelementBssType_str(eWscVendorExtSubelementBssType enum_value) {
    switch (enum_value) {
    case BACKHAUL_STA:                                 return "BACKHAUL_STA";
    case BACKHAUL_BSS:                                 return "BACKHAUL_BSS";
    case FRONTHAUL_BSS:                                return "FRONTHAUL_BSS";
    case TEARDOWN:                                     return "TEARDOWN";
    case PROFILE1_BACKHAUL_STA_ASSOCIATION_DISALLOWED: return "PROFILE1_BACKHAUL_STA_ASSOCIATION_DISALLOWED";
    case PROFILE2_BACKHAUL_STA_ASSOCIATION_DISALLOWED: return "PROFILE2_BACKHAUL_STA_ASSOCIATION_DISALLOWED";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscVendorExtSubelementBssType value) { return out << eWscVendorExtSubelementBssType_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end

enum eWscVendorExtVersionIE {
    WSC_VERSION2 = 0x20,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscVendorExtVersionIE_str(eWscVendorExtVersionIE enum_value) {
    switch (enum_value) {
    case WSC_VERSION2: return "WSC_VERSION2";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscVendorExtVersionIE value) { return out << eWscVendorExtVersionIE_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCVENDOREXT_H_
