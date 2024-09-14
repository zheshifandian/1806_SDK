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

#ifndef _TLVF_WSC_EWSCRFBANDS_H_
#define _TLVF_WSC_EWSCRFBANDS_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace WSC {

enum eWscRfBands: uint8_t {
    WSC_RF_BAND_2GHZ = 0x1,
    WSC_RF_BAND_5GHZ = 0x2,
    WSC_RF_BAND_60GHZ = 0x4,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eWscRfBands_str(eWscRfBands enum_value) {
    switch (enum_value) {
    case WSC_RF_BAND_2GHZ:  return "WSC_RF_BAND_2GHZ";
    case WSC_RF_BAND_5GHZ:  return "WSC_RF_BAND_5GHZ";
    case WSC_RF_BAND_60GHZ: return "WSC_RF_BAND_60GHZ";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eWscRfBands value) { return out << eWscRfBands_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: WSC

#endif //_TLVF/WSC_EWSCRFBANDS_H_
