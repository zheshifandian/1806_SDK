/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_utils.h>
#include <bcl/son/son_assoc_frame_utils.h>
#include <bcl/son/son_wireless_utils.h>

#include <easylogging++.h>

using namespace son;

void assoc_frame_utils::get_default_mcs_from_supported_rates(const std::vector<uint8_t> &supp_rates,
                                                             uint8_t &default_mcs,
                                                             uint8_t &default_short_gi)
{
    uint16_t max_rate_100kb = 0;
    for (auto rate : supp_rates) {
        max_rate_100kb = std::max(max_rate_100kb, uint16_t((rate & 0x7F) * 5));
    }
    son::wireless_utils::get_mcs_from_rate(max_rate_100kb, beerocks::ANT_MODE_1X1_SS1,
                                           beerocks::BANDWIDTH_20, default_mcs, default_short_gi);
}

namespace son {
template <>
bool assoc_frame_utils::get_station_capabilities_from_assoc_field<>(
    const std::shared_ptr<assoc_frame::cSupportedChannels> &suppChans,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!suppChans) {
        return false;
    }
    size_t idx       = 0;
    auto findChanSet = suppChans->supported_channel_sets(idx);
    while (std::get<0>(findChanSet)) {
        auto freqType = wireless_utils::which_freq(std::get<1>(findChanSet).first_ch_num);
        sta_caps.band_2g_capable = (freqType == beerocks::eFreqType::FREQ_24G);
        sta_caps.band_5g_capable = (freqType == beerocks::eFreqType::FREQ_5G);
        findChanSet              = suppChans->supported_channel_sets(++idx);
    }
    return true;
}

template <>
bool assoc_frame_utils::get_station_capabilities_from_assoc_field<>(
    const std::shared_ptr<assoc_frame::cRmEnabledCaps> &rmCaps,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!rmCaps) {
        return false;
    }
    auto data1                     = rmCaps->data1();
    auto data2                     = rmCaps->data2();
    sta_caps.rrm_supported         = 1;
    sta_caps.nr_enabled            = data1.neighbor_report;
    sta_caps.link_meas             = data1.link_measurement;
    sta_caps.beacon_report_passive = data1.beacon_passive_measure;
    sta_caps.beacon_report_active  = data1.beacon_active_measure;
    sta_caps.beacon_report_table   = data1.beacon_table_measure;
    sta_caps.lci_meas              = data1.lci_measure;
    sta_caps.fmt_range_report      = data2.ftm_range_report;
    return true;
}

template <>
bool assoc_frame_utils::get_station_capabilities_from_assoc_field<>(
    const std::shared_ptr<assoc_frame::cExtendedCap> &extCap,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!extCap) {
        return false;
    }
    auto extCapB3 = reinterpret_cast<assoc_frame::cExtendedCap::sExtendedCapFieldB3 *>(
        extCap->extended_cap_field(2));
    if (extCapB3) {
        sta_caps.btm_supported = extCapB3->bss_transition;
    }
    return true;
}

template <>
bool assoc_frame_utils::get_station_capabilities_from_assoc_field<>(
    const std::shared_ptr<assoc_frame::cSupportRates> &suppRates,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!suppRates) {
        return false;
    }
    std::vector<uint8_t> rates(suppRates->supported_rated(),
                               suppRates->supported_rated() + suppRates->length());
    get_default_mcs_from_supported_rates(rates, sta_caps.default_mcs, sta_caps.default_short_gi);
    return true;
}

template <>
bool assoc_frame_utils::get_station_capabilities_from_assoc_field<>(
    const std::shared_ptr<assoc_frame::cExtendedSupportRates> &extSuppRates,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!extSuppRates) {
        return false;
    }
    std::vector<uint8_t> rates(extSuppRates->extended_suport_rated(),
                               extSuppRates->extended_suport_rated() + extSuppRates->length());
    get_default_mcs_from_supported_rates(rates, sta_caps.default_mcs, sta_caps.default_short_gi);
    return true;
}

template <>
bool assoc_frame_utils::get_station_capabilities_from_assoc_field<>(
    const std::shared_ptr<assoc_frame::cPowerCapability> &pwrCap,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!pwrCap) {
        return false;
    }
    sta_caps.max_tx_power = pwrCap->max_tx_power();
    return true;
}

