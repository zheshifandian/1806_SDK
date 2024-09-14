/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/cmdu_parser_stream_impl.h>

#include <bcl/beerocks_cmdu_utils.h>
#include <bcl/beerocks_utils.h>

namespace beerocks {
namespace net {

bool CmduParserStreamImpl::parse_cmdu(Buffer &buffer, uint32_t &iface_index, sMacAddr &dst_mac,
                                      sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    // Check if UDS Header exists
    size_t length = buffer.length();
    if (length < sizeof(message::sUdsHeader)) {
        LOG(DEBUG) << "Buffer length (" << length << ") is less than UDS header length ("
                   << sizeof(message::sUdsHeader) << ")";
        return false;
    }

    // Get pointer to received data
    uint8_t *data = buffer.data();

    // Header ready
    auto uds_header       = reinterpret_cast<message::sUdsHeader *>(data);
    size_t cmdu_length    = uds_header->length;
    uint8_t *cmdu_data    = data + sizeof(message::sUdsHeader);
    size_t message_length = sizeof(message::sUdsHeader) + cmdu_length;

    // Try to read the message
    if (length < message_length) {
        LOG(DEBUG) << "Buffer length (" << length << ") is less than UDS header length ("
                   << sizeof(message::sUdsHeader) << ") + CMDU length (" << cmdu_length << ")";
        return false;
    }

    // Verify and parse the message
    bool result = false;
    if (!CmduUtils::verify_cmdu(cmdu_data, cmdu_length)) {
        LOG(ERROR) << "Failed verifying CMDU!";
        LOG(DEBUG) << "hex_dump (" << cmdu_length << " bytes):" << std::endl
                   << utils::dump_buffer(cmdu_data, cmdu_length);
    } else if (cmdu_length > cmdu_rx.getMessageBuffLength()) {
        LOG(ERROR) << "CMDU length (" << cmdu_length << ") is greater than CMDU buffer size ("
                   << cmdu_rx.getMessageBuffLength() << ")";
    } else {
        std::copy_n(cmdu_data, cmdu_length, cmdu_rx.getMessageBuff());
        if (!cmdu_rx.parse()) {
            LOG(ERROR) << "Failed parsing CMDU!";
            LOG(DEBUG) << "hex_dump (" << cmdu_length << " bytes):" << std::endl
                       << utils::dump_buffer(cmdu_data, cmdu_length);
        } else {
            iface_index = uds_header->if_index;
            tlvf::mac_from_array(uds_header->dst_bridge_mac, dst_mac);
            tlvf::mac_from_array(uds_header->src_bridge_mac, src_mac);

            result = true;
        }
    }

    // Shift bytes remaining in buffer (i.e.: consume processed bytes and return bytes not
    // processed yet, if any)
    buffer.shift(message_length);

    return result;
}

} // namespace net
} // namespace beerocks
