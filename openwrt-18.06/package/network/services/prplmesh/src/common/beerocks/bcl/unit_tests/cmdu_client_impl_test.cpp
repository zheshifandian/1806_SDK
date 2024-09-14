/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_cmdu_client_impl.h>

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

constexpr int connected_socket_fd = 1;

class CmduClientImplTest : public ::testing::Test {
protected:
    CmduClientImplTest() : m_cmdu_tx(m_tx_buffer, sizeof(m_tx_buffer))
    {
        ON_CALL(*m_connection_raw_ptr, socket()).WillByDefault(Return(m_connected_socket));
        ON_CALL(*m_connected_socket, fd()).WillByDefault(Return(connected_socket_fd));
    }

    uint8_t m_tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageTx m_cmdu_tx;

    std::shared_ptr<StrictMock<beerocks::net::SocketMock>> m_connected_socket =
        std::make_shared<StrictMock<beerocks::net::SocketMock>>();
    std::unique_ptr<StrictMock<beerocks::net::SocketConnectionMock>> m_connection =
        std::make_unique<StrictMock<beerocks::net::SocketConnectionMock>>();
    std::shared_ptr<StrictMock<beerocks::net::CmduParserMock>> m_cmdu_parser =
        std::make_shared<StrictMock<beerocks::net::CmduParserMock>>();
    std::shared_ptr<StrictMock<beerocks::net::CmduSerializerMock>> m_cmdu_serializer =
        std::make_shared<StrictMock<beerocks::net::CmduSerializerMock>>();
    std::shared_ptr<StrictMock<beerocks::EventLoopMock>> m_event_loop =
        std::make_shared<StrictMock<beerocks::EventLoopMock>>();

    // Since the unique_ptr to the connection mock is moved into the CMDU client when constructed,
    // it is not available to be used by the expectations. To overcome this problem, we use the raw
    // pointer instead.
    StrictMock<beerocks::net::SocketConnectionMock> *m_connection_raw_ptr = m_connection.get();
};

TEST_F(CmduClientImplTest, destructor_should_remove_connection_if_still_open)
{
    {
        InSequence sequence;

        EXPECT_CALL(*m_connection_raw_ptr, socket()).Times(2);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(Return(true));

        // Destructor removes event handlers for connected socket if connection is still open
        EXPECT_CALL(*m_connection_raw_ptr, socket()).Times(1);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(connected_socket_fd)).WillOnce(Return(true));
    }

    beerocks::CmduClientImpl cmdu_client(std::move(m_connection), m_cmdu_parser, m_cmdu_serializer,
                                         m_event_loop);
}

TEST_F(CmduClientImplTest, receive_cmdu_should_succeed)
{
    constexpr int bytes_received = 256;

    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_connection_raw_ptr, socket()).Times(2);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*m_connection_raw_ptr, receive(_))
            .WillOnce(Invoke([&](beerocks::net::Buffer &buffer) {
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
    }

    beerocks::CmduClientImpl cmdu_client(std::move(m_connection), m_cmdu_parser, m_cmdu_serializer,
                                         m_event_loop);

    // Flags to assert that events were actually fired
    bool cmdu_received     = false;
    bool connection_closed = false;

    // Install the event handler functions
    beerocks::CmduClient::EventHandlers handlers{
        .on_cmdu_received     = [&](uint32_t iface_index, const sMacAddr &dst_mac,
                                const sMacAddr &src_mac,
                                ieee1905_1::CmduMessageRx &cmdu_rx) { cmdu_received = true; },
        .on_connection_closed = [&]() { connection_closed = true; },
    };
    cmdu_client.set_handlers(handlers);

    // Emulate the client receives a CMDU message
    connected_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

    // Assert that CMDU-received event handler function has been called back
    ASSERT_TRUE(cmdu_received);

    // Emulate the server closes the connection
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);

    // Assert that connection-closed event handler function has been called back
    ASSERT_TRUE(connection_closed);
}

TEST_F(CmduClientImplTest, send_cmdu_should_succeed)
{
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_connection_raw_ptr, socket()).Times(2);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*m_cmdu_serializer,
                    serialize_cmdu(0, beerocks::net::network_utils::ZERO_MAC,
                                   beerocks::net::network_utils::ZERO_MAC, _, _))
            .WillOnce(Return(true));

        EXPECT_CALL(*m_connection_raw_ptr, send(_)).WillOnce(Return(true));
    }

    beerocks::CmduClientImpl cmdu_client(std::move(m_connection), m_cmdu_parser, m_cmdu_serializer,
                                         m_event_loop);

    // Method `send_cmdu` should succeed on a connected socket
    ASSERT_TRUE(cmdu_client.send_cmdu(m_cmdu_tx));

    // Emulate the server closes the connection
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
}

TEST_F(CmduClientImplTest, send_cmdu_should_fail_with_invalid_cmdu)
{
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_connection_raw_ptr, socket()).Times(2);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));
    }

    beerocks::CmduClientImpl cmdu_client(std::move(m_connection), m_cmdu_parser, m_cmdu_serializer,
                                         m_event_loop);

    // Invalid CMDU
    ieee1905_1::CmduMessageTx cmdu_tx(nullptr, 0);

    // Method `send_cmdu` should fail with invalid CMDU
    ASSERT_FALSE(cmdu_client.send_cmdu(cmdu_tx));

    // Emulate the server closes the connection
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
}

TEST_F(CmduClientImplTest, send_cmdu_should_fail_if_serialize_cmdu_fails)
{
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_connection_raw_ptr, socket()).Times(2);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        // Serialization fails!
        EXPECT_CALL(*m_cmdu_serializer,
                    serialize_cmdu(0, beerocks::net::network_utils::ZERO_MAC,
                                   beerocks::net::network_utils::ZERO_MAC, _, _))
            .WillOnce(Return(false));

        EXPECT_CALL(*m_connection_raw_ptr, socket()).Times(1);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
    }

    beerocks::CmduClientImpl cmdu_client(std::move(m_connection), m_cmdu_parser, m_cmdu_serializer,
                                         m_event_loop);

    // Method `send_cmdu` should fail if serialization fails
    ASSERT_FALSE(cmdu_client.send_cmdu(m_cmdu_tx));

    // Emulate the server closes the connection
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
}

TEST_F(CmduClientImplTest, send_cmdu_should_fail_if_connection_is_closed)
{
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    {
        InSequence sequence;

        EXPECT_CALL(*m_connection_raw_ptr, socket()).Times(2);
        EXPECT_CALL(*m_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));
    }

    beerocks::CmduClientImpl cmdu_client(std::move(m_connection), m_cmdu_parser, m_cmdu_serializer,
                                         m_event_loop);

    // Emulate the server closes the connection
    connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);

    // Method `send_cmdu` should fail if socket is not connected
    ASSERT_FALSE(cmdu_client.send_cmdu(m_cmdu_tx));
}

} // namespace
