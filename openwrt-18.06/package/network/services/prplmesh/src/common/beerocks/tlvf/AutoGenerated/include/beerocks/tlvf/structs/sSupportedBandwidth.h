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

#ifndef _BEEROCKS_TLVF_STRUCTS_SSUPPORTEDBANDWIDTH_H_
#define _BEEROCKS_TLVF_STRUCTS_SSUPPORTEDBANDWIDTH_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include "bcl/beerocks_defines.h"

namespace beerocks_message {

typedef struct sSupportedBandwidth {
    beerocks::eWiFiBandwidth bandwidth;
    //0 = Best Rank, INT32_MAX = Worst Rank, -1 = Undefined/Not Operable.
    int32_t rank;
    //The rank converted to preference value:
    //0  - Unusable (e.g due to country code limitation).
    //1  - Lowest preference.
    //14 - Highest preference.
    uint8_t multiap_preference;
    void struct_swap(){
        tlvf_swap(8*sizeof(beerocks::eWiFiBandwidth), reinterpret_cast<uint8_t*>(&bandwidth));
        tlvf_swap(32, reinterpret_cast<uint8_t*>(&rank));
    }
    void struct_init(){
        rank = -0x1;
        multiap_preference = 0x0;
    }
} __attribute__((packed)) sSupportedBandwidth;


}; // close namespace: beerocks_message

#endif //_BEEROCKS/TLVF/STRUCTS_SSUPPORTEDBANDWIDTH_H_