template <>
bool assoc_frame_utils::get_station_capabilities_from_assoc_field<>(
    const std::shared_ptr<assoc_frame::cMultiBand> &multiBand,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!multiBand) {
        return false;
    }
    switch (multiBand->band_id()) {
    case assoc_frame::cMultiBand::BAND_2_4_GHZ:
        sta_caps.band_2g_capable = 1;
        break;
    case assoc_frame::cMultiBand::BAND_4_9_AND_5_GHZ:
        sta_caps.band_5g_capable = 1;
        break;
    default:
        break;
    }
    return true;
}

template <>
bool assoc_frame_utils::get_station_capabilities_from_assoc_field<>(
    const std::shared_ptr<assoc_frame::cStaHtCapability> &htcap,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!htcap) {
        return false;
    }
    sta_caps.ht_ss   = 1;
    auto ht_cap_info = htcap->ht_cap_info();

    // 20 == 0 / 40 == 1
    if (ht_cap_info.support_ch_width_set) {
        sta_caps.ht_bw = beerocks::BANDWIDTH_40;
    } else {
        sta_caps.ht_bw = beerocks::BANDWIDTH_20;
    }
    sta_caps.ht_sm_power_save =
        ht_cap_info.sm_power_save; // 0=static, 1=dynamic, 2=reserved, 3=disabled
    sta_caps.ht_low_bw_short_gi  = ht_cap_info.short_gi20mhz;
    sta_caps.ht_high_bw_short_gi = ht_cap_info.short_gi40mhz;
    //up to 4 ss x 8 bits => ht mcs 0-31
    for (int8_t idx = 3; idx >= 0; idx--) {
        auto mcs_set = htcap->ht_mcs_set(idx);
        for (int8_t flag = 7; mcs_set && flag >= 0; flag--) {
            if (*mcs_set & (1 << flag)) {
                sta_caps.ht_ss   = idx + 1;
                sta_caps.ant_num = idx + 1;
                sta_caps.ht_mcs  = flag;
                idx              = 0;
                break;
            }
        }
    }
    sta_caps.max_mcs      = sta_caps.ht_mcs;
    sta_caps.max_streams  = sta_caps.ht_ss;
    sta_caps.max_ch_width = sta_caps.ht_bw;
    return true;
}

template <>
bool assoc_frame_utils::get_station_capabilities_from_assoc_field<>(
    const std::shared_ptr<assoc_frame::cStaVhtCapability> &vhtcap,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!vhtcap) {
        return false;
    }
    auto vht_cap_info         = vhtcap->vht_cap_info();
    uint8_t supported_bw_bits = vht_cap_info.support_ch_width_set;
    if (supported_bw_bits == 0x03) { // reserved mode
        LOG(ERROR) << "INFORMATION ERROR! STA SENT RESERVED BIT COMBINATION";
    }

    // if supported_bw_bits=0 max bw is 80 Mhz, else max bw is 160 Mhz
    sta_caps.vht_bw = beerocks::BANDWIDTH_80;
    if (supported_bw_bits > 0) {
        sta_caps.vht_bw = beerocks::BANDWIDTH_160;
    }
    sta_caps.vht_low_bw_short_gi  = vht_cap_info.short_gi80mhz_tvht_mode4c; // 80 Mhz
    sta_caps.vht_high_bw_short_gi = vht_cap_info.short_gi160mhz80_80mhz;    // 160 Mhz

    uint16_t vht_mcs_rx = static_cast<uint16_t>(vhtcap->supported_vht_mcs().rx_mcs_map);
    for (uint8_t i = 8; i > 0; i--) { // up to 8ss
        auto vht_mcs_temp = (vht_mcs_rx >> (2 * (i - 1))) & 0x03;
        // 0 indicates support for VHT-MCS 0-7 for n spatial streams
        // 1 indicates support for VHT-MCS 0-8 for n spatial streams
        // 2 indicates support for VHT-MCS 0-9 for n spatial streams
        // 3 indicates that n spatial streams is not supported
        if (vht_mcs_temp != 0x3) { //0x3 == not supported
            sta_caps.ant_num = std::max(i, sta_caps.ant_num);
            sta_caps.vht_ss  = i;
            sta_caps.vht_mcs = vht_mcs_temp + 7;
            break;
        }
    }
    sta_caps.max_mcs      = sta_caps.vht_mcs;
    sta_caps.max_streams  = sta_caps.vht_ss;
    sta_caps.max_ch_width = sta_caps.vht_bw;

    sta_caps.vht_su_beamformer = vht_cap_info.su_beamformer;
    sta_caps.vht_mu_beamformer = vht_cap_info.mu_beamformer;
    // 11ac-wave2: mumimo_supported if mu_beamformer or mu_beamformee ?
    return true;
}

