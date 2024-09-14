/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "mon_wlan_hal_nl80211.h"

#include <bcl/beerocks_utils.h>
#include <bcl/network/network_utils.h>
#include <bwl/key_value_parser.h>

#include <easylogging++.h>

#include <cmath>

extern "C" {
#include <wpa_ctrl.h>
}

#include <linux/nl80211.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>
#include <netlink/msg.h>
#include <netlink/netlink.h>

namespace bwl {
namespace nl80211 {

//////////////////////////////////////////////////////////////////////////////
////////////////////////// Local Module Definitions //////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define GET_OP_CLASS(channel) ((channel < 14) ? 4 : 5)
#define BUFFER_SIZE 4096

//////////////////////////////////////////////////////////////////////////////
/////////////////////////// Local Module Functions ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

static mon_wlan_hal::Event wav_to_bwl_event(const std::string &opcode)
{
    if (opcode == "AP-STA-CONNECTED") {
        return mon_wlan_hal::Event::STA_Connected;
    } else if (opcode == "AP-STA-DISCONNECTED") {
        return mon_wlan_hal::Event::STA_Disconnected;
    } else if (opcode == "BEACON-REQ-TX-STATUS") {
        return mon_wlan_hal::Event::RRM_Beacon_Request_Status;
    } else if (opcode == "BEACON-RESP-RX") {
        return mon_wlan_hal::Event::RRM_Beacon_Response;
    }
    // } else if (opcode == "RRM-LINK-MEASUREMENT-RECEIVED") {

    return mon_wlan_hal::Event::Invalid;
}

#if 0

static void calc_curr_traffic(std::string buff, uint64_t &total, uint32_t &curr)
{
    // Convert to numeric value
    uint64_t val = beerocks::string_utils::stoi(buff);

    if (val >= total) {
        curr = val - total;
    } else {
        curr = val;
    }
    total = val;
}

#endif

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Implementation ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

mon_wlan_hal_nl80211::mon_wlan_hal_nl80211(const std::string &iface_name, hal_event_cb_t callback,
                                           const bwl::hal_conf_t &hal_conf)
    : base_wlan_hal(bwl::HALType::Monitor, iface_name, IfaceType::Intel, callback, hal_conf),
      base_wlan_hal_nl80211(bwl::HALType::Monitor, iface_name, callback, BUFFER_SIZE, hal_conf)
{
    std::string events[] = {};
    int events_size      = sizeof(events) / sizeof(std::string);
    m_filtered_events.insert(events, events + events_size);
}

mon_wlan_hal_nl80211::~mon_wlan_hal_nl80211() {}

bool mon_wlan_hal_nl80211::update_radio_stats(SRadioStats &radio_stats)
{
    beerocks::net::sInterfaceStats iface_stats;
    if (!beerocks::net::network_utils::get_iface_stats(get_iface_name(), iface_stats)) {
        LOG(ERROR) << "Failed to get interface statistics for interface " << get_iface_name();
        return false;
    }

    calc_curr_traffic(iface_stats.tx_bytes, radio_stats.tx_bytes_cnt, radio_stats.tx_bytes);
    calc_curr_traffic(iface_stats.rx_bytes, radio_stats.rx_bytes_cnt, radio_stats.rx_bytes);
    calc_curr_traffic(iface_stats.tx_packets, radio_stats.tx_packets_cnt, radio_stats.tx_packets);
    calc_curr_traffic(iface_stats.rx_packets, radio_stats.rx_packets_cnt, radio_stats.rx_packets);

    radio_stats.errors_sent     = iface_stats.tx_errors;
    radio_stats.errors_received = iface_stats.rx_errors;
    radio_stats.noise           = 0;

    return true;
}

bool mon_wlan_hal_nl80211::update_vap_stats(const std::string &vap_iface_name, SVapStats &vap_stats)
{
    beerocks::net::sInterfaceStats iface_stats;
    if (!beerocks::net::network_utils::get_iface_stats(vap_iface_name, iface_stats)) {
        LOG(ERROR) << "Failed to get interface statistics for interface " << vap_iface_name;
        return false;
    }

    calc_curr_traffic(iface_stats.tx_bytes, vap_stats.tx_bytes_cnt, vap_stats.tx_bytes);
    calc_curr_traffic(iface_stats.rx_bytes, vap_stats.rx_bytes_cnt, vap_stats.rx_bytes);
    calc_curr_traffic(iface_stats.tx_packets, vap_stats.tx_packets_cnt, vap_stats.tx_packets);
    calc_curr_traffic(iface_stats.rx_packets, vap_stats.rx_packets_cnt, vap_stats.rx_packets);

    vap_stats.errors_sent     = iface_stats.tx_errors;
    vap_stats.errors_received = iface_stats.rx_errors;
    vap_stats.retrans_count   = 0;

    return true;
}

bool mon_wlan_hal_nl80211::update_stations_stats(const std::string &vap_iface_name,
                                                 const std::string &sta_mac, SStaStats &sta_stats)
{
    static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1];
    stats_policy[NL80211_STA_INFO_INACTIVE_TIME] = {NLA_U32, 0, 0};
    stats_policy[NL80211_STA_INFO_RX_BYTES]      = {NLA_U32, 0, 0};
    stats_policy[NL80211_STA_INFO_TX_BYTES]      = {NLA_U32, 0, 0};
    stats_policy[NL80211_STA_INFO_RX_PACKETS]    = {NLA_U32, 0, 0};
    stats_policy[NL80211_STA_INFO_TX_PACKETS]    = {NLA_U32, 0, 0};
    stats_policy[NL80211_STA_INFO_SIGNAL]        = {NLA_U8, 0, 0};
    stats_policy[NL80211_STA_INFO_T_OFFSET]      = {NLA_U64, 0, 0};
    stats_policy[NL80211_STA_INFO_TX_BITRATE]    = {NLA_NESTED, 0, 0};
    stats_policy[NL80211_STA_INFO_RX_BITRATE]    = {NLA_NESTED, 0, 0};
    stats_policy[NL80211_STA_INFO_LLID]          = {NLA_U16, 0, 0};
    stats_policy[NL80211_STA_INFO_PLID]          = {NLA_U16, 0, 0};
    stats_policy[NL80211_STA_INFO_PLINK_STATE]   = {NLA_U8, 0, 0};
    stats_policy[NL80211_STA_INFO_TX_RETRIES]    = {NLA_U32, 0, 0};
    stats_policy[NL80211_STA_INFO_TX_FAILED]     = {NLA_U32, 0, 0};
    stats_policy[NL80211_STA_INFO_STA_FLAGS]  = {NLA_UNSPEC, sizeof(struct nl80211_sta_flag_update),
                                                0};
    stats_policy[NL80211_STA_INFO_LOCAL_PM]   = {NLA_U32, 0, 0};
    stats_policy[NL80211_STA_INFO_PEER_PM]    = {NLA_U32, 0, 0};
    stats_policy[NL80211_STA_INFO_NONPEER_PM] = {NLA_U32, 0, 0};
    stats_policy[NL80211_STA_INFO_CHAIN_SIGNAL]     = {NLA_NESTED, 0, 0};
    stats_policy[NL80211_STA_INFO_CHAIN_SIGNAL_AVG] = {NLA_NESTED, 0, 0};

