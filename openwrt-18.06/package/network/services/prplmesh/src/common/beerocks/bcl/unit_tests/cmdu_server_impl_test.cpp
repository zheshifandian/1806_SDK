/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_cmdu_server_impl.h>

#include <bcl/beerocks_event_loop_mock.h>
#include <bcl/network/cmdu_parser_mock.h>
#include <bcl/network/cmdu_serializer_mock.h>
#include <bcl/network/network_utils.h>
#include <bcl/network/sockets_mock.h>

#include <bcl/beerocks_backport.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::StrictMock;

namespace {

constexpr int server_socket_fd    = 1;
constexpr int connected_socket_fd = 2;

class CmduServerImplTest : public ::testing::Test {
protected:
    CmduServerImplTest()
        : m_cmdu_tx(m_tx_buffer, sizeof(m_tx_buffer)), m_cmdu_rx(m_rx_buffer, sizeof(m_rx_buffer))
    {
        ON_CALL(*m_server_socket, socket()).WillByDefault(Return(m_socket));
        ON_CALL(*m_socket, fd()).WillByDefault(Return(server_socket_fd));

        ON_CALL(*m_connection, socket()).WillByDefault(Return(m_connected_socket));
        ON_CALL(*m_connected_socket, fd()).WillByDefault(Return(connected_socket_fd));
    }

    uint8_t m_tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageTx m_cmdu_tx;

    uint8_t m_rx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageRx m_cmdu_rx;

    std::shared_ptr<StrictMock<beerocks::net::SocketMock>> m_socket =
        std::make_shared<StrictMock<beerocks::net::SocketMock>>();
    std::unique_ptr<StrictMock<beerocks::net::ServerSocketMock>> m_server_socket =
        std::make_unique<StrictMock<beerocks::net::ServerSocketMock>>();
    std::shared_ptr<StrictMock<beerocks::net::CmduParserMock>> m_cmdu_parser =
        std::make_shared<StrictMock<beerocks::net::CmduParserMock>>();
    std::shared_ptr<StrictMock<beerocks::net::CmduSerializerMock>> m_cmdu_serializer =
        std::make_shared<StrictMock<beerocks::net::CmduSerializerMock>>();
    std::shared_ptr<StrictMock<beerocks::EventLoopMock>> m_event_loop =
        std::make_shared<StrictMock<beerocks::EventLoopMock>>();

    // Google Mock cannot mock a factory method that returns a non copyable return value.
    // To work around this, we have defined a proxy method in the mock.
    // The `ServerSocketMock::accept_proxy()` method returns a raw pointer instead of a unique_ptr
    StrictMock<beerocks::net::SocketConnectionMock> *m_connection =
        new StrictMock<beerocks::net::SocketConnectionMock>();
    std::shared_ptr<StrictMock<beerocks::net::SocketMock>> m_connected_socket =
        std::make_shared<StrictMock<beerocks::net::SocketMock>>();
};

TEST_F(CmduServerImplTest, destructor_should_remove_remaining_connections)
{
    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(2);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(m_connection));

        EXPECT_CALL(*m_connection, socket()).Times(1);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        // Destructor removes event handlers for remaining connections
        EXPECT_CALL(*m_event_loop, remove_handlers(connected_socket_fd)).WillOnce(Return(true));

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    beerocks::CmduServerImpl cmdu_server(std::move(m_server_socket), m_cmdu_parser,
                                         m_cmdu_serializer, m_event_loop);

    // Emulate a new client is connected to the server socket.
    // Client remains connected when destructor is executed.
    server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);
}

TEST_F(CmduServerImplTest, receive_cmdu_should_succeed)
{
    constexpr int bytes_received = 256;

    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(2);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(m_connection));

        EXPECT_CALL(*m_connection, socket()).Times(1);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*m_connection, receive(_)).WillOnce(Invoke([&](beerocks::net::Buffer &buffer) {
            buffer.length() += bytes_received;
            return bytes_received;
        }));

        EXPECT_CALL(*m_cmdu_parser, parse_cmdu(_, _, _, _, _))
            .WillOnce(
                Invoke([&](beerocks::net::Buffer &buffer, uint32_t &iface_index, sMacAddr &dst_mac,
                           sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx) {
                    buffer.length() = 0;
                    return true;
                }));

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    beerocks::CmduServerImpl cmdu_server(std::move(m_server_socket), m_cmdu_parser,
                                         m_cmdu_serializer, m_event_loop);

    // Flags to assert that events were actually fired
    bool client_connected    = false;
    bool client_disconnected = false;
    bool cmdu_received       = false;

    // Install the event handler functions
    beerocks::CmduServer::EventHandlers handlers{
        .on_client_connected =
            [&](int fd) {
                if (fd == connected_socket_fd) {
                    client_connected = true;
                }
            },
        .on_client_disconnected =
            [&](int fd) {
                if (fd == connected_socket_fd) {
                    client_disconnected = true;
                }
            },
        .on_cmdu_received =
            [&](int fd, uint32_t iface_index, const sMacAddr &dst_mac, const sMacAddr &src_mac,
                ieee1905_1::CmduMessageRx &cmdu_rx) {
                if (fd == connected_socket_fd) {
                    cmdu_received = true;
                }
            },
    };
    cmdu_server.set_handlers(handlers);

    // Emulate a new client is connected to the server socket
    server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

    // Assert that client-connected event handler function has been called back
    ASSERT_TRUE(client_connected);

    // Emulate the client sends a CMDU message
    connected_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

    // Assert that CMDU-received event handler function has been called back
    ASSERT_TRUE(cmdu_received);

    // Emulate the client gets disconnected from the server socket
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);

    // Assert that client-disconnected event handler function has been called back
    ASSERT_TRUE(client_disconnected);
}

