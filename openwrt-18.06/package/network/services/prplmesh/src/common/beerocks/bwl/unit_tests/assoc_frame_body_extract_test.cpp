/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <gtest/gtest.h>

#include <bwl/ap_wlan_hal.h>

namespace bwl {
namespace tests {

TEST(action_frame_body_extract_test, extract_body_ok)
{
    // Valid Assoc Req frame
    const char assoc_req[] = "00003A01029A96FB591100504322565F029A96FB591110E431141400000E4D75"
                             "6C74692D41502D3234472D31010802040B0C121618242102001430140100000F"
                             "AC040100000FAC040100000FAC02000032043048606C3B105151535473747576"
                             "77787C7D7E7F80823B160C01020304050C161718191A1B1C1D1E1F2021808182"
                             "46057000000000460571505000047F0A04000A82214000408000DD070050F202"
                             "0001002D1A2D1103FFFF0000000000000000000000000000000018E6E10900BF"
                             "0CB079D133FAFF0C03FAFF0C03C70110DD07506F9A16030103";

    //get assoc req frame body as binary string
    auto binary_str = bwl::ap_wlan_hal::get_binary_association_frame(assoc_req);

    // Validate the length of frame body binary buffer
    ASSERT_EQ((binary_str.length() + sizeof(bwl::s80211MgmtFrame::sHeader)) * 2,
              sizeof(assoc_req) - 1);

    // Convert the frame data from hex string to vector
    std::vector<int8_t> data;
    data = beerocks::string_utils::hex_to_bytes<decltype(data)>(std::string(assoc_req));

    //Validate the frame body content
    ASSERT_TRUE(std::equal(data.begin() + sizeof(bwl::s80211MgmtFrame::sHeader), data.end(),
                           binary_str.data()));
}

TEST(action_frame_body_extract_test, frame_too_short)
{
    // Valid Assoc Req frame
    const char assoc_req[] = "00003A01029A96FB591100504322565F029A96FB591110E4";

    //get assoc req frame body as binary string
    auto binary_str = bwl::ap_wlan_hal::get_binary_association_frame(assoc_req);

    // Validate empty frame body binary buffer
    ASSERT_EQ(binary_str.length(), 0);
}

} // namespace tests
} // namespace bwl
