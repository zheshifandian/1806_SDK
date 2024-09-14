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

#ifndef _BEEROCKS_TLVF_ENUMS_EDFSSTATE_H_
#define _BEEROCKS_TLVF_ENUMS_EDFSSTATE_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <ostream>

namespace beerocks_message {

enum eDfsState: uint8_t {
    USABLE = 0x0,
    UNAVAILABLE = 0x1,
    AVAILABLE = 0x2,
    NOT_DFS = 0x3,
};


}; // close namespace: beerocks_message

#endif //_BEEROCKS/TLVF/ENUMS_EDFSSTATE_H_
