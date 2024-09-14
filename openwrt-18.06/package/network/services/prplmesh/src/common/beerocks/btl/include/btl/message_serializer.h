/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BTL_MESSAGE_SERIALIZER_H_
#define BTL_MESSAGE_SERIALIZER_H_

#include <bcl/network/buffer.h>

namespace beerocks {
namespace transport {
namespace messages {

class Message;
}
} // namespace transport
} // namespace beerocks

namespace beerocks {
namespace btl {

/**
 * This interface models a transport message serializer. Implementations of this interface will be
 * used to serialize transport messages to an array of bytes to be transmitted through a socket
 * connection.
 *
 * Different implementations of this interface can be provided, depending on if the socket used is
 * message-oriented or stream-oriented and, in the later case, which framing protocol is used to
 * delimiter the start and end of messages.
 *
 * This interface and its implementations allow the separation of the logic around sockets and the
 * logic around message serializing. The goal of this separation is to be able to test message
 * serialization without having a peer connected at the other end of the wire receiving messages.
 * During testing, we can serialize any message to a buffer ready to be sent, but without actually
 * sending it. This way it is possible to test the serializer with any message but without the
 * overhead and possible errors of sending to the network.
 */
class MessageSerializer {
public:
    /**
     * @brief Class destructor
     */
    virtual ~MessageSerializer() = default;

    /**
     * @brief Serializes a transport message to a byte buffer.
     *
     * @param[in] message Message to serialize.
     * @param[in,out] buffer Array of bytes containing the serialized message.
     * @return true if message could be serialized to the buffer and false otherwise (i.e.: message
     * does not fit into output buffer).
     */
    virtual bool serialize_message(const beerocks::transport::messages::Message &message,
                                   beerocks::net::Buffer &buffer) = 0;
};

} // namespace btl
} // namespace beerocks

#endif /* BTL_MESSAGE_SERIALIZER_H_ */
