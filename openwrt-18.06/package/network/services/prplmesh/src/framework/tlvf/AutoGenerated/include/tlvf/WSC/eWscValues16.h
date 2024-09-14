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

#ifndef _TLVF_WSC_EWSCVALUES16_H_
#define _TLVF_WSC_EWSCVALUES16_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscValues16: uint16_t {
    WSC_CONFIG_VIRT_PUSHBUTTON = 0x280,
    WSC_CONFIG_PHY_PUSHBUTTON = 0x480,
    DEV_PW_PUSHBUTTON = 0x4,
    WSC_CFG_NO_ERROR = 0x0,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscValues16_str(eWscValues16 enum_value) {
    switch (enum_value) {
    case WSC_CONFIG_VIRT_PUSHBUTTON: return "WSC_CONFIG_VIRT_PUSHBUTTON";
    case WSC_CONFIG_PHY_PUSHBUTTON:  return "WSC_CONFIG_PHY_PUSHBUTTON";
    case DEV_PW_PUSHBUTTON:          return "DEV_PW_PUSHBUTTON";
    case WSC_CFG_NO_ERROR:           return "WSC_CFG_NO_ERROR";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscValues16 value) { return out << eWscValues16_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCVALUES16_H_
