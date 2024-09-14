/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_ucc_server_impl.h>

#include <bcl/beerocks_event_loop_mock.h>
#include <bcl/beerocks_ucc_parser_mock.h>
#include <bcl/beerocks_ucc_serializer_mock.h>
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

class UccServerImplTest : public ::testing::Test {
protected:
    const std::string m_command = "command";
    const std::string m_reply   = "reply";

    std::shared_ptr<StrictMock<beerocks::net::SocketMock>> m_socket =
        std::make_shared<StrictMock<beerocks::net::SocketMock>>();
    std::unique_ptr<StrictMock<beerocks::net::ServerSocketMock>> m_server_socket =
        std::make_unique<StrictMock<beerocks::net::ServerSocketMock>>();
    std::shared_ptr<StrictMock<beerocks::UccParserMock>> m_ucc_parser =
        std::make_shared<StrictMock<beerocks::UccParserMock>>();
    std::shared_ptr<StrictMock<beerocks::UccSerializerMock>> m_ucc_serializer =
        std::make_shared<StrictMock<beerocks::UccSerializerMock>>();
    std::shared_ptr<StrictMock<beerocks::EventLoopMock>> m_event_loop =
        std::make_shared<StrictMock<beerocks::EventLoopMock>>();
};

/**
 * UccServerImpl must remove pending client connections, if any, when it goes out of scope.
 * This test checks that resources are properly deallocated if a client remains connected to the 
 * server when the server's destructor is executed. 
 * 
 * In all other tests we emulate that a client connects to the server at the beginning of the test 
 * and disconnects from the server at the end of the test. In this test, on the contrary, the client 
 * does not disconnect from the server to set up the required scenario.
 * 
 * To verify that the connection is closed by the server, we set an expectation for 
 * EventLoop::remove_handlers() on the connected socket. In all other tests no expectation is set
 * because when the client closes the connection, it is the EventLoop implementation who would 
 * remove the handlers before notifying the disconnected event.
 */
TEST_F(UccServerImplTest, destructor_should_remove_existing_connection)
{
    auto connection       = new StrictMock<beerocks::net::SocketConnectionMock>();
    auto connected_socket = std::make_shared<StrictMock<beerocks::net::SocketMock>>();

    constexpr int server_socket_fd    = 1;
    constexpr int connected_socket_fd = 2;

    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    // Expectations that use this default return value increase the `use_count` of shared_ptr
    // `socket`. To avoid a leak detection when test ends, `socket` must be destroyed at (or before)
    // the end of TEST.
    // In order to work around this, call `VerifyAndClearExpectations` right before the end of
    // the test.
    ON_CALL(*m_server_socket, socket()).WillByDefault(Return(m_socket));
    ON_CALL(*m_socket, fd()).WillByDefault(Return(server_socket_fd));

    // The same concern applies to shared_ptr `connected_socket`
    ON_CALL(*connection, socket()).WillByDefault(Return(connected_socket));
    ON_CALL(*connected_socket, fd()).WillByDefault(Return(connected_socket_fd));

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(connection));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        // Destructor removes event handlers for existing connection
        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);
        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(connected_socket_fd)).WillOnce(Return(true));

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    // Destructor of `ucc_server` must be executed before calling `VerifyAndClearExpectations` so
    // all expectations for methods called in such destructor are satisfied.
    {
        beerocks::UccServerImpl ucc_server(std::move(m_server_socket), m_ucc_parser,
                                           m_ucc_serializer, m_event_loop);

        // Emulate a new client is connected to the server socket.
        // Client remains connected when destructor is executed.
        server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);
    }

    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_socket.get()));
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(connected_socket.get()));
}

