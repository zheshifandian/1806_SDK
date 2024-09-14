/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BPL_BOARD_H_
#define _BPL_BOARD_H_

#include "bpl.h"

#include <string>

namespace beerocks {
namespace bpl {

/****************************************************************************/
/******************************* Definitions ********************************/
/****************************************************************************/

struct sDeviceInfo {
    std::string manufacturer;
    std::string serial_number;
    std::string manufacturer_model;
};

/****************************************************************************/
/******************************** Functions *********************************/
/****************************************************************************/

/**
 * @brief Get board info parameters.
 *
 * @param [out] device_info_params structure only with manufacturer/vendor and S/N parameters.
 * @return Returns true in case of success.
 */
bool get_board_info(sDeviceInfo &device_info_params);

} // namespace bpl
} // namespace beerocks

#endif // _BPL_BOARD_H_
