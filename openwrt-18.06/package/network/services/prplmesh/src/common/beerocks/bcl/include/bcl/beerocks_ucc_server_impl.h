/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_UCC_SERVER_IMPL_H_
#define _BEEROCKS_UCC_SERVER_IMPL_H_

#include <bcl/beerocks_ucc_server.h>

#include <bcl/beerocks_event_loop.h>
#include <bcl/beerocks_ucc_parser.h>
#include <bcl/beerocks_ucc_serializer.h>
#include <bcl/network/buffer_impl.h>
#include <bcl/network/sockets.h>

namespace beerocks {

class UccServerImpl : public UccServer {
    /**
     * Size of the buffer to store UCC commands and replies
     */
    static constexpr size_t ucc_buffer_size = 4096;

public:
    /**
     * @brief Class constructor.
     *
     * @param server_socket Server socket used to accept incoming connection requests from clients.
     * @param event_loop Application event loop used by the process to wait for I/O events.
     */
    UccServerImpl(std::unique_ptr<beerocks::net::ServerSocket> server_socket,
                  std::shared_ptr<beerocks::UccParser> ucc_parser,
                  std::shared_ptr<beerocks::UccSerializer> ucc_serializer,
                  std::shared_ptr<beerocks::EventLoop> event_loop);

    /**
     * @brief Class destructor.
     */
    ~UccServerImpl() override;

    /**
     * @brief Sends a reply string to a previously received UCC command.
     * @see CmduServer::send_cmdu
     */
    virtual bool send_reply(int fd, const std::string &reply) override;

private:
    /**
     * @brief Adds a new connection.
     *
     * Registers given event handlers for the connected socket so the appropriate action is taken
     * whenever data is received or socket get disconnected.
     *
     * Stores the connection object so event handlers that have been registered in the event loop
     * can be removed on exit.
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
     * Removes event handlers for the connected socket and removes stored connection.
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
     * Server socket used to accept incoming connection requests from clients that will
     * communicate with this process by exchanging UCC commands through those connections.
     */
    std::unique_ptr<beerocks::net::ServerSocket> m_server_socket;

    /**
     * UCC string parser used to get string messages out of a byte array received through a socket
     * connection.
     */
    std::shared_ptr<beerocks::UccParser> m_ucc_parser;

    /**
     * UCC string serializer used to put string messages into a byte array ready to be sent through
     * a socket connection.
     */
    std::shared_ptr<beerocks::UccSerializer> m_ucc_serializer;

    /**
     * Application event loop used by the process to wait for I/O events.
     */
    std::shared_ptr<EventLoop> m_event_loop;

    /**
     * Accepted socket connection, used to send and receive data to/from the socket.
     * UCC server keeps one connection open at a time. If one client tries to connect to the server
     * when there is a connection already in place, the second connection will replace the first
     * one.
     */
    std::unique_ptr<beerocks::net::Socket::Connection> m_connection;

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
    beerocks::net::BufferImpl<ucc_buffer_size> m_buffer;
};

} // namespace beerocks

#endif // _BEEROCKS_UCC_SERVER_IMPL_H_