TEST_F(UccServerImplTest, new_connection_overwrites_existing_connection)
{
    auto first_connection       = new StrictMock<beerocks::net::SocketConnectionMock>();
    auto first_connected_socket = std::make_shared<StrictMock<beerocks::net::SocketMock>>();

    auto second_connection       = new StrictMock<beerocks::net::SocketConnectionMock>();
    auto second_connected_socket = std::make_shared<StrictMock<beerocks::net::SocketMock>>();

    constexpr int server_socket_fd           = 1;
    constexpr int first_connected_socket_fd  = 2;
    constexpr int second_connected_socket_fd = 3;

    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers first_connected_socket_handlers;
    beerocks::EventLoop::EventHandlers second_connected_socket_handlers;

    // Expectations that use this default return value increase the `use_count` of shared_ptr
    // `socket`. To avoid a leak detection when test ends, `socket` must be destroyed at (or before)
    // the end of TEST.
    // In order to work around this, call `VerifyAndClearExpectations` right before the end of
    // the test.
    ON_CALL(*m_server_socket, socket()).WillByDefault(Return(m_socket));
    ON_CALL(*m_socket, fd()).WillByDefault(Return(server_socket_fd));

    // The same concern applies to shared_ptr `connected_socket`
    ON_CALL(*first_connection, socket()).WillByDefault(Return(first_connected_socket));
    ON_CALL(*first_connected_socket, fd()).WillByDefault(Return(first_connected_socket_fd));
    ON_CALL(*second_connection, socket()).WillByDefault(Return(second_connected_socket));
    ON_CALL(*second_connected_socket, fd()).WillByDefault(Return(second_connected_socket_fd));

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        // A first connection takes place, which is not closed before a second one is established
        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(first_connection));

        EXPECT_CALL(*first_connection, socket()).Times(1);
        EXPECT_CALL(*first_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(first_connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&first_connected_socket_handlers), Return(true)));

        // A new connection is established before previous one is closed.
        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(second_connection));

        EXPECT_CALL(*second_connection, socket()).Times(1);
        EXPECT_CALL(*second_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(second_connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&second_connected_socket_handlers), Return(true)));

        // UCC server removes first connection because UCC client has created a second connection
        // without previously closing the first one.
        EXPECT_CALL(*first_connection, socket()).Times(1);
        EXPECT_CALL(*first_connected_socket, fd()).Times(1);
        EXPECT_CALL(*first_connection, socket()).Times(1);
        EXPECT_CALL(*first_connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(first_connected_socket_fd))
            .WillOnce(Return(true));

        EXPECT_CALL(*second_connection, socket()).Times(1);
        EXPECT_CALL(*second_connected_socket, fd()).Times(1);

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    // Destructor of `ucc_server` must be executed before calling `VerifyAndClearExpectations` so
    // all expectations for methods called in such destructor are satisfied.
    {
        beerocks::UccServerImpl ucc_server(std::move(m_server_socket), m_ucc_parser,
                                           m_ucc_serializer, m_event_loop);

        // Emulate a new client is connected to the server socket.
        server_socket_handlers.on_read(first_connected_socket_fd, *m_event_loop);

        // Emulate a new client is connected before previous one gets disconnected.
        server_socket_handlers.on_read(second_connected_socket_fd, *m_event_loop);

        // Emulate the second client gets disconnected from the server socket.
        second_connected_socket_handlers.on_disconnect(second_connected_socket_fd, *m_event_loop);
    }

    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_socket.get()));
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(first_connected_socket.get()));
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(second_connected_socket.get()));
}

TEST_F(UccServerImplTest, receive_command_should_succeed)
{
    auto connection       = new StrictMock<beerocks::net::SocketConnectionMock>();
    auto connected_socket = std::make_shared<StrictMock<beerocks::net::SocketMock>>();

    constexpr int server_socket_fd    = 1;
    constexpr int connected_socket_fd = 2;
    constexpr int bytes_received      = 16;

    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    // Expectations that use this default return value increase the `use_count` of shared_ptr
    // `socket`. To avoid a leak detection when test ends, `socket` must be destroyed at (or before)
    // the end of TEST.
    // In order to work around this, call `VerifyAndClearExpectations` right before the end of
    // the test.
    ON_CALL(*m_server_socket, socket()).WillByDefault(Return(m_socket));
    ON_CALL(*m_socket, fd()).WillByDefault(Return(server_socket_fd));

    // The same concern applies to shared_ptr `connected_socket`
    ON_CALL(*connection, socket()).WillByDefault(Return(connected_socket));
    ON_CALL(*connected_socket, fd()).WillByDefault(Return(connected_socket_fd));

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(connection));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);
        EXPECT_CALL(*connection, receive(_)).WillOnce(Invoke([&](beerocks::net::Buffer &buffer) {
            buffer.length() += bytes_received;
            return bytes_received;
        }));

        EXPECT_CALL(*m_ucc_parser, parse_command(_, _))
            .WillOnce(Invoke([&](beerocks::net::Buffer &buffer, std::string &command) {
                buffer.length() = 0;
                command         = m_command;
                return true;
            }));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    // Destructor of `ucc_server` must be executed before calling `VerifyAndClearExpectations` so
    // all expectations for methods called in such destructor are satisfied.
    {
        beerocks::UccServerImpl ucc_server(std::move(m_server_socket), m_ucc_parser,
                                           m_ucc_serializer, m_event_loop);

        // Install the command-received event handler function
        bool command_received         = false;
        auto command_received_handler = [&](int fd, const std::string &command) {
            if ((fd == connected_socket_fd) && (command == m_command)) {
                command_received = true;
            }
        };
        ucc_server.set_command_received_handler(command_received_handler);

        // Emulate a new client is connected to the server socket
        server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

        // Emulate the client sends a message
        connected_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

        // Assert that command-received event handler function has been called back
        ASSERT_TRUE(command_received);

        // Emulate the client gets disconnected from the server socket
        connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
    }

    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_socket.get()));
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(connected_socket.get()));
}

