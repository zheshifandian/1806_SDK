/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "ap_wlan_hal_dummy.h"

#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_os_utils.h>
#include <bcl/beerocks_string_utils.h>
#include <bcl/beerocks_utils.h>
#include <bcl/network/network_utils.h>
#include <bcl/son/son_wireless_utils.h>
#include <easylogging++.h>
#include <math.h>
#include <sstream>

//////////////////////////////////////////////////////////////////////////////
////////////////////////// Local Module Definitions //////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace bwl {
namespace dummy {

#define CSA_EVENT_FILTERING_TIMEOUT_MS 1000

// Temporary storage for station capabilities
struct SRadioCapabilitiesStrings {
    std::string supported_rates;
    std::string ht_cap;
    std::string ht_mcs;
    std::string vht_cap;
    std::string vht_mcs;
    std::string rrm_caps;
};

//////////////////////////////////////////////////////////////////////////////
/////////////////////////// Local Module Functions ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

static ap_wlan_hal::Event dummy_to_bwl_event(const std::string &opcode)
{
    if (opcode == "AP-ENABLED") {
        return ap_wlan_hal::Event::AP_Enabled;
    } else if (opcode == "AP-DISABLED") {
        return ap_wlan_hal::Event::AP_Disabled;
    } else if (opcode == "AP-STA-CONNECTED") {
        return ap_wlan_hal::Event::STA_Connected;
    } else if (opcode == "AP-STA-DISCONNECTED") {
        return ap_wlan_hal::Event::STA_Disconnected;
    } else if (opcode == "UNCONNECTED_STA_RSSI") {
        return ap_wlan_hal::Event::STA_Unassoc_RSSI;
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
    } else if (opcode == "BSS-TM-RESP") {
        return ap_wlan_hal::Event::BSS_TM_Response;
    } else if (opcode == "DFS-CAC-COMPLETED") {
        return ap_wlan_hal::Event::DFS_CAC_Completed;
    } else if (opcode == "DFS-NOP-FINISHED") {
        return ap_wlan_hal::Event::DFS_NOP_Finished;
    } else if (opcode == "MGMT-FRAME") {
        return ap_wlan_hal::Event::MGMT_Frame;
    } else if (opcode == "AP-STA-POSSIBLE-PSK-MISMATCH") {
        return ap_wlan_hal::Event::AP_Sta_Possible_Psk_Mismatch;
    }

    return ap_wlan_hal::Event::Invalid;
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Implementation ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// NOTE: Since *base_wlan_hal_dummy* inherits *base_wlan_hal* virtually, we
//       need to explicitly call it's from any deriving class
ap_wlan_hal_dummy::ap_wlan_hal_dummy(const std::string &iface_name, hal_event_cb_t callback,
                                     const hal_conf_t &hal_conf)
    : base_wlan_hal(bwl::HALType::AccessPoint, iface_name, IfaceType::Intel, callback, hal_conf),
      base_wlan_hal_dummy(bwl::HALType::AccessPoint, iface_name, callback, hal_conf)
{
    std::string events[] = {};
    int events_size      = sizeof(events) / sizeof(std::string);
    m_filtered_events.insert(events, events + events_size);
}

ap_wlan_hal_dummy::~ap_wlan_hal_dummy() {}

HALState ap_wlan_hal_dummy::attach(bool block)
{
    auto state = base_wlan_hal_dummy::attach(block);

    // Initialize status files
    if (m_radio_info.is_5ghz) {
        set_channel(149, beerocks::eWiFiBandwidth::BANDWIDTH_80, 5775);
    } else {
        set_channel(1, beerocks::eWiFiBandwidth::BANDWIDTH_40, 0);
    }

    // On Operational send the AP_Attached event to the AP Manager
    if (state == HALState::Operational) {
        event_queue_push(Event::AP_Attached);
    }

    return state;
}

bool ap_wlan_hal_dummy::enable() { return true; }

bool ap_wlan_hal_dummy::disable() { return true; }

bool ap_wlan_hal_dummy::set_start_disabled(bool enable, int vap_id) { return true; }

bool ap_wlan_hal_dummy::set_channel(int chan, beerocks::eWiFiBandwidth bw, int center_channel)
{
    m_radio_info.channel         = chan;
    m_radio_info.bandwidth       = beerocks::utils::convert_bandwidth_to_int(bw);
    m_radio_info.vht_center_freq = center_channel;
    m_radio_info.is_dfs_channel  = son::wireless_utils::is_dfs_channel(chan);
    std::stringstream value;
    value << "channel: " << chan << std::endl;
    value << "bw: " << m_radio_info.bandwidth << std::endl;
    value << "center_channel: " << center_channel << std::endl;
    return write_status_file("channel", value.str());
}

bool ap_wlan_hal_dummy::sta_allow(const std::string &mac, const std::string &bssid)
{
    LOG(DEBUG) << "Got client allow request for " << mac << " on bssid " << bssid;
    return true;
}

bool ap_wlan_hal_dummy::sta_deny(const std::string &mac, const std::string &bssid)
{
    LOG(DEBUG) << "Got client disallow request for " << mac << " on bssid " << bssid
               << " reject_sta: 33";
    return true;
}

bool ap_wlan_hal_dummy::sta_disassoc(int8_t vap_id, const std::string &mac, uint32_t reason)
{
    return true;
}

bool ap_wlan_hal_dummy::sta_deauth(int8_t vap_id, const std::string &mac, uint32_t reason)
{
    return true;
}

bool ap_wlan_hal_dummy::sta_bss_steer(int8_t vap_id, const std::string &mac,
                                      const std::string &bssid, int oper_class, int chan,
                                      int disassoc_timer_btt, int valid_int_btt, int reason)
{
    LOG(DEBUG) << "Got steer request for " << mac << " steer to " << bssid;

    auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE));
    auto msg      = reinterpret_cast<sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE *>(msg_buff.get());
    LOG_IF(!msg, FATAL) << "Memory allocation failed!";

    memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE));

    msg->params.mac         = tlvf::mac_from_string(mac);
    msg->params.status_code = 0;
    // source_bssid should be the vap bssid and not radio_mac, but
    // dummy mode doesn't use vaps yet
    msg->params.source_bssid = tlvf::mac_from_string(get_radio_mac());

    // Add the message to the queue
    event_queue_push(Event::BSS_TM_Response, msg_buff);

    return true;
}

bool ap_wlan_hal_dummy::update_vap_credentials(
    std::list<son::wireless_utils::sBssInfoConf> &bss_info_conf_list,
    const std::string &backhaul_wps_ssid, const std::string &backhaul_wps_passphrase)
{
    int vap_id = beerocks::IFACE_VAP_ID_MIN;

    for (auto bss_info_conf : bss_info_conf_list) {
        auto auth_type =
            son::wireless_utils::wsc_to_bwl_authentication(bss_info_conf.authentication_type);
        if (auth_type == "INVALID") {
            LOG(ERROR) << "Invalid auth_type " << int(bss_info_conf.authentication_type);
            return false;
        }
        auto enc_type = son::wireless_utils::wsc_to_bwl_encryption(bss_info_conf.encryption_type);
        if (enc_type == "INVALID") {
            LOG(ERROR) << "Invalid enc_type " << int(bss_info_conf.encryption_type);
            return false;
        }

        LOG(DEBUG) << "Autoconfiguration for ssid: " << bss_info_conf.ssid
                   << " auth_type: " << auth_type << " encr_type: " << enc_type
                   << " network_key: " << bss_info_conf.network_key
                   << " fronthaul: " << beerocks::string_utils::bool_str(bss_info_conf.fronthaul)
                   << " backhaul: " << beerocks::string_utils::bool_str(bss_info_conf.backhaul);

        m_radio_info.available_vaps[vap_id].fronthaul = bss_info_conf.fronthaul;
        m_radio_info.available_vaps[vap_id].backhaul  = bss_info_conf.backhaul;
        m_radio_info.available_vaps[vap_id++].ssid    = bss_info_conf.ssid;
    }

    /* Tear down all other VAPs */
    while (vap_id < predefined_vaps_num) {
        m_radio_info.available_vaps[vap_id].fronthaul = false;
        m_radio_info.available_vaps[vap_id].backhaul  = false;
        m_radio_info.available_vaps[vap_id++].ssid.clear();
    }

    /* Write current conf to tmp file*/
    std::stringstream value;
    for (int id = beerocks::IFACE_VAP_ID_MIN; id < predefined_vaps_num; id++) {
        value << "- vap_" << id << ":" << std::endl;
        value << "  bssid: " << m_radio_info.available_vaps[id].mac << std::endl;
        value << "  ssid: '" << m_radio_info.available_vaps[id].ssid << "'" << std::endl;
        value << "  fronthaul: " << m_radio_info.available_vaps[id].fronthaul << std::endl;
        value << "  backhaul: " << m_radio_info.available_vaps[id].backhaul << std::endl;
    }

    write_status_file("vap", value.str());

    return true;
}

