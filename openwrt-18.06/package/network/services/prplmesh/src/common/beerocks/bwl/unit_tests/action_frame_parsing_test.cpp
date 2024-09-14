/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <gtest/gtest.h>

#include <bcl/beerocks_string_utils.h>
#include <bwl/base_802_11_defs.h>

namespace bwl {
namespace tests {

TEST(action_frame_parsing_test, parse_hex)
{
    // Dummy management frame
    const char mgmt_frame_hex[] = "D0453C00AC9A96FB0C62A0510B4A6ABEAC9A96FB0C62A0000A1A01DDDD08506F"
                                  "9A027328000173D30EBB7CA01E4A";

    // Store the received data as a hex string
    std::string hex_data(mgmt_frame_hex);

    // Validate the length of the received event
    // The length is divided by 2, since it's received in hex string representation
    ASSERT_FALSE(hex_data.length() / 2 < sizeof(s80211MgmtFrame::sHeader));

    // Convert the frame data from hex string to vector
    std::vector<uint8_t> data;
    data = beerocks::string_utils::hex_to_bytes<decltype(data)>(hex_data);

    // Check the type of the received event
    s80211MgmtFrame *mgmt_frame_header = reinterpret_cast<s80211MgmtFrame *>(data.data());

    // Swap fields
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    mgmt_frame_header->header.duration = (((mgmt_frame_header->header.duration & 0xFF00) >> 8) |
                                          ((mgmt_frame_header->header.duration & 0x00FF) << 8));

    mgmt_frame_header->header.seq_ctrl = (((mgmt_frame_header->header.seq_ctrl & 0xFF00) >> 8) |
                                          ((mgmt_frame_header->header.seq_ctrl & 0x00FF) << 8));
#endif

    // Validate control frame bits
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.protocol_version, 0);
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.type, 0);
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.subtype, 13); // Action Frame
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.to_ds, 1);
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.from_ds, 0);
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.more_fragments, 1);
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.retry, 0);
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.power_mgmt, 0);
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.more_data, 0);
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.protected_frame, 1);
    ASSERT_EQ(mgmt_frame_header->header.frame_control.bits.order, 0);

    // Validate header fields
    ASSERT_EQ(mgmt_frame_header->header.duration, 0x3c);
    ASSERT_EQ(mgmt_frame_header->header.seq_ctrl, 0xa0);

    const uint8_t da[] = {0xAC, 0x9A, 0x96, 0xFB, 0x0C, 0x62};
    ASSERT_EQ(memcmp(mgmt_frame_header->header.da.oct, da, sizeof(da)), 0);

    const uint8_t sa[] = {0xA0, 0x51, 0x0B, 0x4A, 0x6A, 0xBE};
    ASSERT_EQ(memcmp(mgmt_frame_header->header.sa.oct, sa, sizeof(sa)), 0);

    const uint8_t bssid[] = {0xAC, 0x9A, 0x96, 0xFB, 0x0C, 0x62};
    ASSERT_EQ(memcmp(mgmt_frame_header->header.bssid.oct, bssid, sizeof(bssid)), 0);
}

} // namespace tests
} // namespace bwl
