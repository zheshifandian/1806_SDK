/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_CMDU_SERIALIZER_STREAM_IMPL_H_
#define BCL_NETWORK_CMDU_SERIALIZER_STREAM_IMPL_H_

#include <bcl/network/cmdu_serializer.h>

#include <bcl/beerocks_message_structs.h>

namespace beerocks {
namespace net {

class CmduSerializerStreamImpl : public CmduSerializer {
public:
    /**
     * @brief Serializes a CMDU to a byte buffer.
     *
     * @see CmduSerializer::serialize_cmdu
     *
     * This implementation is intended to be used together with a stream-oriented socket
     * (SOCK_STREAM in UDS or TCP).
     *
     * The framing protocol used in this implementation is a variable-sized framing protocol. Each
     * frame is made of a header plus a payload. The header includes a length field that
     * determines the size of the frame. Other fields in the header the destination MAC address and
     * the source MAC address (@see beerocks::message::sUdsHeader). The payload is the CMDU "as is".
     *
     * Both source and destination MAC addresses can be ZERO_MAC but, if destination MAC address is
     * set, then source MAC address must be set too. When sending a CMDU, these two fields are
     * intended to signal the receiver that the CMDU has to be processed locally (destination MAC
     * address is ZERO_MAC) or forwarded to another machine via the transport process (source and
     * destination MAC addresses both contain a valid value). When forwarding a received CMDU, these
     * two fields together with the interface index contain the routing information: the interface
     * the CMDU was received on and source and destination MAC addresses set by the original remote
     * sender.
     */
    bool serialize_cmdu(uint32_t iface_index, const sMacAddr &dst_mac, const sMacAddr &src_mac,
                        ieee1905_1::CmduMessage &cmdu, Buffer &buffer) override;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_CMDU_SERIALIZER_STREAM_IMPL_H_ */