TEST_F(UccServerImplTest, receive_command_should_fail_if_parse_command_fails)
{
    auto connection       = new StrictMock<beerocks::net::SocketConnectionMock>();
    auto connected_socket = std::make_shared<StrictMock<beerocks::net::SocketMock>>();

    constexpr int server_socket_fd    = 1;
    constexpr int connected_socket_fd = 2;
    constexpr int bytes_received      = 16;

    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    // Expectations that use this default return value increase the `use_count` of shared_ptr
    // `socket`. To avoid a leak detection when test ends, `socket` must be destroyed at (or before)
    // the end of TEST.
    // In order to work around this, call `VerifyAndClearExpectations` right before the end of
    // the test.
    ON_CALL(*m_server_socket, socket()).WillByDefault(Return(m_socket));
    ON_CALL(*m_socket, fd()).WillByDefault(Return(server_socket_fd));

    // The same concern applies to shared_ptr `connected_socket`
    ON_CALL(*connection, socket()).WillByDefault(Return(connected_socket));
    ON_CALL(*connected_socket, fd()).WillByDefault(Return(connected_socket_fd));

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(connection));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);
        EXPECT_CALL(*connection, receive(_)).WillOnce(Invoke([&](beerocks::net::Buffer &buffer) {
            buffer.length() += bytes_received;
            return bytes_received;
        }));

        // This time, parse command fails
        EXPECT_CALL(*m_ucc_parser, parse_command(_, _))
            .WillOnce(Invoke([&](beerocks::net::Buffer &buffer, std::string &command) {
                buffer.length() = 0;
                return false;
            }));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    // Destructor of `ucc_server` must be executed before calling `VerifyAndClearExpectations` so
    // all expectations for methods called in such destructor are satisfied.
    {
        beerocks::UccServerImpl ucc_server(std::move(m_server_socket), m_ucc_parser,
                                           m_ucc_serializer, m_event_loop);

        // Install the command-received event handler function
        bool command_received         = false;
        auto command_received_handler = [&](int fd, const std::string &command) {
            if ((fd == connected_socket_fd) && (command == m_command)) {
                command_received = true;
            }
        };
        ucc_server.set_command_received_handler(command_received_handler);

        // Emulate a new client is connected to the server socket
        server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

        // Emulate the client sends a message
        connected_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

        // Assert that command-received event handler function has not been called back
        ASSERT_FALSE(command_received);

        // Emulate the client gets disconnected from the server socket
        connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
    }

    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_socket.get()));
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(connected_socket.get()));
}

TEST_F(UccServerImplTest, send_reply_should_succeed)
{
    auto connection       = new StrictMock<beerocks::net::SocketConnectionMock>();
    auto connected_socket = std::make_shared<StrictMock<beerocks::net::SocketMock>>();

    constexpr int server_socket_fd    = 1;
    constexpr int connected_socket_fd = 2;

    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    // Expectations that use this default return value increase the `use_count` of shared_ptr
    // `socket`. To avoid a leak detection when test ends, `socket` must be destroyed at (or before)
    // the end of TEST.
    // In order to work around this, call `VerifyAndClearExpectations` right before the end of
    // the test.
    ON_CALL(*m_server_socket, socket()).WillByDefault(Return(m_socket));
    ON_CALL(*m_socket, fd()).WillByDefault(Return(server_socket_fd));

    // The same concern applies to shared_ptr `connected_socket`
    ON_CALL(*connection, socket()).WillByDefault(Return(connected_socket));
    ON_CALL(*connected_socket, fd()).WillByDefault(Return(connected_socket_fd));

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(connection));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);

        EXPECT_CALL(*m_ucc_serializer, serialize_reply(m_reply, _)).WillOnce(Return(true));

        EXPECT_CALL(*connection, send(_)).WillOnce(Return(true));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    // Destructor of `ucc_server` must be executed before calling `VerifyAndClearExpectations` so
    // all expectations for methods called in such destructor are satisfied.
    {
        beerocks::UccServerImpl ucc_server(std::move(m_server_socket), m_ucc_parser,
                                           m_ucc_serializer, m_event_loop);

        // Emulate a new client is connected to the server socket
        server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

        // Method `send_reply` should succeed on a connected socket
        ASSERT_TRUE(ucc_server.send_reply(connected_socket_fd, m_reply));

        // Emulate the client gets disconnected from the server socket
        connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
    }

    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_socket.get()));
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(connected_socket.get()));
}