    static struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1];
    rate_policy[NL80211_RATE_INFO_BITRATE]         = {NLA_U16, 0, 0};
    rate_policy[NL80211_RATE_INFO_BITRATE32]       = {NLA_U32, 0, 0};
    rate_policy[NL80211_RATE_INFO_MCS]             = {NLA_U8, 0, 0};
    rate_policy[NL80211_RATE_INFO_40_MHZ_WIDTH]    = {NLA_FLAG, 0, 0};
    rate_policy[NL80211_RATE_INFO_80_MHZ_WIDTH]    = {NLA_FLAG, 0, 0};
    rate_policy[NL80211_RATE_INFO_80P80_MHZ_WIDTH] = {NLA_FLAG, 0, 0};
    rate_policy[NL80211_RATE_INFO_160_MHZ_WIDTH]   = {NLA_FLAG, 0, 0};
    rate_policy[NL80211_RATE_INFO_SHORT_GI]        = {NLA_FLAG, 0, 0};

    auto ret = send_nl80211_msg(
        NL80211_CMD_GET_STATION, 0,
        // Create the message
        [&](struct nl_msg *msg) -> bool {
            auto mac = tlvf::mac_from_string(sta_mac);
            nla_put(msg, NL80211_ATTR_MAC, ETH_ALEN, mac.oct);
            return true;
        },
        // Handle the reponse
        [&](struct nl_msg *msg) -> bool {
            struct nlattr *tb[NL80211_ATTR_MAX + 1];
            struct genlmsghdr *gnlh = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
            struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
            struct nlattr *rinfo[NL80211_RATE_INFO_MAX + 1];

            // Parse the netlink message
            nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0),
                      NULL);

