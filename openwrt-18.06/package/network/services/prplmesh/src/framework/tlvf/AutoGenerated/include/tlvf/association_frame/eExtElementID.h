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

#ifndef _TLVF_ASSOCIATION_FRAME_EEXTELEMENTID_H_
#define _TLVF_ASSOCIATION_FRAME_EEXTELEMENTID_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>
enum eExtElementID: uint8_t {
    EXTID_HE_CAPABILITIES = 0x23,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eExtElementID_str(eExtElementID enum_value) {
    switch (enum_value) {
    case EXTID_HE_CAPABILITIES: return "EXTID_HE_CAPABILITIES";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eExtElementID value) { return out << eExtElementID_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


#endif //_TLVF/ASSOCIATION_FRAME_EEXTELEMENTID_H_
