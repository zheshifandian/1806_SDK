/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_CMDU_UTILS_H_
#define _BEEROCKS_CMDU_UTILS_H_

#include <stddef.h>
#include <stdint.h>

namespace beerocks {

class CmduUtils {
public:
    /**
     * @brief Verifies if given byte array contains a valid CMDU.
     *
     * A valid CMDU message contains a CMDU header, 0 or more TLVs and an end-of-message TLV.
     *
     * @param data Array of bytes possibly containing a CMDU message.
     * @param length Number of bytes in the array.
     * @return true if given data contains a valid CMDU message and false otherwise.
     */
    static bool verify_cmdu(uint8_t *data, size_t length);
};

} //namespace beerocks

#endif //_BEEROCKS_CMDU_UTILS_H_
