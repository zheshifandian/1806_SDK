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

#ifndef _TLVF_IEEE_1905_1_S802_11SPECIFICINFORMATION_H_
#define _TLVF_IEEE_1905_1_S802_11SPECIFICINFORMATION_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>
#include "tlvf/common/sMacAddr.h"

namespace ieee1905_1 {

enum eRole: uint8_t {
    AP = 0x0,
    NON_AP_NON_PCP_STA = 0x40,
    WI_FI_P2P_CLIENT = 0x80,
    WI_FI_P2P_GROUP_OWNER = 0x90,
    IEEE_802_11AD_PCP = 0xa0,
};
// Enum AutoPrint generated code snippet begining- DON'T EDIT!
// clang-format off
static const char *eRole_str(eRole enum_value) {
    switch (enum_value) {
    case AP:                    return "AP";
    case NON_AP_NON_PCP_STA:    return "NON_AP_NON_PCP_STA";
    case WI_FI_P2P_CLIENT:      return "WI_FI_P2P_CLIENT";
    case WI_FI_P2P_GROUP_OWNER: return "WI_FI_P2P_GROUP_OWNER";
    case IEEE_802_11AD_PCP:     return "IEEE_802_11AD_PCP";
    }
    static std::string out_str = std::to_string(int(enum_value));
    return out_str.c_str();
}
inline std::ostream &operator<<(std::ostream &out, eRole value) { return out << eRole_str(value); }
// clang-format on
// Enum AutoPrint generated code snippet end

typedef struct s802_11SpecificInformation {
    sMacAddr network_membership;
    eRole role;
    //Hex value of dot11CurrentChannelBandwidth
    uint8_t ap_channel_bandwidth;
    //Hex value of dot11CurrentChannelCenterFrequencyIndex1
    uint8_t ap_channel_center_frequency_index1;
    //Hex value of dot11CurrentChannelCenterFrequencyIndex2
    uint8_t ap_channel_center_frequency_index2;
    void struct_swap(){
        network_membership.struct_swap();
        tlvf_swap(8*sizeof(eRole), reinterpret_cast<uint8_t*>(&role));
    }
    void struct_init(){
        network_membership.struct_init();
    }
} __attribute__((packed)) s802_11SpecificInformation;


}; // close namespace: ieee1905_1

#endif //_TLVF/IEEE_1905_1_S802_11SPECIFICINFORMATION_H_
