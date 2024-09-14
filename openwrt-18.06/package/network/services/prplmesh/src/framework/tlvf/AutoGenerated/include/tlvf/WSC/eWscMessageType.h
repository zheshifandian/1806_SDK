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

#ifndef _TLVF_WSC_EWSCMESSAGETYPE_H_
#define _TLVF_WSC_EWSCMESSAGETYPE_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscMessageType: uint8_t {
    WSC_MSG_TYPE_M1 = 0x4,
    WSC_MSG_TYPE_M2 = 0x5,
    WSC_MSG_TYPE_INVALID = 0xff,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscMessageType_str(eWscMessageType enum_value) {
    switch (enum_value) {
    case WSC_MSG_TYPE_M1:      return "WSC_MSG_TYPE_M1";
    case WSC_MSG_TYPE_M2:      return "WSC_MSG_TYPE_M2";
    case WSC_MSG_TYPE_INVALID: return "WSC_MSG_TYPE_INVALID";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscMessageType value) { return out << eWscMessageType_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCMESSAGETYPE_H_