bool ap_wlan_hal_dummy::sta_unassoc_rssi_measurement(const std::string &mac, int chan, int bw,
                                                     int vht_center_frequency, int delay,
                                                     int window_size)
{
    return true;
}

bool ap_wlan_hal_dummy::sta_softblock_add(const std::string &vap_name,
                                          const std::string &client_mac, uint8_t reject_error_code,
                                          uint8_t probe_snr_threshold_hi,
                                          uint8_t probe_snr_threshold_lo,
                                          uint8_t authetication_snr_threshold_hi,
                                          uint8_t authetication_snr_threshold_lo)
{
    return true;
}

bool ap_wlan_hal_dummy::sta_softblock_remove(const std::string &vap_name,
                                             const std::string &client_mac)
{
    return true;
}

bool ap_wlan_hal_dummy::switch_channel(int chan, int bw, int vht_center_frequency)
{
    LOG(TRACE) << __func__ << " channel: " << chan << ", bw: " << bw
               << ", vht_center_frequency: " << vht_center_frequency;

    m_radio_info.last_csa_sw_reason = ChanSwReason::Unknown;

    event_queue_push(Event::ACS_Started);
    event_queue_push(Event::ACS_Completed);
    event_queue_push(Event::CSA_Finished);

    return set_channel(chan, beerocks::utils::convert_bandwidth_to_enum(bw), vht_center_frequency);
}

bool ap_wlan_hal_dummy::cancel_cac(int chan, beerocks::eWiFiBandwidth bw, int vht_center_frequency,
                                   int secondary_chan)
{
    return set_channel(chan, bw, vht_center_frequency);
}

bool ap_wlan_hal_dummy::failsafe_channel_set(int chan, int bw, int vht_center_frequency)
{
    return true;
}

bool ap_wlan_hal_dummy::failsafe_channel_get(int &chan, int &bw) { return false; }

// zero wait dfs APIs
bool ap_wlan_hal_dummy::is_zwdfs_supported() { return false; }
bool ap_wlan_hal_dummy::set_zwdfs_antenna(bool enable) { return false; }
bool ap_wlan_hal_dummy::is_zwdfs_antenna_enabled() { return false; }

bool ap_wlan_hal_dummy::hybrid_mode_supported() { return true; }

bool ap_wlan_hal_dummy::restricted_channels_set(char *channel_list) { return true; }

bool ap_wlan_hal_dummy::restricted_channels_get(char *channel_list) { return false; }

bool ap_wlan_hal_dummy::read_acs_report() { return true; }

bool ap_wlan_hal_dummy::set_tx_power_limit(int tx_pow_limit)
{
    LOG(TRACE) << " setting power limit: " << tx_pow_limit << " dBm";
    m_radio_info.tx_power = tx_pow_limit;
    std::stringstream value;
    value << "tx_power: " << m_radio_info.tx_power << std::endl;
    write_status_file("tx_power", value.str());
    return true;
}

bool ap_wlan_hal_dummy::set_vap_enable(const std::string &iface_name, const bool enable)
{
    return true;
}

bool ap_wlan_hal_dummy::get_vap_enable(const std::string &iface_name, bool &enable) { return true; }

bool ap_wlan_hal_dummy::generate_connected_clients_events(
    bool &is_finished_all_clients, std::chrono::steady_clock::time_point max_iteration_timeout)
{
    return true;
}

bool ap_wlan_hal_dummy::pre_generate_connected_clients_events() { return true; }

bool ap_wlan_hal_dummy::start_wps_pbc()
{
    LOG(DEBUG) << "Start WPS PBC";
    return true;
}

bool ap_wlan_hal_dummy::set_mbo_assoc_disallow(const std::string &bssid, bool enable)
{
    LOG(DEBUG) << "Set MBO ASSOC DISALLOW for bssid " << bssid << " to " << enable;
    return true;
}

bool ap_wlan_hal_dummy::set_radio_mbo_assoc_disallow(bool enable)
{
    LOG(DEBUG) << "Set MBO ASSOC DISALLOW for radio to " << enable;
    return true;
}

bool ap_wlan_hal_dummy::set_primary_vlan_id(uint16_t primary_vlan_id)
{
    LOG(DEBUG) << "set_primary_vlan_id " << primary_vlan_id;
    return true;
}

