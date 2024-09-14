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

#ifndef _BEEROCKS_TLVF_STRUCTS_SCACSTARTEDNOTIFICATIONPARAMS_H_
#define _BEEROCKS_TLVF_STRUCTS_SCACSTARTEDNOTIFICATIONPARAMS_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include "bcl/beerocks_defines.h"

namespace beerocks_message {

typedef struct sCacStartedNotificationParams {
    uint8_t channel;
    uint8_t secondary_channel;
    beerocks::eWiFiBandwidth bandwidth;
    uint16_t cac_duration_sec;
    void struct_swap(){
        tlvf_swap(8*sizeof(beerocks::eWiFiBandwidth), reinterpret_cast<uint8_t*>(&bandwidth));
        tlvf_swap(16, reinterpret_cast<uint8_t*>(&cac_duration_sec));
    }
    void struct_init(){
    }
} __attribute__((packed)) sCacStartedNotificationParams;


}; // close namespace: beerocks_message

#endif //_BEEROCKS/TLVF/STRUCTS_SCACSTARTEDNOTIFICATIONPARAMS_H_
