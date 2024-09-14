/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "ap_wlan_hal_nl80211.h"
#include <bcl/beerocks_os_utils.h>
#include <bcl/beerocks_utils.h>
#include <bcl/network/network_utils.h>
#include <bcl/son/son_assoc_frame_utils.h>
#include <bcl/son/son_wireless_utils.h>
#include <bwl/key_value_parser.h>
#include <cmath>
#include <easylogging++.h>
#include <hostapd/configuration.h>
#include <linux/nl80211.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>
#include <netlink/msg.h>
#include <netlink/netlink.h>
#include <type_traits>
#include <fstream>
using namespace std;
//////////////////////////////////////////////////////////////////////////////
////////////////////////// Local Module Definitions //////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace bwl {
namespace nl80211 {

#define BUFFER_SIZE 4096
#define CSA_EVENT_FILTERING_TIMEOUT_MS 1000

// Temporary storage for station capabilities
struct SRadioCapabilitiesStrings {
    std::string supported_rates;
    std::string ht_cap;
    std::string ht_mcs;
    std::string vht_cap;
    std::string vht_mcs;
    std::string btm_supported;
    std::string nr_enabled;
    std::string non_pref_chan;
    std::string cell_capa;
};

//////////////////////////////////////////////////////////////////////////////
/////////////////////////// Local Module Functions ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

static ap_wlan_hal::Event nl80211_to_bwl_event(const std::string &opcode)
{
    if (opcode == "AP-ENABLED") {
        return ap_wlan_hal::Event::AP_Enabled;
    } else if (opcode == "AP-DISABLED") {
        return ap_wlan_hal::Event::AP_Disabled;
    } else if (opcode == "AP-STA-CONNECTED") {
        return ap_wlan_hal::Event::STA_Connected;
    } else if (opcode == "AP-STA-DISCONNECTED") {
        return ap_wlan_hal::Event::STA_Disconnected;
    } else if (opcode == "INTERFACE-ENABLED") {
        return ap_wlan_hal::Event::Interface_Enabled;
    } else if (opcode == "INTERFACE-DISABLED") {
        return ap_wlan_hal::Event::Interface_Disabled;
    } else if (opcode == "ACS-STARTED") {
        return ap_wlan_hal::Event::ACS_Started;
    } else if (opcode == "ACS-COMPLETED") {
        return ap_wlan_hal::Event::ACS_Completed;
    } else if (opcode == "ACS-FAILED") {
        return ap_wlan_hal::Event::ACS_Failed;
    } else if (opcode == "AP-CSA-FINISHED") {
        return ap_wlan_hal::Event::CSA_Finished;
    } else if (opcode == "CTRL-EVENT-CHANNEL-SWITCH") {
        return ap_wlan_hal::Event::CTRL_Channel_Switch;
    } else if (opcode == "BSS-TM-QUERY") {
        return ap_wlan_hal::Event::BSS_TM_Query;
    } else if (opcode == "BSS-TM-RESP") {
        return ap_wlan_hal::Event::BSS_TM_Response;
    } else if (opcode == "DFS-CAC-COMPLETED") {
        return ap_wlan_hal::Event::DFS_CAC_Completed;
    } else if (opcode == "DFS-NOP-FINISHED") {
        return ap_wlan_hal::Event::DFS_NOP_Finished;
    } else if (opcode == "AP-MGMT-FRAME-RECEIVED") {
        return ap_wlan_hal::Event::AP_MGMT_FRAME_RECEIVED;
    } else if (opcode == "AP-STA-POSSIBLE-PSK-MISMATCH") {
        return ap_wlan_hal::Event::AP_Sta_Possible_Psk_Mismatch;
    }

    return ap_wlan_hal::Event::Invalid;
}

static uint8_t wpa_bw_to_beerocks_bw(const std::string &chan_width)
{
    // 20 MHz (no HT)
    // 20 MHz
    // 40 MHz
    // 80 MHz
    // 80+80 MHz
    // 160 MHz

    return (chan_width == "80+80") ? 160 : beerocks::string_utils::stoi(chan_width);
}

/// @brief figures out hostapd config file name by the
//  interface name and loads its content
static prplmesh::hostapd::Configuration load_hostapd_config(const std::string &radio_iface_name)
{
    std::vector<std::string> hostapd_cfg_names = {
        "/tmp/run/hostapd-phy0.conf", "/tmp/run/hostapd-phy1.conf", "/var/run/hostapd-phy0.conf",
        "/var/run/hostapd-phy1.conf", "/var/run/hostapd-phy2.conf", "/var/run/hostapd-phy3.conf",
        "/nvram/hostapd0.conf",       "/nvram/hostapd1.conf",       "/nvram/hostapd2.conf",
        "/nvram/hostapd3.conf",       "/nvram/hostapd4.conf",       "/nvram/hostapd5.conf",
        "/nvram/hostapd6.conf",       "/nvram/hostapd7.conf"};

    for (const auto &try_fname : hostapd_cfg_names) {
        LOG(DEBUG) << "Trying to load " << try_fname << "...";

        if (!beerocks::os_utils::file_exists(try_fname)) {
            continue;
        }

        prplmesh::hostapd::Configuration hostapd_conf(try_fname);

        // try loading
        if (!hostapd_conf.load("interface=", "bss=")) {
            LOG(ERROR) << "Failed to load hostapd config file: " << hostapd_conf;
            continue;
        }

        // check if it is the right one:
        // we are looking for the line in the vap that declares this vap: interface=radio_iface_name
        // we could equaly ask hostapd_conf if it has this vap, but there is no such interface
        // to do this. it should be something like: hostapd_conf.is_vap_exists(radio_iface_name);
        if (hostapd_conf.get_vap_value(radio_iface_name, "interface") != radio_iface_name &&
            hostapd_conf.get_vap_value(radio_iface_name, "bss") != radio_iface_name) {
            LOG(DEBUG) << radio_iface_name << " does not exists in " << try_fname;
            continue;
        }

        // all is good, return the conf
        return hostapd_conf;
    }

    // return an empty one since we couldn't find the
    return prplmesh::hostapd::Configuration("file not found");
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Implementation ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// NOTE: Since *base_wlan_hal_nl80211* inherits *base_wlan_hal* virtually, we
//       need to explicitly call it's from any deriving class
ap_wlan_hal_nl80211::ap_wlan_hal_nl80211(const std::string &iface_name, hal_event_cb_t callback,
                                         const hal_conf_t &hal_conf)
    : base_wlan_hal(bwl::HALType::AccessPoint, iface_name, IfaceType::Intel, callback, hal_conf),
      base_wlan_hal_nl80211(bwl::HALType::AccessPoint, iface_name, callback, BUFFER_SIZE, hal_conf)
{
    std::string events[] = {};
    int events_size      = sizeof(events) / sizeof(std::string);
    m_filtered_events.insert(events, events + events_size);
}

ap_wlan_hal_nl80211::~ap_wlan_hal_nl80211() {}

HALState ap_wlan_hal_nl80211::attach(bool block)
{
    auto state = base_wlan_hal_nl80211::attach(block);

    // On Operational send the AP_Attached event to the AP Manager
    if (state == HALState::Operational) {
        event_queue_push(Event::AP_Attached);
    }

    return state;
}

bool ap_wlan_hal_nl80211::refresh_radio_info()
{
    // Obtain radio information (frequency band, maximum supported bandwidth, capabilities and
    // supported channels) using NL80211.
    // Most of this data does not change during runtime. An exception is, for example, the DFS
    // state. So, to read the latest value, we need to refresh data on every call.

    nl80211_client::radio_info radio_info;
    if (!m_nl80211_client->get_radio_info(get_iface_name(), radio_info)) {
        LOG(ERROR) << "Unable to read radio info for interface " << m_radio_info.iface_name;
        return false;
    }

    if (radio_info.bands.empty()) {
        LOG(ERROR) << "Unable to read any band in radio for interface " << m_radio_info.iface_name;
        return false;
    }

    if (m_radio_info.frequency_band == beerocks::eFreqType::FREQ_UNKNOWN) {

        if (radio_info.bands.size() == 1) {

            // If there is just one band, then select it.
            m_radio_info.frequency_band = radio_info.bands[0].get_frequency_band();

        } else {

            // If there are multiple bands, then select the band which frequency matches the
            // operation mode read from hostapd.conf file.
            // For efficiency reasons, parse hostapd.conf only once, the first time this method is
            // called.

            // Load hostapd config for the radio
            prplmesh::hostapd::Configuration conf = load_hostapd_config(m_radio_info.iface_name);
            if (!conf) {
                LOG(ERROR) << "Unable to load hostapd config for interface "
                           << m_radio_info.iface_name;
                return false;
            }

            // Compute frequency band out of parameter `hw_mode` in hostapd.conf
            auto hw_mode = conf.get_head_value("hw_mode");

            // The mode used by hostapd (11b, 11g, 11n, 11ac, 11ax) is governed by several
            // parameters in the configuration file. However, as explained in the comment below from
            // hostapd.conf, the hw_mode parameter is sufficient to determine the band.
            //
            // # Operation mode (a = IEEE 802.11a (5 GHz), b = IEEE 802.11b (2.4 GHz),
            // # g = IEEE 802.11g (2.4 GHz), ad = IEEE 802.11ad (60 GHz); a/g options are used
            // # with IEEE 802.11n (HT), too, to specify band). For IEEE 802.11ac (VHT), this
            // # needs to be set to hw_mode=a. For IEEE 802.11ax (HE) on 6 GHz this needs
            // # to be set to hw_mode=a.
            //
            // Note that this will need to be revisited for 6GHz operation, which we don't support
            // at the moment.
            if (hw_mode.empty() || (hw_mode == "b") || (hw_mode == "g")) {
                m_radio_info.frequency_band = beerocks::eFreqType::FREQ_24G;
            } else if (hw_mode == "a") {
                m_radio_info.frequency_band = beerocks::eFreqType::FREQ_5G;
            } else {
                LOG(ERROR) << "Unknown operation mode for interface " << m_radio_info.iface_name;
                return false;
            }
        }
    }

    for (const auto &band_info : radio_info.bands) {
        if (m_radio_info.frequency_band != band_info.get_frequency_band()) {
            continue;
        }

        m_radio_info.max_bandwidth = band_info.get_max_bandwidth();
        m_radio_info.ht_supported  = band_info.ht_supported;
        m_radio_info.ht_capability = band_info.ht_capability;
        std::copy_n(band_info.ht_mcs_set, m_radio_info.ht_mcs_set.size(),
                    m_radio_info.ht_mcs_set.begin());
        m_radio_info.vht_supported  = band_info.vht_supported;
        m_radio_info.vht_capability = band_info.vht_capability;
        std::copy_n(band_info.vht_mcs_set, m_radio_info.vht_mcs_set.size(),
                    m_radio_info.vht_mcs_set.begin());

        for (auto const &pair : band_info.supported_channels) {
            auto &supported_channel_info = pair.second;
            auto &channel_info        = m_radio_info.channels_list[supported_channel_info.number];
            channel_info.tx_power_dbm = supported_channel_info.tx_power;
            channel_info.dfs_state    = supported_channel_info.is_dfs
                                         ? supported_channel_info.dfs_state
                                         : beerocks::eDfsState::DFS_STATE_MAX;

            for (auto bw : supported_channel_info.supported_bandwidths) {
                // Since bwl nl8011 does not support ranking, set all ranking to highest rank (1).
                channel_info.bw_info_list[bw] = 1;
            }
        }

        break;
    }

    return base_wlan_hal_nl80211::refresh_radio_info();
}

bool ap_wlan_hal_nl80211::enable()
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::disable()
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::set_start_disabled(bool enable, int vap_id)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::set_channel(int chan, beerocks::eWiFiBandwidth bw, int center_channel)
{
    if (chan < 0) {
        LOG(ERROR) << "Invalid input: channel(" << chan << ") < 0";
        return false;
    }

    // ACS is triggered in BackhaulManager::send_slaves_enable() on radio which does not have BH link
    if (chan == 0) {
        LOG(INFO) << "ACS is not supported";

        // TODO: Returning false exits whole AP initialization (PPM-1928)
        return true;
    }

    // Load hostapd config for the radio
    prplmesh::hostapd::Configuration conf = load_hostapd_config(m_radio_info.iface_name);
    if (!conf) {
        LOG(ERROR) << "Unable to load hostapd config for interface " << m_radio_info.iface_name;
        return false;
    }

    std::string chan_string = std::to_string(chan);

    LOG(DEBUG) << "Set channel to " << chan_string << ", bw " << bw << ", center channel "
               << center_channel;

    if (!conf.set_create_head_value("channel", chan_string)) {
        LOG(ERROR) << "Failed setting channel";
        return false;
    }

    if (bw != beerocks::eWiFiBandwidth::BANDWIDTH_UNKNOWN) {
        int wifi_bw = 0;
        // based on hostapd.conf @ https://w1.fi/cgit/hostap/plain/hostapd/hostapd.conf
        // # 0 = 20 or 40 MHz operating Channel width
        // # 1 = 80 MHz channel width
        // # 2 = 160 MHz channel width
        // # 3 = 80+80 MHz channel width
        // #vht_oper_chwidth=1

        if (bw == beerocks::eWiFiBandwidth::BANDWIDTH_20 ||
            bw == beerocks::eWiFiBandwidth::BANDWIDTH_40) {
            wifi_bw = 0;
        } else if (bw == beerocks::eWiFiBandwidth::BANDWIDTH_80) {
            wifi_bw = 1;
        } else if (bw == beerocks::eWiFiBandwidth::BANDWIDTH_160) {
            wifi_bw = 2;
        } else if (bw == beerocks::eWiFiBandwidth::BANDWIDTH_80_80) {
            wifi_bw = 3;
        } else {
            LOG(ERROR) << "Unknown BW " << bw;
            return false;
        }

        if (!conf.set_create_head_value("vht_oper_chwidth", std::to_string(wifi_bw))) {
            LOG(ERROR) << "Failed setting vht_oper_chwidth";
            return false;
        }
    }

    if (center_channel > 0) {
        if (!conf.set_create_head_value("vht_oper_centr_freq_seg0_idx",
                                        std::to_string(center_channel))) {
            LOG(ERROR) << "Failed setting vht_oper_centr_freq_seg0_idx";
            return false;
        }
    }

    // store the result:
    if (!conf.store()) {
        LOG(ERROR) << "set_channel: cannot save hostapd config!";
        return false;
    }

    // make hostapd reload its configuration file:
    const std::string cmd{"UPDATE "};
    if (!wpa_ctrl_send_msg(cmd)) {
        LOG(ERROR) << "'" << cmd << "' command to hostapd failed";
        return false;
    }

    LOG(DEBUG) << "set_channel done";

    return true;
}

bool ap_wlan_hal_nl80211::sta_allow(const std::string &mac, const std::string &bssid)
{
    LOG(TRACE) << __func__ << " mac: " << mac << ", bssid: " << bssid;

    // Build command string
    // We use the DENY_ACL list only
    const std::string cmd = "DENY_ACL DEL_MAC " + mac;

    auto vap_id = get_vap_id_with_mac(bssid);
    if (vap_id < 0) {
        LOG(ERROR) << "no vap has bssid " << bssid;
        return false;
    }
    std::string ifname = m_radio_info.available_vaps[vap_id].bss;

    // Send command
    if (!wpa_ctrl_send_msg(cmd, ifname)) {
        LOG(ERROR) << "sta_allow() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_nl80211::sta_deny(const std::string &mac, const std::string &bssid)
{
    LOG(TRACE) << __func__ << " mac: " << mac << ", bssid: " << bssid;

    // Build command string
    // We use the DENY_ACL list only
    const std::string cmd = "DENY_ACL ADD_MAC " + mac;

    auto vap_id = get_vap_id_with_mac(bssid);
    if (vap_id < 0) {
        LOG(ERROR) << "no vap has bssid " << bssid;
        return false;
    }
    std::string ifname = m_radio_info.available_vaps[vap_id].bss;

    // Send command
    if (!wpa_ctrl_send_msg(cmd, ifname)) {
        LOG(ERROR) << "sta_deny() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_nl80211::sta_disassoc(int8_t vap_id, const std::string &mac, uint32_t reason)
{
    LOG(TRACE) << __func__ << " mac: " << mac << " vap_id: " << vap_id;

    if (!check_vap_id(vap_id)) {
        LOG(ERROR) << "invalid vap_id " << vap_id;
        return false;
    }

    // Build command string
    const std::string cmd = "DISASSOCIATE " + mac + " reason=" + std::to_string(reason) + " tx=0";

    std::string ifname = m_radio_info.available_vaps[vap_id].bss;

    // Send command
    if (!wpa_ctrl_send_msg(cmd, ifname)) {
        LOG(ERROR) << "sta_disassoc() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_nl80211::sta_deauth(int8_t vap_id, const std::string &mac, uint32_t reason)
{
    LOG(TRACE) << __func__ << " mac: " << mac << " vap_id: " << vap_id;

    if (!check_vap_id(vap_id)) {
        LOG(ERROR) << "invalid vap_id " << vap_id;
        return false;
    }

    // Build command string
    const std::string cmd = "DEAUTHENTICATE " + mac + " reason=" + std::to_string(reason) + " tx=0";

    std::string ifname = m_radio_info.available_vaps[vap_id].bss;

    // Send command
    if (!wpa_ctrl_send_msg(cmd, ifname)) {
        LOG(ERROR) << "sta_disassoc() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_nl80211::sta_bss_steer(int8_t vap_id, const std::string &mac,
                                        const std::string &bssid, int oper_class, int chan,
                                        int disassoc_timer_btt, int valid_int_btt, int reason)
{
    LOG(TRACE) << __func__ << " vap_id: " << vap_id << " mac: " << mac << ", BSS: " << bssid
               << ", oper_class: " << oper_class << ", channel: " << chan
               << ", disassoc: " << disassoc_timer_btt << ", valid_int: " << valid_int_btt
               << ", reason: " << reason;

    if (!check_vap_id(vap_id)) {
        LOG(ERROR) << "invalid source vap_id " << vap_id;
        return false;
    }

    // Build command string
    std::string cmd =
        // Set the STA MAC address
        "BSS_TM_REQ " +
        mac
        // Transition management parameters
        + " pref=" + "1" + " abridged=" + "1";

    // Add only valid (positive) reason codes
    // Upper layers may set the reason value to a (-1) value to mark that the reason is not present
    if (reason >= 0) {
        // mbo format is mbo=<reason>:<reassoc_delay>:<cell_pref>
        // since the <reassoc_delay>:<cell_pref> variables are not part of the Steering Request TLV, we hard code it.
        // See discussion here:
        // https://gitlab.com/prpl-foundation/prplmesh/prplMesh/-/merge_requests/1948#note_457733802
        cmd += " mbo=" + std::to_string(reason);

        // BTM request (MBO): Assoc retry delay is only valid in disassoc imminent mode
        if (disassoc_timer_btt) {
            cmd += ":100:0";
        } else {
            cmd += ":0:0";
        }
    }

    if (disassoc_timer_btt) {
        cmd += std::string() + " disassoc_imminent=" + "1" +
               " disassoc_timer=" + std::to_string(disassoc_timer_btt);
    }
    // " bss_term="  // Unused Param
    // " url="       // Unused Param

    if (valid_int_btt) {
        cmd += " valid_int=" + std::to_string(valid_int_btt);
    }

    // Target BSSID
    cmd += std::string() + " neighbor=" + bssid + ",0," + std::to_string(oper_class) + "," +
           std::to_string(chan) + ",0";

    // Send command
    if (!wpa_ctrl_send_msg(cmd, m_radio_info.available_vaps[vap_id].bss)) {
        LOG(ERROR) << "sta_bss_steer() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_nl80211::update_vap_credentials(
    std::list<son::wireless_utils::sBssInfoConf> &bss_info_conf_list,
    const std::string &backhaul_wps_ssid, const std::string &backhaul_wps_passphrase)
{
    // Load hostapd config for the radio
    prplmesh::hostapd::Configuration conf = load_hostapd_config(m_radio_info.iface_name);
    if (!conf) {
        LOG(ERROR) << "Autoconfiguration: no hostapd config to apply configuration!";
        return false;
    }

    // Save old conf info and len
    std::string old_temp;
    int old_len = 0;
    char old_conf[256][1024] = {0};
    std::ifstream old_ifs;
    old_ifs.open(conf.m_configuration_file, ios::in);
    for (int i = 0; getline(old_ifs, old_temp); i++) {
        old_len++;
        strcpy(old_conf[i], old_temp.c_str());
    }
    old_ifs.close();

    // Clear all VAPs from the available container, since we preset it with configuration.
    m_radio_info.available_vaps.clear();

    // If a Multi-AP Agent receives an AP-Autoconfiguration WSC message containing one or
    // more M2, it shall validate each M2 (based on its 1905 AL MAC address) and configure
    // a BSS on the corresponding radio for each of the M2. If the Multi-AP Agent is currently
    // operating a BSS with operating parameters that do not completely match any of the M2 in
    // the received AP-Autoconfiguration WSC message, it shall tear down that BSS.

    // decalre a function for iterating over bss-conf and ap-vaps
    bool abort = false;
    auto configure_func =
        [&abort, &conf, &bss_info_conf_list, &backhaul_wps_ssid, &backhaul_wps_passphrase](
            const std::string &vap,
            std::remove_reference<decltype(bss_info_conf_list)>::type::iterator bss_it) {
            if (abort) {
                return;
            }

            if (bss_it != bss_info_conf_list.end()) {

                // we still have data on the input bss-conf list

                // escape I
                auto auth_type =
                    son::wireless_utils::wsc_to_bwl_authentication(bss_it->authentication_type);
                if (auth_type == "INVALID") {
                    LOG(ERROR) << "Autoconfiguration: auth type is 'INVALID'; number: "
                               << (uint16_t)bss_it->authentication_type;
                    abort = true;
                    return;
                }

                // escape II
                auto enc_type = son::wireless_utils::wsc_to_bwl_encryption(bss_it->encryption_type);
                if (enc_type == "INVALID") {
                    LOG(ERROR) << "Autoconfiguration: enc_type is 'INVALID'; number: "
                               << int(bss_it->encryption_type);
                    abort = true;
                    return;
                }

                // escape III
                // check explicit bssid value only in case of Multiple BSSID support
                if ((!conf.get_vap_value(vap, "bss").empty()) &&
                    conf.get_vap_value(vap, "bssid").empty()) {
                    LOG(ERROR) << "Failed to get BSSID for vap: " << vap;
                    abort = true;
                    return;
                }

                // settings

                // Hostapd "wpa" field.
                // This field is a bit field that can be used to enable WPA (IEEE 802.11i/D3.0)
                // and/or WPA2 (full IEEE 802.11i/RSN):
                // bit0 = WPA
                // bit1 = IEEE 802.11i/RSN (WPA2) (dot11RSNAEnabled)
                int wpa = 0;

                // Set of accepted key management algorithms (WPA-PSK, WPA-EAP, or both). The
                // entries are separated with a space. WPA-PSK-SHA256 and WPA-EAP-SHA256 can be
                // added to enable SHA256-based stronger algorithms.
                // WPA-PSK = WPA-Personal / WPA2-Personal
                std::string wpa_key_mgmt; // default to empty -> delete from hostapd config

                // (dot11RSNAConfigPairwiseCiphersTable)
                // Pairwise cipher for WPA (v1) (default: TKIP)
                //  wpa_pairwise=TKIP CCMP
                // Pairwise cipher for RSN/WPA2 (default: use wpa_pairwise value)
                //  rsn_pairwise=CCMP
                std::string wpa_pairwise; // default to empty -> delete from hostapd config

                // WPA pre-shared keys for WPA-PSK. This can be either entered as a 256-bit
                // secret in hex format (64 hex digits), wpa_psk, or as an ASCII passphrase
                // (8..63 characters), wpa_passphrase.
                std::string wpa_passphrase;
                std::string wpa_psk;

                // ieee80211w: Whether management frame protection (MFP) is enabled
                // 0 = disabled (default)
                // 1 = optional
                // 2 = required
                std::string ieee80211w;

                // This parameter can be used to disable caching of PMKSA created through EAP
                // authentication. RSN preauthentication may still end up using PMKSA caching if
                // it is enabled (rsn_preauth=1).
                // 0 = PMKSA caching enabled (default)
                // 1 = PMKSA caching disabled
                std::string disable_pmksa_caching;

                // Opportunistic Key Caching (aka Proactive Key Caching)
                // Allow PMK cache to be shared opportunistically among configured interfaces
                // and BSSes (i.e., all configurations within a single hostapd process).
                // 0 = disabled (default)
                // 1 = enabled
                std::string okc;

                // This parameter can be used to disable retransmission of EAPOL-Key frames that
                // are used to install keys (EAPOL-Key message 3/4 and group message 1/2). This
                // is similar to setting wpa_group_update_count=1 and
                std::string wpa_disable_eapol_key_retries;

                // EasyMesh R1 only allows Open and WPA2 PSK auth&encryption methods.
                // Quote: A Multi-AP Controller shall set the Authentication Type attribute
                //        in M2 to indicate WPA2-Personal or Open System Authentication.
                // bss_it->authentication_type is a bitfield, but we are not going
                // to accept any combinations due to the above limitation.
                if (bss_it->authentication_type == WSC::eWscAuth::WSC_AUTH_OPEN) {
                    wpa = 0x0;
                    if (bss_it->encryption_type != WSC::eWscEncr::WSC_ENCR_NONE) {
                        LOG(ERROR) << "Autoconfiguration: " << vap << " encryption set on open VAP";
                        abort = true;
                        return;
                    }
                    if (bss_it->network_key.length() > 0) {
                        LOG(ERROR)
                            << "Autoconfiguration: " << vap << " network key set for open VAP";
                        abort = true;
                        return;
                    }
                } else if (bss_it->authentication_type == WSC::eWscAuth::WSC_AUTH_WPA2PSK) {
                    wpa = 0x2;
                    wpa_key_mgmt.assign("WPA-PSK");
                    // Cipher must include AES for WPA2, TKIP is optional
                    if ((static_cast<uint16_t>(bss_it->encryption_type) &
                         static_cast<uint16_t>(WSC::eWscEncr::WSC_ENCR_AES)) == 0) {
                        LOG(ERROR)
                            << "Autoconfiguration:  " << vap << " CCMP(AES) is required for WPA2";
                        abort = true;
                        return;
                    }
                    if ((uint16_t(bss_it->encryption_type) &
                         uint16_t(WSC::eWscEncr::WSC_ENCR_TKIP)) != 0) {
                        wpa_pairwise.assign("TKIP CCMP");
                    } else {
                        wpa_pairwise.assign("CCMP");
                    }
                    if (bss_it->network_key.length() < 8 || bss_it->network_key.length() > 64) {
                        LOG(ERROR) << "Autoconfiguration: " << vap << " invalid network key length "
                                   << bss_it->network_key.length();
                        abort = true;
                        return;
                    }
                    if (bss_it->network_key.length() < 64) {
                        wpa_passphrase.assign(bss_it->network_key);
                    } else {
                        wpa_psk.assign(bss_it->network_key);
                    }
                    ieee80211w.assign("0");
                    disable_pmksa_caching.assign("1");
                    okc.assign("0");
                    wpa_disable_eapol_key_retries.assign("0");
                } else if (bss_it->authentication_type ==
                           WSC::eWscAuth(WSC::eWscAuth::WSC_AUTH_WPA2PSK |
                                         WSC::eWscAuth::WSC_AUTH_SAE)) {
                    wpa = 0x2;
                    wpa_key_mgmt.assign("WPA-PSK SAE");

                    if (bss_it->encryption_type != WSC::eWscEncr::WSC_ENCR_AES) {
                        LOG(ERROR)
                            << "Autoconfiguration:  " << vap << " CCMP(AES) is required for WPA3";
                        return;
                    }
                    wpa_pairwise.assign("CCMP");

                    if (bss_it->network_key.length() < 8 || bss_it->network_key.length() > 64) {
                        LOG(ERROR) << "Autoconfiguration: " << vap << " invalid network key length "
                                   << bss_it->network_key.length();
                        return;
                    }
                    wpa_passphrase.assign(bss_it->network_key);

                    ieee80211w.assign("2");
                    disable_pmksa_caching.assign("1");
                    okc.assign("1");
                    wpa_disable_eapol_key_retries.assign("0");
                } else if (bss_it->authentication_type == WSC::eWscAuth::WSC_AUTH_SAE) {
                    wpa = 0x2;
                    wpa_key_mgmt.assign("SAE");

                    if (bss_it->encryption_type != WSC::eWscEncr::WSC_ENCR_AES) {
                        LOG(ERROR)
                            << "Autoconfiguration:  " << vap << " CCMP(AES) is required for WPA3";
                        return;
                    }
                    wpa_pairwise.assign("CCMP");

                    if (bss_it->network_key.length() < 8 || bss_it->network_key.length() > 64) {
                        LOG(ERROR) << "Autoconfiguration: " << vap << " invalid network key length "
                                   << bss_it->network_key.length();
                        return;
                    }
                    wpa_passphrase.assign(bss_it->network_key);

                    ieee80211w.assign("2");
                    disable_pmksa_caching.assign("1");
                    okc.assign("1");
                    wpa_disable_eapol_key_retries.assign("0");
                } else {
                    LOG(ERROR) << "Autoconfiguration: " << vap << " invalid authentication type: "
                               << son::wireless_utils::wsc_to_bwl_authentication(
                                      bss_it->authentication_type);
                    abort = true;
                    return;
                }

                LOG(DEBUG) << "Autoconfiguration for ssid: " << bss_it->ssid
                           << " auth_type: " << auth_type << " encr_type: " << enc_type
                           << " network_key: " << bss_it->network_key
                           << " fronthaul: " << beerocks::string_utils::bool_str(bss_it->fronthaul)
                           << " backhaul: " << beerocks::string_utils::bool_str(bss_it->backhaul);

                conf.set_create_vap_value(vap, "ssid", bss_it->ssid);
                conf.set_create_vap_value(vap, "wps_state", bss_it->fronthaul ? "2" : "");
                conf.set_create_vap_value(vap, "wps_independent", "0");
                conf.set_create_vap_value(vap, "max_num_sta", bss_it->backhaul ? "2" : "");
                std::string multi_ap;
                if (bss_it->fronthaul) {
                    if (bss_it->backhaul) {
                        multi_ap = "3";
                    } else {
                        multi_ap = "2";
                    }
                } else {
                    if (bss_it->backhaul) {
                        multi_ap = "1";
                    } else {
                        LOG(WARNING) << "BSS configured with fronthaul nor backhaul";
                        multi_ap = "0";
                    }
                }
                conf.set_create_vap_value(vap, "multi_ap", multi_ap);

                // oddly enough, multi_ap_backhaul_wpa_passphrase has to be
                // quoted, while wpa_passphrase does not...
                if (bss_it->fronthaul && !backhaul_wps_ssid.empty()) {
                    conf.set_create_vap_value(vap, "multi_ap_backhaul_ssid",
                                              "\"" + backhaul_wps_ssid + "\"");
                    conf.set_create_vap_value(vap, "multi_ap_backhaul_wpa_passphrase",
                                              backhaul_wps_passphrase);
                }

                // remove when not needed
                if (!bss_it->fronthaul && backhaul_wps_ssid.empty()) {
                    conf.set_create_vap_value(vap, "multi_ap_backhaul_ssid", "");
                    conf.set_create_vap_value(vap, "multi_ap_backhaul_wpa_passphrase", "");
                }

                conf.set_create_vap_value(vap, "wpa", wpa);
                conf.set_create_vap_value(vap, "okc", okc);
                conf.set_create_vap_value(vap, "wpa_key_mgmt", wpa_key_mgmt);
                conf.set_create_vap_value(vap, "wpa_pairwise", wpa_pairwise);
                conf.set_create_vap_value(vap, "wpa_psk", wpa_psk);
                conf.set_create_vap_value(vap, "ieee80211w", ieee80211w);
                conf.set_create_vap_value(vap, "wpa_passphrase", wpa_passphrase);
                conf.set_create_vap_value(vap, "disable_pmksa_caching", disable_pmksa_caching);
                conf.set_create_vap_value(vap, "wpa_disable_eapol_key_retries",
                                          wpa_disable_eapol_key_retries);

                // we always need to get the mgmt frames (assoc req) for capability reports:
                conf.set_create_vap_value(vap, "notify_mgmt_frames", "1");

                // finally enable the vap (remove any previously set start_disabled)
                conf.set_create_vap_value(vap, "start_disabled", "");
            } else {
                // no more data in the input bss-conf list
                // disable the rest of the vaps
                LOG(DEBUG) << "Disabling VAP " << vap;
                conf.disable_vap(vap);
            }
        };

    conf.for_all_ap_vaps(configure_func, bss_info_conf_list.begin(), bss_info_conf_list.end(),
                         [](const std::string &) { return true; });

    if (abort) {
        return false;
    }

    if (!conf.store()) {
        LOG(ERROR) << "Autoconfiguration: cannot save hostapd config!";
        return false;
    }

    // Get new conf info and len
    std::string new_temp;
    int new_len = 0;
    char new_conf[256][1024] = {0};
    std::ifstream new_ifs;
    new_ifs.open(conf.m_configuration_file, ios::in);
    for (int i = 0; getline(new_ifs, new_temp); i++) {
        new_len++;
        strcpy(new_conf[i], new_temp.c_str());
    }
    new_ifs.close();

    // Compare two files to decide whether to reload conf
    if (new_len == old_len) {
        for (int i = 0; i < new_len; i++) {
            if (strcmp(new_conf[i], old_conf[i]))
                goto reload_conf;
        }
        return true;
    }

reload_conf:
    const std::string cmd("UPDATE ");
    if (!wpa_ctrl_send_msg(cmd)) {
        LOG(ERROR) << "Autoconfiguration: \"" << cmd << "\" command to hostapd has failed";
        return false;
    }

    LOG(DEBUG) << "Autoconfiguration: done:\n" << conf;
    return true;
}

bool ap_wlan_hal_nl80211::sta_unassoc_rssi_measurement(const std::string &mac, int chan, int bw,
                                                       int vht_center_frequency, int delay,
                                                       int window_size)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return false;
}

bool ap_wlan_hal_nl80211::sta_softblock_add(
    const std::string &vap_name, const std::string &client_mac, uint8_t reject_error_code,
    uint8_t probe_snr_threshold_hi, uint8_t probe_snr_threshold_lo,
    uint8_t authetication_snr_threshold_hi, uint8_t authetication_snr_threshold_lo)
{
    // softblock is used to block stations at the probe request level
    // (instead of during the authentication or association).  It
    // doesn't seem to be part of nl80211, so it cannot be
    // implemented. In prplMesh, it is only used by BML commands which
    // are triggered by an external process.

    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return false;
}

bool ap_wlan_hal_nl80211::sta_softblock_remove(const std::string &vap_name,
                                               const std::string &client_mac)
{
    return false;
}

bool ap_wlan_hal_nl80211::switch_channel(int chan, int bw, int vht_center_frequency)
{
    LOG(TRACE) << __func__ << " channel: " << chan << ", bw: " << bw
               << ", vht_center_frequency: " << vht_center_frequency;

    // CHAN_SWITCH cs_count freq [center_freq1] [center_freq2] [bandwidth] [sec_channel_offset]
    //             [ht] [vht] [blocktx]
    // cs_count - CSA_BCN_COUNT, beacon count before switch. 5 is default value in
    //            hostapd_dfs_start_channel_switch()
    std::string cmd = "CHAN_SWITCH 5 ";

    if (chan == 0) {
        LOG(ERROR) << "ACS is not supported";
        return false;
    }

    int freq                              = son::wireless_utils::channel_to_freq(chan);
    std::string freq_str                  = std::to_string(freq);
    std::string wave_vht_center_frequency = std::to_string(vht_center_frequency);

    // Center Freq
    cmd += freq_str; // CenterFrequency

    // Extension Channel
    if (bw != beerocks::BANDWIDTH_20) {
        if (freq < vht_center_frequency) {
            cmd += " sec_channel_offset=1";
        } else {
            cmd += " sec_channel_offset=-1";
        }
    }

    // Channel bandwidth
    if (bw == beerocks::BANDWIDTH_80) {
        cmd += " center_freq1=" + wave_vht_center_frequency;
    }

    cmd += " bandwidth=" +
           std::to_string(beerocks::utils::convert_bandwidth_to_int((beerocks::eWiFiBandwidth)bw));

    // Supported Standard n/ac
    if (bw == beerocks::BANDWIDTH_20 || bw == beerocks::BANDWIDTH_40) {
        cmd += " ht"; //n
    } else if (bw == beerocks::BANDWIDTH_80 || bw == beerocks::BANDWIDTH_160) {
        cmd += " vht"; // ac
    }

    // Send command
    if (!wpa_ctrl_send_msg(cmd)) {
        LOG(ERROR) << "wpa_ctrl_send_msg() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_nl80211::cancel_cac(int chan, beerocks::eWiFiBandwidth bw,
                                     int vht_center_frequency, int secondary_chan)
{
    // TODO: implement
    return false;
}

bool ap_wlan_hal_nl80211::failsafe_channel_set(int chan, int bw, int vht_center_frequency)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::failsafe_channel_get(int &chan, int &bw)
{
    LOG(TRACE) << __func__;

    // Build command string
    std::string cmd = "GET_FAILSAFE_CHAN";
    char *reply;

    // Send command
    if (!wpa_ctrl_send_msg(cmd, &reply)) {
        LOG(ERROR) << "failsafe_channel_get() failed!";
        return false;
    }

    // Custom reply parsing

    std::string reply_str(reply);
    if (reply_str.find("UNSPECIFIED", 0, 11) != std::string::npos) {
        chan = -1;
        bw   = -1;
    } else if (reply_str.find("ACS", 0, 3) != std::string::npos) {
        chan = bw = 0;

    } else {
        int freq;
        std::string tmp;
        std::stringstream ss(reply_str);
        // parsing string in form: "%d %*s %*s bandwidth=%d"
        ss >> freq >> tmp >> tmp >> tmp;
        auto tmp_vec = beerocks::string_utils::str_split(tmp, '=');
        if (tmp_vec.size() != 2 || tmp_vec[0] != std::string("bandwidth")) {
            return false;
        }
        bw   = beerocks::string_utils::stoi(tmp_vec[1]);
        chan = son::wireless_utils::freq_to_channel(freq);
    }

    return true;
}

bool ap_wlan_hal_nl80211::is_zwdfs_supported()
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return false;
}

bool ap_wlan_hal_nl80211::set_zwdfs_antenna(bool enable)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return false;
}

bool ap_wlan_hal_nl80211::is_zwdfs_antenna_enabled()
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return false;
}

bool ap_wlan_hal_nl80211::hybrid_mode_supported() { return true; }

bool ap_wlan_hal_nl80211::restricted_channels_set(char *channel_list)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::restricted_channels_get(char *channel_list)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    memset(channel_list, 0, beerocks::message::RESTRICTED_CHANNEL_LENGTH);
    return true;
}

bool ap_wlan_hal_nl80211::read_acs_report()
{
    LOG(TRACE) << __func__ << " for interface: " << get_radio_info().iface_name;

    // We're not going to support ACS in the NL80211 flavor of BWL.
    // The channel selection will instead be done by making the agent send channel scan reports to
    // the controller and the controller will take a decision based on the report (as part of
    // EasyMesh R2).
    return true;
}

bool ap_wlan_hal_nl80211::set_tx_power_limit(int tx_pow_limit)
{
    return m_nl80211_client->set_tx_power_limit(m_radio_info.iface_name, tx_pow_limit);
}

bool ap_wlan_hal_nl80211::set_vap_enable(const std::string &iface_name, const bool enable)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::set_mbo_assoc_disallow(const std::string &bssid, bool enable)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::set_radio_mbo_assoc_disallow(bool enable)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::set_primary_vlan_id(uint16_t primary_vlan_id)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::get_vap_enable(const std::string &iface_name, bool &enable)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::generate_connected_clients_events(
    bool &is_finished_all_clients, std::chrono::steady_clock::time_point max_iteration_timeout)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    is_finished_all_clients = true;
    return true;
}

bool ap_wlan_hal_nl80211::pre_generate_connected_clients_events()
{

    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool ap_wlan_hal_nl80211::start_wps_pbc()
{
    LOG(DEBUG) << "Start WPS PBC on interface " << m_radio_info.iface_name;
    std::string cmd = "WPS_PBC";
    if (!wpa_ctrl_send_msg(cmd)) {
        LOG(ERROR) << "start_wps_pbc() failed!";
        return false;
    }
    return true;
}

bool ap_wlan_hal_nl80211::process_nl80211_event(parsed_obj_map_t &parsed_obj)
{
    // Filter out empty events
    std::string opcode;
    if (!(parsed_obj.find(bwl::EVENT_KEYLESS_PARAM_OPCODE) != parsed_obj.end() &&
          !(opcode = parsed_obj[bwl::EVENT_KEYLESS_PARAM_OPCODE]).empty())) {
        return true;
    }

    // LOG(TRACE) << __func__ << " - opcode: |" << opcode << "|";

    auto event = nl80211_to_bwl_event(opcode);

    std::string interface;
    if (parsed_obj.find(bwl::EVENT_KEYLESS_PARAM_IFACE) != parsed_obj.end()) {
        interface = parsed_obj.at(bwl::EVENT_KEYLESS_PARAM_IFACE);
    }
    if (interface.empty()) {
        LOG(DEBUG) << "Could not find interface name.";
    }

    auto vap_id    = get_vap_id_with_bss(interface);
    auto iface_ids = beerocks::utils::get_ids_from_iface_string(interface);
    if ((vap_id < 0) && (iface_ids.vap_id != beerocks::IFACE_RADIO_ID)) {
        LOG(DEBUG) << "Unknown vap_id " << vap_id;
    }

    switch (event) {

    case Event::AP_MGMT_FRAME_RECEIVED: {
        if (parsed_obj.find("buf") == parsed_obj.end() || parsed_obj["buf"].empty()) {
            LOG(ERROR) << "Management frame without data!";
            return false;
        }
        // Tunnel the Management request to the controller
        auto mgmt_frame = create_mgmt_frame_notification(parsed_obj["buf"].c_str());
        if (mgmt_frame) {
            event_queue_push(Event::MGMT_Frame, mgmt_frame);
            // only save association and re-association request frames:
            if ((mgmt_frame->type == eManagementFrameType::ASSOCIATION_REQUEST) ||
                (mgmt_frame->type == eManagementFrameType::REASSOCIATION_REQUEST)) {
                std::string src_mac           = tlvf::mac_to_string(mgmt_frame->mac);
                m_latest_assoc_frame[src_mac] = mgmt_frame;
                LOG(DEBUG) << "Saved assoc frame";
            }
        }

    } break;

    // STA Connected
    case Event::STA_Connected: {

        // TODO: Change to HAL objects
        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION));
        auto msg =
            reinterpret_cast<sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION));

        if (vap_id < 0) {
            LOG(ERROR) << "Invalid vap_id " << vap_id;
            return false;
        }

        std::string src_mac      = parsed_obj[bwl::EVENT_KEYLESS_PARAM_MAC];
        msg->params.vap_id       = vap_id;
        msg->params.mac          = tlvf::mac_from_string(src_mac);
        msg->params.capabilities = {};

        //init the freq band cap with the target radio freq band info
        msg->params.capabilities.band_5g_capable = m_radio_info.is_5ghz;
        msg->params.capabilities.band_2g_capable =
            (son::wireless_utils::which_freq(m_radio_info.channel) ==
             beerocks::eFreqType::FREQ_24G);

        auto assoc_frame_type = assoc_frame::AssocReqFrame::UNKNOWN;

        if (m_latest_assoc_frame.find(src_mac) == m_latest_assoc_frame.end()) {
            LOG(WARNING) << "STA-CONNECTED without previously receiving a (re-)association frame!";
            msg->params.association_frame_length = 0;
        } else {
            // Add the latest association frame
            // PPM-1718: parse the association frame and make sure it belongs to the right MAC address.
            auto &frame_info  = m_latest_assoc_frame[src_mac];
            msg->params.bssid = frame_info->bssid;
            auto &frame_body  = frame_info->data;
            // Add the latest association frame
            std::copy(frame_body.begin(), frame_body.end(), msg->params.association_frame);
            msg->params.association_frame_length = frame_body.size();
            assoc_frame_type                     = assoc_frame::AssocReqFrame::ASSOCIATION_REQUEST;
            if (frame_info->type == eManagementFrameType::REASSOCIATION_REQUEST) {
                assoc_frame_type = assoc_frame::AssocReqFrame::REASSOCIATION_REQUEST;
            }
        }

        auto assoc_frame = assoc_frame::AssocReqFrame::parse(
            msg->params.association_frame, msg->params.association_frame_length, assoc_frame_type);

        auto res = son::assoc_frame_utils::get_station_capabilities_from_assoc_frame(
            assoc_frame, msg->params.capabilities);
        if (!res) {
            LOG(ERROR) << "Failed to get station capabilities.";
        } else {
            son::wireless_utils::print_station_capabilities(msg->params.capabilities);
        }

        // Add the message to the queue
        event_queue_push(Event::STA_Connected, msg_buff);

    } break;

    // STA Disconnected
    case Event::STA_Disconnected: {

        // TODO: Change to HAL objects
        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION));
        auto msg =
            reinterpret_cast<sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION));