TEST_F(UccServerImplTest, send_reply_should_fail_if_serialize_reply_fails)
{
    auto connection       = new StrictMock<beerocks::net::SocketConnectionMock>();
    auto connected_socket = std::make_shared<StrictMock<beerocks::net::SocketMock>>();

    constexpr int server_socket_fd    = 1;
    constexpr int connected_socket_fd = 2;

    beerocks::EventLoop::EventHandlers server_socket_handlers;
    beerocks::EventLoop::EventHandlers connected_socket_handlers;

    // Expectations that use this default return value increase the `use_count` of shared_ptr
    // `socket`. To avoid a leak detection when test ends, `socket` must be destroyed at (or before)
    // the end of TEST.
    // In order to work around this, call `VerifyAndClearExpectations` right before the end of
    // the test.
    ON_CALL(*m_server_socket, socket()).WillByDefault(Return(m_socket));
    ON_CALL(*m_socket, fd()).WillByDefault(Return(server_socket_fd));

    // The same concern applies to shared_ptr `connected_socket`
    ON_CALL(*connection, socket()).WillByDefault(Return(connected_socket));
    ON_CALL(*connected_socket, fd()).WillByDefault(Return(connected_socket_fd));

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&server_socket_handlers), Return(true)));

        EXPECT_CALL(*m_server_socket, accept_proxy(_)).WillOnce(Return(connection));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(connected_socket_fd, _))
            .WillOnce(DoAll(SaveArg<1>(&connected_socket_handlers), Return(true)));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);

        // Serialization fails!
        EXPECT_CALL(*m_ucc_serializer, serialize_reply(m_reply, _)).WillOnce(Return(false));

        EXPECT_CALL(*connection, socket()).Times(1);
        EXPECT_CALL(*connected_socket, fd()).Times(1);

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    // Destructor of `ucc_server` must be executed before calling `VerifyAndClearExpectations` so
    // all expectations for methods called in such destructor are satisfied.
    {
        beerocks::UccServerImpl ucc_server(std::move(m_server_socket), m_ucc_parser,
                                           m_ucc_serializer, m_event_loop);

        // Emulate a new client is connected to the server socket
        server_socket_handlers.on_read(connected_socket_fd, *m_event_loop);

        // Method `send_reply` should fail if serialization fails
        ASSERT_FALSE(ucc_server.send_reply(connected_socket_fd, m_reply));

        // Emulate the client gets disconnected from the server socket
        connected_socket_handlers.on_disconnect(connected_socket_fd, *m_event_loop);
    }

    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_socket.get()));
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(connected_socket.get()));
}

TEST_F(UccServerImplTest, send_reply_should_fail_with_unknown_socket_fd)
{
    constexpr int server_socket_fd  = 1;
    constexpr int unknown_socket_fd = 2;

    // Expectations that use this default return value increase the `use_count` of shared_ptr
    // `socket`. To avoid a leak detection when test ends, `socket` must be destroyed at (or before)
    // the end of TEST.
    // In order to work around this, call `VerifyAndClearExpectations` right before the end of
    // the test.
    ON_CALL(*m_server_socket, socket()).WillByDefault(Return(m_socket));
    ON_CALL(*m_socket, fd()).WillByDefault(Return(server_socket_fd));

    {
        InSequence sequence;

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, register_handlers(server_socket_fd, _)).WillOnce(Return(true));

        EXPECT_CALL(*m_server_socket, socket()).Times(1);
        EXPECT_CALL(*m_socket, fd()).Times(1);
        EXPECT_CALL(*m_event_loop, remove_handlers(server_socket_fd)).WillOnce(Return(true));
    }

    // Destructor of `ucc_server` must be executed before calling `VerifyAndClearExpectations` so
    // all expectations for methods called in such destructor are satisfied.
    {
        beerocks::UccServerImpl ucc_server(std::move(m_server_socket), m_ucc_parser,
                                           m_ucc_serializer, m_event_loop);

        // Method `send_reply` should fail if socket is not connected
        ASSERT_FALSE(ucc_server.send_reply(unknown_socket_fd, m_reply));
    }

    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_socket.get()));
}

} // namespace
