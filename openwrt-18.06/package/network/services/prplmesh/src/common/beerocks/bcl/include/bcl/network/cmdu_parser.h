/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_CMDU_PARSER_H_
#define BCL_NETWORK_CMDU_PARSER_H_

#include <bcl/network/buffer.h>

#include <tlvf/CmduMessageRx.h>
#include <tlvf/common/sMacAddr.h>

namespace beerocks {
namespace net {

/**
 * This interface models a CMDU (Control Message Data Unit) parser. Implementations of this
 * interface will be used to parse CMDU messages out of an array of bytes received through a
 * socket connection.
 *
 * Different implementations of this interface can be provided, depending on if the socket used is
 * message-oriented or stream-oriented and, in the later case, which framing protocol is used to
 * delimiter the start and end of messages.
 *
 * Buffer contents can contain a full message or a fraction of it, depending on how many bytes were
 * ready for read when the recv() call on the socket was issued. Buffer contents might also contain
 * more than one message or one message and a fraction of the next one. Implementations should take
 * care of all possible scenarios.
 *
 * When calling the parsing method, if buffer contains a full CMDU, then CMDU parameter is filled
 * in and method returns true. This will be always the case when using a message-oriented socket
 * (SOCK_DGRAM in UDS or UDP).
 *
 * On the contrary, when using a stream-oriented socket (SOCK_STREAM in UDS or TCP), buffer might
 * not contain a full message. In this case, the parsing method must save given buffer contents as
 * a fragment of the full message and return false. The rest of the message is supposed to be
 * provided in next calls to the parsing method, but that is not granted and must be checked. A
 * more elaborated implementation might also check how much time has elapsed between the arrival
 * of the different fragments of a message and discard rotten ones.
 *
 * To deal with the case where buffer contains more than one message, parsing method shall be
 * called in a loop until it returns false. The buffer parameter is an in/out parameter and bytes
 * processed on each call are removed (shifted) before returning.
 *
 * This interface and its implementations allow the separation of the logic around sockets and the
 * logic around message parsing. The goal of this separation is to be able to test message parsing
 * without having a peer connected at the other end of the wire sending messages. During testing,
 * we can fill the buffer with any contents we want and pretend that those bytes have been
 * received through the socket connection. This way it is possible to test the parser with very odd
 * combinations of data that would otherwise be very difficult to set and reproduce. The only limit
 * for the tests we can write is our imagination.
 */
class CmduParser {
public:
    /**
     * @brief Class destructor
     */
    virtual ~CmduParser() = default;

    /**
     * @brief Parses a CMDU out of a byte buffer.
     *
     * @param[in,out] buffer Array of bytes containing the message to parse. On output, bytes not
     * yet processed.
     * @param[out] iface_index Index of the network interface that the message was received on.
     * @param[out] dst_mac Destination MAC address.
     * @param[out] src_mac Source MAC address.
     * @param[out] cmdu_rx Parsed CMDU message.
     * @return true if a CMDU could be parsed out of the buffer and false otherwise (i.e.: because
     * an incomplete or invalid CMDU was found).
     */
    virtual bool parse_cmdu(Buffer &buffer, uint32_t &iface_index, sMacAddr &dst_mac,
                            sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx) = 0;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_CMDU_PARSER_H_ */