        if (vap_id < 0) {
            LOG(ERROR) << "Invalid vap_id " << vap_id;
            return false;
        }

        // Store the MAC address of the disconnected STA
        msg->params.vap_id = vap_id;
        msg->params.mac    = tlvf::mac_from_string(parsed_obj[bwl::EVENT_KEYLESS_PARAM_MAC]);

        // Add the message to the queue
        event_queue_push(Event::STA_Disconnected, msg_buff);

        m_latest_assoc_frame.erase(parsed_obj[bwl::EVENT_KEYLESS_PARAM_MAC]);

    } break;

    // BSS Transition Query (802.11v)
    case Event::BSS_TM_Query: {

        auto val_iter = parsed_obj.find(bwl::EVENT_KEYLESS_PARAM_MAC);
        if (val_iter == parsed_obj.end()) {
            LOG(ERROR) << "No STA mac found";
            return false;
        }
        const auto client_mac = val_iter->second;

        if (interface.empty()) {
            LOG(ERROR) << "No interface name found";
            return false;
        }
        const auto vap_name = interface;

        if (vap_id < 0) {
            LOG(ERROR) << "Invalid vap_id " << vap_id;
            return false;
        }
        std::string bssid = m_radio_info.available_vaps[vap_id].mac;

        auto op_class = son::wireless_utils::get_operating_class_by_channel(
            beerocks::message::sWifiChannel(m_radio_info.channel, m_radio_info.bandwidth));
        // According to easymesh R2 specification when STA sends BSS_TM_QUERY
        // AP should respond with BSS_TM_REQ with at least one neighbor AP.
        // This commit adds the answer to the BSS_TM_QUERY. The answer adds only
        // one neighbor to the BSS_TM_REQ - the current VAP that the STA is
        // connected to, which in turn makes the STA to stay on the current VAP.
        // Since it's not an "active" transition and it makes the STA stay on the
        // current VAP, there is no need to notify the upper layer.
        // disassoc_timer_btt = 0 valid_int_btt=2 (200ms) reason=0 (not specified)
        sta_bss_steer(vap_id, client_mac, bssid, op_class, m_radio_info.channel, 0, 2, 0);
        break;
    }

    // BSS Transition (802.11v)
    case Event::BSS_TM_Response: {

        // TODO: Change to HAL objects
        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE));
        auto msg = reinterpret_cast<sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE));

        // Client params
        msg->params.mac         = tlvf::mac_from_string(parsed_obj[bwl::EVENT_KEYLESS_PARAM_MAC]);
        msg->params.status_code = beerocks::string_utils::stoi(parsed_obj["status_code"]);

        // Open source hostapd does not contain bssid where client connected
        // BSSID should be retrieved from Agent database, for simplify logic in the
        // Agent need to fill up BSSID here with ZERO_MAC.
        msg->params.source_bssid = beerocks::net::network_utils::ZERO_MAC;

        // Add the message to the queue
        event_queue_push(Event::BSS_TM_Response, msg_buff);

    } break;

    case Event::CTRL_Channel_Switch: {
        std::string bandwidth = parsed_obj["ch_width"];
        if (bandwidth.empty()) {
            LOG(ERROR) << "Invalid bandwidth";
            return false;
        }
        m_radio_info.channel =
            son::wireless_utils::freq_to_channel(beerocks::string_utils::stoi(parsed_obj["freq"]));
        m_radio_info.bandwidth          = wpa_bw_to_beerocks_bw(bandwidth);
        m_radio_info.channel_ext_above  = beerocks::string_utils::stoi(parsed_obj["ch_offset"]);
        m_radio_info.vht_center_freq    = beerocks::string_utils::stoi(parsed_obj["cf1"]);
        m_radio_info.is_dfs_channel     = beerocks::string_utils::stoi(parsed_obj["dfs"]);
        m_radio_info.last_csa_sw_reason = ChanSwReason::Unknown;
        if (son::wireless_utils::which_freq(m_radio_info.channel) == beerocks::eFreqType::FREQ_5G) {
            m_radio_info.is_5ghz = true;
        }
    } break;
    // ACS/CSA Completed
    case Event::ACS_Completed:
    case Event::CSA_Finished:
        event_queue_push(event);
        break;
    case Event::Interface_Disabled:
    case Event::ACS_Failed: {
        // Forward to the AP manager
        event_queue_push(event);
    } break;

    case Event::AP_Disabled: {
        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sHOSTAP_DISABLED_NOTIFICATION));
        auto msg      = reinterpret_cast<sHOSTAP_DISABLED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        memset(msg_buff.get(), 0, sizeof(sHOSTAP_DISABLED_NOTIFICATION));

        if (interface.empty()) {
            LOG(ERROR) << "Could not find interface name.";
            return false;
        }

        // Case of boards where main VAP and radio have same name
        if (vap_id == 0 && iface_ids.vap_id == beerocks::IFACE_RADIO_ID) {
            vap_id = beerocks::IFACE_RADIO_ID;
        }
        if (vap_id == beerocks::IFACE_ID_INVALID) {
            LOG(ERROR) << "Invalid vap_id " << vap_id;
            return false;
        }
        msg->vap_id = vap_id;

        event_queue_push(Event::AP_Disabled, msg_buff); // send message to the AP manager

    } break;
    case Event::AP_Enabled: {
        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sHOSTAP_ENABLED_NOTIFICATION));
        auto msg      = reinterpret_cast<sHOSTAP_ENABLED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        memset(msg_buff.get(), 0, sizeof(sHOSTAP_ENABLED_NOTIFICATION));

        // Case of boards where main VAP and radio have same name
        if (vap_id == 0 && iface_ids.vap_id == beerocks::IFACE_RADIO_ID) {
            vap_id = beerocks::IFACE_RADIO_ID;
        }
        if (vap_id == beerocks::IFACE_ID_INVALID) {
            LOG(ERROR) << "Invalid vap_id " << vap_id;
            return false;
        }
        msg->vap_id = vap_id;

        // same as in dwpal: AP enabling is done per vap
        if (msg->vap_id == beerocks::IFACE_RADIO_ID) {
            // Ignore AP-ENABLED on radio
            return true;
        }

        event_queue_push(Event::AP_Enabled, msg_buff);
    } break;

    case Event::AP_Sta_Possible_Psk_Mismatch: {

        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sSTA_MISMATCH_PSK));
        auto msg      = reinterpret_cast<sSTA_MISMATCH_PSK *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        memset(msg_buff.get(), 0, sizeof(sSTA_MISMATCH_PSK));

        msg->sta_mac = tlvf::mac_from_string(parsed_obj[bwl::EVENT_KEYLESS_PARAM_MAC]);
        LOG(DEBUG) << "STA possible PSK mismatch: offending MAC: " << msg->sta_mac;

        event_queue_push(Event::AP_Sta_Possible_Psk_Mismatch, msg_buff);
        break;
    }

    // Gracefully ignore unhandled events
    // TODO: Probably should be changed to an error once WAV will stop
    //       sending empty or irrelevant events...
    default: {
        LOG(DEBUG) << "Unhandled event received: " << opcode;
    } break;
    }

    return true;
}

} // namespace nl80211

std::shared_ptr<ap_wlan_hal> ap_wlan_hal_create(std::string iface_name, hal_conf_t hal_conf,
                                                base_wlan_hal::hal_event_cb_t callback)
{
    return std::make_shared<nl80211::ap_wlan_hal_nl80211>(iface_name, callback, hal_conf);
}

} // namespace bwl
