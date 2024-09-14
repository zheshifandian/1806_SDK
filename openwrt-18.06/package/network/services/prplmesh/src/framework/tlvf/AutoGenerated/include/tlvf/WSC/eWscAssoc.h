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

#ifndef _TLVF_WSC_EWSCASSOC_H_
#define _TLVF_WSC_EWSCASSOC_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscAssoc: uint16_t {
    WSC_ASSOC_NOT_ASSOC = 0x0,
    WSC_ASSOC_CONN_SUCCESS = 0x1,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscAssoc_str(eWscAssoc enum_value) {
    switch (enum_value) {
    case WSC_ASSOC_NOT_ASSOC:    return "WSC_ASSOC_NOT_ASSOC";
    case WSC_ASSOC_CONN_SUCCESS: return "WSC_ASSOC_CONN_SUCCESS";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscAssoc value) { return out << eWscAssoc_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCASSOC_H_
