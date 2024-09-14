/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <btl/message_parser_stream_impl.h>

#include <mapf/transport/ieee1905_transport_messages.h>

#include <easylogging++.h>

namespace beerocks {
namespace btl {

std::unique_ptr<beerocks::transport::messages::Message>
MessageParserStreamImpl::parse_message(beerocks::net::Buffer &buffer)
{
    // Check if header exists
    size_t length        = buffer.length();
    size_t header_length = sizeof(beerocks::transport::messages::Message::Header);
    if (length < header_length) {
        LOG(DEBUG) << "Buffer length (" << length << ") is less than header length ("
                   << header_length << ")";
        return nullptr;
    }

    // Get pointer to received data
    uint8_t *data = buffer.data();

    // Get the header
    auto header = reinterpret_cast<beerocks::transport::messages::Message::Header *>(data);

    // Reading an invalid message magic or length means that somehow the data synchronization
    // was lost. Since the data is not necessarily aligned to any known size, we have two
    // options here:
    // 1. Slowest (but safer) - Discard 1 byte at a time, until finding the magic word
    // 2. Faster - Discard sizeof(Header) bytes and see if a valid header comes after
    // 2. Fastest - Discard all the bytes and assume the sender will re-send the message
    // For now we'll use the "Faster" method. If we ever move to a message-oriented socket,
    // then this will not be necessary at all
    auto discard_invalid_data = [&]() {
        // Discard the header and trust that a valid header comes after those bytes
        buffer.shift(header_length);
    };

    // Validate the header
    if (beerocks::transport::messages::Message::kMessageMagic != header->magic) {
        LOG(ERROR) << "Invalid message header: magic = 0x" << std::hex << header->magic << std::dec
                   << ", length = " << header->len;
        discard_invalid_data();
        return nullptr;
    }

    // Validate payload length
    size_t payload_length = header->len;
    if (payload_length > beerocks::transport::messages::Message::kMaxFrameLength) {
        LOG(ERROR) << "Message length is greater than maximum length: " << payload_length << " > "
                   << beerocks::transport::messages::Message::kMaxFrameLength;
        discard_invalid_data();
        return nullptr;
    }

    // Check if the full message has been received
    size_t message_length = header_length + payload_length;
    if (length < message_length) {
        LOG(DEBUG) << "Buffer length (" << length << ") is less than header length ("
                   << header_length << ") + payload length (" << payload_length << ")";
        return nullptr;
    }

    // Create transport message according to its type and with the payload received
    auto type = static_cast<beerocks::transport::messages::Type>(header->type);
    std::unique_ptr<beerocks::transport::messages::Message> message;
    if (0 == payload_length) {
        message = beerocks::transport::messages::create_transport_message(type, {});
    } else {
        beerocks::transport::messages::Message::Frame frame(payload_length);
        std::copy_n(data + header_length, payload_length, frame.data());

        message = beerocks::transport::messages::create_transport_message(type, {frame});
    }

    // Shift bytes remaining in buffer (i.e.: consume processed bytes and return bytes not
    // processed yet, if any)
    buffer.shift(message_length);

    LOG_IF(!message, ERROR) << "Failed creating message object for type: " << header->type;
    return message;
}

} // namespace btl
} // namespace beerocks
