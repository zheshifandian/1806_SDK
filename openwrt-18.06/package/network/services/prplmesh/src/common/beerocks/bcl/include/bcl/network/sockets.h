/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_SOCKETS_H_
#define BCL_NETWORK_SOCKETS_H_

#include "buffer.h"
#include "file_descriptor.h"

#include <sys/socket.h>

#include <memory>

namespace beerocks {
namespace net {

/**
 * Sockets are OS resources implementing the file descriptor interface. The way this fact is
 * modeled is by extending the FileDescriptor interface.
 */
class Socket : public FileDescriptor {
public:
    /**
     * Wrapper class around sockaddr (structure describing a generic socket address that contains a
     * member `sa_family` that tells you whether it should be cast to `struct sockaddr_in`,
     * `struct sockaddr_in6`, `struct sockaddr_ll` or something else)
     */
    class Address {
    public:
        /**
         * @brief Class destructor
         */
        virtual ~Address() = default;

        /**
         * @brief Returns address of sockaddr structure.
         *
         * @return address of sockaddr.
         */
        virtual const struct sockaddr *sockaddr() const = 0;

        /**
         * @brief Returns the length of the sockaddr structure.
         *
         * @return length of sockaddr
         */
        virtual const socklen_t &length() const = 0;

        /**
         * @brief Returns the size of the sockaddr structure.
         *
         * @return size of sockaddr
         */
        virtual socklen_t size() const = 0;

        /**
         * @brief Returns the address name.
         *
         * @return std::string Address name.
         */
        virtual const std::string &name() const = 0;

        /**
         * @brief Returns address of sockaddr structure.
         *
         * This is the non-const version of the method with the same name.
         *
         * @return address of sockaddr.
         */
        struct sockaddr *sockaddr()
        {
            /**
             * This is a way to "Avoid Duplication in const and Non-const Member Function" as
             * described in "Effective C++, 3rd ed" by Scott Meyers.
             * The two casts and function call may be ugly but they're correct and the method is
             * implemented in the interface class, so available to all implementation classes for
             * free.
             */
            return const_cast<struct sockaddr *>(const_cast<const Address *>(this)->sockaddr());
        }

        /**
         * @brief Returns the length of the sockaddr structure.
         *
         * This is the non-const version of the method with the same name.
         *
         * @return length of sockaddr
         */
        socklen_t &length()
        {
            return const_cast<socklen_t &>(const_cast<const Address *>(this)->length());
        }

    protected:
        std::string m_name;
    };

    /**
     * Classes implementing this interface model either the socket connection established at the
     * server side when accept() system call is called or at the client side when connect() is
     * called.
     *
     * The interface defines the methods to send data over a socket and to receive data from a
     * socket.
     */
    class Connection {
    public:
        /**
         * @brief Class destructor
         */
        virtual ~Connection() = default;

        /**
         * @brief Returns the underlying socket used by this connection.
         *
         * Access to the underlying socket is required to obtain the socket file descriptor with
         * which wait for read or write events using select() or epoll() functions.
         *
         * @return Socket used by the connection
         */
        virtual std::shared_ptr<Socket> socket() = 0;

        /**
         * @brief Receives data through the socket connection.
         *
         * Received data is appended to existing buffer contents, if any.
         *
         * @param[in, out] buffer Buffer to hold received data.
         * @return Number of bytes received, -1 on failure.
         */
        virtual int receive(Buffer &buffer) = 0;

        /**
         * @brief Receives data through the socket connection.
         *
         * @param[in, out] buffer Buffer to hold received data.
         * @param[out] address Address where the data came from.
         * @return Number of bytes received, -1 on failure.
         */
        virtual int receive_from(Buffer &buffer, Address &address) = 0;

        /**
         * @brief Sends data through the socket connection.
         *
         * @param[in] buffer Buffer holding data to send.
         * @return Number of bytes transmitted, -1 on failure.
         */
        virtual int send(const Buffer &buffer) = 0;

        /**
         * @brief Sends data through the socket connection.
         *
         * @param[in] buffer Buffer holding data to send.
         * @param[in] address Destination address.
         * @return Number of bytes transmitted, -1 on failure.
         */
        virtual int send_to(const Buffer &buffer, const Address &address) = 0;
    };

    std::string m_name;
};

class ServerSocket {
public:
    /**
     * @brief Class destructor
     */
    virtual ~ServerSocket() = default;

    /**
     * @brief Returns the underlying socket used by this server.
     *
     * Access to the underlying socket is required to obtain the socket file descriptor with which
     * wait for read or write events using select() or epoll() functions.
     *
     * @return Socket used by the server.
     */
    virtual std::shared_ptr<Socket> socket() = 0;

    /**
     * @brief Accepts a connection request.
     *
     * @param address Address of the peer socket.
     * @return First connection request on the queue of pending connections for the listening
     * socket.
     */
    virtual std::unique_ptr<Socket::Connection> accept(Socket::Address &address) = 0;
};

class ClientSocket {
public:
    /**
     * @brief Class destructor
     */
    virtual ~ClientSocket() = default;

    /**
     * @brief Returns the underlying socket used by this client.
     *
     * Access to the underlying socket is required to obtain the socket file descriptor with which
     * wait for read or write events using select() or epoll() functions.
     *
     * @return Socket used by the client.
     */
    virtual std::shared_ptr<Socket> socket() = 0;

    /**
     * @brief Connects the socket to the address specified.
     *
     * @param address Destination address.
     * @return Connection established with peer socket.
     */
    virtual std::unique_ptr<Socket::Connection> connect(const Socket::Address &address) = 0;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_SOCKETS_H_ */
