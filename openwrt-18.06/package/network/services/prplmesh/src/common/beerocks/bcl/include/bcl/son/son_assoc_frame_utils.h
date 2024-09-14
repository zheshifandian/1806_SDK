/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _SON_ASSOC_FRAME_UTILS_H_
#define _SON_ASSOC_FRAME_UTILS_H_

#include "../beerocks_defines.h"
#include "../beerocks_message_structs.h"

#include <tlvf/AssociationRequestFrame/AssocReqFrame.h>

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace son {
class assoc_frame_utils {
public:
    /**
     * @brief Get station capabilities from provided assocReqFrame object.
     *
     * @param source Association request frame object, parsed from received raw data.
     * @param sta_caps Resulting station capabilities.
     * @return True if parsing succeeded, false otherwise.
     */
    static bool get_station_capabilities_from_assoc_frame(
        const std::shared_ptr<assoc_frame::AssocReqFrame> &source,
        beerocks::message::sRadioCapabilities &sta_caps);

private:
    /**
     * @brief Template function to parse (/convert) provided assoc req field data
     * into station capabilities.
     *
     * @param source Data to be parsed.
     * All supported source types are handled in private full template specialization.
     * @param result to be filled.
     * @return True if parsing succeeded, false otherwise (eg: source type can not be parsed).
     */
    template <typename T>
    static bool
    get_station_capabilities_from_assoc_field(const T &source,
                                              beerocks::message::sRadioCapabilities &result)
    {
        return false;
    }

    /**
     * @brief Set default mcs and default short GI from station's supported rates
     * (by getting nearest reference value for rate[max_supp_rate]/bw[20MHz]/nss[1]).
     *
     * @param supp_rates List of supported rates as available in assocReq data.
     * @param default_mcs Resulting default mcs.
     * @param default_short_gi Resulting default short GI flag.
     * @return void.
     */
    static void get_default_mcs_from_supported_rates(const std::vector<uint8_t> &supp_rates,
                                                     uint8_t &default_mcs,
                                                     uint8_t &default_short_gi);
};
} // namespace son

#endif