TEST_F(CmduServerImplTest, send_cmdu_should_succeed)
{
    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(2);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(m_connection));

        EXPECT_CALL(*m_connection, socket()).Times(1);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*m_cmdu_serializer,
                    serialize_cmdu(0, beerocks::net::network_utils::ZERO_MAC,
                                   beerocks::net::network_utils::ZERO_MAC, _, _))
            .WillOnce(Return(true));

        EXPECT_CALL(*m_connection, send(_)).WillOnce(Return(true));

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    beerocks::CmduServerImpl cmdu_server(std::move(m_server_socket), m_cmdu_parser,
                                         m_cmdu_serializer, m_event_loop);

    // Emulate a new client is connected to the server socket
    server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

    // Method `send_cmdu` should succeed on a connected socket
    ASSERT_TRUE(cmdu_server.send_cmdu(connected_socket_fd, m_cmdu_tx));

    // Emulate the client gets disconnected from the server socket
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
}

TEST_F(CmduServerImplTest, send_cmdu_should_fail_with_invalid_cmdu)
{
    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(2);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(m_connection));

        EXPECT_CALL(*m_connection, socket()).Times(1);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    beerocks::CmduServerImpl cmdu_server(std::move(m_server_socket), m_cmdu_parser,
                                         m_cmdu_serializer, m_event_loop);

    // Emulate a new client is connected to the server socket
    server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

    // Invalid CMDU
    ieee1905_1::CmduMessageTx cmdu_tx(nullptr, 0);

    // Method `send_cmdu` should fail with invalid CMDU
    ASSERT_FALSE(cmdu_server.send_cmdu(connected_socket_fd, cmdu_tx));

    // Emulate the client gets disconnected from the server socket
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
}

TEST_F(CmduServerImplTest, send_cmdu_should_fail_if_serialize_cmdu_fails)
{
    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(2);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(m_connection));

        EXPECT_CALL(*m_connection, socket()).Times(1);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        // Serialization fails!
        EXPECT_CALL(*m_cmdu_serializer,
                    serialize_cmdu(0, beerocks::net::network_utils::ZERO_MAC,
                                   beerocks::net::network_utils::ZERO_MAC, _, _))
            .WillOnce(Return(false));
        EXPECT_CALL(*m_connection, socket()).Times(1);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    beerocks::CmduServerImpl cmdu_server(std::move(m_server_socket), m_cmdu_parser,
                                         m_cmdu_serializer, m_event_loop);

    // Emulate a new client is connected to the server socket
    server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

    // Method `send_cmdu` should fail if serialization fails
    ASSERT_FALSE(cmdu_server.send_cmdu(connected_socket_fd, m_cmdu_tx));

    // Emulate the client gets disconnected from the server socket
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
}

TEST_F(CmduServerImplTest, send_cmdu_should_fail_with_disconnected_socket_fd)
{
    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(2);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(m_connection));

        EXPECT_CALL(*m_connection, socket()).Times(1);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    beerocks::CmduServerImpl cmdu_server(std::move(m_server_socket), m_cmdu_parser,
                                         m_cmdu_serializer, m_event_loop);

    // Emulate a new client is connected to the server socket
    server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

    // Emulate the client gets disconnected from the server socket
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);

    // Method `send_cmdu` should fail if socket is not connected
    ASSERT_FALSE(cmdu_server.send_cmdu(connected_socket_fd, m_cmdu_tx));
}

TEST_F(CmduServerImplTest, forward_cmdu_should_succeed)
{
    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    constexpr uint32_t iface_index = 1;
    const sMacAddr dst_mac{.oct = {0xb}};
    const sMacAddr src_mac{.oct = {0xa}};

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(2);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(m_connection));

        EXPECT_CALL(*m_connection, socket()).Times(1);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*m_cmdu_serializer, serialize_cmdu(iface_index, dst_mac, src_mac, _, _))
            .WillOnce(Return(true));

        EXPECT_CALL(*m_connection, send(_)).WillOnce(Return(true));

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    beerocks::CmduServerImpl cmdu_server(std::move(m_server_socket), m_cmdu_parser,
                                         m_cmdu_serializer, m_event_loop);

    // Emulate a new client is connected to the server socket
    server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

    // Method `forward_cmdu` should succeed on a connected socket
    ASSERT_TRUE(
        cmdu_server.forward_cmdu(connected_socket_fd, iface_index, dst_mac, src_mac, m_cmdu_rx));

    // Emulate the client gets disconnected from the server socket
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
}

} // namespace