            if (!tb[NL80211_ATTR_STA_INFO]) {
                LOG(ERROR) << "sta stats missing!";
                return false;
            }

            // Parse nested station stats
            if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX, tb[NL80211_ATTR_STA_INFO],
                                 stats_policy)) {
                LOG(ERROR) << "failed to parse nested attributes!";
                return false;
            }

            // RX RSSI
            if (sinfo[NL80211_STA_INFO_SIGNAL]) {
                int8_t signal          = int8_t(nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]));
                sta_stats.rx_rssi_watt = pow(10, (signal / 10.0));
                sta_stats.rx_rssi_watt_samples_cnt++;
            }

            // RX SNR is not supported
            sta_stats.rx_snr_watt             = 0;
            sta_stats.rx_snr_watt_samples_cnt = 0;

            // Bitrate parsing helper function
            auto parse_bitrate_func = [&](struct nlattr *bitrate_attr, int &rate,
                                          uint8_t &bw) -> void {
                rate = 0;
                bw   = 0;
                if (nla_parse_nested(rinfo, NL80211_RATE_INFO_MAX, bitrate_attr, rate_policy)) {
                    LOG(ERROR) << "Failed to parse nested rate attributes!";
                    return;
                }

                //rate is returned with unit 100kbps
                if (rinfo[NL80211_RATE_INFO_BITRATE32])
                    rate = nla_get_u32(rinfo[NL80211_RATE_INFO_BITRATE32]);
                else if (rinfo[NL80211_RATE_INFO_BITRATE])
                    rate = nla_get_u16(rinfo[NL80211_RATE_INFO_BITRATE]);

                bw = beerocks::BANDWIDTH_20;
                if (rinfo[NL80211_RATE_INFO_40_MHZ_WIDTH]) {
                    bw = beerocks::BANDWIDTH_40;
                } else if (rinfo[NL80211_RATE_INFO_80_MHZ_WIDTH]) {
                    bw = beerocks::BANDWIDTH_80;
                } else if ((rinfo[NL80211_RATE_INFO_80P80_MHZ_WIDTH]) ||
                           (rinfo[NL80211_RATE_INFO_160_MHZ_WIDTH])) {
                    bw = beerocks::BANDWIDTH_160;
                } else if ((rinfo[NL80211_RATE_INFO_5_MHZ_WIDTH]) ||
                           (rinfo[NL80211_RATE_INFO_10_MHZ_WIDTH])) {
                    bw = beerocks::BANDWIDTH_UNKNOWN;
                }
            };

            int rate;
            uint8_t bw;
            // TX Phy Rate
            if (sinfo[NL80211_STA_INFO_TX_BITRATE]) {
                parse_bitrate_func(sinfo[NL80211_STA_INFO_TX_BITRATE], rate, bw);
                sta_stats.tx_phy_rate_100kb = rate;
                sta_stats.dl_bandwidth      = bw;
            }

            // RX Phy Rate
            if (sinfo[NL80211_STA_INFO_RX_BITRATE]) {
                parse_bitrate_func(sinfo[NL80211_STA_INFO_RX_BITRATE], rate, bw);
                sta_stats.rx_phy_rate_100kb = rate;
            }

            // Traffic values calculations helper function
            auto calc_curr_traffic = [](uint64_t val, uint64_t &total, uint32_t &curr) {
                if (val >= total) {
                    curr = val - total;
                } else {
                    curr = val;
                }
                total = val;
            };

            // TX Bytes
            if (sinfo[NL80211_STA_INFO_TX_BYTES]) {
                calc_curr_traffic(nla_get_u32(sinfo[NL80211_STA_INFO_TX_BYTES]),
                                  sta_stats.tx_bytes_cnt, sta_stats.tx_bytes);
            }

            // RX Bytes
            if (sinfo[NL80211_STA_INFO_RX_BYTES]) {
                calc_curr_traffic(nla_get_u32(sinfo[NL80211_STA_INFO_RX_BYTES]),
                                  sta_stats.rx_bytes_cnt, sta_stats.rx_bytes);
            }

            // TX Packets
            if (sinfo[NL80211_STA_INFO_TX_PACKETS]) {
                calc_curr_traffic(nla_get_u32(sinfo[NL80211_STA_INFO_TX_PACKETS]),
                                  sta_stats.tx_packets_cnt, sta_stats.tx_packets);
            }

            // RX Packets
            if (sinfo[NL80211_STA_INFO_RX_PACKETS]) {
                calc_curr_traffic(nla_get_u32(sinfo[NL80211_STA_INFO_RX_PACKETS]),
                                  sta_stats.rx_packets_cnt, sta_stats.rx_packets);
            }

            // TX Retries
            if (sinfo[NL80211_STA_INFO_TX_RETRIES]) {
                sta_stats.retrans_count = nla_get_u32(sinfo[NL80211_STA_INFO_TX_RETRIES]);
            }

            return true;
        },
        vap_iface_name);

    if (!ret) {
        LOG(ERROR) << "Failed updating stats for station: " << sta_mac;
        return false;
    }

    return true;
}

