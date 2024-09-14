/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_SOCKETS_MOCK_H_
#define BCL_NETWORK_SOCKETS_MOCK_H_

#include <bcl/network/sockets.h>

#include <gmock/gmock.h>

namespace beerocks {
namespace net {

class SocketMock : public Socket {
public:
    MOCK_METHOD(int, fd, (), (override));
};

class SocketConnectionMock : public Socket::Connection {
public:
    MOCK_METHOD(std::shared_ptr<Socket>, socket, (), (override));
    MOCK_METHOD(int, receive, (Buffer & buffer));
    MOCK_METHOD(int, receive_from, (Buffer & buffer, Socket::Address &address), (override));
    MOCK_METHOD(int, send, (const Buffer &buffer), (override));
    MOCK_METHOD(int, send_to, (const Buffer &buffer, const Socket::Address &address), (override));
};

class ServerSocketMock : public ServerSocket {
public:
    MOCK_METHOD(std::shared_ptr<Socket>, socket, (), (override));
    // Google Mock cannot mock a factory method that returns a non copyable return value.
    // To work around this, we add an indirection through a proxy method.
    // Production code will use the overridden method and unit tests will set expectations in the
    // mocked helper method instead.
    virtual std::unique_ptr<Socket::Connection> accept(Socket::Address &address) override
    {
        return std::unique_ptr<Socket::Connection>(accept_proxy(address));
    };
    MOCK_METHOD(Socket::Connection *, accept_proxy, (Socket::Address & address));
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_SOCKETS_MOCK_H_ */
