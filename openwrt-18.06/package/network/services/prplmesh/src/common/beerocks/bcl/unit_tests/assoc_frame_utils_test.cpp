/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/son/son_assoc_frame_utils.h>

#include <gtest/gtest.h>

namespace {

TEST(parse_station_capabilities, sta_caps_from_rcvd_raw_data)
{
    /*
     * frame raw buffer in network byte order (S10e)
     *
     * 0x00, 0x08, 0x62, 0x64, 0x6b, 0x5f, 0x31, 0x32, 0x33, 0x34
     * SSID: bdk_1234
     *
     * 0x01, 0x08, 0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c
     * Supported Rates: 6(B), 9, 12(B), 18, 24(B), 36, 48, 54 Mbps
     *
     * 0x21, 0x02, 0xf7, 0x11
     * Power: min -9, max 20 db
     *
     * 0x24, 0x0a, 0x24, 0x04, 0x34, 0x04, 0x64, 0x0b, 0x95, 0x04, 0xa5, 0x01
     * Supported Channels ranges: 36:(4), 52:(4), 100:(11), 149:(4), 165:(1)
     *
     * 0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00,
     * 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x80, 0x00
     * RSN:
     *
     * 0x46, 0x05, 0x71, 0x08, 0x01, 0x00, 0x00
     * RM Cap: Link + Beacon(active/passive/table)
     *
     * 0x3b, 0x15, 0x80, 0x70, 0x73, 0x74, 0x75, 0x7c, 0x7d, 0x7e, 0x7f,
     * 0x80, 0x81, 0x82, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x51, 0x53, 0x54
     * Supported Operating classes:
     *
     * 0x2d, 0x1a, 0x6f, 0x00, 0x1b, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     * 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     * HT Caps: capinfo:0x006f : 20/40(+shortGI) nss:2 mcs:0-15
     *
     * 0x7f, 0x0a, 0x00, 0x00, 0x08, 0x80, 0x00, 0x00, 0x00, 0x40, 0x00, 0x20
     * Extended caps: bss transition, Interworking, Oper Mode Notif, TWT Req
     *
     * 0xbf, 0x0c, 0x32, 0x78, 0x91, 0x0f, 0xfa, 0xff, 0x00, 0x00, 0xfa, 0xff, 0x00, 0x00
     * VHT Caps: vhtCapInfo:0x0f917832, 80(+shortGI), su bfr/bfe, mu bfe, nss:2 mcs:0-9
     *
     * 0xff, 0x1c, 0x23, 0x03, 0x08, 0x00, 0x00, 0x00, 0x80, 0x64, 0x30, 0x00, 0x00,
     * 0x0d, 0x00, 0x9f, 0x00, 0x0c, 0x00, 0x00, 0xfa, 0xff, 0xfa, 0xff, 0x39, 0x1c,
     * 0xc7, 0x71, 0x1c, 0x07
     * HE Caps:
     *
     * VendorSpecific: Samsung
     * VendorSpecific: Epigram Inc.
     * VendorSpecific: Broadcom
     * VendorSpecific: MS.Corp (WMM/WME)
     * VendorSpecific: Wifi-Alliance
     */
    std::vector<uint8_t> assoc_req_frame_body_buffer = {
        0x11, 0x11, 0x0a, 0x00, 0x00, 0x08, 0x62, 0x64, 0x6b, 0x5f, 0x31, 0x32, 0x33, 0x34, 0x01,
        0x08, 0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c, 0x21, 0x02, 0xf7, 0x11, 0x24, 0x0a,
        0x24, 0x04, 0x34, 0x04, 0x64, 0x0b, 0x95, 0x04, 0xa5, 0x01, 0x30, 0x14, 0x01, 0x00, 0x00,
        0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02,
        0x80, 0x00, 0x46, 0x05, 0x71, 0x08, 0x01, 0x00, 0x00, 0x3b, 0x15, 0x80, 0x70, 0x73, 0x74,
        0x75, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x51,
        0x53, 0x54, 0x2d, 0x1a, 0x6f, 0x00, 0x1b, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x7f, 0x0a, 0x00, 0x00, 0x08, 0x80, 0x00, 0x00, 0x00, 0x40, 0x00, 0x20, 0xbf, 0x0c, 0x32,
        0x78, 0x91, 0x0f, 0xfa, 0xff, 0x00, 0x00, 0xfa, 0xff, 0x00, 0x00, 0xff, 0x1c, 0x23, 0x03,
        0x08, 0x00, 0x00, 0x00, 0x80, 0x64, 0x30, 0x00, 0x00, 0x0d, 0x00, 0x9f, 0x00, 0x0c, 0x00,
        0x00, 0xfa, 0xff, 0xfa, 0xff, 0x39, 0x1c, 0xc7, 0x71, 0x1c, 0x07, 0xdd, 0x0b, 0x00, 0x00,
        0xf0, 0x22, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x03, 0xdd, 0x05, 0x00, 0x90, 0x4c, 0x04,
        0x17, 0xdd, 0x0a, 0x00, 0x10, 0x18, 0x02, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xdd, 0x07,
        0x00, 0x50, 0xf2, 0x02, 0x00, 0x01, 0x00, 0xdd, 0x09, 0x50, 0x6f, 0x9a, 0x16, 0x02, 0x00,
        0x03, 0x01, 0x01};

    auto assoc_frame = assoc_frame::AssocReqFrame::parse(
        assoc_req_frame_body_buffer.data(), assoc_req_frame_body_buffer.size(),
        assoc_frame::AssocReqFrame::ASSOCIATION_REQUEST);

    beerocks::message::sRadioCapabilities resCaps = {};
    auto success =
        son::assoc_frame_utils::get_station_capabilities_from_assoc_frame(assoc_frame, resCaps);
    EXPECT_TRUE(success);

    /*
     * expected capabilities
     */
    const auto staCaps = []() {
        beerocks::message::sRadioCapabilities caps;
        caps.ant_num       = 2;
        caps.wifi_standard = beerocks::STANDARD_A | beerocks::STANDARD_N | beerocks::STANDARD_AC |
                             beerocks::STANDARD_AX;
        caps.ht_ss                 = 2;
        caps.ht_mcs                = beerocks::MCS_7;
        caps.ht_bw                 = beerocks::BANDWIDTH_40;
        caps.ht_low_bw_short_gi    = 1;
        caps.ht_high_bw_short_gi   = 1;
        caps.ht_sm_power_save      = beerocks::HT_SM_POWER_SAVE_MODE_DISABLED;
        caps.vht_ss                = 2;
        caps.vht_mcs               = beerocks::MCS_9;
        caps.vht_bw                = beerocks::BANDWIDTH_80;
        caps.vht_low_bw_short_gi   = 1;
        caps.vht_high_bw_short_gi  = 0;
        caps.rrm_supported         = 1;
        caps.nr_enabled            = 0;
        caps.link_meas             = 1;
        caps.beacon_report_passive = 1;
        caps.beacon_report_active  = 1;
        caps.beacon_report_table   = 1;
        caps.lci_meas              = 0;
        caps.fmt_range_report      = 0;
        caps.btm_supported         = 1;
        caps.band_2g_capable       = 0;
        caps.band_5g_capable       = 1;
        caps.max_ch_width          = beerocks::BANDWIDTH_80;
        caps.max_mcs               = beerocks::MCS_9;
        caps.max_tx_power          = 17;
        caps.max_streams           = 2;
        return caps;
    }();

    EXPECT_EQ(resCaps.ant_num, staCaps.ant_num);
    EXPECT_EQ(resCaps.wifi_standard, staCaps.wifi_standard);
    EXPECT_EQ(resCaps.ht_ss, staCaps.ht_ss);
    EXPECT_EQ(resCaps.ht_mcs, staCaps.ht_mcs);
    EXPECT_EQ(resCaps.ht_bw, staCaps.ht_bw);
    EXPECT_EQ(resCaps.ht_low_bw_short_gi, staCaps.ht_low_bw_short_gi);
    EXPECT_EQ(resCaps.vht_high_bw_short_gi, staCaps.vht_high_bw_short_gi);
    EXPECT_EQ(resCaps.ht_sm_power_save, staCaps.ht_sm_power_save);
    EXPECT_EQ(resCaps.vht_ss, staCaps.vht_ss);
    EXPECT_EQ(resCaps.vht_mcs, staCaps.vht_mcs);
    EXPECT_EQ(resCaps.vht_bw, staCaps.vht_bw);
    EXPECT_EQ(resCaps.vht_low_bw_short_gi, staCaps.vht_low_bw_short_gi);
    EXPECT_EQ(resCaps.vht_high_bw_short_gi, staCaps.vht_high_bw_short_gi);
    EXPECT_EQ(resCaps.rrm_supported, staCaps.rrm_supported);
    EXPECT_EQ(resCaps.link_meas, staCaps.link_meas);
    EXPECT_EQ(resCaps.beacon_report_passive, staCaps.beacon_report_passive);
    EXPECT_EQ(resCaps.beacon_report_active, staCaps.beacon_report_active);
    EXPECT_EQ(resCaps.beacon_report_table, staCaps.beacon_report_table);
    EXPECT_EQ(resCaps.lci_meas, staCaps.lci_meas);
    EXPECT_EQ(resCaps.fmt_range_report, staCaps.fmt_range_report);
    EXPECT_EQ(resCaps.btm_supported, staCaps.btm_supported);
    EXPECT_EQ(resCaps.band_2g_capable, staCaps.band_2g_capable);
    EXPECT_EQ(resCaps.band_5g_capable, staCaps.band_5g_capable);
    EXPECT_EQ(resCaps.max_ch_width, staCaps.max_ch_width);
    EXPECT_EQ(resCaps.max_mcs, staCaps.max_mcs);
    EXPECT_EQ(resCaps.max_tx_power, staCaps.max_tx_power);
    EXPECT_EQ(resCaps.max_streams, staCaps.max_streams);
}

TEST(parse_station_capabilities, sta_caps_from_formatted_assoc_frame)
{
    std::array<uint8_t, 2048> assoc_frame_body = {};

    auto fields = std::make_shared<assoc_frame::AssocReqFrame>(
        assoc_frame_body.data(), assoc_frame_body.size(),
        assoc_frame::AssocReqFrame::ASSOCIATION_REQUEST, false);

    EXPECT_FALSE(!fields);

    /*
     * mandatory IEs in the formatted assoc req frame
     * 1) add empty capInfoDmgSta field (capInfo + ListenInterval)
     */
    fields->addAttr<assoc_frame::cCapInfoDmgSta>();

    /*
     * 2) add empty ssid field
     */
    fields->addAttr<assoc_frame::cSSID>();

    /*
     * 3) add HT Capability field
     * frame in network byte order:
     * 2D 1A 2D00 1B FF000000000000000000000000000000 0000 18E6E109 00
     * => ID_HT_CAPABILITY: capinfo 002d sm_power disable, htLdpc, 20(+shortGI), nss_1, mcs 0-7
     */
    auto htCapField = fields->addAttr<assoc_frame::cStaHtCapability>();
    EXPECT_FALSE(!htCapField);

    // filling HT cap info value, instead of setting it bit per bit
    htCapField->ht_cap_info() =
        assoc_frame::convert<uint16_t, assoc_frame::sStaHtCapabilityInfo>(0x002D);

    // filling ampdu param value, instead of setting it bit per bit
    htCapField->a_mpdu_param() =
        assoc_frame::convert<uint8_t, assoc_frame::cStaHtCapability::sA_MpduParam>(0x1B);

    // filling the first 4 mcs group (ss 1->4)
    const std::vector<uint8_t> ht_mcs_values = {
        0xff,
        0x00,
        0x00,
        0x00,
    };
    EXPECT_TRUE(htCapField->set_ht_mcs_set(static_cast<const void *>(ht_mcs_values.data()),
                                           ht_mcs_values.size()));

    // filling tx_beamforming_caps value
    htCapField->tx_beamforming_caps() = 0x09E1E618;

    /*
     * 4) add VHT Capability field
     * frame in network byte order:
     * BF 0C 3278910F FDFF 0000 FDFF 0000
     * => ID_VHT_CAPS vht_cap_info:0F917832 rx_ldpc, 80+shortGI, rx_ldpc, su bfr/bfe, mu bfe, nss 1, mcs 0-8
     */
    auto vhtCapField = fields->addAttr<assoc_frame::cStaVhtCapability>();
    EXPECT_FALSE(!vhtCapField);

    // filling VHT cap info value, instead of setting it bit per bit
    vhtCapField->vht_cap_info() =
        assoc_frame::convert<uint32_t, assoc_frame::sStaVhtCapInfo>(0x0F917832);

    // filling rx_mcs_map
    vhtCapField->supported_vht_mcs().rx_mcs_map = 0xFFFD;

    // filling tx_mcs_map
    vhtCapField->supported_vht_mcs().tx_mcs_map = 0xFFFD;

    // finalize the assoc req frame: => ready to be sent (buffer in network byte order)
    fields->finalize();

    // tweak: swap the assoc_frame obj get data in host byte order:
    // Otherwise, we need to parse the built assoc_frame object buffer
    // into a new parsed assoc_frame object and use it instead
    fields->swap();

    beerocks::message::sRadioCapabilities resCaps = {};
    auto success =
        son::assoc_frame_utils::get_station_capabilities_from_assoc_frame(fields, resCaps);
    EXPECT_TRUE(success);

    /*
     * expected capabilities
     */
    const auto staCaps = []() {
        beerocks::message::sRadioCapabilities caps;
        caps.ant_num       = 1;
        caps.wifi_standard = beerocks::STANDARD_A | beerocks::STANDARD_N | beerocks::STANDARD_AC;
        caps.ht_ss         = 1;
        caps.ht_mcs        = beerocks::MCS_7;
        caps.ht_bw         = beerocks::BANDWIDTH_20;
        caps.ht_low_bw_short_gi   = 1;
        caps.ht_high_bw_short_gi  = 0;
        caps.ht_sm_power_save     = beerocks::HT_SM_POWER_SAVE_MODE_DISABLED;
        caps.vht_ss               = 1;
        caps.vht_mcs              = beerocks::MCS_8;
        caps.vht_bw               = beerocks::BANDWIDTH_80;
        caps.vht_low_bw_short_gi  = 1;
        caps.vht_high_bw_short_gi = 0;
        caps.band_5g_capable      = 1;
        caps.max_ch_width         = beerocks::BANDWIDTH_80;
        caps.max_mcs              = beerocks::MCS_8;
        caps.max_streams          = 1;
        return caps;
    }();

    EXPECT_EQ(resCaps.ant_num, staCaps.ant_num);
    EXPECT_EQ(resCaps.wifi_standard, staCaps.wifi_standard);
    EXPECT_EQ(resCaps.ht_ss, staCaps.ht_ss);
    EXPECT_EQ(resCaps.ht_mcs, staCaps.ht_mcs);
    EXPECT_EQ(resCaps.ht_bw, staCaps.ht_bw);
    EXPECT_EQ(resCaps.ht_low_bw_short_gi, staCaps.ht_low_bw_short_gi);
    EXPECT_EQ(resCaps.ht_sm_power_save, staCaps.ht_sm_power_save);
    EXPECT_EQ(resCaps.vht_high_bw_short_gi, staCaps.vht_high_bw_short_gi);
    EXPECT_EQ(resCaps.vht_ss, staCaps.vht_ss);
    EXPECT_EQ(resCaps.vht_mcs, staCaps.vht_mcs);
    EXPECT_EQ(resCaps.vht_bw, staCaps.vht_bw);
    EXPECT_EQ(resCaps.vht_low_bw_short_gi, staCaps.vht_low_bw_short_gi);
    EXPECT_EQ(resCaps.vht_high_bw_short_gi, staCaps.vht_high_bw_short_gi);
    EXPECT_EQ(resCaps.band_5g_capable, staCaps.band_5g_capable);
    EXPECT_EQ(resCaps.max_ch_width, staCaps.max_ch_width);
    EXPECT_EQ(resCaps.max_mcs, staCaps.max_mcs);
    EXPECT_EQ(resCaps.max_streams, staCaps.max_streams);
}

} // namespace
