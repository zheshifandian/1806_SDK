/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_UCC_PARSER_MOCK_H_
#define _BEEROCKS_UCC_PARSER_MOCK_H_

#include <bcl/beerocks_ucc_parser.h>

#include <gmock/gmock.h>

namespace beerocks {

class UccParserMock : public UccParser {
public:
    MOCK_METHOD(bool, parse_command, (beerocks::net::Buffer & buffer, std::string &command),
                (override));
};

} // namespace beerocks

#endif /* _BEEROCKS_UCC_PARSER_MOCK_H_ */
