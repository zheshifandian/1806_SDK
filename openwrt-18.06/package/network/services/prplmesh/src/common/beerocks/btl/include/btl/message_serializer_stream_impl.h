/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BTL_MESSAGE_SERIALIZER_STREAM_IMPL_H_
#define BTL_MESSAGE_SERIALIZER_STREAM_IMPL_H_

#include <btl/message_serializer.h>

namespace beerocks {
namespace btl {

class MessageSerializerStreamImpl : public MessageSerializer {
public:
    /**
     * @brief Serializes a transport message to a byte buffer.
     *
     * @see MessageSerializer::serialize_message
     *
     * This implementation is intended to be used together with a stream-oriented socket
     * (SOCK_STREAM in UDS or TCP).
     *
     * The framing protocol used in this implementation is a variable-sized framing protocol. Each
     * frame is made of a header plus a payload. The header includes a length field that
     * determines the size of the payload (@see beerocks::transport::messages::Message::Header).
     */
    bool serialize_message(const beerocks::transport::messages::Message &message,
                           beerocks::net::Buffer &buffer) override;
};

} // namespace btl
} // namespace beerocks

#endif /* BTL_MESSAGE_SERIALIZER_STREAM_IMPL_H_ */
