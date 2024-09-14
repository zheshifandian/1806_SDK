/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <btl/message_serializer_stream_impl.h>

#include <mapf/transport/ieee1905_transport_messages.h>

#include <easylogging++.h>

namespace beerocks {
namespace btl {

bool MessageSerializerStreamImpl::serialize_message(
    const beerocks::transport::messages::Message &message, beerocks::net::Buffer &buffer)
{
    // Check if given buffer already contains some data
    if (buffer.length() > 0) {
        LOG(ERROR) << "Buffer is not empty!";
        return false;
    }

    // Get header length and data
    auto header          = message.header();
    size_t header_length = sizeof(header);
    uint8_t *header_data = reinterpret_cast<uint8_t *>(&header);

    // Get payload length and data
    size_t payload_length = header.len;
    uint8_t *payload_data = message.frame().data();

    // Check if serialized data fits into given buffer
    size_t length = header_length + payload_length;
    size_t size   = buffer.size();
    if (length > size) {
        LOG(ERROR) << "Buffer is too small! Required size: " << length << ", actual size: " << size;
        return false;
    }

    // Fill in the buffer with header and payload
    if (!buffer.append(header_data, header_length)) {
        LOG(ERROR) << "Failed appending header to the buffer!";
        return false;
    }
    if (!buffer.append(payload_data, payload_length)) {
        LOG(ERROR) << "Failed appending payload to the buffer!";
        return false;
    }

    return true;
}

} // namespace btl
} // namespace beerocks