bool mon_wlan_hal_nl80211::update_station_qos_control_params(const std::string &vap_iface_name,
                                                             const std::string &sta_mac,
                                                             SStaQosCtrlParams &sta_qos_ctrl_params)
{
    constexpr uint8_t TID_QUEUE_SIZE_DUMMY =
        3; // Dummy values to initialize queue size for associated TIDs
    // To Do: NL call to update qos control params to be handled in PPM-1941.

    // Setting dummy values to SStaQosCtrlParams until PPM-1941 is closed.
    std::fill_n(sta_qos_ctrl_params.tid_queue_size, IEEE80211_QOS_TID_MAX_UP, TID_QUEUE_SIZE_DUMMY);

    return true;
}

bool mon_wlan_hal_nl80211::sta_channel_load_11k_request(const std::string &vap_iface_name,
                                                        const SStaChannelLoadRequest11k &req)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool mon_wlan_hal_nl80211::sta_beacon_11k_request(const std::string &vap_iface_name,
                                                  const SBeaconRequest11k &req, int &dialog_token)
{
    LOG(TRACE) << __func__;

    // parameters preperations

    // Mode
    auto request = (!req.enable) ? 0 : req.request;
    auto report  = (!req.enable) ? 0 : req.report;

    uint8_t req_mode = (req.parallel | (req.enable ? 0x02 : 0) | (request ? 0x04 : 0) |
                        (report ? 0x08 : 0) | (req.mandatory_duration ? 0x10 : 0));

    auto op_class = req.op_class < 0 ? GET_OP_CLASS(get_radio_info().channel) : req.op_class;

    if (req.op_class <= 0) {
        LOG(WARNING) << __func__ << " op_class not set!";
    }

    uint8_t measurement_mode;
    switch ((SBeaconRequest11k::MeasurementMode)(req.measurement_mode)) {
    case SBeaconRequest11k::MeasurementMode::Passive:
        measurement_mode = 0;
        break;
    case SBeaconRequest11k::MeasurementMode::Active:
        measurement_mode = 1;
        break;
    case SBeaconRequest11k::MeasurementMode::Table:
        measurement_mode = 2;
        break;
    default: {
        LOG(WARNING) << "Invalid measuremetn mode: " << int(req.measurement_mode)
                     << ", using PASSIVE...";

        measurement_mode = 0;
    }
    }

    // REQ_BEACON 02:14:5f:b8:ae:9f req_mode=01 510B000011110004f021318193
    // op_class (2), channel (2), randomization_interval (4), measurement duration (4), measurement mode (2), bssid (12)

    // build command
    std::string cmd = "REQ_BEACON " + tlvf::mac_to_string(req.sta_mac.oct) +
                      " " + // Destination MAC Address
                      "req_mode=" + beerocks::string_utils::int_to_hex_string(req_mode, 2) +
                      " " + // Measurements Request Mode
                      beerocks::string_utils::int_to_hex_string(op_class, 2) +
                      beerocks::string_utils::int_to_hex_string(req.channel, 2) +
                      beerocks::string_utils::int_to_hex_string(htobe16(req.rand_ival), 4) +
                      beerocks::string_utils::int_to_hex_string(htobe16(req.duration), 4) +
                      beerocks::string_utils::int_to_hex_string(measurement_mode, 2) +
                      beerocks::string_utils::int_to_hex_string(req.bssid.oct[0], 2) +
                      beerocks::string_utils::int_to_hex_string(req.bssid.oct[1], 2) +
                      beerocks::string_utils::int_to_hex_string(req.bssid.oct[2], 2) +
                      beerocks::string_utils::int_to_hex_string(req.bssid.oct[3], 2) +
                      beerocks::string_utils::int_to_hex_string(req.bssid.oct[4], 2) +
                      beerocks::string_utils::int_to_hex_string(req.bssid.oct[5], 2) +
                      // Reporting detail subelement
                      // subelement id (2):
                      beerocks::string_utils::int_to_hex_string(2, 2) +
                      // length:
                      beerocks::string_utils::int_to_hex_string(1, 2) +
                      // value:
                      beerocks::string_utils::int_to_hex_string(req.reporting_detail, 2);

    if (req.use_optional_ap_ch_report != 0) {
        if (req.channel != 255) {
            LOG(ERROR) << "ap_ch_report is set but channel is NOT set to 255.";
            return false;
        }

        // AP channel report
        // subelement id:
        cmd += beerocks::string_utils::int_to_hex_string(51, 2) +
               // length:
               beerocks::string_utils::int_to_hex_string(req.use_optional_ap_ch_report, 2);
        // operating class followed by list of channels:
        unsigned ap_ch_report_size = req.use_optional_ap_ch_report;
        if (ap_ch_report_size > sizeof(req.ap_ch_report)) {
            LOG(ERROR)
                << "use_optional_ap_ch_report is bigger than the total size of ap_ch_report! "
                   "use_optional_ap_ch_report is "
                << req.use_optional_ap_ch_report;
            return false;
        }
        for (unsigned i = 0; i < ap_ch_report_size; i++) {
            cmd += beerocks::string_utils::int_to_hex_string(req.ap_ch_report[i], 2);
        }
    } else {
        if (req.channel == 255) {
            LOG(ERROR) << "Channel is set to 255 but ap_ch_report is NOT set.";
        }
    }

    // Print the command
    LOG(DEBUG) << __func__ << " - " << cmd;

    // Send the command
    parsed_obj_map_t reply;
    if (!wpa_ctrl_send_msg(cmd, reply, vap_iface_name)) {
        LOG(ERROR) << __func__ << " failed";
        return false;
    }

    dialog_token = 0; //tmp_int;

    return true;
}

