/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_CMDU_SERVER_IMPL_H_
#define _BEEROCKS_CMDU_SERVER_IMPL_H_

#include <bcl/beerocks_cmdu_peer.h>
#include <bcl/beerocks_cmdu_server.h>

#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_event_loop.h>
#include <bcl/network/buffer_impl.h>

#include <unordered_map>

namespace beerocks {

/**
 * @brief The CMDU server is a component to send and receive CMDU messages between local processes.
 *
 * In this implementation of the @see CmduServer interface, the connection between communicating
 * processes is established with a server socket. The framing protocol used is defined by the CMDU
 * parser and serializer objects given constructor.
 */
class CmduServerImpl : public CmduServer {
public:
    /**
     * @brief Class constructor.
     * 
     * @param server_socket Server socket used to accept incoming connection requests from clients.
     * @param cmdu_parser CMDU parser used to get CMDU messages out of a byte array received
     * through a socket connection.
     * @param cmdu_serializer CMDU serializer used to put CMDU messages into a byte array ready to
     * be sent through a socket connection.
     * @param event_loop Application event loop used by the process to wait for I/O events.
     */
    CmduServerImpl(std::unique_ptr<beerocks::net::ServerSocket> server_socket,
                   std::shared_ptr<beerocks::net::CmduParser> cmdu_parser,
                   std::shared_ptr<beerocks::net::CmduSerializer> cmdu_serializer,
                   std::shared_ptr<beerocks::EventLoop> event_loop);

    /**
     * @brief Class destructor.
     */
    ~CmduServerImpl() override;

    /**
     * @brief Disconnects a client socket connection.
     * @see CmduServer::disconnect
     */
    bool disconnect(int fd) override;

    /**
     * @brief Sends a CMDU message.
     * @see CmduServer::send_cmdu
     *
     * This implementation uses the CMDU serializer to build a frame and sends it through given
     * socket connection.
     */
    bool send_cmdu(int fd, ieee1905_1::CmduMessageTx &cmdu_tx) override;

    /**
     * @brief Forwards a CMDU message that was sent by a remote process.
     * @see CmduServer::forward_cmdu
     *
     * This implementation uses the CMDU serializer to build a frame and sends it through given
     * socket connection.
     */
    bool forward_cmdu(int fd, uint32_t iface_index, const sMacAddr &dst_mac,
                      const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx) override;

    /**
     * @brief Set the client name on EventLoop file descriptor handler.
     *
     * @see CmduServer::set_client_name
     */
    bool set_client_name(int fd, const std::string &client_name) override;

private:
    /**
     * @brief Adds a new connection.
     *
     * Registers given event handlers for the connected socket so the appropriate action is taken
     * whenever data is received or socket get disconnected.
     *
     * Adds the connection object to the list of current socket connections so event handlers
     * that have been registered in the event loop can be removed on exit.
     *
     * @param fd File descriptor of the socket used by the connection.
     * @param connection Connection object used to send/receive data.
     * @param handlers Event handlers to install into the event loop to handle blocking I/O events.
     * @return true on success and false otherwise.
     */
    bool add_connection(int fd, std::unique_ptr<beerocks::net::Socket::Connection> connection,
                        const beerocks::EventLoop::EventHandlers &handlers);

    /**
     * @brief Removes connection.
     *
     * Removes event handlers for the connected socket and removes the connection from the list of
     * current connections.
     *
     * This method gets called when connection is closed, an error occurs on the socket and on exit.
     *
     * When connection is closed by the other peer there is no need to remove installed event
     * handlers for the disconnected socket as it has already been done by the event loop itself.
     * It is only on exit when we have to explicitly remove event handlers from the event loop for
     * the remaining connections.
     *
     * @param fd File descriptor of the socket used by the connection.
     * @param remove_handlers Flag to signal if event handlers must be removed from event loop.
     * @return true on success and false otherwise.
     */
    bool remove_connection(int fd, bool remove_handlers = false);

    /**
     * @brief Handles the read event in the server socket, which corresponds to an incoming
     * connection request from a client socket.
     *
     * Accepts the connection request and adds the new connection to the list of current
     * connections.
     *
     * @param fd File descriptor of the server socket.
     */
    void handle_connect(int fd);

    /**
     * @brief Handles the read event in a client socket connected to the server socket.
     *
     * Reads data received through the socket and parses CMDU messages out of the bytes received.
     * Valid CMDU messages received are processed by calling the `notify_cmdu_received()` method.
     *
     * @param fd File descriptor of the connected socket.
     */
    void handle_read(int fd);

    /**
     * @brief Handles the disconnect and error events in a client socket connected to the server
     * socket.
     *
     * Removes connection from the list of current connections.
     *
     * @param fd File descriptor of the connected socket.
     */
    void handle_close(int fd);

    /**
     * Class used to send and receive CMDU messages through a socket connection.
     */
    CmduPeer m_peer;

    /**
     * Server socket used to accept incoming connection requests from clients that will
     * communicate with this process by exchanging CMDU messages through those connections.
     */
    std::unique_ptr<beerocks::net::ServerSocket> m_server_socket;

    /**
     * Application event loop used by the process to wait for I/O events.
     */
    std::shared_ptr<EventLoop> m_event_loop;

    /**
     * Structure to hold context information for each ongoing socket connection.
     */
    struct sConnectionContext {
        /**
         * Accepted socket connection, used to send and receive data to/from the socket.
         * Connections are stored so event handlers that have been registered in the event loop
         * can be removed on exit.
         */
        std::unique_ptr<beerocks::net::Socket::Connection> connection;

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
        beerocks::net::BufferImpl<message::MESSAGE_BUFFER_LENGTH> buffer;

        /**
         * @brief Struct constructor.
         *
         * @param connection Socket connection.
         */
        explicit sConnectionContext(std::unique_ptr<beerocks::net::Socket::Connection> connection)
            : connection(std::move(connection)){};
    };

    /**
     * Map of current socket connections.
     * Key value is the file descriptor of the accepted socket and the object value is the
     * context information of the connection.
     */
    std::unordered_map<int, sConnectionContext> m_connections;
};

} // namespace beerocks

#endif // _BEEROCKS_CMDU_SERVER_IMPL_H_
