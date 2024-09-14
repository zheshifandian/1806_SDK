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

#ifndef _TLVF_IEEE_1905_1_ELINKMETRICSTYPE_H_
#define _TLVF_IEEE_1905_1_ELINKMETRICSTYPE_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace ieee1905_1 {

enum eLinkMetricsType: uint8_t {
    TX_LINK_METRICS_ONLY = 0x0,
    RX_LINK_METRICS_ONLY = 0x1,
    BOTH_TX_AND_RX_LINK_METRICS = 0x2,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eLinkMetricsType_str(eLinkMetricsType enum_value) {
    switch (enum_value) {
    case TX_LINK_METRICS_ONLY:        return "TX_LINK_METRICS_ONLY";
    case RX_LINK_METRICS_ONLY:        return "RX_LINK_METRICS_ONLY";
    case BOTH_TX_AND_RX_LINK_METRICS: return "BOTH_TX_AND_RX_LINK_METRICS";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eLinkMetricsType value) { return out << eLinkMetricsType_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end


}; // close namespace: ieee1905_1

#endif //_TLVF/IEEE_1905_1_ELINKMETRICSTYPE_H_
