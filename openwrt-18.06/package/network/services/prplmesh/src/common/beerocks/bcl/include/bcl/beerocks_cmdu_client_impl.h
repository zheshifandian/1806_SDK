/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_CMDU_CLIENT_IMPL_H_
#define _BEEROCKS_CMDU_CLIENT_IMPL_H_

#include <bcl/beerocks_cmdu_client.h>
#include <bcl/beerocks_cmdu_peer.h>

#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_event_loop.h>
#include <bcl/network/buffer_impl.h>

namespace beerocks {

/**
 * @brief The CMDU client is a component to send and receive CMDU messages between local processes.
 *
 * In this implementation of the @see CmduClient interface, the connection between communicating
 * processes must have already been established with a server socket. The framing protocol used is
 * defined by the CMDU parser and serializer objects given constructor.
 */
class CmduClientImpl : public CmduClient {
public:
    /**
     * @brief Class constructor.
     * 
     * @param connection Connection established with CMDU server.
     * @param cmdu_parser CMDU parser used to get CMDU messages out of a byte array received
     * through a socket connection.
     * @param cmdu_serializer CMDU serializer used to put CMDU messages into a byte array ready to
     * be sent through a socket connection.
     * @param event_loop Application event loop used by the process to wait for I/O events.
     */
    CmduClientImpl(std::unique_ptr<beerocks::net::Socket::Connection> connection,
                   std::shared_ptr<beerocks::net::CmduParser> cmdu_parser,
                   std::shared_ptr<beerocks::net::CmduSerializer> cmdu_serializer,
                   std::shared_ptr<beerocks::EventLoop> event_loop);

    /**
     * @brief Class destructor.
     */
    ~CmduClientImpl() override;

    /**
     * @brief Sends a CMDU message to the connected server.
     * @see CmduClient::send_cmdu
     */
    bool send_cmdu(ieee1905_1::CmduMessageTx &cmdu_tx) const override;

    /**
     * @brief Forwards a CMDU message to the connected server.
     * @see CmduClient::forward_cmdu
     */
    bool forward_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx) override;

    /**
     * @brief Get the file descriptor of the client.
     * @see CmduClient::get_fd
     */
    int get_fd() override { return m_connection->socket()->fd(); }

private:
    /**
     * @brief Handles the read event in the connected socket.
     *
     * Reads data received through the socket and parses CMDU messages out of the bytes received.
     * Valid CMDU messages received are processed by calling the `notify_cmdu_received()` method.
     *
     * @param fd File descriptor of the connected socket.
     */
    void handle_read(int fd);

    /**
     * @brief Handles the disconnect and error events in the connected socket.
     *
     * Closes the socket connection and notifies that connection to the server has been closed.
     *
     * @param fd File descriptor of the connected socket.
     */
    void handle_close(int fd);

    /**
     * @brief Closes connection between client and server socket.
     *
     * Removes handlers for events on this connection in the event loop (optionally, depending on
     * given parameter value). Marks the connection as terminated so it cannot be used from now on
     * (as a consequence, calls to method for sending a message to the server will fail).
     *
     * @param remove_handlers Flag to signal if event handlers must be removed from event loop.
     */
    void close_connection(bool remove_handlers = false);

    /**
     * Class used to send and receive CMDU messages through a socket connection.
     */
    CmduPeer m_peer;

    /**
     * Connection established with CMDU server.
     */
    std::unique_ptr<beerocks::net::Socket::Connection> m_connection;

    /**
     * Application event loop used by the process to wait for I/O events.
     */
    std::shared_ptr<EventLoop> m_event_loop;

    /**
     * Buffer to hold data received through the socket connection.
     * If connection uses a stream-oriented socket, it needs its own buffer to hold received
     * data.
     * A stream-oriented socket provides a stream of bytes, it is not message-oriented, and
     * does not provide boundaries. One write call could take several read calls to get that
     * data. Data from several write calls could be read by one read call. And anything in
     * between is also possible.
     * If connection uses a message-oriented socket instead, this buffer and the code that
     * uses it is also valid.
     */
    beerocks::net::BufferImpl<message::MESSAGE_BUFFER_LENGTH> m_buffer;
};

} // namespace beerocks

#endif // _BEEROCKS_CMDU_CLIENT_IMPL_H_
