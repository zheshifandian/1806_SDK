/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_CMDU_PARSER_MOCK_H_
#define BCL_NETWORK_CMDU_PARSER_MOCK_H_

#include <bcl/network/cmdu_parser.h>

#include <gmock/gmock.h>

namespace beerocks {
namespace net {

class CmduParserMock : public CmduParser {
public:
    MOCK_METHOD(bool, parse_cmdu,
                (Buffer & buffer, uint32_t &iface_index, sMacAddr &dst_mac, sMacAddr &src_mac,
                 ieee1905_1::CmduMessageRx &cmdu_rx),
                (override));
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_CMDU_PARSER_MOCK_H_ */
