/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_cmdu_peer.h>

#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_utils.h>
#include <bcl/network/buffer_impl.h>
#include <bcl/network/network_utils.h>

#include <easylogging++.h>

namespace beerocks {

CmduPeer::CmduPeer(std::shared_ptr<beerocks::net::CmduParser> cmdu_parser,
                   std::shared_ptr<beerocks::net::CmduSerializer> cmdu_serializer)
    : m_cmdu_parser(cmdu_parser), m_cmdu_serializer(cmdu_serializer)
{
    LOG_IF(!m_cmdu_parser, FATAL) << "CMDU parser is a null pointer!";
    LOG_IF(!m_cmdu_serializer, FATAL) << "CMDU serializer is a null pointer!";
}

bool CmduPeer::send_cmdu(beerocks::net::Socket::Connection &connection,
                         ieee1905_1::CmduMessageTx &cmdu_tx) const
{
    // Finalize CMDU
    size_t cmdu_length = cmdu_tx.getMessageLength();
    uint8_t *cmdu_data = cmdu_tx.getMessageBuff();
    if (!cmdu_tx.finalize()) {
        LOG(ERROR) << "Failed finalizing cmdu!";
        LOG(DEBUG) << "hex_dump (" << cmdu_length << " bytes):" << std::endl
                   << utils::dump_buffer(cmdu_data, cmdu_length);
        return false;
    }

    // Serialize CMDU into a byte array
    uint32_t iface_index = 0;
    sMacAddr dst_mac     = beerocks::net::network_utils::ZERO_MAC;
    sMacAddr src_mac     = beerocks::net::network_utils::ZERO_MAC;
    beerocks::net::BufferImpl<message::MESSAGE_BUFFER_LENGTH> buffer;
    if (!m_cmdu_serializer->serialize_cmdu(iface_index, dst_mac, src_mac, cmdu_tx, buffer)) {
        LOG(ERROR) << "Failed to serialize CMDU! fd = " << connection.socket()->fd();
        return false;
    }

    // Send data
    return connection.send(buffer) > 0;
}

bool CmduPeer::forward_cmdu(beerocks::net::Socket::Connection &connection, uint32_t iface_index,
                            const sMacAddr &dst_mac, const sMacAddr &src_mac,
                            ieee1905_1::CmduMessageRx &cmdu_rx)
{
    // Swap bytes before forwarding, from host to network byte order.
    cmdu_rx.swap();

    // Use a shared_ptr with a custom deleter and the RAII programming idiom to emulate the
    // `finally` block of a `try-finally` clause.
    std::shared_ptr<int> finally(nullptr, [&cmdu_rx](int *p) {
        // Swap bytes back from network to host byte order after forwarding.
        cmdu_rx.swap();
    });

    // Serialize CMDU into a byte array
    beerocks::net::BufferImpl<message::MESSAGE_BUFFER_LENGTH> buffer;
    if (!m_cmdu_serializer->serialize_cmdu(iface_index, dst_mac, src_mac, cmdu_rx, buffer)) {
        LOG(ERROR) << "Failed to serialize CMDU! fd = " << connection.socket()->fd();
        return false;
    }

    // Send data
    return connection.send(buffer);
}

void CmduPeer::receive_cmdus(beerocks::net::Socket::Connection &connection,
                             beerocks::net::Buffer &buffer, const CmduReceivedHandler &handler)
{
    // Read available bytes into buffer
    int bytes_received = connection.receive(buffer);
    if (bytes_received <= 0) {
        LOG(ERROR) << "Failed to received data! bytes received: " << bytes_received
                   << ", fd = " << connection.socket()->fd();
        return;
    }

    // These parameters are obtained from the UDS header. Sender process will fill them in only
    // if CMDU was originally received by a remote process and then forwarded to this process.
    uint32_t iface_index;
    sMacAddr dst_mac;
    sMacAddr src_mac;

    // Buffer for the received CMDU and received CMDU itself
    uint8_t cmdu_rx_buffer[message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageRx cmdu_rx(cmdu_rx_buffer, sizeof(cmdu_rx_buffer));

    // CMDU parsing & handling loop
    // Note: must be done in a loop because data received through a stream-oriented socket might
    // contain more than one CMDU. If data was received through a message-oriented socket, then
    // only one message would be received at a time and the loop would be iterated only once.
    while ((buffer.length() > 0) &&
           m_cmdu_parser->parse_cmdu(buffer, iface_index, dst_mac, src_mac, cmdu_rx)) {
        if (!handler(connection, iface_index, dst_mac, src_mac, cmdu_rx)) {
            LOG(ERROR) << "Stop processing buffer";
            break;
        }
    }
}

} // namespace beerocks
