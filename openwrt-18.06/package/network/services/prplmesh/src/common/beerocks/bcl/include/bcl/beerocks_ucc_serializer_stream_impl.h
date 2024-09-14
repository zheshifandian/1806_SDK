/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_UCC_SERIALIZER_STREAM_IMPL_H_
#define _BEEROCKS_UCC_SERIALIZER_STREAM_IMPL_H_

#include <bcl/beerocks_ucc_serializer.h>

namespace beerocks {

class UccSerializerStreamImpl : public UccSerializer {
public:
    /**
     * @brief Serializes a UCC reply string to a byte buffer.
     *
     * @see UccSerializer::serialize_reply
     *
     * This implementation is intended to be used together with a stream-oriented socket
     * (SOCK_STREAM in UDS or TCP).
     *
     * The framing protocol used in this implementation consists of a Line Feed ("LF") character
     * (0x0A, \n) added at the end of the reply.
     */
    bool serialize_reply(const std::string &reply, beerocks::net::Buffer &buffer) override;
};

} // namespace beerocks

#endif /* _BEEROCKS_UCC_SERIALIZER_STREAM_IMPL_H_ */
