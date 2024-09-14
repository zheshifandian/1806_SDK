/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_UCC_PARSER_STREAM_IMPL_H_
#define _BEEROCKS_UCC_PARSER_STREAM_IMPL_H_

#include <bcl/beerocks_ucc_parser.h>

namespace beerocks {

class UccParserStreamImpl : public UccParser {
public:
    /**
     * @brief Parses a UCC command string out of a byte buffer.
     *
     * @see UccParser::parse_command
     *
     * This implementation is intended to be used together with a stream-oriented socket
     * (SOCK_STREAM in UDS or TCP). Parsing procedure takes into account that buffer with received
     * data might not contain a full message or might contain more than one message.
     *
     * The framing protocol used in this implementation consists of a Line Feed ("LF") character
     * (0x0A, \n) added at the end of the command.
     */
    bool parse_command(beerocks::net::Buffer &buffer, std::string &command) override;
};

} // namespace beerocks

#endif /* _BEEROCKS_UCC_PARSER_STREAM_IMPL_H_ */
