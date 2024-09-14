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

#ifndef _TLVF_WSC_EWSCENCR_H_
#define _TLVF_WSC_EWSCENCR_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum class eWscEncr : uint16_t {
    WSC_ENCR_NONE = 0x1,
    WSC_ENCR_WEP = 0x2,
    WSC_ENCR_TKIP = 0x4,
    WSC_ENCR_AES = 0x8,
    WSC_ENCR_INVALID = 0xffff,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscEncr_str(eWscEncr enum_value) {
    switch (enum_value) {
    case eWscEncr::WSC_ENCR_NONE:    return "eWscEncr::WSC_ENCR_NONE";
    case eWscEncr::WSC_ENCR_WEP:     return "eWscEncr::WSC_ENCR_WEP";
    case eWscEncr::WSC_ENCR_TKIP:    return "eWscEncr::WSC_ENCR_TKIP";
    case eWscEncr::WSC_ENCR_AES:     return "eWscEncr::WSC_ENCR_AES";
    case eWscEncr::WSC_ENCR_INVALID: return "eWscEncr::WSC_ENCR_INVALID";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscEncr value) { return out << eWscEncr_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end
class eWscEncrValidate {
public:
    static bool check(uint16_t value) {
        bool ret = false;
        switch (value) {
        case 0x1:
        case 0x2:
        case 0x4:
        case 0x8:
        case 0xffff:
                ret = true;
                break;
            default:
                ret = false;
                break;
        }
        return ret;
    }
};


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCENCR_H_
