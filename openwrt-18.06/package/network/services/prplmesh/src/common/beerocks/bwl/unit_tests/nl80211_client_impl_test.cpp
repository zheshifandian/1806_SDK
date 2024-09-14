/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "bwl/nl80211_client_factory.h"

#include <tlvf/tlvftypes.h>

#include <gtest/gtest.h>

namespace {

class nl_80211_client_impl_test : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_nl80211_client = bwl::nl80211_client_factory::create_instance();
        ASSERT_FALSE(m_nl80211_client == nullptr);

        ASSERT_TRUE(m_nl80211_client->get_interfaces(m_wireless_interfaces));
    }

    std::vector<std::string> m_wireless_interfaces;
    std::unique_ptr<bwl::nl80211_client> m_nl80211_client;
};

void print_interface_info(const bwl::nl80211_client::interface_info &interface_info)
{
    std::cout << interface_info.name << ", index: " << std::to_string(interface_info.index)
              << ", MAC: " << tlvf::mac_to_string(interface_info.addr) << std::endl;
}

void print_radio_info(const std::string &interface_name,
                      const bwl::nl80211_client::radio_info &radio_info)
{
    for (const auto &band_info : radio_info.bands) {
        std::string supported_channels;
        for (const auto &entry : band_info.supported_channels) {
            auto supported_channel_number = entry.second.number;

            if (!supported_channels.empty()) {
                supported_channels += ", ";
            }
            supported_channels += std::to_string(supported_channel_number);
        }
        std::cout << interface_name << ", band: " << std::to_string(band_info.get_frequency_band())
                  << ", max_bandwidth: " << std::to_string(band_info.get_max_bandwidth())
                  << ", supported_channels: " << supported_channels << std::endl;
    }
}

TEST_F(nl_80211_client_impl_test, get_interface_info_should_succeed)
{
    for (const auto &interface_name : m_wireless_interfaces) {
        bwl::nl80211_client::interface_info interface_info;

        ASSERT_TRUE(m_nl80211_client->get_interface_info(interface_name, interface_info));
        ASSERT_EQ(interface_name, interface_info.name);
        print_interface_info(interface_info);
    }
}

TEST_F(nl_80211_client_impl_test, get_radio_info_should_succeed)
{
    for (const auto &interface_name : m_wireless_interfaces) {
        bwl::nl80211_client::radio_info radio_info;

        ASSERT_TRUE(m_nl80211_client->get_radio_info(interface_name, radio_info));
        print_radio_info(interface_name, radio_info);
    }
}
} // namespace