bool ap_wlan_hal_dummy::process_dummy_data(parsed_obj_map_t &parsed_obj) { return true; }

bool ap_wlan_hal_dummy::process_dummy_event(parsed_obj_map_t &parsed_obj)
{
    char *tmp_str;

    // Filter out empty events
    std::string opcode;
    if (!(parsed_obj.find(DUMMY_EVENT_KEYLESS_PARAM_OPCODE) != parsed_obj.end() &&
          !(opcode = parsed_obj[DUMMY_EVENT_KEYLESS_PARAM_OPCODE]).empty())) {
        return true;
    }

    LOG(TRACE) << __func__ << " - opcode: |" << opcode << "|";

    auto event = dummy_to_bwl_event(opcode);

    switch (event) {
    // STA Connected
    case Event::STA_Connected: {
        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION));
        auto msg =
            reinterpret_cast<sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION));

        msg->params.vap_id = beerocks::IFACE_VAP_ID_MIN;
        LOG(DEBUG) << "iface name = " << get_iface_name()
                   << ", vap_id = " << int(msg->params.vap_id);

        if (!dummy_obj_read_str(DUMMY_EVENT_KEYLESS_PARAM_MAC, parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading mac parameter!";
            return false;
        }
        msg->params.mac = tlvf::mac_from_string(tmp_str);
        const char assoc_req[] =
            "00003A01029A96FB591100504322565F029A96FB591110E431141400000E4D756C74692D41502D3234472D"
            "31010802040B0C121618242102001430140100000FAC040100000FAC040100000FAC02000032043048606C"
            "3B10515153547374757677787C7D7E7F80823B160C01020304050C161718191A1B1C1D1E1F202180818246"
            "057000000000460571505000047F0A04000A82214000408000DD070050F2020001002D1A2D1103FFFF0000"
            "000000000000000000000000000018E6E10900BF0CB079D133FAFF0C03FAFF0C03C70110DD07506F9A1603"
            "0103";

        //convert the hex string to binary
        auto binary_str                      = get_binary_association_frame(assoc_req);
        msg->params.association_frame_length = binary_str.length();

        std::copy_n(&binary_str[0], binary_str.length(), msg->params.association_frame);
        bool caps_valid = true;
        SRadioCapabilitiesStrings caps_strings;
        if (!dummy_obj_read_str("SupportedRates", parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading SupportedRates parameter!";
            caps_valid = false;
        } else {
            caps_strings.supported_rates.assign(tmp_str);
        }

        if (!dummy_obj_read_str("HT_CAP", parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading HT_CAP parameter!";
            caps_valid = false;
        } else {
            caps_strings.ht_cap.assign(tmp_str);
        }

        if (!dummy_obj_read_str("HT_MCS", parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading HT_MCS parameter!";
            caps_valid = false;
        } else {
            caps_strings.ht_mcs.assign(tmp_str);
        }

        if (!dummy_obj_read_str("VHT_CAP", parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading VHT_CAP parameter!";
            caps_valid = false;
        } else {
            caps_strings.vht_cap.assign(tmp_str);
        }

        if (!dummy_obj_read_str("VHT_MCS", parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading VHT_CAP parameter!";
            caps_valid = false;
        } else {
            caps_strings.vht_mcs.assign(tmp_str);
        }

        if (caps_valid) {
            //get_sta_caps(caps_strings, msg->params.capabilities, get_radio_info().is_5ghz);
        } else {
            LOG(ERROR) << "One or more of required capability strings is missing!";

            // Setting minimum default values
            msg->params.capabilities.ant_num             = 1;
            msg->params.capabilities.wifi_standard       = STANDARD_N;
            msg->params.capabilities.default_mcs         = MCS_6;
            msg->params.capabilities.ht_ss               = 1;
            msg->params.capabilities.ht_bw               = beerocks::BANDWIDTH_20;
            msg->params.capabilities.ht_mcs              = beerocks::MCS_7;
            msg->params.capabilities.ht_low_bw_short_gi  = 1;
            msg->params.capabilities.ht_high_bw_short_gi = 0;
            if (m_radio_info.is_5ghz) {
                msg->params.capabilities.wifi_standard |= STANDARD_AC;
                msg->params.capabilities.vht_ss               = 1;
                msg->params.capabilities.vht_bw               = beerocks::BANDWIDTH_80;
                msg->params.capabilities.vht_mcs              = beerocks::MCS_9;
                msg->params.capabilities.vht_low_bw_short_gi  = 1;
                msg->params.capabilities.vht_high_bw_short_gi = 0;
            }
        }

        // Add the message to the queue
        event_queue_push(Event::STA_Connected, msg_buff);
    } break;
    case Event::STA_Disconnected: {
        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION));
        auto msg =
            reinterpret_cast<sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION));

        msg->params.vap_id = beerocks::IFACE_VAP_ID_MIN;
        LOG(DEBUG) << "iface name = " << get_iface_name()
                   << ", vap_id = " << int(msg->params.vap_id);

        if (!dummy_obj_read_str(DUMMY_EVENT_KEYLESS_PARAM_MAC, parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading mac parameter!";
            return false;
        }

        // Store the MAC address of the disconnected STA
        msg->params.mac = tlvf::mac_from_string(tmp_str);

        // Add the message to the queue
        event_queue_push(Event::STA_Disconnected, msg_buff);
    } break;

    // STA 802.11 management frame event
    case Event::MGMT_Frame: {
        // Read frame data
        if (!dummy_obj_read_str("DATA", parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading DATA parameter!";
            return false;
        }

        // Create the management frame notification event
        auto mgmt_frame = create_mgmt_frame_notification(tmp_str);
        if (!mgmt_frame) {
            LOG(WARNING) << "Failed creating management frame notification!";
            return true; // Just a warning, do not fail
        }

        event_queue_push(Event::MGMT_Frame, mgmt_frame);
    } break;

    case Event::AP_Sta_Possible_Psk_Mismatch: {
        LOG(DEBUG) << "Ap STA Possible PSK Mismatch";
        if (!dummy_obj_read_str(DUMMY_EVENT_KEYLESS_PARAM_MAC, parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading mac parameter of mismatched psk station!";
            return false;
        }
        auto mismatch_psk     = std::make_shared<sSTA_MISMATCH_PSK>();
        mismatch_psk->sta_mac = tlvf::mac_from_string(tmp_str);
        event_queue_push(Event::AP_Sta_Possible_Psk_Mismatch, mismatch_psk);
    } break;

    case Event::AP_Disabled: {
        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sHOSTAP_DISABLED_NOTIFICATION));
        auto msg      = reinterpret_cast<sHOSTAP_DISABLED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        memset(msg_buff.get(), 0, sizeof(sHOSTAP_DISABLED_NOTIFICATION));

        std::string interface = parsed_obj[DUMMY_EVENT_KEYLESS_PARAM_IFACE];
        if (interface.empty()) {
            LOG(ERROR) << "Could not find interface name.";
            return false;
        }

        m_radio_info.radio_state = eRadioState::DISABLED;

        auto iface_ids = beerocks::utils::get_ids_from_iface_string(interface);
        msg->vap_id    = iface_ids.vap_id;

        event_queue_push(Event::AP_Disabled, msg_buff); // send message to the AP manager

    } break;

    case Event::AP_Enabled: {
        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sHOSTAP_ENABLED_NOTIFICATION));
        auto msg      = reinterpret_cast<sHOSTAP_ENABLED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        memset(msg_buff.get(), 0, sizeof(sHOSTAP_ENABLED_NOTIFICATION));

        std::string interface = parsed_obj[DUMMY_EVENT_KEYLESS_PARAM_IFACE];
        if (interface.empty()) {
            LOG(ERROR) << "Could not find interface name.";
            return false;
        }

        m_radio_info.radio_state = eRadioState::ENABLED;

        auto iface_ids = beerocks::utils::get_ids_from_iface_string(interface);
        msg->vap_id    = iface_ids.vap_id;

        event_queue_push(Event::AP_Enabled, msg_buff);
    } break;

    // Gracefully ignore unhandled events
    default: {
        LOG(DEBUG) << "Unhandled event received: " << opcode;
    } break;
    }

    return true;
}

bool ap_wlan_hal_dummy::set(const std::string &param, const std::string &value, int vap_id)
{
    LOG(TRACE) << __func__;
    return true;
}

} // namespace dummy

std::shared_ptr<ap_wlan_hal> ap_wlan_hal_create(std::string iface_name, bwl::hal_conf_t hal_conf,
                                                base_wlan_hal::hal_event_cb_t callback)
{
    return std::make_shared<dummy::ap_wlan_hal_dummy>(iface_name, callback, hal_conf);
}

} // namespace bwl
