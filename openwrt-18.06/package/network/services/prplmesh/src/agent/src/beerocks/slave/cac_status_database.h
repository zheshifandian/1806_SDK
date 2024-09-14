/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _CAC_STAUS_DATABASE_H_
#define _CAC_STAUS_DATABASE_H_

#include "cac_status_interface.h"

namespace beerocks {

class CacStatusDatabase : public CacStatusInterface {
public:
    CacAvailableChannels get_available_channels(const sMacAddr &radio) const override;
    CacCompletionStatus get_completion_status(const sMacAddr &radio) const override;
};

// utilities based on CacCapabilities interface

} // namespace beerocks

#endif
