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

#ifndef _TLVF_IEEE_1905_1_EMEDIATYPE_H_
#define _TLVF_IEEE_1905_1_EMEDIATYPE_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace ieee1905_1 {

enum eMediaType: uint16_t {
    IEEE_802_3U_FAST_ETHERNET = 0x0,
    IEEE_802_3AB_GIGABIT_ETHERNET = 0x1,
    IEEE_802_11B_2_4_GHZ = 0x100,
    IEEE_802_11G_2_4_GHZ = 0x101,
    IEEE_802_11A_5_GHZ = 0x102,
    IEEE_802_11N_2_4_GHZ = 0x103,
    IEEE_802_11N_5_GHZ = 0x104,
    IEEE_802_11AC_5_GHZ = 0x105,
    IEEE_802_11AD_60_GHZ = 0x106,
    IEEE_802_11AF = 0x107,
    IEEE_802_11AX = 0x108,
    IEEE_1901_WAVELET = 0x200,
    IEEE_1901_FFT = 0x201,
    MOCA_V1_1 = 0x300,
    UNKNOWN_MEDIA = 0xffff,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eMediaType_str(eMediaType enum_value) {
    switch (enum_value) {
    case IEEE_802_3U_FAST_ETHERNET:     return "IEEE_802_3U_FAST_ETHERNET";
    case IEEE_802_3AB_GIGABIT_ETHERNET: return "IEEE_802_3AB_GIGABIT_ETHERNET";
    case IEEE_802_11B_2_4_GHZ:          return "IEEE_802_11B_2_4_GHZ";
    case IEEE_802_11G_2_4_GHZ:          return "IEEE_802_11G_2_4_GHZ";
    case IEEE_802_11A_5_GHZ:            return "IEEE_802_11A_5_GHZ";
    case IEEE_802_11N_2_4_GHZ:          return "IEEE_802_11N_2_4_GHZ";
    case IEEE_802_11N_5_GHZ:            return "IEEE_802_11N_5_GHZ";
    case IEEE_802_11AC_5_GHZ:           return "IEEE_802_11AC_5_GHZ";
    case IEEE_802_11AD_60_GHZ:          return "IEEE_802_11AD_60_GHZ";
    case IEEE_802_11AF:                 return "IEEE_802_11AF";
    case IEEE_802_11AX:                 return "IEEE_802_11AX";
    case IEEE_1901_WAVELET:             return "IEEE_1901_WAVELET";
    case IEEE_1901_FFT:                 return "IEEE_1901_FFT";
    case MOCA_V1_1:                     return "MOCA_V1_1";
    case UNKNOWN_MEDIA:                 return "UNKNOWN_MEDIA";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eMediaType value) { return out << eMediaType_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end

enum eMediaTypeGroup: uint8_t {
    IEEE_802_3 = 0x0,
    IEEE_802_11 = 0x1,
    IEEE_1901 = 0x2,
    MoCA = 0x3,
    UNKNOWN = 0xff,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eMediaTypeGroup_str(eMediaTypeGroup enum_value) {
    switch (enum_value) {
    case IEEE_802_3:  return "IEEE_802_3";
    case IEEE_802_11: return "IEEE_802_11";
    case IEEE_1901:   return "IEEE_1901";
    case MoCA:        return "MoCA";
    case UNKNOWN:     return "UNKNOWN";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eMediaTypeGroup value) { return out << eMediaTypeGroup_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: ieee1905_1

#endif //_TLVF/IEEE_1905_1_EMEDIATYPE_H_
