/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <btl/broker_client_impl.h>

#include <bcl/beerocks_cmdu_utils.h>
#include <bcl/beerocks_utils.h>
#include <bcl/network/network_utils.h>

#include <easylogging++.h>

namespace beerocks {

namespace btl {

BrokerClientImpl::BrokerClientImpl(std::unique_ptr<beerocks::net::Socket::Connection> connection,
                                   std::shared_ptr<MessageParser> message_parser,
                                   std::shared_ptr<MessageSerializer> message_serializer,
                                   std::shared_ptr<beerocks::EventLoop> event_loop)
    : m_connection(std::move(connection)), m_message_parser(message_parser),
      m_message_serializer(message_serializer), m_event_loop(event_loop)
{
    LOG_IF(!m_connection, FATAL) << "Connection is a null pointer!";
    LOG_IF(!m_message_parser, FATAL) << "Message parser is a null pointer!";
    LOG_IF(!m_message_serializer, FATAL) << "Message serializer is a null pointer!";
    LOG_IF(!m_event_loop, FATAL) << "Event loop is a null pointer!";

    // Register event handlers for the client socket
    beerocks::EventLoop::EventHandlers handlers{
        .name = "BrokerClient",
        .on_read =
            [&](int fd, EventLoop &loop) {
                handle_read(fd);
                return true;
            },

        // Not implemented
        .on_write = nullptr,

        // Fail on server socket disconnection or error
        .on_disconnect =
            [&](int fd, EventLoop &loop) {
                LOG(ERROR) << "Client socket disconnected!";
                handle_close(fd);
                // Return false to stop event-loop and controller process.
                // This is since controller is strictly dependant on the broker and cannot recover it.
                return false;
            },
        .on_error =
            [&](int fd, EventLoop &loop) {
                LOG(ERROR) << "Client socket error!";
                handle_close(fd);
                return true;
            },
    };

    LOG_IF(!m_event_loop->register_handlers(m_connection->socket()->fd(), handlers), FATAL)
        << "Failed registering event handlers for the connection!";
}

BrokerClientImpl::~BrokerClientImpl() { close_connection(true); }

bool BrokerClientImpl::subscribe(const std::set<ieee1905_1::eMessageType> &msg_types)
{
    const size_t max_msg_types =
        beerocks::transport::messages::SubscribeMessage::MAX_SUBSCRIBE_TYPES;
    if (msg_types.size() > max_msg_types) {
        LOG(ERROR) << "Subscribing to " << msg_types.size()
                   << " is not supported. Maximum number of types allowed: " << max_msg_types;

        return false;
    }

    // Build a subscription message
    beerocks::transport::messages::SubscribeMessage message;
    message.metadata()->type = beerocks::transport::messages::SubscribeMessage::ReqType::SUBSCRIBE;

    // Set the filter with the types of CMDU messages we are interested in
    message.metadata()->msg_types_count = 0;
    for (const auto &msg_type : msg_types) {
        message.metadata()->msg_types[message.metadata()->msg_types_count].bits = {
            .internal        = 0,
            .vendor_specific = 0,
            .reserved        = 0,
            .type            = static_cast<uint32_t>(msg_type)};

        ++message.metadata()->msg_types_count;
    }

    return send_message(message);
}

bool BrokerClientImpl::configure_interfaces(const std::string &iface_name,
                                            const std::string &bridge_name, bool is_bridge,
                                            bool add)
{
    beerocks::transport::messages::InterfaceConfigurationRequestMessage message;

    string_utils::copy_string(message.metadata()->iface_name, iface_name.c_str(), IF_NAMESIZE);
    string_utils::copy_string(message.metadata()->bridge_name, bridge_name.c_str(), IF_NAMESIZE);
    message.metadata()->is_bridge = is_bridge;
    message.metadata()->add       = add;

    LOG(DEBUG) << "Configuring bridge " << iface_name << " to ieee1905 transport";

    return send_message(message);
}

bool BrokerClientImpl::configure_al_mac(const sMacAddr &al_mac)
{
    beerocks::transport::messages::AlMacAddressConfigurationMessage message;

    message.metadata()->al_mac = al_mac;

    LOG(DEBUG) << "Configuring AL MAC " << al_mac << " to ieee1905 transport";

    return send_message(message);
}

bool BrokerClientImpl::send_cmdu(ieee1905_1::CmduMessageTx &cmdu_tx, const sMacAddr &dst_mac,
                                 const sMacAddr &src_mac, uint32_t iface_index)
{
    if (!cmdu_tx.is_finalized()) {
        size_t cmdu_length = cmdu_tx.getMessageLength();
        uint8_t *cmdu_data = cmdu_tx.getMessageBuff();
        if (!cmdu_tx.finalize()) {
            LOG(ERROR) << "Failed finalizing CMDU!";
            LOG(DEBUG) << "hex_dump (" << cmdu_length << " bytes):" << std::endl
                       << utils::dump_buffer(cmdu_data, cmdu_length);
            return false;
        }
    }

    return send_cmdu_message(cmdu_tx, dst_mac, src_mac, iface_index);
}

bool BrokerClientImpl::forward_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &dst_mac,
                                    const sMacAddr &src_mac, uint32_t iface_index)
{
    // Swap bytes before forwarding, from host to network byte order.
    cmdu_rx.swap();

    // Use a shared_ptr with a custom deleter and the RAII programming idiom to emulate the
    // `finally` block of a `try-finally` clause.
    std::shared_ptr<int> finally(nullptr, [&cmdu_rx](int *p) {
        // Swap bytes back from network to host byte order after forwarding.
        cmdu_rx.swap();
    });

    return send_cmdu_message(cmdu_rx, dst_mac, src_mac, iface_index);
}

void BrokerClientImpl::handle_read(int fd)
{
    // Read available bytes into buffer
    int bytes_received = m_connection->receive(m_buffer);
    if (bytes_received <= 0) {
        LOG(ERROR) << "Bytes received through connection: " << bytes_received << ", fd = " << fd;
        return;
    }

    // Transport message parsing & handling loop
    // Note: must be done in a loop because data received through a stream-oriented socket might
    // contain more than one message. If data was received through a message-oriented socket, then
    // only one message would be received at a time and the loop would be iterated only once.
    while (m_buffer.length() > 0) {
        auto message = m_message_parser->parse_message(m_buffer);
        if (message) {
            handle_message(*message);
        } else {
            // Message is incomplete.
            // Keep bytes received so far and wait till the next read event
            break;
        }
    }
}

void BrokerClientImpl::handle_close(int fd)
{
    // Close the connection
    close_connection();

    // Notify that connection with server has been closed (no more messages can be exchanged with
    // the server from now on).
    // A handler for this event might, for example, implement a recovery mechanism that includes
    // restoring the connection with the server and creating another client.
    notify_connection_closed();
}

void BrokerClientImpl::handle_message(const beerocks::transport::messages::Message &message)
{
    // Check if received message contains a CMDU
    if (beerocks::transport::messages::Type::CmduRxMessage != message.type()) {
        LOG(ERROR) << "Received non CmduRxMessage:\n\tMessage: " << message
                   << "\n\tFrame: " << message.frame().str();
        return;
    }

    auto cmdu_rx_msg =
        reinterpret_cast<const beerocks::transport::messages::CmduRxMessage *>(&message);

    // Buffer to hold CMDU received
    uint8_t cmdu_rx_buffer[message::MESSAGE_BUFFER_LENGTH];

    // Check if CMDU fits into buffer
    size_t cmdu_length = cmdu_rx_msg->metadata()->length;
    uint8_t *cmdu_data = cmdu_rx_msg->data();
    if (sizeof(cmdu_rx_buffer) < cmdu_length) {
        LOG(DEBUG) << "Buffer size (" << sizeof(cmdu_rx_buffer) << ") is less than CMDU length ("
                   << cmdu_length << ")";
        return;
    }

    // Verify CMDU
    if (!CmduUtils::verify_cmdu(cmdu_data, cmdu_length)) {
        LOG(ERROR) << "Failed verifying CMDU!";
        LOG(DEBUG) << "hex_dump (" << cmdu_length << " bytes):" << std::endl
                   << utils::dump_buffer(cmdu_data, cmdu_length);
        return;
    }

    // Copy the data to rx_buffer
    std::copy_n(cmdu_data, cmdu_length, cmdu_rx_buffer);

    // Parse CMDU
    ieee1905_1::CmduMessageRx cmdu_rx(cmdu_rx_buffer, sizeof(cmdu_rx_buffer));
    if (!cmdu_rx.parse()) {
        LOG(ERROR) << "Failed parsing CMDU!";
        LOG(DEBUG) << "hex_dump (" << cmdu_length << " bytes):" << std::endl
                   << utils::dump_buffer(cmdu_data, cmdu_length);
        return;
    }

    cmdu_rx.received_time = std::chrono::time_point<std::chrono::system_clock>(
        std::chrono::seconds(cmdu_rx_msg->metadata()->received_time));

    // Set the rest of fields to notify
    uint32_t iface_index = cmdu_rx_msg->metadata()->if_index;
    sMacAddr dst_mac     = tlvf::mac_from_array(cmdu_rx_msg->metadata()->dst);
    sMacAddr src_mac     = tlvf::mac_from_array(cmdu_rx_msg->metadata()->src);

    // Finally, notify that a CMDU has been received from broker server
    notify_cmdu_received(iface_index, dst_mac, src_mac, cmdu_rx);
}

void BrokerClientImpl::close_connection(bool remove_handlers)
{
    // If connection with server is still open ...
    if (m_connection) {

        // If requested, remove event handlers for the connected socket
        if (remove_handlers) {
            m_event_loop->remove_handlers(m_connection->socket()->fd());
        }

        // Terminate the connection
        m_connection.reset();
    }
}

bool BrokerClientImpl::send_cmdu_message(ieee1905_1::CmduMessage &cmdu, const sMacAddr &dst_mac,
                                         const sMacAddr &src_mac, uint32_t iface_index)
{
    if (beerocks::net::network_utils::ZERO_MAC == dst_mac) {
        LOG(ERROR) << "Destination MAC address is empty!";
        return false;
    }

    if (beerocks::net::network_utils::ZERO_MAC == src_mac) {
        LOG(ERROR) << "Source MAC address is empty!";
        return false;
    }

    beerocks::transport::messages::CmduTxMessage message;

    tlvf::mac_to_array(src_mac, message.metadata()->src);
    tlvf::mac_to_array(dst_mac, message.metadata()->dst);

    message.metadata()->ether_type        = ETH_P_1905_1;
    message.metadata()->length            = cmdu.getMessageLength();
    message.metadata()->msg_type          = static_cast<uint16_t>(cmdu.getMessageType());
    message.metadata()->preset_message_id = cmdu.getMessageId() ? 1 : 0;
    message.metadata()->if_index          = iface_index;

    std::copy_n(cmdu.getMessageBuff(), message.metadata()->length, message.data());

    return send_message(message);
}

bool BrokerClientImpl::send_message(const beerocks::transport::messages::Message &message)
{
    // Check if connection with server is still open
    if (!m_connection) {
        LOG(ERROR) << "Connection with server has been closed!";
        return false;
    }

    // Serialize message into a byte array
    beerocks::net::BufferImpl<broker_buffer_size> buffer;
    if (!m_message_serializer->serialize_message(message, buffer)) {
        LOG(ERROR) << "Failed to serialize message!";
        return false;
    }

    // Send data
    return m_connection->send(buffer);
}

} // namespace btl

} // namespace beerocks
