/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_CMDU_PARSER_STREAM_IMPL_H_
#define BCL_NETWORK_CMDU_PARSER_STREAM_IMPL_H_

#include <bcl/network/cmdu_parser.h>

#include <bcl/beerocks_message_structs.h>

namespace beerocks {
namespace net {

class CmduParserStreamImpl : public CmduParser {
public:
    /**
     * @brief Parses a CMDU out of a byte buffer.
     *
     * @see CmduParser::parse_cmdu
     *
     * This implementation is intended to be used together with a stream-oriented socket
     * (SOCK_STREAM in UDS or TCP). Parsing procedure takes into account that buffer with received
     * data might not contain a full message or might contain more than one message.
     *
     * The framing protocol used in this implementation is a variable-sized framing protocol. Each
     * frame is made of a header plus a payload. The header includes a length field that
     * determines the size of the frame. Other fields in the header are the index of the network
     * interface that the message was received on, the destination MAC address and the source MAC
     * address (@see beerocks::message::sUdsHeader). The payload is the CMDU "as is".
     */
    bool parse_cmdu(Buffer &buffer, uint32_t &iface_index, sMacAddr &dst_mac, sMacAddr &src_mac,
                    ieee1905_1::CmduMessageRx &cmdu_rx) override;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_CMDU_PARSER_STREAM_IMPL_H_ */
