/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_cmdu_client_impl.h>

#include <bcl/beerocks_backport.h>

#include <easylogging++.h>

namespace beerocks {

CmduClientImpl::CmduClientImpl(std::unique_ptr<beerocks::net::Socket::Connection> connection,
                               std::shared_ptr<beerocks::net::CmduParser> cmdu_parser,
                               std::shared_ptr<beerocks::net::CmduSerializer> cmdu_serializer,
                               std::shared_ptr<beerocks::EventLoop> event_loop)
    : m_peer(cmdu_parser, cmdu_serializer), m_connection(std::move(connection)),
      m_event_loop(event_loop)
{
    LOG_IF(!m_connection, FATAL) << "Connection is a null pointer!";
    LOG_IF(!m_event_loop, FATAL) << "Event loop is a null pointer!";

    // Register event handlers for the client socket
    beerocks::EventLoop::EventHandlers handlers{
        .name = m_connection->socket()->m_name,
        .on_read =
            [&](int fd, EventLoop &loop) {
                handle_read(fd);
                return true;
            },

        // Not implemented
        .on_write = nullptr,

        // Fail on socket disconnection or error
        .on_disconnect =
            [&](int fd, EventLoop &loop) {
                LOG(ERROR) << "Connection closed!";
                handle_close(fd);
                return true;
            },
        .on_error =
            [&](int fd, EventLoop &loop) {
                LOG(ERROR) << "Socket error!";
                handle_close(fd);
                return true;
            },
    };

    LOG_IF(!m_event_loop->register_handlers(m_connection->socket()->fd(), handlers), FATAL)
        << "Failed registering event handlers for the connection!";
}

CmduClientImpl::~CmduClientImpl() { close_connection(true); }

bool CmduClientImpl::send_cmdu(ieee1905_1::CmduMessageTx &cmdu_tx) const
{
    // Check if connection with server is still open
    if (!m_connection) {
        LOG(ERROR) << "Connection with server has been closed!";
        return false;
    }

    // Send given CMDU through the socket connection established with CMDU server
    return m_peer.send_cmdu(*m_connection, cmdu_tx);
}

bool CmduClientImpl::forward_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    // Check if connection with server is still open
    if (!m_connection) {
        LOG(ERROR) << "Connection with server has been closed!";
    }

    /** 
     * Fill @a iface_index, @a dst_mac and @a arc_mac with empty values since they are irrelevant.
     */
    return m_peer.forward_cmdu(*m_connection, 0, {}, {}, cmdu_rx);
}

void CmduClientImpl::handle_read(int fd)
{
    // Read all CMDU messages received and notify their reception
    auto handler = [&](beerocks::net::Socket::Connection &connection, uint32_t iface_index,
                       const sMacAddr &dst_mac, const sMacAddr &src_mac,
                       ieee1905_1::CmduMessageRx &cmdu_rx) {
        notify_cmdu_received(iface_index, dst_mac, src_mac, cmdu_rx);
        return true;
    };
    m_peer.receive_cmdus(*m_connection, m_buffer, handler);
}

void CmduClientImpl::handle_close(int fd)
{
    // Close the connection. No need to remove handlers as they have already been removed by the
    // event loop
    close_connection();

    // Notify that connection with server has been closed (no more messages can be exchanged with
    // the server from now on).
    // A handler for this event might, for example, implement a recovery mechanism that includes
    // restoring the connection with the server and creating another client.
    notify_connection_closed();
}

void CmduClientImpl::close_connection(bool remove_handlers)
{
    // If connection with server is still open ...
    if (m_connection) {

        // If requested, remove event handlers for the connected socket
        if (remove_handlers) {
            m_event_loop->remove_handlers(m_connection->socket()->fd());
        }

        // Terminate the connection
        m_connection.reset();
    }
}

} // namespace beerocks
