/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bpl/bpl_board.h>

namespace beerocks {
namespace bpl {

bool get_board_info(sDeviceInfo &device_info_params)
{
    device_info_params.manufacturer       = "prplFoundation";
    device_info_params.serial_number      = "prpl12345";
    device_info_params.manufacturer_model = "Ubuntu";
    return true;
}

} // namespace bpl
} // namespace beerocks
