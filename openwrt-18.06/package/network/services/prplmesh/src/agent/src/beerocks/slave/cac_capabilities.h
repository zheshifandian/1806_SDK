/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _CAC_CAPABILITIES_H_
#define _CAC_CAPABILITIES_H_

#include <tlvf/common/sMacAddr.h>
#include <tlvf/wfa_map/tlvProfile2CacCapabilities.h>

#include <map>
#include <vector>

namespace beerocks {

// 2 letters country code (see ISO-3166)
using CountryCode = std::array<char, 2>;

/**
 * @brief Interface to query cac capabilities of an agent
 * this is just the interface, implementation may get this information
 * from various sources, such as: database, driver, etc.
 */
class CacCapabilities {
public:
    virtual ~CacCapabilities() = default;

    /**
     * @brief iso 3166 country code this agent is running at.
     * @return a CountryCode structure
     */
    virtual const CountryCode get_country_code() const = 0;

    /**
     * @brief get the radios that can perform cac
     * @return a vector of rdios (mac) that may perform cac
     */
    virtual std::vector<sMacAddr> get_cac_radios() const = 0;

    /**
     * @brief indication about specific cac method support for 
     * specific radio
     * @param radio the mac of the radio in question
     * @param cac_method the cac method in question
     * @return true if the requested cac is supported for this radio
     * false otherwise (including wrong radio for example)
     */
    virtual bool is_cac_method_supported(const sMacAddr &radio,
                                         wfa_map::eCacMethod cac_method) const = 0;

    /**
     * @brief cac duration
     * @param radio the mac of the radio in question
     * @param cac-method the cac method in quesiton
     * @return seconds the number of seconds it takes for the
     * given radio to complete cac scan using the given method
     */
    virtual uint32_t get_cac_completion_duration(const sMacAddr &radio,
                                                 wfa_map::eCacMethod cac_method) const = 0;

    /**
     * @brief operating classes and channels
     * @param radio the mac of the radio in question
     * @param cac-method the cac method in quesiton
     * @return a map: key - operating class, value: array of channles
     * for given radio and given method
     */
    using CacOperatingClasses = std::map<uint8_t, std::vector<uint8_t>>;
    virtual CacOperatingClasses get_cac_operating_classes(const sMacAddr &radio,
                                                          wfa_map::eCacMethod cac_method) const = 0;
};

// utilities based on CacCapabilities interface

/**
  * @brief a structure that holds for each radio its cac supported methods
  * an empty vector indicates that cac is not supported at all
  */
using CacMethodForRadio = std::pair<sMacAddr, std::vector<wfa_map::eCacMethod>>;

/**
 * @brief fill radio to method capabilities structure
 * @param Cac capabilities object
 * @return CacMethodForRadio see above
 */
CacMethodForRadio get_radio_cac_methods(const CacCapabilities &capabilities, const sMacAddr &radio);

} // namespace beerocks

#endif
