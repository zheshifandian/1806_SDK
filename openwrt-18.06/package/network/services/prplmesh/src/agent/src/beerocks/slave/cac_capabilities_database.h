/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef CAC_CAPABILITIES_DATABASE_H
#define CAC_CAPABILITIES_DATABASE_H

#include "cac_capabilities.h"

namespace beerocks {

/* @brief implementing CacCapabilities interface by reading 
 * and transforming the values from AgentDB
 * No initialization is required as the AgentDB is available from
 * anywhere.
 */
class CacCapabilitiesDatabase : public beerocks::CacCapabilities {
public:
    CacCapabilitiesDatabase() = default;
    const beerocks::CountryCode get_country_code() const override;
    std::vector<sMacAddr> get_cac_radios() const override;
    bool is_cac_method_supported(const sMacAddr &radio_mac,
                                 wfa_map::eCacMethod method) const override;
    uint32_t get_cac_completion_duration(const sMacAddr &radio_mac,
                                         wfa_map::eCacMethod method) const override;
    CacOperatingClasses get_cac_operating_classes(const sMacAddr &radio_mac,
                                                  wfa_map::eCacMethod method) const override;
};

} // namespace beerocks

#endif
