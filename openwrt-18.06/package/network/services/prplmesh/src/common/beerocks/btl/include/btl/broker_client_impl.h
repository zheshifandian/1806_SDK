/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BTL_BROKER_CLIENT_IMPL_H_
#define BTL_BROKER_CLIENT_IMPL_H_

#include <btl/broker_client.h>

#include <btl/message_parser.h>
#include <btl/message_serializer.h>

#include <bcl/beerocks_event_loop.h>
#include <bcl/network/buffer_impl.h>
#include <bcl/network/sockets.h>
#include <mapf/transport/ieee1905_transport_messages.h>

namespace beerocks {
namespace btl {

class BrokerClientImpl : public BrokerClient {
    /**
     * Size of the buffer to store messages exchanged with broker server
     */
    static constexpr size_t broker_buffer_size =
        sizeof(beerocks::transport::messages::Message::Header) +
        beerocks::transport::messages::Message::kMaxFrameLength;

public:
    /**
     * @brief Class constructor.
     *
     * @param connection Connection established with broker server.
     * @param message_parser Message parser used to get transport messages out of a byte array
     * received through a socket connection.
     * @param message_serializer Message serializer used to put transport messages into a byte
     * array ready to be sent through a socket connection.
     * @param event_loop Application event loop used by the process to wait for I/O events.
     */
    BrokerClientImpl(std::unique_ptr<beerocks::net::Socket::Connection> connection,
                     std::shared_ptr<MessageParser> message_parser,
                     std::shared_ptr<MessageSerializer> message_serializer,
                     std::shared_ptr<beerocks::EventLoop> event_loop);

    /**
     * @brief Class destructor
     */
    ~BrokerClientImpl() override;

    /**
     * @brief Subscribes for the reception of a set of CMDU messages.
     *
     * @see BrokerClient::subscribe()
     */
    bool subscribe(const std::set<ieee1905_1::eMessageType> &msg_types) override;

    /**
     * @brief Configures the transport process to use given network bridge.
     *
     * @see BrokerClient::configure_interfaces()
     */
    bool configure_interfaces(const std::string &iface_name, const std::string &bridge_name,
                              bool is_bridge, bool add) override;

    /**
     * @brief Configures the transport process to bind a given local al_mac address.
     *
     * @see BrokerClient::configure_al_mac()
     */
    bool configure_al_mac(const sMacAddr &al_mac) override;

    /**
     * @brief Sends a CDMU message to the transport process for dispatching.
     *
     * @see BrokerClient::send_cmdu()
     */
    bool send_cmdu(ieee1905_1::CmduMessageTx &cmdu_tx, const sMacAddr &dst_mac,
                   const sMacAddr &src_mac, uint32_t iface_index = 0) override;

    /**
     * @brief Forwards a CMDU message to the transport process for dispatching.
     *
     * @see BrokerClient::forward_cmdu()
     */
    bool forward_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &dst_mac,
                      const sMacAddr &src_mac, uint32_t iface_index = 0) override;

private:
    /**
     * @brief Handles the read event in a client socket connected to the server socket.
     *
     * Reads data received through the socket and parses transport messages out of the bytes
     * received (using message parser provided in constructor). Valid transport messages received
     * are processed by calling the `handle_message()` method.
     *
     * @param fd File descriptor of the connected socket.
     */
    void handle_read(int fd);

    /**
     * @brief Handles the disconnect and error events in a client socket connected to the server
     * socket.
     *
     * Closes connection with the server socket and notifies that connection to the server has been
     * closed.
     *
     * @param fd File descriptor of the connected socket.
     */
    void handle_close(int fd);

    /**
     * @brief Handles transport message received from server.
     *
     * If the received transport message contains a valid CMDU, then notifies that a CMDU has been
     * received from broker server. Otherwise message is ignored.
     *
     * @param message Transport message received from server.
     */
    void handle_message(const beerocks::transport::messages::Message &message);

    /**
     * @brief Closes connection between client and server socket.
     *
     * Removes handlers for events on this connection in the event loop. Marks the connection as
     * terminated so it cannot be used from now on (as a consequence, all calls to methods in this
     * class that end up sending a message to the server will fail).
     *
     * @param remove_handlers Flag to signal if event handlers must be removed from event loop.
     */
    void close_connection(bool remove_handlers = false);

    /**
     * @brief Sends a CDMU message to the transport process for dispatching.
     *
     * @param cmdu CMDU message to send.
     * @param dst_mac Destination MAC address (must not be empty).
     * @param src_mac Source MAC address (must not be empty).
     * @param iface_index Index of the network interface to use (set to 0 to send on all available
     * interfaces).
     * @return true on success and false otherwise.
     */
    bool send_cmdu_message(ieee1905_1::CmduMessage &cmdu, const sMacAddr &dst_mac,
                           const sMacAddr &src_mac, uint32_t iface_index);
    /**
     * @brief Sends a transport message to the server.
     *
     * If connection with server is still open, then serializes given message into a byte array
     * (using the message serializer provided in constructor) and sends it to the server.
     *
     * @param message Transport message to send.
     * @return true on success and false otherwise.
     */
    bool send_message(const beerocks::transport::messages::Message &message);

    /**
     * Connection established with broker server.
     */
    std::unique_ptr<beerocks::net::Socket::Connection> m_connection;

    /**
     * Message parser used to get transport messages out of a byte array received through a socket
     * connection.
     */
    std::shared_ptr<MessageParser> m_message_parser;

    /**
     * Message serializer used to put transport messages into a byte array ready to be sent through
     * a socket connection.
     */
    std::shared_ptr<MessageSerializer> m_message_serializer;

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
    beerocks::net::BufferImpl<broker_buffer_size> m_buffer;
};

} // namespace btl

} // namespace beerocks

#endif //BTL_BROKER_CLIENT_IMPL_H_
