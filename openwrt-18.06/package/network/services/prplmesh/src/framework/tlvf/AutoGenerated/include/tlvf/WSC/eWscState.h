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

#ifndef _TLVF_WSC_EWSCSTATE_H_
#define _TLVF_WSC_EWSCSTATE_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscState: uint8_t {
    WSC_STATE_NOT_CONFIGURED = 0x1,
    WSC_STATE_CONFIGURED = 0x2,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscState_str(eWscState enum_value) {
    switch (enum_value) {
    case WSC_STATE_NOT_CONFIGURED: return "WSC_STATE_NOT_CONFIGURED";
    case WSC_STATE_CONFIGURED:     return "WSC_STATE_CONFIGURED";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscState value) { return out << eWscState_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCSTATE_H_
