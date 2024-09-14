/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_ucc_server_impl.h>

#include <bcl/network/network_utils.h>
#include <bcl/network/sockets_impl.h>

#include <easylogging++.h>

#include <iomanip>

namespace beerocks {

UccServerImpl::UccServerImpl(std::unique_ptr<beerocks::net::ServerSocket> server_socket,
                             std::shared_ptr<beerocks::UccParser> ucc_parser,
                             std::shared_ptr<beerocks::UccSerializer> ucc_serializer,
                             std::shared_ptr<beerocks::EventLoop> event_loop)
    : m_server_socket(std::move(server_socket)), m_ucc_parser(ucc_parser),
      m_ucc_serializer(ucc_serializer), m_event_loop(event_loop)
{
    LOG_IF(!m_server_socket, FATAL) << "Server socket is a null pointer!";
    LOG_IF(!m_ucc_parser, FATAL) << "UCC parser is a null pointer!";
    LOG_IF(!m_ucc_serializer, FATAL) << "UCC serializer is a null pointer!";
    LOG_IF(!m_event_loop, FATAL) << "Event loop is a null pointer!";

    // Register event handlers for the server socket
    beerocks::EventLoop::EventHandlers handlers{
        // Name
        .name = "UCC Server",

        // Accept incoming connections
        .on_read =
            [&](int fd, EventLoop &loop) {
                handle_connect(fd);
                return true;
            },

        // Not implemented
        .on_write = nullptr,

        // Fail on server socket disconnection or error
        .on_disconnect =
            [&](int fd, EventLoop &loop) {
                LOG(ERROR) << "Server socket disconnected!";
                return false;
            },
        .on_error =
            [&](int fd, EventLoop &loop) {
                LOG(ERROR) << "Server socket error!";
                return false;
            },
    };

    LOG_IF(!m_event_loop->register_handlers(m_server_socket->socket()->fd(), handlers), FATAL)
        << "Failed registering event handlers for the server socket!";
}

UccServerImpl::~UccServerImpl()
{
    // Remove connection and its installed event handlers
    if (m_connection) {
        remove_connection(m_connection->socket()->fd(), true);
    }

    // Remove installed event handlers for the server socket
    m_event_loop->remove_handlers(m_server_socket->socket()->fd());
}

bool UccServerImpl::send_reply(int fd, const std::string &reply)
{
    // Check if given file descriptor matches that of current connection
    if ((!m_connection) || (fd != m_connection->socket()->fd())) {
        LOG(ERROR) << "Unable to send reply through an unknown connection, fd = " << fd;
        return false;
    }

    // Serialize reply string into a byte array
    beerocks::net::BufferImpl<ucc_buffer_size> buffer;
    if (!m_ucc_serializer->serialize_reply(reply, buffer)) {
        LOG(ERROR) << "Failed serializing reply string, fd = " << fd;
        return false;
    }

    // Send data
    return m_connection->send(buffer);
}

bool UccServerImpl::add_connection(int fd,
                                   std::unique_ptr<beerocks::net::Socket::Connection> connection,
                                   const beerocks::EventLoop::EventHandlers &handlers)
{
    LOG(DEBUG) << "Adding new connection, fd = " << fd;

    // Register event handlers for the connected socket
    if (!m_event_loop->register_handlers(fd, handlers)) {
        LOG(ERROR) << "Failed registering event handlers for the accepted socket!";
        return false;
    }

    // If any, remove existing connection and its installed event handlers.
    if (m_connection) {
        remove_connection(m_connection->socket()->fd(), true);
    }

    // Save connection.
    m_connection = std::move(connection);

    return true;
}

bool UccServerImpl::remove_connection(int fd, bool remove_handlers)
{
    LOG(DEBUG) << "Removing connection, fd = " << fd;

    // Check if given file descriptor matches that of current connection
    if ((!m_connection) || (fd != m_connection->socket()->fd())) {
        LOG(ERROR) << "Unable to remove connection!";
        return false;
    }

    // If requested, remove event handlers for the connected socket
    if (remove_handlers && (!m_event_loop->remove_handlers(fd))) {
        LOG(ERROR) << "Failed to remove event handlers for the connected socket! fd = " << fd;
    }

    // Remove connection
    m_connection.reset();

    // Clear the buffer just in case it contained some data from removed connection.
    m_buffer.clear();

    return true;
}

void UccServerImpl::handle_connect(int fd)
{
    LOG(DEBUG) << "Accepting connection, fd = " << fd;

    beerocks::net::InternetAddress address;
    auto connection = m_server_socket->accept(address);

    if (!connection) {
        LOG(ERROR) << "Unable to accept incoming connection request!";
        return;
    }

    int connected_socket = connection->socket()->fd();
    LOG(INFO) << "Client connected from "
              << beerocks::net::network_utils::ipv4_to_string(address.address()) << ":"
              << address.port() << ", fd = " << connected_socket;

    beerocks::EventLoop::EventHandlers handlers;
    handlers.name    = "UCC Client";
    handlers.on_read = [&](int fd, beerocks::EventLoop &loop) {
        handle_read(fd);
        return true;
    };
    handlers.on_disconnect = [&](int fd, beerocks::EventLoop &loop) {
        handle_close(fd);
        return true;
    };
    handlers.on_error = handlers.on_disconnect;

    if (!add_connection(connected_socket, std::move(connection), handlers)) {
        LOG(ERROR) << "Unable to add connection!";
    }
}

void UccServerImpl::handle_read(int fd)
{
    // Check if given file descriptor matches that of current connection
    if ((!m_connection) || (fd != m_connection->socket()->fd())) {
        LOG(ERROR) << "Data received through an unknown connection, fd = " << fd;
        return;
    }

    // Read available bytes into buffer
    int bytes_received = m_connection->receive(m_buffer);
    if (bytes_received <= 0) {
        LOG(ERROR) << "Bytes received through connection: " << bytes_received << ", fd = " << fd;
        return;
    }

    std::stringstream ss;
    ss << "Bytes received (" << std::to_string(bytes_received) << "): ";

    const uint8_t *data = m_buffer.data();
    for (size_t i = 0; i < m_buffer.length(); i++) {
        char ch = data[i];
        if (isprint(ch)) {
            ss << ch;
        } else {
            ss << "[" << std::setw(2) << std::setfill('0') << std::hex << (int)ch << "]";
        }
    }

    LOG(INFO) << ss.str();

    // UCC command string parsing & handling loop
    // Note: must be done in a loop because data received through a stream-oriented socket might
    // contain more than one message. If data was received through a message-oriented socket, then
    // only one message would be received at a time and the loop would be iterated only once.
    std::string command;
    while ((m_buffer.length() > 0) && m_ucc_parser->parse_command(m_buffer, command)) {
        notify_command_received(fd, command);
    }
}

void UccServerImpl::handle_close(int fd)
{
    // Remove connection to the given client socket
    remove_connection(fd);
}

} // namespace beerocks
