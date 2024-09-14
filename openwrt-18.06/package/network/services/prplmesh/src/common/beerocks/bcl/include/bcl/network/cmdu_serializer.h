/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_CMDU_SERIALIZER_H_
#define BCL_NETWORK_CMDU_SERIALIZER_H_

#include <bcl/network/buffer.h>

#include <tlvf/CmduMessage.h>
#include <tlvf/common/sMacAddr.h>

namespace beerocks {
namespace net {

/**
 * This interface models a CMDU (Control Message Data Unit) serializer. Implementations of this
 * interface will be used to serialize CMDU messages to an array of bytes to be transmitted through
 * a socket connection.
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
class CmduSerializer {
public:
    /**
     * @brief Class destructor
     */
    virtual ~CmduSerializer() = default;

    /**
     * @brief Serializes a CMDU to a byte buffer.
     *
     * @param[in] iface_index Index of the network interface the message was received on.
     * @param[in] dst_mac Destination MAC address.
     * @param[in] src_mac Source MAC address.
     * @param[in/out] cmdu CMDU message to serialize.
     * @param[in,out] buffer Array of bytes containing the serialized message.
     * @return true if CMDU could be serialized to the buffer and false otherwise (i.e.: message
     * does not fit into output buffer).
     */
    virtual bool serialize_cmdu(uint32_t iface_index, const sMacAddr &dst_mac,
                                const sMacAddr &src_mac, ieee1905_1::CmduMessage &cmdu,
                                Buffer &buffer) = 0;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_CMDU_SERIALIZER_H_ */
