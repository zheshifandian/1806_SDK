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

#ifndef _TLVF_IEEE_1905_1_ELINKMETRICNEIGHBORTYPE_H_
#define _TLVF_IEEE_1905_1_ELINKMETRICNEIGHBORTYPE_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace ieee1905_1 {

enum eLinkMetricNeighborType: uint8_t {
    ALL_NEIGHBORS = 0x0,
    SPECIFIC_NEIGHBOR = 0x1,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eLinkMetricNeighborType_str(eLinkMetricNeighborType enum_value) {
    switch (enum_value) {
    case ALL_NEIGHBORS:     return "ALL_NEIGHBORS";
    case SPECIFIC_NEIGHBOR: return "SPECIFIC_NEIGHBOR";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eLinkMetricNeighborType value) { return out << eLinkMetricNeighborType_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: ieee1905_1

#endif //_TLVF/IEEE_1905_1_ELINKMETRICNEIGHBORTYPE_H_
