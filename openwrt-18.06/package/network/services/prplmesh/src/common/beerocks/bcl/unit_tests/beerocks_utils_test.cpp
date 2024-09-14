/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_utils.h>

#include <gtest/gtest.h>

#include <unordered_map>

namespace {

TEST(BeerocksUtilsTest, extract_prefix_from_iface_name)
{
    std::unordered_map<std::string, std::string> maps = {
        /* success cases */
        {"wl", "wl"},
        {"wlan0", "wlan"},
        {"wlan0.0", "wlan"},
        {"eth10.10", "eth"},
        {"eth1p0.10", "eth1p"},
        {"wlan1-1", "wlan"},
        /* error cases */
        {"", ""},
        {"10", ""},
        {"eth10.x10", ""},
        {"eth10.10x", ""},
        {"wlan1-2.sta1", ""},
        {"wlan1-1.1", "wlan1-"},
    };

    for (auto &it : maps) {
        ASSERT_STREQ(beerocks::utils::get_prefix_from_iface_string(it.first).c_str(),
                     it.second.c_str());
    }
}

TEST(BeerocksUtilsTest, check_allowed_iface_prefix_regardless_name_format)
{
    std::map<std::pair<std::string, bool>, bool> maps = {
        /* success cases */
        {std::make_pair("wl", false), true},
        {std::make_pair("wlan", false), true},
        {std::make_pair("wl1.1", true), true},
        {std::make_pair("wlan0", true), true},
        {std::make_pair("wlx0", true), true},
        {std::make_pair("wlan1-2", true), true},
        {std::make_pair("wlan1-2.sta1", true), true},
        /* error cases */
        {std::make_pair("lwl", true), false},
        {std::make_pair("wl0", false), false},
        {std::make_pair("wlan1-2", false), false},
        {std::make_pair("eth0", false), false},
        {std::make_pair("eth0", true), false}};

    for (auto &it : maps) {
        ASSERT_EQ(beerocks::utils::is_allowed_ifname_prefix(it.first.first, it.first.second),
                  it.second);
    }
}

TEST(BeerocksUtilsTest, extract_ids_from_iface_name)
{
    std::unordered_map<std::string, std::pair<int8_t, int8_t>> maps = {
        /* success cases */
        {"wl0", std::make_pair(0, beerocks::IFACE_RADIO_ID)},
        {"wl1", std::make_pair(1, beerocks::IFACE_RADIO_ID)},
        {"wl1.0", std::make_pair(1, 0)},
        {"wlan1", std::make_pair(1, beerocks::IFACE_RADIO_ID)},
        {"wlan1-2", std::make_pair(1, 2)},
        /* error cases */
        {"wl", std::make_pair(beerocks::IFACE_ID_INVALID, beerocks::IFACE_ID_INVALID)},
        {"eth1", std::make_pair(beerocks::IFACE_ID_INVALID, beerocks::IFACE_ID_INVALID)},
        {"wl1.100", std::make_pair(1, beerocks::IFACE_ID_INVALID)},
        {"wlan2-100", std::make_pair(2, beerocks::IFACE_ID_INVALID)},
        {"wlan1-2.sta1", std::make_pair(beerocks::IFACE_ID_INVALID, beerocks::IFACE_ID_INVALID)},
        {"wlan1-2.1", std::make_pair(beerocks::IFACE_ID_INVALID, beerocks::IFACE_ID_INVALID)},
    };

    for (auto &it : maps) {
        auto iface_ids = beerocks::utils::get_ids_from_iface_string(it.first);
        ASSERT_EQ(iface_ids.iface_id, it.second.first);
        ASSERT_EQ(iface_ids.vap_id, it.second.second);
    }
}

TEST(BeerocksUtilsTest, format_iface_name_from_ids)
{
    std::map<std::pair<std::string, int8_t>, std::string> maps = {
        /* success cases */
        {std::make_pair("wl0", beerocks::IFACE_RADIO_ID), "wl0"},
        {std::make_pair("wlan1", beerocks::IFACE_RADIO_ID), "wlan1"},
        {std::make_pair("wl1.1", 2), "wl1.2"},
        {std::make_pair("wlan0-1", 2), "wlan0-2"},
        {std::make_pair("wlan0-1", beerocks::IFACE_RADIO_ID), "wlan0"},
        {std::make_pair("wl0", 0), "wl0.0"},
        {std::make_pair("wlan1", 1), "wlan1.1"},
        /* error cases */
        {std::make_pair("wl", 0), ""},
        {std::make_pair("wlx0", 0), ""},
        {std::make_pair("wlx0", beerocks::IFACE_RADIO_ID), ""},
        {std::make_pair("wlan1-2", beerocks::IFACE_VAP_ID_MAX + 1), ""},
        {std::make_pair("eth0", 0), ""}};

    for (auto &it : maps) {
        auto result =
            beerocks::utils::get_iface_string_from_iface_vap_ids(it.first.first, it.first.second);
        ASSERT_STREQ(result.c_str(), it.second.c_str());
    }
}

} // namespace
