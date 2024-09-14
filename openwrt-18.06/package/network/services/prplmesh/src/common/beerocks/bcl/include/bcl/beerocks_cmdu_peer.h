/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_CMDU_PEER_H_
#define _BEEROCKS_CMDU_PEER_H_

#include <bcl/network/buffer.h>
#include <bcl/network/cmdu_parser.h>
#include <bcl/network/cmdu_serializer.h>
#include <bcl/network/sockets.h>

#include <tlvf/CmduMessageRx.h>
#include <tlvf/CmduMessageTx.h>
#include <tlvf/common/sMacAddr.h>

namespace beerocks {

/**
 * @brief The CMDU peer class is used by @see CmduServerImpl and @see CmduClientImpl classes.
 * It contains the methods common to both classes to send and receive CMDU messages through a socket
 * connection, using the frame protocol defined by a CmduParser and CmduSerializer.
 */
class CmduPeer {
public:
    /**
     * @brief CMDU-received event handler function.
     *
     * Note: parameters iface_index, dst_mac and src_mac are only filled in if CMDU was originally
     * sent by a remote process and then forwarded by the local process that receives it to this
     * process.
     *
     * @param connection Socket connection the CMDU was received through.
     * @param iface_index Index of the network interface that the CMDU message was received on.
     * @param dst_mac Destination MAC address.
     * @param src_mac Source MAC address.
     * @param cmdu_rx The CMDU message received.
     */
    using CmduReceivedHandler = std::function<bool(
        beerocks::net::Socket::Connection &connection, uint32_t iface_index,
        const sMacAddr &dst_mac, const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx)>;

    /**
     * @brief Class constructor.
     *
     * @param cmdu_parser CMDU parser used to get CMDU messages out of a byte array received
     * through a socket connection.
     * @param cmdu_serializer CMDU serializer used to put CMDU messages into a byte array ready to
     * be sent through a socket connection.
     */
    CmduPeer(std::shared_ptr<beerocks::net::CmduParser> cmdu_parser,
             std::shared_ptr<beerocks::net::CmduSerializer> cmdu_serializer);

    /**
     * @brief Sends a CMDU message.
     *
     * Sends a CMDU message to a remote peer through the given socket connection. Uses the CMDU
     * message serializer provided in constructor to serialize data to send.
     *
     * @param connection Socket connection to send CMDU through.
     * @param cmdu_tx The CMDU message to send.
     * @return true on success and false otherwise.
     */
    bool send_cmdu(beerocks::net::Socket::Connection &connection,
                   ieee1905_1::CmduMessageTx &cmdu_tx) const;

    /**
     * @brief Forwards a CMDU message that was sent by a remote process.
     *
     * Forwards a received CMDU message to a client through the given socket connection.
     *
     * The CMDU message was originally sent by a remote process running in a different device
     * (interface index, source and destination MAC addresses provide routing information).
     *
     * @param connection Socket connection to send CMDU through.
     * @param iface_index Index of the network interface that the CMDU message was received on.
     * @param dst_mac Destination MAC address.
     * @param src_mac Source MAC address.
     * @param cmdu_rx The received CMDU message to forward.
     * @return true on success and false otherwise.
     */
    bool forward_cmdu(beerocks::net::Socket::Connection &connection, uint32_t iface_index,
                      const sMacAddr &dst_mac, const sMacAddr &src_mac,
                      ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Reads and handles received CMDU messages.
     *
     * Reads data received through the given socket connection into the buffer. Uses the CMDU
     * message parser provided in constructor to parse received data. If a full and valid CMDU has
     * been received, calls back given handler to process it. If not all bytes in a frame have been
     * received yet (when using a stream-oriented socket connection), then bytes received but not
     * processed are returned in given buffer. Given buffer can contain the bytes received but not
     * processed in previous call and then received data is appended to the buffer.
     *
     * @param connection Socket connection to receive CMDU messages through.
     * @param[in,out] buffer The buffer to hold received data. On return, bytes received but not
     * processed.
     * @param handler Handler function to call back to process received CMDU messages. When using a
     * stream-oriented socket connection, more than one CMDU might have been received. The handler
     * is called back once for each CMDU received.
     */
    void receive_cmdus(beerocks::net::Socket::Connection &connection, beerocks::net::Buffer &buffer,
                       const CmduReceivedHandler &handler);

private:
    /**
     * CMDU parser used to get CMDU messages out of a byte array received through a socket
     * connection.
     */
    std::shared_ptr<beerocks::net::CmduParser> m_cmdu_parser;

    /**
     * CMDU serializer used to put CMDU messages into a byte array ready to be sent through a
     * socket connection.
     */
    std::shared_ptr<beerocks::net::CmduSerializer> m_cmdu_serializer;
};

} // namespace beerocks

#endif // _BEEROCKS_CMDU_PEER_H_
