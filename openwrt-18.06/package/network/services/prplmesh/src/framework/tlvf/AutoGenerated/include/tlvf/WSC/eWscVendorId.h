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

#ifndef _TLVF_WSC_EWSCVENDORID_H_
#define _TLVF_WSC_EWSCVENDORID_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscVendorId {
    WSC_VENDOR_ID_WFA_1 = 0x0,
    WSC_VENDOR_ID_WFA_2 = 0x37,
    WSC_VENDOR_ID_WFA_3 = 0x2a,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscVendorId_str(eWscVendorId enum_value) {
    switch (enum_value) {
    case WSC_VENDOR_ID_WFA_1: return "WSC_VENDOR_ID_WFA_1";
    case WSC_VENDOR_ID_WFA_2: return "WSC_VENDOR_ID_WFA_2";
    case WSC_VENDOR_ID_WFA_3: return "WSC_VENDOR_ID_WFA_3";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscVendorId value) { return out << eWscVendorId_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCVENDORID_H_