template <>
bool assoc_frame_utils::get_station_capabilities_from_assoc_field<>(
    const std::shared_ptr<assoc_frame::cStaHeCapability> &hecap,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!hecap) {
        return false;
    }
    sta_caps.wifi_standard |= beerocks::STANDARD_AX;
    auto phyCapInfoB1 =
        reinterpret_cast<assoc_frame::cStaHeCapability::sHePhyCapInfoB1 *>(hecap->phy_cap_info(0));
    if (phyCapInfoB1) {
        //detect support multi-band
        if (phyCapInfoB1->bw_40_in_2_4) {
            sta_caps.band_2g_capable = 1;
        }
        if (phyCapInfoB1->bw_40_80_in_5 || phyCapInfoB1->bw_160_in_5) {
            sta_caps.band_5g_capable = 1;
        }
    }
    // mumimo_supported, based on HE Phy Cap Inf:
    // Partial DL MU-MIMO, Partial/Full UL MU-MO
    return true;
}
} // namespace son

bool assoc_frame_utils::get_station_capabilities_from_assoc_frame(
    const std::shared_ptr<assoc_frame::AssocReqFrame> &assoc_frame,
    beerocks::message::sRadioCapabilities &sta_caps)
{
    if (!assoc_frame) {
        return false;
    }
    sta_caps.ant_num      = 1;
    sta_caps.ht_bw        = beerocks::BANDWIDTH_UNKNOWN;
    sta_caps.vht_bw       = beerocks::BANDWIDTH_UNKNOWN;
    sta_caps.max_ch_width = beerocks::BANDWIDTH_20;
    sta_caps.max_streams  = 1;

    auto suppChans = assoc_frame->getAttr<assoc_frame::cSupportedChannels>();
    get_station_capabilities_from_assoc_field<>(suppChans, sta_caps);

    // init standard
    if (sta_caps.band_2g_capable) {
        sta_caps.wifi_standard = beerocks::STANDARD_B;
        sta_caps.wifi_standard |= beerocks::STANDARD_G;
    }
    if (sta_caps.band_5g_capable) {
        sta_caps.wifi_standard = beerocks::STANDARD_A;
    }

    auto htcap = assoc_frame->getAttr<assoc_frame::cStaHtCapability>();
    get_station_capabilities_from_assoc_field<>(htcap, sta_caps);

    auto vhtcap = assoc_frame->getAttr<assoc_frame::cStaVhtCapability>();
    get_station_capabilities_from_assoc_field<>(vhtcap, sta_caps);

    auto hecap = assoc_frame->getAttr<assoc_frame::cStaHeCapability>();
    get_station_capabilities_from_assoc_field<>(hecap, sta_caps);

    // update standard
    if (sta_caps.ht_ss) {
        sta_caps.wifi_standard |= beerocks::STANDARD_N;
    }
    if (sta_caps.vht_ss) {
        sta_caps.wifi_standard |= beerocks::STANDARD_AC;

        //VHT only supported in freq band 5GHz
        sta_caps.band_5g_capable = 1;
        sta_caps.wifi_standard |= beerocks::STANDARD_A;
    }

    auto rmCaps = assoc_frame->getAttr<assoc_frame::cRmEnabledCaps>();
    get_station_capabilities_from_assoc_field<>(rmCaps, sta_caps);

    auto extCap = assoc_frame->getAttr<assoc_frame::cExtendedCap>();
    get_station_capabilities_from_assoc_field<>(extCap, sta_caps);

    std::vector<uint8_t> rates;
    auto suppRates = assoc_frame->getAttr<assoc_frame::cSupportRates>();
    if (suppRates) {
        rates.insert(rates.end(), suppRates->supported_rated(),
                     suppRates->supported_rated() + suppRates->length());
    }
    auto extSuppRates = assoc_frame->getAttr<assoc_frame::cExtendedSupportRates>();
    if (extSuppRates) {
        rates.insert(rates.end(), extSuppRates->extended_suport_rated(),
                     extSuppRates->extended_suport_rated() + extSuppRates->length());
    }
    get_default_mcs_from_supported_rates(rates, sta_caps.default_mcs, sta_caps.default_short_gi);

    auto pwrCap = assoc_frame->getAttr<assoc_frame::cPowerCapability>();
    get_station_capabilities_from_assoc_field<>(pwrCap, sta_caps);

    auto multiBand = assoc_frame->getAttr<assoc_frame::cMultiBand>();
    get_station_capabilities_from_assoc_field<>(multiBand, sta_caps);

    return true;
}
