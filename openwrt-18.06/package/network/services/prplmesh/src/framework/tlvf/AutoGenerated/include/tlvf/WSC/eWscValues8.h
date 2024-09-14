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

#ifndef _TLVF_WSC_EWSCVALUES8_H_
#define _TLVF_WSC_EWSCVALUES8_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscValues8: uint8_t {
    WSC_VERSION = 0x10,
    WFA_ELEM_VERSION2 = 0x0,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscValues8_str(eWscValues8 enum_value) {
    switch (enum_value) {
    case WSC_VERSION:       return "WSC_VERSION";
    case WFA_ELEM_VERSION2: return "WFA_ELEM_VERSION2";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscValues8 value) { return out << eWscValues8_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCVALUES8_H_