bool mon_wlan_hal_nl80211::sta_link_measurements_11k_request(const std::string &vap_iface_name,
                                                             const std::string &sta_mac)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool mon_wlan_hal_nl80211::channel_scan_trigger(int dwell_time_msec,
                                                const std::vector<unsigned int> &channel_pool)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return false;
}

bool mon_wlan_hal_nl80211::channel_scan_dump_cached_results()
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED";
    return false;
}

bool mon_wlan_hal_nl80211::channel_scan_dump_results()
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED";
    return false;
}

bool mon_wlan_hal_nl80211::generate_connected_clients_events(
    bool &is_finished_all_clients, std::chrono::steady_clock::time_point max_iteration_timeout)
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED";
    is_finished_all_clients = true;

    // TODO: implement the API (PPM-1152)
    // currently returning true even though not implemented in order not to break
    // the flow if this HAL is used by any flow, since the API return value is checked by
    // a common flow in the monitor.
    return true;
}

bool mon_wlan_hal_nl80211::pre_generate_connected_clients_events()
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED";

    // TODO: implement the API (PPM-1152)
    // currently returning true even though not implemented in order not to break
    // the flow if this HAL is used by any flow, since the API return value is checked by
    // a common flow in the monitor.
    return true;
}

bool mon_wlan_hal_nl80211::process_nl80211_event(parsed_obj_map_t &parsed_obj)
{
    // Filter out empty events
    std::string opcode;
    if (!(parsed_obj.find("_opcode") != parsed_obj.end() &&
          !(opcode = parsed_obj["_opcode"]).empty())) {
        return true;
    }

    auto event = wav_to_bwl_event(opcode);

    std::string interface;
    if (parsed_obj.find(bwl::EVENT_KEYLESS_PARAM_IFACE) != parsed_obj.end()) {
        interface = parsed_obj.at(bwl::EVENT_KEYLESS_PARAM_IFACE);
    }
    if (interface.empty()) {
        LOG(DEBUG) << "Could not find interface name.";
    }

    auto vap_id = get_vap_id_with_bss(interface);
    if (vap_id < 0) {
        LOG(DEBUG) << "Unknown vap_id " << vap_id;
    }

    // Handle the event
    switch (event) {

    case Event::STA_Connected: {

        // TODO: Change to HAL objects
        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sACTION_MONITOR_CLIENT_ASSOCIATED_NOTIFICATION));
        auto msg =
            reinterpret_cast<sACTION_MONITOR_CLIENT_ASSOCIATED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_MONITOR_CLIENT_ASSOCIATED_NOTIFICATION));

        if (vap_id < 0) {
            LOG(ERROR) << "Invalid vap_id " << vap_id;
            return false;
        }
        msg->vap_id = vap_id;
        msg->mac    = tlvf::mac_from_string(parsed_obj["_mac"]);

        // Add the message to the queue
        event_queue_push(Event::STA_Connected, msg_buff);

    } break;

    case Event::STA_Disconnected: {

        // TODO: Change to HAL objects
        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_MONITOR_CLIENT_DISCONNECTED_NOTIFICATION));
        auto msg =
            reinterpret_cast<sACTION_MONITOR_CLIENT_DISCONNECTED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_MONITOR_CLIENT_DISCONNECTED_NOTIFICATION));

        // Store the MAC address of the disconnected STA
        msg->mac = tlvf::mac_from_string(parsed_obj["_mac"]);

        // Add the message to the queue
        event_queue_push(Event::STA_Disconnected, msg_buff);

    } break;

    case Event::RRM_Beacon_Request_Status: {

        // Allocate response object
        auto resp_buff = ALLOC_SMART_BUFFER(sizeof(SBeaconRequestStatus11k));
        auto resp      = reinterpret_cast<SBeaconRequestStatus11k *>(resp_buff.get());
        LOG_IF(!resp, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(resp_buff.get(), 0, sizeof(SBeaconRequestStatus11k));

        // STA Mac Address
        tlvf::mac_from_string(resp->sta_mac.oct, parsed_obj["_mac"]);

        // Dialog token and ACK
        resp->dialog_token = beerocks::string_utils::stoi(parsed_obj["_arg0"]);
        resp->ack          = beerocks::string_utils::stoi(parsed_obj["ack"]);

        // Add the message to the queue
        event_queue_push(event, resp_buff);

    } break;

    case Event::RRM_Beacon_Response: {

        // Allocate response object
        auto resp_buff = ALLOC_SMART_BUFFER(sizeof(SBeaconResponse11k));
        auto resp      = reinterpret_cast<SBeaconResponse11k *>(resp_buff.get());
        LOG_IF(!resp, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(resp_buff.get(), 0, sizeof(SBeaconResponse11k));

        // STA Mac Address
        tlvf::mac_from_string(resp->sta_mac.oct, parsed_obj["_mac"]);

        // Dialog token and rep_mode
        resp->dialog_token = beerocks::string_utils::stoi(parsed_obj["_arg0"]);
        resp->rep_mode     = beerocks::string_utils::stoi(parsed_obj["_arg1"]);

        // Parse the report
        auto report = parsed_obj["_arg2"];
        if (report.length() < 52) {
            LOG(WARNING) << "Invalid 11k report length!";
            break;
        }

        // TODO: Check for argument validity and use a safer stoi version
        int idx = 0;

        // op_class
        resp->op_class = std::strtoul(report.substr(idx, 2).c_str(), 0, 16);
        idx += 2;

        // channel
        resp->channel = std::strtoul(report.substr(idx, 2).c_str(), 0, 16);
        idx += 2;

        // start_time
        resp->start_time = std::strtoull(report.substr(idx, 16).c_str(), 0, 16);
        resp->start_time = be64toh(resp->start_time);
        idx += 16;

        // measurement_duration
        resp->duration = std::strtoul(report.substr(idx, 4).c_str(), 0, 16);
        resp->duration = be16toh(resp->duration);
        idx += 4;

        // phy_type
        resp->phy_type = std::strtoul(report.substr(idx, 2).c_str(), 0, 16);
        idx += 2;

        // rcpi
        resp->rcpi = std::strtol(report.substr(idx, 2).c_str(), 0, 16);
        idx += 2;

        // rsni
        resp->rsni = std::strtol(report.substr(idx, 2).c_str(), 0, 16);
        idx += 2;

        // bssid
        resp->bssid.oct[0] = std::strtoul(report.substr(idx + 0, 2).c_str(), 0, 16);
        resp->bssid.oct[1] = std::strtoul(report.substr(idx + 2, 2).c_str(), 0, 16);
        resp->bssid.oct[2] = std::strtoul(report.substr(idx + 4, 2).c_str(), 0, 16);
        resp->bssid.oct[3] = std::strtoul(report.substr(idx + 6, 2).c_str(), 0, 16);
        resp->bssid.oct[4] = std::strtoul(report.substr(idx + 8, 2).c_str(), 0, 16);
        resp->bssid.oct[5] = std::strtoul(report.substr(idx + 10, 2).c_str(), 0, 16);
        idx += 12;

        // ant_id
        resp->ant_id = std::strtoul(report.substr(idx, 2).c_str(), 0, 16);
        idx += 2;

        // parent_tsf
        resp->parent_tsf = std::strtoull(report.substr(idx, 8).c_str(), 0, 16);
        idx += 8;

        // TODO: Ignore everything else?
        // WLAN_BEACON_REPORT_SUBELEM_FRAME_BODY == 01
        // frame_body.length = ??

        LOG(DEBUG) << "Beacon Response:" << std::endl
                   << "  op_class = " << int(resp->op_class) << std::endl
                   << "  channel = " << int(resp->channel) << std::endl
                   << "  start_time = " << int(resp->start_time) << std::endl
                   << "  duration = " << int(resp->duration) << std::endl
                   << "  phy_type = " << int(resp->phy_type) << std::endl
                   << "  rcpi = " << int(resp->rcpi) << std::endl
                   << "  rsni = " << int(resp->rsni) << std::endl
                   << "  bssid = " << resp->bssid.oct << std::endl
                   << "  ant_id = " << int(resp->ant_id) << std::endl
                   << "  parent_tfs = " << int(resp->parent_tsf);

        // Add the message to the queue
        event_queue_push(event, resp_buff);

    } break;

    // Gracefully ignore unhandled events
    // TODO: Probably should be changed to an error once WAV will stop
    //       sending empty or irrelevant events...
    default: {
        LOG(DEBUG) << "Unhandled event received: " << opcode;
        break;
    };
    }

    return true;
}

bool mon_wlan_hal_nl80211::channel_scan_abort()
{
    LOG(TRACE) << __func__ << " - NOT IMPLEMENTED!";
    return true;
}

bool mon_wlan_hal_nl80211::set_estimated_service_parameters(uint8_t *esp_info_field)
{
    // TO DO: Implement with PPM-1499
    return true;
}

} // namespace nl80211

std::shared_ptr<mon_wlan_hal> mon_wlan_hal_create(const std::string &iface_name,
                                                  base_wlan_hal::hal_event_cb_t callback,

                                                  const bwl::hal_conf_t &hal_conf)
{
    return std::make_shared<nl80211::mon_wlan_hal_nl80211>(iface_name, callback, hal_conf);
}

} // namespace bwl
