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

#ifndef _TLVF_WSC_EWSCCONN_H_
#define _TLVF_WSC_EWSCCONN_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscConn: uint8_t {
    WSC_CONN_ESS = 0x1,
    WSC_CONN_IBSS = 0x2,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscConn_str(eWscConn enum_value) {
    switch (enum_value) {
    case WSC_CONN_ESS:  return "WSC_CONN_ESS";
    case WSC_CONN_IBSS: return "WSC_CONN_IBSS";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscConn value) { return out << eWscConn_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCCONN_H_
