/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BTL_MESSAGE_PARSER_STREAM_IMPL_H_
#define BTL_MESSAGE_PARSER_STREAM_IMPL_H_

#include <btl/message_parser.h>

namespace beerocks {
namespace btl {

class MessageParserStreamImpl : public MessageParser {
public:
    /**
     * @brief Parses a transport message out of a byte buffer.
     *
     * @see MessageParser::parse_message
     *
     * This implementation is intended to be used together with a stream-oriented socket
     * (SOCK_STREAM in UDS or TCP). Parsing procedure takes into account that buffer with received
     * data might not contain a full message or might contain more than one message.
     *
     * The framing protocol used in this implementation is a variable-sized framing protocol. Each
     * frame is made of a header plus a payload and the header includes a length field that
     * determines the size of the payload (@see beerocks::transport::messages::Message::Header).
     */
    std::unique_ptr<beerocks::transport::messages::Message>
    parse_message(beerocks::net::Buffer &buffer) override;
};

} // namespace btl
} // namespace beerocks

#endif /* BTL_MESSAGE_PARSER_STREAM_IMPL_H_ */
