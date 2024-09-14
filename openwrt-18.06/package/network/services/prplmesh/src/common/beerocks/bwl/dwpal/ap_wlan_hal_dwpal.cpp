/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "ap_wlan_hal_dwpal.h"

#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_os_utils.h>
#include <bcl/beerocks_string_utils.h>
#include <bcl/beerocks_utils.h>
#include <bcl/network/network_utils.h>
#include <bcl/son/son_wireless_utils.h>
#include <easylogging++.h>
#include <math.h>
#include <net/if.h> // if_nametoindex

#ifdef USE_LIBSAFEC
#define restrict __restrict
#include <libsafec/safe_str_lib.h>
#elif USE_SLIBC
#include <slibc/string.h>
#else
#error "No safe C library defined, define either USE_LIBSAFEC or USE_SLIBC"
#endif

extern "C" {
#include <dwpal.h>
}

//////////////////////////////////////////////////////////////////////////////
////////////////////////// Local Module Definitions //////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace bwl {
namespace dwpal {

#define CSA_EVENT_FILTERING_TIMEOUT_MS 1000

//////////////////////////////////////////////////////////////////////////////
/////////////////////////// Local Module Functions ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

static ap_wlan_hal::Event dwpal_to_bwl_event(const std::string &opcode)
{
    if (opcode == "AP-ENABLED") {
        return ap_wlan_hal::Event::AP_Enabled;
    } else if (opcode == "AP-DISABLED") {
        return ap_wlan_hal::Event::AP_Disabled;
    } else if (opcode == "AP-STA-CONNECTED") {
        return ap_wlan_hal::Event::STA_Connected;
    } else if (opcode == "AP-STA-DISCONNECTED") {
        return ap_wlan_hal::Event::STA_Disconnected;
    } else if (opcode == "UNCONNECTED-STA-RSSI") {
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
    } else if (opcode == "BSS-TM-QUERY") {
        return ap_wlan_hal::Event::BSS_TM_Query;
    } else if (opcode == "BSS-TM-RESP") {
        return ap_wlan_hal::Event::BSS_TM_Response;
    } else if (opcode == "DFS-CAC-START") {
        return ap_wlan_hal::Event::DFS_CAC_Started;
    } else if (opcode == "DFS-CAC-COMPLETED") {
        return ap_wlan_hal::Event::DFS_CAC_Completed;
    } else if (opcode == "DFS-NOP-FINISHED") {
        return ap_wlan_hal::Event::DFS_NOP_Finished;
    } else if (opcode == "LTQ-SOFTBLOCK-DROP") {
        return ap_wlan_hal::Event::STA_Softblock_Drop;
    } else if (opcode == "AP-ACTION-FRAME-RECEIVED") {
        return ap_wlan_hal::Event::MGMT_Frame;
    } else if (opcode == "AP-STA-POSSIBLE-PSK-MISMATCH") {
        return ap_wlan_hal::Event::AP_Sta_Possible_Psk_Mismatch;
    }

    return ap_wlan_hal::Event::Invalid;
}

static uint8_t dwpal_bw_to_beerocks_bw(const uint8_t chan_width)
{
    // clang-format off
    std::map<uint8_t, beerocks::eWiFiBandwidth> bandwidths {
        // prplMesh does not distinguish between 'CHAN_WIDTH_20_NOHT' and 'CHAN_WIDTH_20'
        { 0 /*CHAN_WIDTH_20_NOHT*/, beerocks::BANDWIDTH_20 },
        { 1 /*CHAN_WIDTH_20     */, beerocks::BANDWIDTH_20 },
        { 2 /*CHAN_WIDTH_40     */, beerocks::BANDWIDTH_40 },
        { 3 /*CHAN_WIDTH_80     */, beerocks::BANDWIDTH_80 },
        { 4 /*CHAN_WIDTH_80P80  */, beerocks::BANDWIDTH_80_80 },
        { 5 /*CHAN_WIDTH_160    */, beerocks::BANDWIDTH_160 },
    };
    // clang-format on

    auto it = bandwidths.find(chan_width);
    if (bandwidths.end() == it) {
        LOG(ERROR) << "Invalid bandwidth value: " << chan_width;
        return beerocks::BANDWIDTH_UNKNOWN;
    }

    return it->second;
}

/*
 * @brief convert standard channel width number (Cf. IEEE 802.11-2020 Table 9-175)
 * to beerocks bandwidth enum
 *
 * @param chan_width_nr standard channel width number enum
 * @return BANDWIDTH_UNKNOWN if no match was found
 */
static beerocks::eWiFiBandwidth dwpal_ch_width_nr_to_beerocks_bw(const uint8_t chan_width_nr)
{
    // clang-format off
    std::map<uint8_t, beerocks::eWiFiBandwidth> bandwidths {
        { NR_CHAN_WIDTH_20,    beerocks::BANDWIDTH_20 },
        { NR_CHAN_WIDTH_40,    beerocks::BANDWIDTH_40 },
        { NR_CHAN_WIDTH_80,    beerocks::BANDWIDTH_80 },
        { NR_CHAN_WIDTH_160,   beerocks::BANDWIDTH_160 },
        { NR_CHAN_WIDTH_80P80, beerocks::BANDWIDTH_80_80 },
    };
    // clang-format on

    auto it = bandwidths.find(chan_width_nr);
    if (bandwidths.end() == it) {
        LOG(ERROR) << "Invalid bandwidth nr value: " << chan_width_nr;
        return beerocks::BANDWIDTH_UNKNOWN;
    }

    return it->second;
}

static void get_ht_mcs_capabilities(int *HT_MCS, std::string &ht_cap_str,
                                    beerocks::message::sRadioCapabilities &sta_caps)
{
    bool break_upper_loop = false;
    sta_caps.ht_bw        = beerocks::BANDWIDTH_UNKNOWN;

    if (!ht_cap_str.empty() && (HT_MCS != nullptr)) {
        uint16_t ht_cap = uint16_t(std::strtoul(ht_cap_str.c_str(), nullptr, 16));

        // flag supported channel width set: 0 ==> 20 / 1 ==> 40
        if (ht_cap & HT_CAP_INFO_SUPP_CHANNEL_WIDTH_SET) {
            sta_caps.ht_bw = beerocks::BANDWIDTH_40;
        } else {
            sta_caps.ht_bw = beerocks::BANDWIDTH_20;
        }
        sta_caps.ht_sm_power_save = ((ht_cap & HT_CAP_INFO_SMPS_MASK) >> 2) &
                                    0x03; // 0=static, 1=dynamic, 2=reserved, 3=disabled
        sta_caps.ht_low_bw_short_gi =
            (ht_cap & HT_CAP_INFO_SHORT_GI20MHZ) != 0; // 20MHz long == 0 / short == 1
        sta_caps.ht_high_bw_short_gi =
            (ht_cap & HT_CAP_INFO_SHORT_GI40MHZ) != 0; // 40MHz long == 0 / short == 1

        // parsing HT_MCS {AA, BB, CC, DD, XX, ...} to 0xDDCCBBAA
        uint32_t ht_mcs = 0;

        for (uint8_t i = 0; i < 4; i++) {
            ht_mcs |= HT_MCS[i] << (8 * i);
        }

        uint32_t mask = pow(2, 4 * 8 - 1); // 0x80000000

        for (uint8_t i = 4; i > 0; i--) {     // 4ss
            for (int8_t j = 7; j >= 0; j--) { // 8bits
                if ((ht_mcs & mask) > 0) {
                    sta_caps.ht_ss   = i;
                    sta_caps.ant_num = i;
                    sta_caps.ht_mcs  = j;
                    break_upper_loop = true;
                    break;
                }
                mask /= 2;
            }
            if (break_upper_loop) {
                break;
            }
        }
    } else {
        // Use default value
        sta_caps.ant_num = 1;
    }
}

static void get_vht_mcs_capabilities(int16_t *VHT_MCS, std::string &vht_cap_str,
                                     beerocks::message::sRadioCapabilities &sta_caps)
{
    sta_caps.vht_bw = beerocks::BANDWIDTH_UNKNOWN;

    if (!vht_cap_str.empty() && (VHT_MCS != nullptr)) {
        uint32_t vht_cap          = uint16_t(std::strtoul(vht_cap_str.c_str(), nullptr, 16));
        uint8_t supported_bw_bits = (vht_cap >> 2) & 0x03;

        if (supported_bw_bits == 0x03) { // reserved mode
            LOG(ERROR) << "INFORMATION ERROR! STA SENT RESERVED BIT COMBINATION";
        }

        // if supported_bw_bits=0 max bw is 80 Mhz, else max bw is 160 Mhz
        if (supported_bw_bits == 0) {
            sta_caps.vht_bw = beerocks::BANDWIDTH_80;
        } else {
            sta_caps.vht_bw = beerocks::BANDWIDTH_160;
        }
        sta_caps.vht_low_bw_short_gi  = (vht_cap >> 5) & 0x01; // 80 Mhz
        sta_caps.vht_high_bw_short_gi = (vht_cap >> 6) & 0x01; // 160 Mhz

        uint16_t vht_mcs_rx = 0;
        uint16_t vht_mcs_temp;

        vht_mcs_rx = VHT_MCS[0];

        for (uint8_t i = 4; i > 0; i--) { // 4ss
            vht_mcs_temp = (vht_mcs_rx >> (2 * (i - 1))) & 0x03;
            // 0 indicates support for VHT-MCS 0-7 for n spatial streams
            // 1 indicates support for VHT-MCS 0-8 for n spatial streams
            // 2 indicates support for VHT-MCS 0-9 for n spatial streams
            // 3 indicates that n spatial streams is not supported
            if (vht_mcs_temp != 0x3) { //0x3 == not supported
                sta_caps.vht_ss  = i;
                sta_caps.ant_num = i;
                sta_caps.vht_mcs = vht_mcs_temp + 7;
                break;
            }
        }
        sta_caps.vht_su_beamformer = (vht_cap >> 11) & 0x01;
        sta_caps.vht_mu_beamformer = (vht_cap >> 19) & 0x01;
    } else {
        // Use default value
        sta_caps.ant_num = 1;
    }

    // update standard
    if (sta_caps.vht_ss) {
        sta_caps.wifi_standard = STANDARD_AC;
    } else if (sta_caps.ht_ss) {
        sta_caps.wifi_standard = STANDARD_N;
    } else {
        sta_caps.wifi_standard = STANDARD_A;
    }
}

static void get_mcs_from_supported_rates(int *supported_rates,
                                         beerocks::message::sRadioCapabilities &sta_caps)
{
    uint16_t temp_rate = 0;
    uint16_t max_rate  = 0;

    for (int i = 0; i < 16; i++) {
        temp_rate = (supported_rates[i] & 0x7F) * 5; // rate/2 * 10

        if (temp_rate > max_rate) {
            max_rate = temp_rate;
        }
    }

    LOG(DEBUG) << "get_mcs_from_supported_rates() | max rate:" << std::dec << int(max_rate);

    if (son::wireless_utils::get_mcs_from_rate(max_rate, beerocks::ANT_MODE_1X1_SS1,
                                               beerocks::BANDWIDTH_20, sta_caps.default_mcs,
                                               sta_caps.default_short_gi)) {
        LOG(DEBUG) << "get_mcs_from_supported_rates() | MCS rate match";
    } else {
        LOG(DEBUG) << "get_mcs_from_supported_rates() | no MCS rate match --> using nearest value";
    }
}

static void parse_rrm_capabilities(int *RRM_CAPS, beerocks::message::sRadioCapabilities &sta_caps)
{
    sta_caps.nr_enabled            = ((RRM_CAPS[0] & WLAN_RRM_CAPS_NEIGHBOR_REPORT) != 0);
    sta_caps.link_meas             = ((RRM_CAPS[0] & WLAN_RRM_CAPS_LINK_MEASUREMENT) != 0);
    sta_caps.beacon_report_passive = ((RRM_CAPS[0] & WLAN_RRM_CAPS_BEACON_REPORT_PASSIVE) != 0);
    sta_caps.beacon_report_active  = ((RRM_CAPS[0] & WLAN_RRM_CAPS_BEACON_REPORT_ACTIVE) != 0);
    sta_caps.beacon_report_table   = ((RRM_CAPS[0] & WLAN_RRM_CAPS_BEACON_REPORT_TABLE) != 0);
    sta_caps.lci_meas              = ((RRM_CAPS[1] & WLAN_RRM_CAPS_LCI_MEASUREMENT) != 0);
    sta_caps.fmt_range_report      = ((RRM_CAPS[4] & WLAN_RRM_CAPS_FTM_RANGE_REPORT) != 0);
}

static std::shared_ptr<char> generate_client_assoc_event(const std::string &event, int vap_id,
                                                         bool radio_5G, int32_t &result)
{
    // TODO: Change to HAL objects
    auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION));
    auto msg = reinterpret_cast<sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION *>(msg_buff.get());

    if (!msg) {
        LOG(FATAL) << "Memory allocation failed";
        return nullptr;
    }

    // Initialize the message
    memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION));
    memset((char *)&msg->params.capabilities, 0, sizeof(msg->params.capabilities));

    char client_mac[MAC_ADDR_SIZE] = {0};
    int supported_rates[16]        = {0}; // example: supported_rates=8c 12 98 24 b0 48 60 6c
    int HT_MCS[16]                 = {0};
    int16_t VHT_MCS[1]             = {0};
    char ht_cap[8]                 = {0};
    char ht_mcs[64]                = {0};
    char vht_cap[16]               = {0};
    char vht_mcs[24]               = {0};
    int32_t conn_time              = 0;
    size_t numOfValidArgs[8]       = {0};

    FieldsToParse fieldsToParse[] = {
        {(void *)client_mac, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, sizeof(client_mac)},
        {(void *)supported_rates, &numOfValidArgs[1], DWPAL_INT_HEX_ARRAY_PARAM,
         "supported_rates=", sizeof(supported_rates)},
        {(void *)ht_cap, &numOfValidArgs[2], DWPAL_STR_PARAM, "ht_caps_info=", sizeof(ht_cap)},
        {(void *)ht_mcs, &numOfValidArgs[3], DWPAL_STR_PARAM, "ht_mcs_bitmask=", sizeof(ht_mcs)},
        {(void *)vht_cap, &numOfValidArgs[4], DWPAL_STR_PARAM, "vht_caps_info=", sizeof(vht_cap)},
        {(void *)vht_mcs, &numOfValidArgs[5], DWPAL_STR_PARAM, "rx_vht_mcs_map=", sizeof(vht_mcs)},
        {(void *)&msg->params.capabilities.max_tx_power, &numOfValidArgs[6], DWPAL_CHAR_PARAM,
         "max_txpower=", 0},
        {(void *)&conn_time, &numOfValidArgs[7], DWPAL_INT_PARAM, "connected_time=", 0},
        /* Must be at the end */
        {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

    if (dwpal_string_to_struct_parse((char *)event.c_str(), event.length(), fieldsToParse,
                                     sizeof(client_mac)) == DWPAL_FAILURE) {
        LOG(ERROR) << "DWPAL parse error ==> Abort";
        result = generate_association_event_result::FAILED_TO_PARSE_DWPAL;
        return nullptr;
    }

    // Clients may be authenticated but not associated.
    // Only associated clients will return the "connected_time" argument.
    // Do not trigger event for non-associated clients
    if (numOfValidArgs[7] == 0) {
        result = generate_association_event_result::SKIP_CLIENT_NOT_ASSOCIATED;
    }

    std::string ht_mcs_str("0x00");
    std::string vht_mcs_str("0x0000");

    // convert string to vector: ht_mcs "ffff0000000000000000" -> HT_MCS {0xFF, 0xFF, 0x00, ...}
    for (uint i = 0; (i < 8) && (i < sizeof(ht_mcs)); i++) {
        ht_mcs_str[2] = ht_mcs[2 * i];
        ht_mcs_str[3] = ht_mcs[2 * i + 1];
        HT_MCS[i]     = std::strtoul(ht_mcs_str.c_str(), nullptr, 16);
    }

    vht_mcs_str[2] = vht_mcs[0];
    vht_mcs_str[3] = vht_mcs[1];
    vht_mcs_str[4] = vht_mcs[2];
    vht_mcs_str[5] = vht_mcs[3];
    VHT_MCS[0]     = std::strtoul(vht_mcs_str.c_str(), nullptr, 16);

    LOG(DEBUG) << "client_mac      : " << client_mac;
    LOG(DEBUG) << "supported_rates : " << std::dec << supported_rates[0]
               << " (first  element only)";
    LOG(DEBUG) << "ht_mcs_bitmask  : 0x" << ht_mcs;
    LOG(DEBUG) << "vht_mcs         : 0x" << vht_mcs;
    LOG(DEBUG) << "ht_caps_info    : " << ht_cap;
    LOG(DEBUG) << "vht_cap         : " << vht_cap;
    LOG(DEBUG) << "max_txpower     : " << std::dec << (int)msg->params.capabilities.max_tx_power;
    LOG(DEBUG) << "connected_time  : " << std::dec << (int)conn_time;

    for (uint8_t i = 0; i < (sizeof(numOfValidArgs) / sizeof(size_t)); i++) {
        if (numOfValidArgs[i] == 0) {
            LOG(ERROR) << "Failed reading parsed parameter " << (int)i
                       << " ==> Continue with default values";
        }
    }

    msg->params.vap_id = vap_id;
    msg->params.mac    = tlvf::mac_from_string(client_mac);
    std::string ht_cap_str(ht_cap);
    get_ht_mcs_capabilities(HT_MCS, ht_cap_str, msg->params.capabilities);

    if (radio_5G) {
        std::string vht_cap_str(vht_cap);
        get_vht_mcs_capabilities(VHT_MCS, vht_cap_str, msg->params.capabilities);
    }

    get_mcs_from_supported_rates(supported_rates, msg->params.capabilities);

    son::wireless_utils::print_station_capabilities(msg->params.capabilities);

    // return the buffer
    return msg_buff;
}

///
/// @brief Finds value by key in a string vector representing hostapd config
///        (head or VAP) and returns value found in the supplied string object.
///
static bool hostapd_config_get_value(const std::vector<std::string> &hostapd_config,
                                     const std::string &key, std::string &value)
{
    std::string key_eq(key + "=");
    auto it_str = std::find_if(hostapd_config.begin(), hostapd_config.end(),
                               [&key_eq](std::string str) -> bool {
                                   return (str.compare(0, key_eq.length(), key_eq) == 0);
                               });
    if (it_str == hostapd_config.end()) {
        return false;
    }
    value.assign(*it_str, key_eq.length(), std::string::npos);
    return true;
}

///
/// @brief Finds a value by key in a string vector representing hostapd config
///        (head or VAP) and replaces it with the supplied one. If the value
///        does not exist, it's added. If the value is empty string it's deleted.
///
static void hostapd_config_set_value(std::vector<std::string> &hostapd_config,
                                     const std::string &key, const std::string &value)
{
    std::string key_eq(key + "=");
    auto it_str = std::find_if(hostapd_config.begin(), hostapd_config.end(),
                               [&key_eq](std::string str) -> bool {
                                   return (str.compare(0, key_eq.length(), key_eq) == 0);
                               });
    // Delete the key-value if found
    if (it_str != hostapd_config.end()) {
        it_str = hostapd_config.erase(it_str);
    }
    // If the new value is provided add the key back with that new value
    if (value.length() != 0) {
        hostapd_config.push_back(key_eq + value);
    }
    return;
}

static void hostapd_config_set_value(std::vector<std::string> &hostapd_config,
                                     const std::string &key, const int value)
{
    return hostapd_config_set_value(hostapd_config, key, std::to_string(value));
}

///
/// @brief Loads hostapd configuration from specified file
///
static bool
load_hostapd_config_file(const std::string &fname, std::vector<std::string> &hostapd_config_head,
                         std::map<std::string, std::vector<std::string>> &hostapd_config_vaps)
{
    std::ifstream ifs(fname);
    std::string line;

    bool parsing_vaps = false;
    std::string cur_vap;
    while (getline(ifs, line)) {
        // Skip empty lines
        if (beerocks::string_utils::is_empty(line)) {
            continue;
        }
        // Check if the string belongs to a VAP config part and
        // capture which one.
        const std::string bss_eq("bss=");
        if (line.compare(0, bss_eq.length(), bss_eq) == 0) {
            parsing_vaps = true;
            cur_vap.assign(line, bss_eq.length(), std::string::npos);
        }
        // If not a VAP line store it in the header part of the config,
        // otherwise add to the currently being parsed VAP storage.
        if (!parsing_vaps) {
            hostapd_config_head.push_back(line);
        } else {
            std::vector<std::string> &hostapd_config_vap = hostapd_config_vaps[cur_vap];
            hostapd_config_vap.push_back(line);
        }
    }

    // Log something if there was an error during the read
    if (ifs.bad()) {
        LOG(ERROR) << "Unable to read " << fname << ": " << strerror(errno);
    }

    // If we've got to parsing VAPs and no read errors, assume all is good
    return parsing_vaps && !ifs.bad();
}

///
/// @brief Figures out hostapd config file name by the interface name and loads
///        its content using load_hostapd_config_file()
///        Warning: the content of the hostapd_config_head and hostapd_config_vaps
///        is destroyed.
///
static bool
load_hostapd_config(const std::string &radio_iface_name, std::string &fname,
                    std::vector<std::string> &hostapd_config_head,
                    std::map<std::string, std::vector<std::string>> &hostapd_config_vaps)
{
    bool loaded                                = false;
    std::vector<std::string> hostapd_cfg_names = {
        "/var/run/hostapd-phy0.conf", "/var/run/hostapd-phy1.conf", "/var/run/hostapd-phy2.conf",
        "/var/run/hostapd-phy3.conf"};

    for (const auto &try_fname : hostapd_cfg_names) {
        LOG(DEBUG) << "Trying to load " << try_fname << "...";

        hostapd_config_head.clear();
        hostapd_config_vaps.clear();
        if (!beerocks::os_utils::file_exists(try_fname)) {
            continue;
        }
        if (!load_hostapd_config_file(try_fname, hostapd_config_head, hostapd_config_vaps)) {
            LOG(ERROR) << "Failed to load hostapd cofig file: " << try_fname;
            continue;
        }
        // See if it's the right one
        std::string ifname;
        if (!hostapd_config_get_value(hostapd_config_head, "interface", ifname)) {
            LOG(ERROR) << "No interface value in " << try_fname;
            continue;
        }
        if (ifname != radio_iface_name) {
            LOG(DEBUG) << "File " << try_fname << " interface " << radio_iface_name << " no match";
            continue;
        }
        LOG(DEBUG) << "Loaded " << try_fname << " for interface " << radio_iface_name;

        loaded = true;
        fname.assign(try_fname);
        break;
    }
    if (!loaded) {
        hostapd_config_head.clear();
        hostapd_config_vaps.clear();
    }

    return loaded;
}

///
/// @brief Saves hostapd configuration to specified file
///
static bool
save_hostapd_config_file(const std::string &fname, std::vector<std::string> &hostapd_config_head,
                         std::map<std::string, std::vector<std::string>> &hostapd_config_vaps)
{
    std::ofstream out(fname, std::ofstream::out | std::ofstream::trunc);

    for (const auto &line : hostapd_config_head) {
        out << line << "\n";
    }
    for (auto &it : hostapd_config_vaps) {
        out << "\n"; // add empty line for readability
        const std::vector<std::string> &hostapd_config_vap = it.second;
        for (auto &line : hostapd_config_vap) {
            out << line << "\n";
        }
    }

    // Log something if there was an error during the write
    out.close();
    if (out.fail()) {
        LOG(ERROR) << "Unable to write " << fname << ": " << strerror(errno);
    }

    return !out.fail();
}

static bool
update_vap_credentials_configure_wpa(const std::string &vap_if,
                                     std::vector<std::string> &vap_hostapd_config,
                                     const son::wireless_utils::sBssInfoConf &bss_info_conf)
{
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
    std::string wpa_key_mgmt(""); // Empty -> delete from hostapd config

    // (dot11RSNAConfigPairwiseCiphersTable)
    // Pairwise cipher for WPA (v1) (default: TKIP)
    //  wpa_pairwise=TKIP CCMP
    // Pairwise cipher for RSN/WPA2 (default: use wpa_pairwise value)
    //  rsn_pairwise=CCMP
    std::string wpa_pairwise(""); // Empty -> delete from hostapd config

    // WPA pre-shared keys for WPA-PSK. This can be either entered as a 256-bit
    // secret in hex format (64 hex digits), wpa_psk, or as an ASCII passphrase
    // (8..63 characters), wpa_passphrase.
    std::string wpa_passphrase("");
    std::string wpa_psk("");

    // ieee80211w: Whether management frame protection (MFP) is enabled
    // 0 = disabled (default)
    // 1 = optional
    // 2 = required
    std::string ieee80211w("");

    // This parameter can be used to disable caching of PMKSA created through EAP
    // authentication. RSN preauthentication may still end up using PMKSA caching if
    // it is enabled (rsn_preauth=1).
    // 0 = PMKSA caching enabled (default)
    // 1 = PMKSA caching disabled
    std::string disable_pmksa_caching("");

    // Opportunistic Key Caching (aka Proactive Key Caching)
    // Allow PMK cache to be shared opportunistically among configured interfaces
    // and BSSes (i.e., all configurations within a single hostapd process).
    // 0 = disabled (default)
    // 1 = enabled
    std::string okc("");

    // This parameter can be used to disable retransmission of EAPOL-Key frames that
    // are used to install keys (EAPOL-Key message 3/4 and group message 1/2). This
    // is similar to setting wpa_group_update_count=1 and
    std::string wpa_disable_eapol_key_retries("");

    // EasyMesh R1 only allows Open and WPA2 PSK auth&encryption methods.
    // Quote: A Multi-AP Controller shall set the Authentication Type attribute
    //        in M2 to indicate WPA2-Personal or Open System Authentication.
    // bss_info_conf.authentication_type is a bitfield, but we are not going
    // to accept any combinations due to the above limitation.
    if (bss_info_conf.authentication_type == WSC::eWscAuth::WSC_AUTH_OPEN) {
        wpa = 0x0;
        if (bss_info_conf.encryption_type != WSC::eWscEncr::WSC_ENCR_NONE) {
            LOG(ERROR) << "Autoconfiguration: " << vap_if << " encryption set on open VAP";
            return false;
        }
        if (bss_info_conf.network_key.length() > 0) {
            LOG(ERROR) << "Autoconfiguration: " << vap_if << " network key set for open VAP";
            return false;
        }
    } else if (bss_info_conf.authentication_type == WSC::eWscAuth::WSC_AUTH_WPA2PSK) {
        wpa = 0x2;
        wpa_key_mgmt.assign("WPA-PSK");
        // Cipher must include AES for WPA2, TKIP is optional
        if ((uint16_t(bss_info_conf.encryption_type) & uint16_t(WSC::eWscEncr::WSC_ENCR_AES)) ==
            0) {
            LOG(ERROR) << "Autoconfiguration:  " << vap_if << " CCMP(AES) is required for WPA2";
            return false;
        }
        if ((uint16_t(bss_info_conf.encryption_type) & uint16_t(WSC::eWscEncr::WSC_ENCR_TKIP)) !=
            0) {
            wpa_pairwise.assign("TKIP CCMP");
        } else {
            wpa_pairwise.assign("CCMP");
        }
        if (bss_info_conf.network_key.length() < 8 || bss_info_conf.network_key.length() > 64) {
            LOG(ERROR) << "Autoconfiguration: " << vap_if << " invalid network key length "
                       << bss_info_conf.network_key.length();
            return false;
        }
        if (bss_info_conf.network_key.length() < 64) {
            wpa_passphrase.assign(bss_info_conf.network_key);
        } else {
            wpa_psk.assign(bss_info_conf.network_key);
        }
        ieee80211w.assign("1");
        disable_pmksa_caching.assign("1");
        okc.assign("0");
        wpa_disable_eapol_key_retries.assign("0");
    } else if (bss_info_conf.authentication_type ==
               WSC::eWscAuth(WSC::eWscAuth::WSC_AUTH_WPA2PSK | WSC::eWscAuth::WSC_AUTH_SAE)) {
        wpa = 0x2;
        wpa_key_mgmt.assign("WPA-PSK SAE");

        if (bss_info_conf.encryption_type != WSC::eWscEncr::WSC_ENCR_AES) {
            LOG(ERROR) << "Autoconfiguration:  " << vap_if << " CCMP(AES) is required for WPA3";
            return false;
        }
        wpa_pairwise.assign("CCMP");

        if (bss_info_conf.network_key.length() < 8 || bss_info_conf.network_key.length() > 64) {
            LOG(ERROR) << "Autoconfiguration: " << vap_if << " invalid network key length "
                       << bss_info_conf.network_key.length();
            return false;
        }
        wpa_passphrase.assign(bss_info_conf.network_key);

        ieee80211w.assign("2");
        disable_pmksa_caching.assign("1");
        okc.assign("1");
        wpa_disable_eapol_key_retries.assign("0");
    } else if (bss_info_conf.authentication_type == WSC::eWscAuth::WSC_AUTH_SAE) {
        wpa = 0x2;
        wpa_key_mgmt.assign("SAE");

        if (bss_info_conf.encryption_type != WSC::eWscEncr::WSC_ENCR_AES) {
            LOG(ERROR) << "Autoconfiguration:  " << vap_if << " CCMP(AES) is required for WPA3";
            return false;
        }
        wpa_pairwise.assign("CCMP");

        if (bss_info_conf.network_key.length() < 8 || bss_info_conf.network_key.length() > 64) {
            LOG(ERROR) << "Autoconfiguration: " << vap_if << " invalid network key length "
                       << bss_info_conf.network_key.length();
            return false;
        }
        wpa_passphrase.assign(bss_info_conf.network_key);

        ieee80211w.assign("2");
        disable_pmksa_caching.assign("1");
        okc.assign("1");
        wpa_disable_eapol_key_retries.assign("0");
    } else {
        LOG(ERROR) << "Autoconfiguration: " << vap_if << " invalid authentication type "
                   << son::wireless_utils::wsc_to_bwl_authentication(
                          bss_info_conf.authentication_type);
        return false;
    }

    hostapd_config_set_value(vap_hostapd_config, "wpa", wpa);
    hostapd_config_set_value(vap_hostapd_config, "okc", okc);
    hostapd_config_set_value(vap_hostapd_config, "wpa_key_mgmt", wpa_key_mgmt);
    hostapd_config_set_value(vap_hostapd_config, "wpa_pairwise", wpa_pairwise);
    hostapd_config_set_value(vap_hostapd_config, "wpa_psk", wpa_psk);
    hostapd_config_set_value(vap_hostapd_config, "ieee80211w", ieee80211w);
    hostapd_config_set_value(vap_hostapd_config, "wpa_passphrase", wpa_passphrase);
    hostapd_config_set_value(vap_hostapd_config, "disable_pmksa_caching", disable_pmksa_caching);
    hostapd_config_set_value(vap_hostapd_config, "wpa_disable_eapol_key_retries",
                             wpa_disable_eapol_key_retries);

    return true;
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Implementation ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// NOTE: Since *base_wlan_hal_dwpal* inherits *base_wlan_hal* virtually, we
//       need to explicitly call it's from any deriving class
ap_wlan_hal_dwpal::ap_wlan_hal_dwpal(const std::string &iface_name, hal_event_cb_t callback,
                                     const hal_conf_t &hal_conf)
    : base_wlan_hal(bwl::HALType::AccessPoint, iface_name, IfaceType::Intel, callback, hal_conf),
      base_wlan_hal_dwpal(bwl::HALType::AccessPoint, iface_name, callback, hal_conf)
{
    std::string events[] = {"WPS-ENROLLEE-SEEN", "AP-PROBE-REQ-RECEIVED", "BEACON-REQ-TX-STATUS",
                            "CTRL-EVENT-BSS-ADDED", "CTRL-EVENT-BSS-REMOVED"};
    int events_size      = sizeof(events) / sizeof(std::string);
    m_filtered_events.insert(events, events + events_size);
}

ap_wlan_hal_dwpal::~ap_wlan_hal_dwpal() {}

HALState ap_wlan_hal_dwpal::attach(bool block)
{
    auto state = base_wlan_hal_dwpal::attach(block);

    // On Operational send the AP_Attached event to the AP Manager
    if (state == HALState::Operational) {
        event_queue_push(Event::AP_Attached);
    }

    return state;
}

bool ap_wlan_hal_dwpal::refresh_radio_info()
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
            std::string fname;
            std::vector<std::string> hostapd_config_head;
            std::map<std::string, std::vector<std::string>> hostapd_config_vaps;
            if (!load_hostapd_config(m_radio_info.iface_name, fname, hostapd_config_head,
                                     hostapd_config_vaps)) {
                LOG(ERROR) << "Unable to load hostapd config for interface "
                           << m_radio_info.iface_name;
                return false;
            }

            auto get_value = [](const std::vector<std::string> &lines, const std::string &key) {
                std::string value;

                for (const auto &line_ : lines) {
                    // Get a non-const copy of the line to be able to make changes on it
                    std::string line = line_;

                    // Remove spaces
                    beerocks::string_utils::trim(line);

                    // Ignore empty lines
                    if (line.empty()) {
                        continue;
                    }

                    // Ignore comments
                    if (line.at(0) == '#') {
                        continue;
                    }

                    // Remove comments to the right
                    auto pos = line.find("#");
                    if (pos != std::string::npos) {
                        line.erase(pos, line.size());
                        beerocks::string_utils::rtrim(line);
                    }

                    pos = line.find("=");

                    // Ignore if not a name=value
                    if (pos == std::string::npos) {
                        continue;
                    }

                    std::string name = line.substr(0, pos);
                    beerocks::string_utils::trim(name);
                    if (name == key) {
                        value = line.substr(pos + 1, line.size());
                        beerocks::string_utils::trim(value);
                        break;
                    }
                }

                return value;
            };

            // Compute frequency band out of parameter `hw_mode` in hostapd.conf
            auto hw_mode = get_value(hostapd_config_head, "hw_mode");

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
                // If rank does not exist, set it to -1. It will be set by "read_acs_report()".
                // This means it will update the rank only on the initial referesh_radio_info,
                // afterwards only the the acs report reading function will update the ranking.
                auto bw_rank_iter = channel_info.bw_info_list.find(bw);
                if (bw_rank_iter == channel_info.bw_info_list.end()) {
                    channel_info.bw_info_list[bw] = -1;
                }
            }
        }

        break;
    }

    return base_wlan_hal_dwpal::refresh_radio_info();
}

bool ap_wlan_hal_dwpal::enable()
{
    if (!dwpal_send_cmd("ENABLE")) {
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::disable()
{
    if (!dwpal_send_cmd("DISABLE")) {
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::set_start_disabled(bool enable, int vap_id)
{
    if (vap_id != beerocks::IFACE_RADIO_ID) {
        return set("start_disabled", std::to_string(enable), vap_id);
    }

    bool ret = true;

    for (auto &vap : m_radio_info.available_vaps) {
        if (!set("start_disabled", std::to_string(enable), vap.first)) {
            LOG(ERROR) << "Failed setting start_disabled on vap=" << vap.first;
            ret = false;
        }
    }

    return ret;
}

bool ap_wlan_hal_dwpal::set_wifi_bw(beerocks::eWiFiBandwidth bw)
{
    if (bw == beerocks::eWiFiBandwidth::BANDWIDTH_UNKNOWN) {
        LOG(INFO) << "unknown bandwidth, skip setting vht_oper_chwidth.";
        return true;
    }

    int wifi_bw = 0;
    // based on hostapd.conf @ https://w1.fi/cgit/hostap/plain/hostapd/hostapd.conf
    // # 0 = 20 or 40 MHz operating Channel width
    // # 1 = 80 MHz channel width
    // # 2 = 160 MHz channel width
    // 80+80 MHz channel width not currently supported by Mxl Wifi driver
    // #vht_oper_chwidth=1

    if (bw == beerocks::eWiFiBandwidth::BANDWIDTH_20 ||
        bw == beerocks::eWiFiBandwidth::BANDWIDTH_40) {
        wifi_bw = 0;
    } else if (bw == beerocks::eWiFiBandwidth::BANDWIDTH_80) {
        wifi_bw = 1;
    } else if (bw == beerocks::eWiFiBandwidth::BANDWIDTH_160) {
        wifi_bw = 2;
    } else if (bw == beerocks::eWiFiBandwidth::BANDWIDTH_80_80) {
        LOG(ERROR) << "80+80 Mhz channel width not currently supported by this platform.";
        return false;
    } else {
        LOG(ERROR) << "Unknown BW " << bw;
        return false;
    }

    // manual channel set in 80211.ax mode is not supported yet
    // set he_ parameters same as vht_ for now (PPM-1784)
    if (!set("he_oper_chwidth", std::to_string(wifi_bw))) {
        LOG(ERROR) << "Failed setting vht_oper_chwidth";
        return false;
    }
    if (!set("vht_oper_chwidth", std::to_string(wifi_bw))) {
        LOG(ERROR) << "Failed setting vht_oper_chwidth";
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::set_channel(int chan, beerocks::eWiFiBandwidth bw, int center_channel)
{
    if (chan < 0) {
        LOG(ERROR) << "Invalid input: channel(" << chan << ") < 0";
        return false;
    }

    std::string chan_string = (chan == 0) ? "acs_smart" : std::to_string(chan);

    LOG(DEBUG) << "Set channel to " << chan_string << ", bw " << bw << ", center channel "
               << center_channel;

    if (!set("channel", chan_string)) {
        LOG(ERROR) << "Failed setting channel";
        return false;
    }

    if (!set_wifi_bw(bw)) {
        LOG(ERROR) << "Failed setting bandwidth";
        return false;
    }

    // Until PPM-643 is fixed setting secondary_channel for 20Mhz should be enough.
    if (bw == beerocks::eWiFiBandwidth::BANDWIDTH_20) {
        if (!set("secondary_channel", "0")) {
            LOG(ERROR) << "Failed setting secondary channel offset 0";
            return false;
        }
    }

    if (center_channel > 0) {
        // manual channel set in 80211.ax mode is not supported yet
        // set he_ parameters same as vht_ for now (PPM-1784)
        if (!set("he_oper_centr_freq_seg0_idx", std::to_string(center_channel))) {
            LOG(ERROR) << "Failed setting vht center frequency " << center_channel;
            return false;
        }
        if (!set("vht_oper_centr_freq_seg0_idx", std::to_string(center_channel))) {
            LOG(ERROR) << "Failed setting vht_oper_centr_freq_seg0_idx";
            return false;
        }
    }

    return true;
}

bool ap_wlan_hal_dwpal::sta_allow(const std::string &mac, const std::string &bssid)
{
    // Check if the requested BSSID is part of this radio
    for (const auto &vap : m_radio_info.available_vaps) {
        if (vap.second.mac == bssid) {
            // Send the command
            std::string cmd = "STA_ALLOW " + mac;
            if (!dwpal_send_cmd(cmd, vap.first)) {
                LOG(ERROR) << "sta_allow() failed!";
                return false;
            }

            return true;
        }
    }

    // If the requested BSSID is not part of this radio, return silently
    return true;
}

bool ap_wlan_hal_dwpal::sta_deny(const std::string &mac, const std::string &bssid)
{
    // Check if the requested BSSID is part of this radio
    for (const auto &vap : m_radio_info.available_vaps) {
        if (vap.second.mac == bssid) {
            // Send the command
            std::string cmd = "DENY_MAC " + mac + " reject_sta=33";
            if (!dwpal_send_cmd(cmd, vap.first)) {
                LOG(ERROR) << "sta_allow() failed!";
                return false;
            }

            return true;
        }
    }

    // If the requested BSSID is not part of this radio, return silently
    return true;
}

bool ap_wlan_hal_dwpal::sta_disassoc(int8_t vap_id, const std::string &mac, uint32_t reason)
{
    // Build command string
    const std::string ifname =
        beerocks::utils::get_iface_string_from_iface_vap_ids(m_radio_info.iface_name, vap_id);
    std::string cmd = "DISASSOCIATE " + ifname + " " + mac;

    if (reason) {
        cmd += " reason=" + std::to_string(reason);
    }

    // Send command
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "sta_disassoc() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::sta_deauth(int8_t vap_id, const std::string &mac, uint32_t reason)
{
    // Build command string
    const std::string ifname =
        beerocks::utils::get_iface_string_from_iface_vap_ids(m_radio_info.iface_name, vap_id);
    std::string cmd = "DEAUTHENTICATE " + ifname + " " + mac;

    if (reason) {
        cmd += " reason=" + std::to_string(reason);
    }

    // Send command
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "sta_deauth() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::sta_bss_steer(int8_t vap_id, const std::string &mac,
                                      const std::string &bssid, int oper_class, int chan,
                                      int disassoc_timer_btt, int valid_int_btt, int reason)
{

    LOG(TRACE) << __func__ << " mac: " << mac << ", BSS: " << bssid
               << ", oper_class: " << oper_class << ", channel: " << chan
               << ", disassoc: " << disassoc_timer_btt << ", valid_int: " << valid_int_btt;

    // Build command string
    std::string cmd =
        // Set the STA MAC address
        "BSS_TM_REQ " +
        mac

        // Transition management parameters
        + " dialog_token=" + "0" + " pref=" + "1" + " abridged=" + "1";

    // Divide disassoc_timer by 100, because the hostapd expects it to be in beacon interval
    // which is 100ms.
    if (disassoc_timer_btt) {
        cmd += std::string() + " disassoc_imminent=" + "1" +
               " disassoc_timer=" + std::to_string(disassoc_timer_btt);
    }

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

    if (valid_int_btt) {
        cmd += " valid_int=" + std::to_string(valid_int_btt);
    }

    // Target BSSID
    cmd += std::string() + " neighbor=" + bssid + ",0," + std::to_string(oper_class) + "," +
           std::to_string(chan) + ",0,255";

    // Send command
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "sta_bss_steer() failed!";
        return false;
    }

    return true;
}

static bool set_vap_multiap_mode(std::vector<std::string> &vap_hostapd_config, bool fronthaul,
                                 bool backhaul, std::string &backhaul_wps_ssid,
                                 const std::string &backhaul_wps_passphrase, bool disallow_profile1,
                                 bool disallow_profile2)
{
    std::string bssid, ifname;
    if (!hostapd_config_get_value(vap_hostapd_config, "bssid", bssid)) {
        LOG(ERROR) << "Failed to get VAP BSSID";
        return false;
    }
    if (!hostapd_config_get_value(vap_hostapd_config, "bss", ifname)) {
        LOG(ERROR) << "Failed to get VAP ifname (bss)";
        return false;
    }

    LOG(DEBUG) << "Configuring VAP " << ifname << ": bssid=" << bssid << ", fronthaul=" << fronthaul
               << ", backhaul=" << backhaul;

    if (backhaul) {
        LOG(DEBUG) << "disallow_profile1=" << disallow_profile1
                   << ", disallow_profile2=" << disallow_profile2;
    }

    // Open source hostapd state that a "multi_ap" field must be set in this way:
    // "Enable Multi-AP functionality
    // 0 = disabled (default)
    // 1 = AP support backhaul BSS
    // 2 = AP support fronthaul BSS
    // 3 = AP supports both backhaul BSS and fronthaul BSS"
    uint8_t multi_ap_mode = 0;
    multi_ap_mode |= backhaul ? 0x01 : 0x00;
    multi_ap_mode |= fronthaul ? 0x02 : 0x00;
    hostapd_config_set_value(vap_hostapd_config, "multi_ap", std::to_string(multi_ap_mode));

    std::string mesh_mode;
    if (backhaul && fronthaul) {
        mesh_mode.assign("ext_hybrid");

        // On ext_hybrid mode, the hostapd will create additional virtual net devices for bAP.
        // The number of new virtual net decives is determined by "num_vrt_bkh_netdevs" parameter,
        // and need to be set to one less than "max_num_sta" if was set on bAP interface (see
        // explanation below), since the main interface is used for 3 address stations.
        hostapd_config_set_value(vap_hostapd_config, "num_vrt_bkh_netdevs", 2);

        // Clear old configuration.
        hostapd_config_set_value(vap_hostapd_config, "max_num_sta", "");

        // By setting the hairpin mode to 1, it is expected that only the real netdev that is
        // configured to 3addr mode, will use this configuration whereas the virtual netdevs will
        // have default system configuration (It does not happens though, but for now we can live
        // with it until it will be fixed in the driver).
        hostapd_config_set_value(vap_hostapd_config, "enable_hairpin", 1);
    } else {
        mesh_mode.assign(backhaul ? "bAP" : "fAP");
        // Setting max_num_sta to an bAP interface defines number of repeaters that can
        // connect to this VAP. What actually Maxlinear driver does (proprietary feature) is
        // to create more virtual vaps. So if wlan2.0 has max_num_sta=3 configuration
        // there will be 2 additional vaps created by the driver.
        // Since currently in prplwrt we are already using 6 vaps (4 fAP, 1 dummy, 1 STA)
        // and WAV654 (wifi card on AX3000) limits number of VAPS to 8 + 8 (8 in 2.4G and 8 in 5G)
        // we can set max_num_sta to 3.
        // note: this works only for 1 backhaul interface per radio.
        // Defining more that one backhaul interfaces requires:
        // 1. Counting number of interfaces in use.
        // 2. Knowing number of VAP supported on the platform.
        hostapd_config_set_value(vap_hostapd_config, "max_num_sta", backhaul ? "3" : "");

        // Clear old configuration.
        hostapd_config_set_value(vap_hostapd_config, "num_vrt_bkh_netdevs", "");
    }

    hostapd_config_set_value(vap_hostapd_config, "mesh_mode", mesh_mode);

    if (backhaul) {
        hostapd_config_set_value(vap_hostapd_config, "multi_ap_profile1_disallow",
                                 disallow_profile1 ? "1" : "0");
        hostapd_config_set_value(vap_hostapd_config, "multi_ap_profile2_disallow",
                                 disallow_profile2 ? "1" : "0");
        hostapd_config_set_value(vap_hostapd_config, "enable_hairpin", 0);

    } else { // fBSS configurations
        // Clear old configuration.
        hostapd_config_set_value(vap_hostapd_config, "multi_ap_profile1_disallow", "");
        hostapd_config_set_value(vap_hostapd_config, "multi_ap_profile2_disallow", "");
        hostapd_config_set_value(vap_hostapd_config, "enable_hairpin", 1);
    }

    // Configuration for fBSS that will be used for WPS.
    // "backhaul_wps_ssid" represent a bBSS credentials (if exist). The wps configurations should be
    // configured only on a single fBSS. Therefore, we clear given "backhaul_wps_ssid" so WPS
    // configuration will not be configured twice.
    if (fronthaul && !backhaul_wps_ssid.empty()) {
        // Oddly enough, multi_ap_backhaul_wpa_passphrase has to be quoted, while wpa_passphrase
        // does not.
        hostapd_config_set_value(vap_hostapd_config, "multi_ap_backhaul_ssid",
                                 "\"" + backhaul_wps_ssid + "\"");
        hostapd_config_set_value(vap_hostapd_config, "multi_ap_backhaul_wpa_passphrase",
                                 backhaul_wps_passphrase);

        hostapd_config_set_value(vap_hostapd_config, "wps_state", "2");
        hostapd_config_set_value(vap_hostapd_config, "eap_server", "1");
        hostapd_config_set_value(vap_hostapd_config, "wps_independent", "0");
        hostapd_config_set_value(vap_hostapd_config, "config_methods", "push_button");

        // This will make sure only the first fBSS is used to WPS connection.
        backhaul_wps_ssid.clear();
    } else {
        hostapd_config_set_value(vap_hostapd_config, "multi_ap_backhaul_ssid", "");
        hostapd_config_set_value(vap_hostapd_config, "multi_ap_backhaul_wpa_passphrase", "");
        hostapd_config_set_value(vap_hostapd_config, "wps_state", "");
        hostapd_config_set_value(vap_hostapd_config, "eap_server", "0");
        hostapd_config_set_value(vap_hostapd_config, "wps_independent", "1");
        hostapd_config_set_value(vap_hostapd_config, "config_methods", "");
    }

    return true;
} // namespace dwpal

bool ap_wlan_hal_dwpal::update_vap_credentials(
    std::list<son::wireless_utils::sBssInfoConf> &bss_info_conf_list,
    const std::string &backhaul_wps_ssid, const std::string &backhaul_wps_passphrase)
{
    // The bss_info_conf_list contains list of all VAPs and STAs that have to be created
    // on the radio. If the list is empty, there should be no VAPs left.
    // At the moment we are taking a shortcut to make this work for certification setup.
    // We are updating the hostapd .conf files with the info from the bss_info_conf_list
    // and sending RELOAD command to hostapd making it update the VAPs.
    // With that approach there are following limitations:
    // - all the VAPs are expected to be always present in the config, but have
    //   start_disabled=1 option if not active
    // - any configuration change causing the hostapd config files to be re-genrated
    //   and the hostapd processes restarted will override the purplMesh configuration

    // The hostapd config is stored here. The items order in the header and VAPs sections
    // is mostly preserved (with the exception of the itemes prplMesh modifies). This helps
    // to keep the configuration the same and easily see what prplMesh is changing.
    std::string fname;
    std::vector<std::string> hostapd_config_head;
    std::map<std::string, std::vector<std::string>> hostapd_config_vaps;

    // Load hostapd config for the radio
    if (!load_hostapd_config(m_radio_info.iface_name, fname, hostapd_config_head,
                             hostapd_config_vaps)) {
        LOG(ERROR) << "Autoconfiguration: no hostapd config to apply configuration!";
        return false;
    }

    // If a Multi-AP Agent receives an AP-Autoconfiguration WSC message containing one or
    // more M2, it shall validate each M2 (based on its 1905 AL MAC address) and configure
    // a BSS on the corresponding radio for each of the M2. If the Multi-AP Agent is currently
    // operating a BSS with operating parameters that do not completely match any of the M2 in
    // the received AP-Autoconfiguration WSC message, it shall tear down that BSS.

    // Find first VAP entry
    auto hostapd_vap_iterator = hostapd_config_vaps.begin();

    // Clear all VAPs from the available container, since we preset it with configuration.
    m_radio_info.available_vaps.clear();

    auto backhaul_wps_ssid_copy(backhaul_wps_ssid);

    // Go through the bss_info_conf_list and change the hostapd config accordingly
    for (const auto &bss_info_conf : bss_info_conf_list) {
        auto auth_type =
            son::wireless_utils::wsc_to_bwl_authentication(bss_info_conf.authentication_type);
        if (auth_type == "INVALID") {
            LOG(ERROR) << "Autoconfiguration: invalid auth_type "
                       << int(bss_info_conf.authentication_type);
            return false;
        }
        auto enc_type = son::wireless_utils::wsc_to_bwl_encryption(bss_info_conf.encryption_type);
        if (enc_type == "INVALID") {
            LOG(ERROR) << "Autoconfiguration: invalid enc_type "
                       << int(bss_info_conf.encryption_type);
            return false;
        }

        LOG(DEBUG) << "Autoconfiguration for ssid: " << bss_info_conf.ssid
                   << " auth_type: " << auth_type << " encr_type: " << enc_type
                   << " network_key: " << bss_info_conf.network_key
                   << " fronthaul: " << bss_info_conf.fronthaul
                   << " backhaul: " << bss_info_conf.backhaul;

        // We only can use AP entries, skip STAs if ended up on one
        std::string entry_mode;
        while (hostapd_vap_iterator != hostapd_config_vaps.end() &&
               hostapd_config_get_value(hostapd_vap_iterator->second, "mode", entry_mode) &&
               entry_mode != "ap") {
            LOG(DEBUG) << "Autoconfiguration: skipping STA entry";
            ++hostapd_vap_iterator;
        }

        // Make sure we still have VAPs available to configure
        if (hostapd_vap_iterator == hostapd_config_vaps.end()) {
            LOG(ERROR) << "Autoconfiguration: not enough VAPs";
            return false;
        }

        // Update VAP settings in hostapd config
        const std::string &vap_if                    = hostapd_vap_iterator->first;
        std::vector<std::string> &vap_hostapd_config = hostapd_vap_iterator->second;

        // SSID
        hostapd_config_set_value(vap_hostapd_config, "ssid", bss_info_conf.ssid);

        // Everything related to switching between open and WPA2
        if (!update_vap_credentials_configure_wpa(vap_if, vap_hostapd_config, bss_info_conf)) {
            // The function prints the error messages itself
            return false;
        }

        // Set multi_ap mode
        if (!set_vap_multiap_mode(vap_hostapd_config, bss_info_conf.fronthaul,
                                  bss_info_conf.backhaul, backhaul_wps_ssid_copy,
                                  backhaul_wps_passphrase,
                                  bss_info_conf.profile1_backhaul_sta_association_disallowed,
                                  bss_info_conf.profile2_backhaul_sta_association_disallowed)) {
            // The function prints the error messages itself
            return false;
        }

        VAPElement vap_info;
        vap_info.bss       = vap_if;
        vap_info.fronthaul = bss_info_conf.fronthaul;
        vap_info.backhaul  = bss_info_conf.backhaul;
        if (vap_info.backhaul) {
            vap_info.ssid = backhaul_wps_ssid;

            vap_info.profile1_backhaul_sta_association_disallowed =
                bss_info_conf.profile1_backhaul_sta_association_disallowed;
            vap_info.profile2_backhaul_sta_association_disallowed =
                bss_info_conf.profile2_backhaul_sta_association_disallowed;
        } else {
            vap_info.ssid = bss_info_conf.ssid;

            vap_info.profile1_backhaul_sta_association_disallowed = 0;
            vap_info.profile2_backhaul_sta_association_disallowed = 0;
        }

        // Save the configured VAP into available VAPS list.
        uint8_t vap_id = beerocks::utils::get_ids_from_iface_string(vap_if).vap_id;

        m_radio_info.available_vaps[vap_id] = vap_info;

        // Finally enable the VAP (remove any previously set start_disabled)
        hostapd_config_set_value(vap_hostapd_config, "start_disabled", "");
        // Successfully updated the VAP, move on to the the next
        ++hostapd_vap_iterator;
    }

    // Disable the remaining VAPs
    for (; hostapd_vap_iterator != hostapd_config_vaps.end(); ++hostapd_vap_iterator) {
        std::vector<std::string> &vap_hostapd_config = hostapd_vap_iterator->second;
        // Skip STAs
        std::string entry_mode;
        if (hostapd_config_get_value(vap_hostapd_config, "mode", entry_mode) &&
            entry_mode != "ap") {
            continue;
        }
        // SSID
        hostapd_config_set_value(vap_hostapd_config, "ssid", std::string());
        // Disable the VAP
        hostapd_config_set_value(vap_hostapd_config, "start_disabled", "1");
        // Disable backhaul BSS
        hostapd_config_set_value(vap_hostapd_config, "multi_ap", "0");
        hostapd_config_set_value(vap_hostapd_config, "multi_ap_backhaul_ssid", "");
        hostapd_config_set_value(vap_hostapd_config, "multi_ap_backhaul_wpa_passphrase", "");
    }

    // If we are still here, then autoconfiguration was successful,
    // update the hostapd configuration files and send RELOAD command.
    if (!save_hostapd_config_file(fname, hostapd_config_head, hostapd_config_vaps)) {
        LOG(ERROR) << "Autoconfiguration: cannot save hostapd config!";
        return false;
    }

    // hostapd help says:
    // RECONF [BSS name] = reconfigure interface (add/remove BSS's while other BSS are unaffected)
    // if BSS name is given, that BSS will be reloaded (main BSS isn't supported)
    // In reality (as of now, Jan 2020) only RECONF with explicitly specified interface name
    // or sending SIGHUP work for making hostapd to reload the configuration.
    size_t vap_ok_count    = 0;
    size_t vap_total_count = 0;
    for (const auto &it : hostapd_config_vaps) {
        // Skip STAs
        std::string entry_mode;
        if (hostapd_config_get_value(it.second, "mode", entry_mode) && entry_mode != "ap") {
            continue;
        }
        // Count the the total of available for reconfiguration VAPs
        ++vap_total_count;
        // Send the command
        std::string cmd("RECONF " + it.first);
        if (!dwpal_send_cmd(cmd)) {
            LOG(ERROR) << "Autoconfiguration: \"" << cmd << "\" command to hostapd has failed!";
            // Keep going and try to complete what we can
            continue;
        }
        ++vap_ok_count;
    }

    LOG(INFO) << "Autoconfiguration: completed successfully for " << vap_ok_count << " out of "
              << vap_total_count << " available hostapd VAP sections";
    return (vap_ok_count == vap_total_count);
}

bool ap_wlan_hal_dwpal::sta_unassoc_rssi_measurement(const std::string &mac, int chan, int bw,
                                                     int vht_center_frequency, int delay,
                                                     int window_size)
{
    // Convert values to strings
    std::string chanBandwidth     = std::to_string(bw);
    std::string centerFreq        = std::to_string(son::wireless_utils::channel_to_freq(chan));
    std::string waveVhtCenterFreq = std::to_string(vht_center_frequency);

    // Build command string
    std::string cmd = "UNCONNECTED_STA_RSSI " + mac + " " + centerFreq +
                      " center_freq1=" + waveVhtCenterFreq + " bandwidth=" + chanBandwidth;

    // Delay the first measurement...
    UTILS_SLEEP_MSEC(delay);

    m_unassoc_measure_start       = std::chrono::steady_clock::now();
    m_unassoc_measure_delay       = delay;
    m_unassoc_measure_window_size = window_size;

    // Trigger a measurement
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "sta_unassoc_rssi_measurement() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::sta_softblock_add(const std::string &vap_name,
                                          const std::string &client_mac, uint8_t reject_error_code,
                                          uint8_t probe_snr_threshold_hi,
                                          uint8_t probe_snr_threshold_lo,
                                          uint8_t authetication_snr_threshold_hi,
                                          uint8_t authetication_snr_threshold_lo)
{
    // Build command string
    std::string cmd = "STA_SOFTBLOCK " + vap_name + " " + client_mac + " remove=0" +
                      " reject_sta=" + std::to_string(reject_error_code) +
                      " snrProbeHWM=" + std::to_string(probe_snr_threshold_hi) +
                      " snrProbeLWM=" + std::to_string(probe_snr_threshold_lo) +
                      " snrAuthHWM=" + std::to_string(authetication_snr_threshold_hi) +
                      " snrAuthLWM=" + std::to_string(authetication_snr_threshold_lo);
    LOG(INFO) << cmd;
    // Trigger a measurement
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "sta_softblock_add() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::sta_softblock_remove(const std::string &vap_name,
                                             const std::string &client_mac)
{
    // Build command string
    std::string cmd = "STA_SOFTBLOCK " + vap_name + " " + client_mac + " remove=1";
    LOG(INFO) << cmd;
    // Trigger a measurement
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "sta_softblock_remove() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::switch_channel(int chan, int bw, int vht_center_frequency)
{
    std::string cmd = "CHAN_SWITCH 5 ";
    // ACS
    if (chan == 0) {
        m_drop_csa = true;
        cmd += "0"; // CenterFrequency
                    // Specific Channel
    } else {
        m_drop_csa = false;

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
        if ((bw == beerocks::BANDWIDTH_80) || (bw == beerocks::BANDWIDTH_160)) {
            cmd += " center_freq1=" + wave_vht_center_frequency;
        }

        cmd += " bandwidth=" + std::to_string(beerocks::utils::convert_bandwidth_to_int(
                                   static_cast<beerocks::eWiFiBandwidth>(bw)));

        // Supported Standard n/ac
        if (bw == beerocks::BANDWIDTH_20 || bw == beerocks::BANDWIDTH_40) {
            cmd += " ht"; //n
        } else if ((bw == beerocks::BANDWIDTH_80) || (bw == beerocks::BANDWIDTH_160)) {
            cmd += " vht"; // ac
        }
    }

    // Send command
    LOG(DEBUG) << "switch channel command: " << cmd;
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "switch_channel() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::cancel_cac(int chan, beerocks::eWiFiBandwidth bw, int vht_center_frequency,
                                   int secondary_chan_offset)
{
    // The following hostapd sequence disables cac and re-enables the radio with the given
    // parameters:
    // disable (cac canceled)
    // SET channel X
    // SET secondary_channel X
    // SET vht_oper_chwidth X
    // SET vht_oper_center_freq_seg0_idx X
    // enable (radio enabled back on channel X)

    // get center channel from the center frequency
    auto center_channel = son::wireless_utils::freq_to_channel(vht_center_frequency);

    LOG(DEBUG) << "Canceling cac with the following parameters:\n"
               << "channel: " << chan << " bandwidth (eWiFiBandwidth): " << bw
               << " vht center frequency (input but not used directly): " << vht_center_frequency
               << " center channel (computed from vht_center_frequency): " << center_channel
               << " secondary_chan_offset: " << secondary_chan_offset;

    if (!disable()) {
        LOG(ERROR) << "Failed disabling radio";
        return false;
    }

    if (!set("channel", std::to_string(chan))) {
        LOG(ERROR) << "Failed setting channel " << chan;
        return false;
    }

    if (!set_wifi_bw(bw)) {
        LOG(ERROR) << "Failed setting bandwidth " << bw;
        return false;
    }

    // manual channel set in 80211.ax mode is not supported yet
    // set he_ parameters same as vht_ for now (PPM-1784)
    if (!set("he_oper_centr_freq_seg0_idx", std::to_string(center_channel))) {
        LOG(ERROR) << "Failed setting vht center frequency " << center_channel;
        return false;
    }
    if (!set("vht_oper_centr_freq_seg0_idx", std::to_string(center_channel))) {
        LOG(ERROR) << "Failed setting vht center frequency " << center_channel;
        return false;
    }

    if (!set("secondary_channel", std::to_string(secondary_chan_offset))) {
        LOG(ERROR) << "Failed setting secondary channel offset " << secondary_chan_offset;
        return false;
    }

    if (!enable()) {
        LOG(ERROR) << "Failed enabling radio";
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::failsafe_channel_set(int chan, int bw, int vht_center_frequency)
{
    // Channel number of the new channel or ‘0’ to trigger low level channel selection algorithm.
    // '0' triggers the same behavior as if the failsafe channel is NOT set.
    std::string cmd = "SET_FAILSAFE_CHAN ";

    // Build command string
    if (chan) {
        std::string bw_str   = std::to_string(bw);
        std::string chan_str = std::to_string(son::wireless_utils::channel_to_freq(chan));
        std::string freq_str = std::to_string(vht_center_frequency);
        LOG(DEBUG) << "chan_str = " << chan_str << " bw_str = " << bw_str
                   << " vht_freq_str = " << freq_str;

        cmd += chan_str + " center_freq1=" + freq_str + " bandwidth=" + bw_str;
    } else {
        cmd += "0";
    }

    // Send command
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "failsafe_channel_set() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::failsafe_channel_get(int &chan, int &bw)
{
    char *reply = nullptr;
    // Build command string
    std::string cmd = "GET_FAILSAFE_CHAN";

    // Send command
    if (!dwpal_send_cmd(cmd, &reply)) {
        LOG(ERROR) << "failsafe_channel_get() failed!";
        return false;
    }

    size_t numOfValidArgs[2] = {0}, replyLen = strnlen(reply, HOSTAPD_TO_DWPAL_MSG_LENGTH);
    char freq[8]{0};
    FieldsToParse fieldsToParse[] = {
        {(void *)freq, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, sizeof(freq)},
        {(void *)&bw, &numOfValidArgs[1], DWPAL_INT_PARAM, "bandwidth=", 0},
        /* Must be at the end */
        {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

    if (dwpal_string_to_struct_parse(reply, replyLen, fieldsToParse, sizeof(freq)) ==
        DWPAL_FAILURE) {
        LOG(ERROR) << "DWPAL parse error ==> Abort";
        return false;
    }

    /* TEMP: Traces... */
    LOG(DEBUG) << "GET_FAILSAFE_CHAN reply= " << reply;
    LOG(DEBUG) << "numOfValidArgs[0]= " << numOfValidArgs[0] << " freq= " << freq;
    LOG(DEBUG) << "numOfValidArgs[1]= " << numOfValidArgs[1] << " bandwidth= " << bw;
    /* End of TEMP: Traces... */

    for (uint8_t i = 0; i < (sizeof(numOfValidArgs) / sizeof(size_t)); i++) {
        if (numOfValidArgs[i] == 0) {
            LOG(ERROR) << "Failed reading parsed parameter " << (int)i << " ==> Abort";
            return false;
        }
    }

    // Custom reply parsing
    if (!strncmp(freq, "UNSPECIFIED", 11)) {
        chan = -1;
        bw   = -1;
    } else if (!strncmp(freq, "ACS", 3)) {
        chan = bw = 0;
    } else {
        chan = son::wireless_utils::freq_to_channel(beerocks::string_utils::stoi(freq));
    }

    return true;
}

static int get_zwdfs_supported_from_wiphy_dump_cb(struct nl_msg *msg, void *arg)
{
    if (!msg) {
        LOG(ERROR) << "Invalid input! msg == NULL";
        return DWPAL_FAILURE;
    }

    if (!arg) {
        LOG(ERROR) << "Invalid input! arg == NULL";
        return DWPAL_FAILURE;
    }

    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = static_cast<genlmsghdr *>(nlmsg_data(nlmsg_hdr(msg)));
    bool *zwdfs_supported   = static_cast<bool *>(arg);

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    if (tb_msg[NL80211_ATTR_WIPHY_DFS_ANTENNA]) {
        LOG(DEBUG) << "zwdfs interface found";
        *zwdfs_supported = true;
    }

    return DWPAL_SUCCESS;
}

bool ap_wlan_hal_dwpal::is_zwdfs_supported()
{
    bool supported = false;
    if (!dwpal_nl_cmd_send_and_recv(NL80211_CMD_GET_WIPHY, get_zwdfs_supported_from_wiphy_dump_cb,
                                    &supported)) {
        LOG(ERROR) << "Failed to check if zwdfs supported by reading NL wiphy info, default value "
                      "is set to 'false'";
        return false;
    }

    LOG(DEBUG) << "ZWDFS is" << ((supported) ? "" : " not") << " supported";

    return supported;
}

bool ap_wlan_hal_dwpal::set_zwdfs_antenna(bool enable)
{
    // Build command string
    char *reply     = nullptr;
    std::string cmd = "ZWDFS_ANT_SWITCH ";

    cmd += std::to_string(enable ? 1 : 0);

    LOG(DEBUG) << "Switch antenna command: " << cmd;

    // Send command
    if (!dwpal_send_cmd(cmd, &reply)) {
        LOG(ERROR) << "set_zwdfs_antenna() failed!";
        return false;
    }

    LOG(DEBUG) << "Switch antenna reply: " << reply;

    return true;
}

bool ap_wlan_hal_dwpal::is_zwdfs_antenna_enabled()
{
    char *reply = nullptr;
    // Build command string
    std::string cmd = "GET_ZWDFS_ANTENNA";

    // Send command
    if (!dwpal_send_cmd(cmd, &reply)) {
        LOG(ERROR) << "get_zwdfs_antenna() failed!";
        return false;
    }

    std::string reply_str(reply);
    beerocks::string_utils::trim(reply_str);
    LOG(DEBUG) << "GET_ZWDFS_ANTENNA returned '" << reply_str << "'";

    return reply_str == "1";
}

bool ap_wlan_hal_dwpal::hybrid_mode_supported() { return true; }

bool ap_wlan_hal_dwpal::restricted_channels_set(char *channel_list)
{
    // For example, the channel_list_str: “1 6 11 12 13”
    // *** WARNING: It is possible to set restricted channel only from the supported channels list!
    // *** setting channel that is not in the list, will cause this function to fail!
    std::stringstream channel_list_str;
    for (int i = 0; i < beerocks::message::RESTRICTED_CHANNEL_LENGTH; i++) {
        // Break after the last channel
        if (channel_list[i] == 0)
            break;

        // Convert the byte-long channels into unsigned integers
        if (i > 0)
            channel_list_str << " ";
        channel_list_str << int(uint8_t(channel_list[i]));
    }

    auto temp_channel_list = channel_list_str.rdbuf()->str();
    LOG(DEBUG) << "temp_channel_list = " << temp_channel_list;

    std::string cmd = "RESTRICTED_CHANNELS " + temp_channel_list;

    // Send command
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "restricted_channels_set() failed!";
        return false;
    }

    return true;
}

bool ap_wlan_hal_dwpal::restricted_channels_get(char *channel_list)
{
    char *reply = nullptr;

    std::string cmd = "GET_RESTRICTED_CHANNELS";

    // Send command
    if (!dwpal_send_cmd(cmd, &reply)) {
        LOG(ERROR) << "restricted_channels_get() failed!";
        return false;
    }

    /* TEMP: Traces... */
    LOG(DEBUG) << "GET_RESTRICTED_CHANNELS reply= " << reply;
    /* End of TEMP: Traces... */

    memset(channel_list, 0, beerocks::message::RESTRICTED_CHANNEL_LENGTH);

    // Convert the string to an array of bytes (int values)
    std::stringstream channel_list_stream(reply);
    int idx = 0;
    while (true) {
        int n;
        channel_list_stream >> n;
        channel_list[idx] = n;

        // Break on end-of-stream
        if (!channel_list_stream)
            break;

        idx++;
    }

    return true;
}

bool ap_wlan_hal_dwpal::read_acs_report()
{
    LOG(TRACE) << __func__ << " for interface: " << get_radio_info().iface_name;

    /**
     * Returned dump:
     * Ch=36 BW=20 DFS=0 pow=47 NF=-128 bss=2 pri=2 load=0 rank=28750
     * Ch=36 BW=40 DFS=0 pow=47 NF=-128 bss=2 pri=2 load=0 rank=28750
     * Ch=36 BW=80 DFS=0 pow=47 NF=-128 bss=2 pri=2 load=0 rank=28750
     * Ch=36 BW=160 DFS=1 pow=47 NF=-128 bss=5 pri=2 load=0 rank=60000
     * Ch=40 BW=20 DFS=0 pow=47 NF=-128 bss=2 pri=2 load=0 rank=28750
     * ...
     */

    parsed_multiline_t reply;
    int64_t tmp_int;

    std::string cmd = "GET_ACS_REPORT_ALL_CH";

    // Send command
    if (!dwpal_send_cmd(cmd, reply)) {
        LOG(ERROR) << "read_acs_report() failed!";
        return false;
    }
    LOG(DEBUG) << "GET_ACS_REPORT_ALL_CH reply:";

    uint32_t ch_idx = 0;
    for (auto &line : reply) {

        std::ostringstream oss;
        // Channel
        if (!read_param("Ch", line, tmp_int)) {
            LOG(ERROR) << "Failed reading Channel parameter!";
            return false;
        }
        auto &channel_info = m_radio_info.channels_list[tmp_int];
        oss << "Ch=" << tmp_int;

        // BW
        if (!read_param("BW", line, tmp_int)) {
            LOG(ERROR) << "Failed reading BW parameter!";
            return false;
        }
        oss << ", BW=" << tmp_int;
        auto bw = beerocks::utils::convert_bandwidth_to_enum(tmp_int);

        // DFS
        if (!read_param("DFS", line, tmp_int)) {
            LOG(ERROR) << "Failed reading DFS parameter!";
            return false;
        }
        oss << ", DFS=" << tmp_int;

        // Rank - May not appear in older Hostapd
        if (read_param("rank", line, tmp_int)) {
            channel_info.bw_info_list[bw] = tmp_int;
            oss << ", rank=" << tmp_int;
        } else {
            channel_info.bw_info_list[bw] = -1;
        }

        LOG(DEBUG) << oss.str();

        // Skip reading parameters since it is not being used.

        ch_idx++;
    }

    // Initialize default values
    m_radio_info.is_5ghz = false;

    // Check if channel is 5GHz
    auto ch = m_radio_info.channels_list.begin()->first;
    if (son::wireless_utils::which_freq(ch) == beerocks::eFreqType::FREQ_5G) {
        m_radio_info.is_5ghz = true;
    }

    return true;
}

bool ap_wlan_hal_dwpal::set_tx_power_limit(int tx_pow_limit)
{
    return m_nl80211_client->set_tx_power_limit(m_radio_info.iface_name, tx_pow_limit);
}

bool ap_wlan_hal_dwpal::set_vap_enable(const std::string &iface_name, const bool enable)
{
    LOG(DEBUG) << "set_vap_enable(): missing function implementation";
    return true;
}

bool ap_wlan_hal_dwpal::get_vap_enable(const std::string &iface_name, bool &enable)
{
    LOG(DEBUG) << "get_vap_enable(): missing function implementation";
    return true;
}

bool ap_wlan_hal_dwpal::set_mbo_assoc_disallow(const std::string &bssid, bool enable)
{
    LOG(DEBUG) << "set_mbo_assoc_disallow " << enable << " for bssid " << bssid;

    std::string cmd = "MBO_BSS_ASSOC_DISALLOW " + bssid + " " + std::to_string(enable);

    // Send command
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "set_mbo_assoc_disallow() failed!";
        return false;
    }
    return true;
}

bool ap_wlan_hal_dwpal::set_radio_mbo_assoc_disallow(bool enable)
{
    for (const auto &vap : m_radio_info.available_vaps) {
        if (!set_mbo_assoc_disallow(vap.second.mac, enable)) {
            return false;
        }
    }
    return true;
}

bool ap_wlan_hal_dwpal::set_primary_vlan_id(uint16_t primary_vlan_id)
{
    LOG(DEBUG) << "set_primary_vlan_id " << primary_vlan_id;

    // Send command
    if (!set("multi_ap_primary_vlanid", std::to_string(primary_vlan_id))) {
        LOG(ERROR) << "set_primary_vlan_id() failed!";
        return false;
    }
    return true;
}

bool ap_wlan_hal_dwpal::generate_connected_clients_events(
    bool &is_finished_all_clients, std::chrono::steady_clock::time_point max_iteration_timeout)
{
    std::string cmd;

    // Get the next vap from available vaps map that was not handled yet (not in the completed vaps set).
    auto get_next_unhandled_vap =
        [this](const std::unordered_map<int, bwl::VAPElement> &available_vaps) {
            auto next_unhandled_vap_it =
                std::find_if(available_vaps.begin(), available_vaps.end(),
                             [this](const std::pair<int, bwl::VAPElement> &element) {
                                 return (std::find(m_completed_vaps.begin(), m_completed_vaps.end(),
                                                   element.first) == m_completed_vaps.end());
                             });
            return ((next_unhandled_vap_it != available_vaps.end()) ? next_unhandled_vap_it->first
                                                                    : INVALID_VAP_ID);
        };

    // if vap not in progress, find next unhandled vap
    if (m_vap_id_in_progress == INVALID_VAP_ID) {
        m_vap_id_in_progress = get_next_unhandled_vap(m_radio_info.available_vaps);

        if (m_vap_id_in_progress == INVALID_VAP_ID) {
            LOG(DEBUG) << "Finished to generate connected clients events for all vaps";
            // if reached this point it means we finished quering all VAPs
            is_finished_all_clients = true;
            m_queried_first         = false;
            m_prev_client_mac       = beerocks::net::network_utils::ZERO_MAC;
            m_handled_clients.clear();

            return true;
        }
    }

    while (m_vap_id_in_progress != INVALID_VAP_ID) {
        char *reply;
        size_t replyLen;

        auto vap_iface_name = beerocks::utils::get_iface_string_from_iface_vap_ids(
            get_iface_name(), m_vap_id_in_progress);
        LOG(TRACE) << __func__ << " for vap interface: " << vap_iface_name;

        do {
            // if thread awake time is too long - return false (means there is more handling to be done on next wake-up)
            if (std::chrono::steady_clock::now() > max_iteration_timeout) {
                LOG(DEBUG)
                    << "Thread is awake too long - will continue on next wakeup, last handled sta:"
                    << m_prev_client_mac;
                is_finished_all_clients = false;
                return true;
            }

            if (m_queried_first) {
                cmd = "STA-NEXT " + vap_iface_name + " " + tlvf::mac_to_string(m_prev_client_mac);
            } else {
                cmd = "STA-FIRST " + vap_iface_name;
            }

            reply = nullptr;

            // Send command
            if (!dwpal_send_cmd(cmd, &reply)) {
                LOG(ERROR) << __func__ << ": cmd='" << cmd << "' failed!";

                is_finished_all_clients = false;
                // If failed and not on get-first-client then last processed client may have disconnected
                // we need to go over the vap from begining
                if (m_queried_first) {
                    m_queried_first   = false;
                    m_prev_client_mac = beerocks::net::network_utils::ZERO_MAC;
                    return true;
                }

                // Failure on the first client for that VAP is certainly an error
                return false;
            }

            m_queried_first = true;

            replyLen = strnlen(reply, HOSTAPD_TO_DWPAL_MSG_LENGTH);

            if (replyLen == 0) {
                LOG(DEBUG) << "cmd:" << cmd << " | reply:EMPTY\n"
                           << "Finished generating client association events for vap="
                           << vap_iface_name << ", vap_id=" << m_vap_id_in_progress;
                m_completed_vaps.insert(m_vap_id_in_progress);
                break;
            }

            LOG(DEBUG) << "cmd:" << cmd << " | replylen:" << (int)replyLen << " | reply:" << reply;

            int32_t result = generate_association_event_result::SUCCESS;
            auto msg_buff  = generate_client_assoc_event(reply, m_vap_id_in_progress,
                                                        get_radio_info().is_5ghz, result);

            if (!msg_buff) {
                LOG(DEBUG) << "Failed to generate client association event from reply";
                break;
            }

            // update client mac
            auto msg = reinterpret_cast<sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION *>(
                msg_buff.get());
            m_prev_client_mac = msg->params.mac;

            if (m_handled_clients.find(m_prev_client_mac) != m_handled_clients.end()) {
                // already generated event for this client
                continue;
            }

            m_handled_clients.insert(m_prev_client_mac);

            if (result == generate_association_event_result::SKIP_CLIENT_NOT_ASSOCIATED) {
                LOG(DEBUG) << "Client information is missing 'connected_time' field - client "
                              "is not associated. Not generating client-association-event";
                continue;
            }

            event_queue_push(Event::STA_Connected, msg_buff); // send message to the AP manager

        } while (replyLen > 0);

        m_queried_first   = false;
        m_prev_client_mac = beerocks::net::network_utils::ZERO_MAC;
        // the clients list is relevant as long as we are in the generation process (even if that client moved to another VAP)
        // set next vap to be handled
        m_vap_id_in_progress = get_next_unhandled_vap(m_radio_info.available_vaps);

        m_queried_first = false;
    }

    LOG(DEBUG) << "Finished to generate connected clients events for all vaps";
    // if reached this point it means we finished quering all VAPs
    m_vap_id_in_progress = INVALID_VAP_ID;
    m_prev_client_mac    = beerocks::net::network_utils::ZERO_MAC;
    m_handled_clients.clear();
    m_queried_first = false;

    is_finished_all_clients = true;

    return true;
}

bool ap_wlan_hal_dwpal::pre_generate_connected_clients_events()
{

    m_vap_id_in_progress = INVALID_VAP_ID;
    m_prev_client_mac    = beerocks::net::network_utils::ZERO_MAC;
    m_completed_vaps.clear();
    m_handled_clients.clear();
    m_queried_first = false;

    return true;
}

bool ap_wlan_hal_dwpal::start_wps_pbc()
{
    LOG(DEBUG) << "Start WPS PBC on interface " << m_radio_info.iface_name;
    std::string cmd = "WPS_PBC";
    if (!dwpal_send_cmd(cmd)) {
        LOG(ERROR) << "start_wps_pbc() failed!";
        return false;
    }
    return true;
}

static bool is_acs_completed_scan(char *buffer, int bufLen)
{
    size_t numOfValidArgs[3]      = {0};
    char scan[32]                 = {0};
    FieldsToParse fieldsToParse[] = {
        {NULL /* opCode */, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, 0},
        {NULL /* iface */, &numOfValidArgs[1], DWPAL_STR_PARAM, NULL, 0},
        {scan /* sub operation */, &numOfValidArgs[2], DWPAL_STR_PARAM, NULL, sizeof(scan)},
        /* Must be at the end */
        {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

    if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(scan)) == DWPAL_FAILURE)
        return false;

    return !strncmp(scan, "SCAN", 4);
}

bool ap_wlan_hal_dwpal::process_dwpal_event(char *buffer, int bufLen, const std::string &opcode)
{
    LOG(TRACE) << __func__ << " - opcode: |" << opcode << "|";

    // For key-value parser.
    int64_t tmp_int;
    const char *tmp_str;

    auto event = dwpal_to_bwl_event(opcode);

    // If there is monitored BSSs list, monitor all BSSs
    if (!m_hal_conf.monitored_BSSs.empty()) {
        if (event == Event::STA_Connected || event == Event::STA_Disconnected ||
            event == Event::AP_Disabled || event == Event::AP_Enabled) {

            std::string tmp_buffer(buffer, MAX_TEMP_BUFFER_SIZE);
            auto BSS_str_begin = tmp_buffer.find(BSS_IFNAME_PREFIX);
            if (BSS_str_begin == std::string::npos) {
                LOG(ERROR) << "No valid BSS information was found";
                return false;
            }
            auto BSS_str_end = tmp_buffer.find(" ", BSS_str_begin);
            if (BSS_str_end == std::string::npos) {
                LOG(ERROR) << "No valid BSS information was found";
                return false;
            }
            auto BSS_str   = std::string(tmp_buffer, BSS_str_begin, BSS_str_end - BSS_str_begin);
            auto iface_ids = beerocks::utils::get_ids_from_iface_string(BSS_str);
            if (iface_ids.vap_id == beerocks::IFACE_ID_INVALID) {
                LOG(DEBUG) << "Event received on invalid BSS ifname, should not process the event!";
                return true;
            }

            // Need to filter events that are received on unmonitored BSSIDs.
            // Check if the event's BSSID is present in the monitored BSSIDs list.
            // If the event's interface is RADIO, no need to filter
            if (iface_ids.vap_id == beerocks::IFACE_RADIO_ID &&
                m_hal_conf.monitored_BSSs.find(BSS_str) == m_hal_conf.monitored_BSSs.end()) {
                // Log print commented as to not flood the logs
                //LOG(DEBUG) << "Event received on BSS " << BSS_str << " that is not on monitored BSSs list, ignoring";
                return true;
            }
        }
    }

    switch (event) {
    case Event::ACS_Completed:
    case Event::CSA_Finished: {
        LOG(DEBUG) << buffer;

        if (event == Event::CSA_Finished) {
            if (std::chrono::steady_clock::now() <
                    (m_csa_event_filtering_timestamp +
                     std::chrono::milliseconds(CSA_EVENT_FILTERING_TIMEOUT_MS)) &&
                m_drop_csa) {
                LOG(DEBUG) << "CSA_Finished - dump event - arrive on csa filtering timeout window";
                return true;
            }
        } else {
            m_csa_event_filtering_timestamp = std::chrono::steady_clock::now();
            // ignore ACS_COMPLETED <iface> SCAN
            if (is_acs_completed_scan(buffer, bufLen)) {
                LOG(DEBUG) << "Received ACS_COMPLETED SCAN event, ignoring";
                return true;
            }
        }

        char reason[32]               = {0};
        char VAP[SSID_MAX_SIZE]       = {0};
        std::string channelStr        = (event == Event::CSA_Finished) ? "Channel=" : "channel=";
        size_t numOfValidArgs[8]      = {0};
        FieldsToParse fieldsToParse[] = {
            {NULL /*opCode*/, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, 0},
            {(void *)VAP, &numOfValidArgs[1], DWPAL_STR_PARAM, NULL, sizeof(VAP)},
            {(void *)&m_radio_info.channel, &numOfValidArgs[2], DWPAL_INT_PARAM, channelStr.c_str(),
             0},
            {(void *)&m_radio_info.bandwidth, &numOfValidArgs[3], DWPAL_INT_PARAM,
             "OperatingChannelBandwidt=", 0},
            {(void *)&m_radio_info.channel_ext_above, &numOfValidArgs[4], DWPAL_INT_PARAM,
             "ExtensionChannel=", 0},
            {(void *)&m_radio_info.vht_center_freq, &numOfValidArgs[5], DWPAL_INT_PARAM, "cf1=", 0},
            {(void *)&m_radio_info.is_dfs_channel, &numOfValidArgs[6], DWPAL_BOOL_PARAM,
             "dfs_chan=", 0},
            {(void *)&reason, &numOfValidArgs[7], DWPAL_STR_PARAM, "reason=", sizeof(reason)},
            /* Must be at the end */
            {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

        if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(m_radio_info)) ==
            DWPAL_FAILURE) {
            LOG(ERROR) << "DWPAL parse error ==> Abort";
            return false;
        }

        /* TEMP: Traces... */
        LOG(DEBUG) << "numOfValidArgs[1]= " << numOfValidArgs[1] << " " << VAP;
        LOG(DEBUG) << "numOfValidArgs[2]= " << numOfValidArgs[2] << " " << channelStr
                   << m_radio_info.channel;
        LOG(DEBUG) << "numOfValidArgs[3]= " << numOfValidArgs[3]
                   << " OperatingChannelBandwidt= " << (int)m_radio_info.bandwidth;
        LOG(DEBUG) << "numOfValidArgs[4]= " << numOfValidArgs[4]
                   << " ExtensionChannel= " << (int)m_radio_info.channel_ext_above;
        LOG(DEBUG) << "numOfValidArgs[5]= " << numOfValidArgs[5]
                   << " cf1= " << (int)m_radio_info.vht_center_freq;
        LOG(DEBUG) << "numOfValidArgs[6]= " << numOfValidArgs[6]
                   << " dfs_chan= " << (int)m_radio_info.is_dfs_channel;
        LOG(DEBUG) << "numOfValidArgs[7]= " << numOfValidArgs[7] << " reason= " << reason;
        /* End of TEMP: Traces... */

        for (uint8_t i = 0; i < (sizeof(numOfValidArgs) / sizeof(size_t)); i++) {
            if (numOfValidArgs[i] == 0) {
                LOG(ERROR) << "Failed reading parsed parameter " << (int)i << " ==> Abort";
                return false;
            }
        }

        if (beerocks::utils::get_ids_from_iface_string(VAP).vap_id != beerocks::IFACE_RADIO_ID) {
            LOG(INFO) << "Ignoring ACS/CSA events on non Radio interface";
            break;
        }

        // Channel switch reason
        ChanSwReason chanSwReason = ChanSwReason::Unknown;
        auto tmpStr               = std::string(reason);
        if (tmpStr.find("RADAR") != std::string::npos) {
            chanSwReason = ChanSwReason::Radar;
        } else if (tmpStr.find("20_COEXISTANCE") != std::string::npos) {
            chanSwReason = ChanSwReason::CoEx_20;
        } else if (tmpStr.find("40_COEXISTANCE") != std::string::npos) {
            chanSwReason = ChanSwReason::CoEx_40;
        }
        m_radio_info.last_csa_sw_reason = chanSwReason;

        // Update the radio information structure
        if (son::wireless_utils::which_freq(m_radio_info.channel) == beerocks::eFreqType::FREQ_5G) {
            m_radio_info.is_5ghz = true;
        }

        event_queue_push(event);

        // TODO: WHY?!
        if (event == Event::ACS_Completed) {
            LOG(DEBUG) << "ACS_COMPLETED >>> adding CSA_FINISHED event as well";
            event_queue_push(Event::CSA_Finished);
        }
        break;
    }

    case Event::STA_Connected: {

        // TODO: Change to HAL objects
        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION));
        auto msg =
            reinterpret_cast<sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION));

        char VAP[SSID_MAX_SIZE]                = {0};
        char MACAddress[MAC_ADDR_SIZE]         = {0};
        int supported_rates[16]                = {0};
        int RRM_CAP[8]                         = {0};
        int HT_MCS[16]                         = {0};
        int16_t VHT_MCS[16]                    = {0};
        char ht_cap[8]                         = {0};
        char vht_cap[16]                       = {0};
        size_t numOfValidArgs[22]              = {0};
        char assoc_req[ASSOCIATION_FRAME_SIZE] = {0};

        //force to unknown if not available
        uint8_t max_ch_width = NR_CHAN_WIDTH_UNKNOWN;

        FieldsToParse fieldsToParse[] = {
            {NULL /*opCode*/, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, 0},
            {(void *)VAP, &numOfValidArgs[1], DWPAL_STR_PARAM, NULL, sizeof(VAP)},
            {(void *)MACAddress, &numOfValidArgs[2], DWPAL_STR_PARAM, NULL, sizeof(MACAddress)},
            {(void *)supported_rates, &numOfValidArgs[3], DWPAL_INT_ARRAY_PARAM,
             "SupportedRates=", sizeof(supported_rates)},
            {(void *)RRM_CAP, &numOfValidArgs[4], DWPAL_INT_HEX_ARRAY_PARAM,
             "rrm_cap=", sizeof(RRM_CAP)},
            {(void *)HT_MCS, &numOfValidArgs[5], DWPAL_INT_HEX_ARRAY_PARAM,
             "HT_MCS=", sizeof(HT_MCS)},
            {(void *)VHT_MCS, &numOfValidArgs[6], DWPAL_INT_HEX_ARRAY_PARAM,
             "VHT_MCS=", sizeof(VHT_MCS)},
            {(void *)ht_cap, &numOfValidArgs[7], DWPAL_STR_PARAM, "HT_CAP=", sizeof(ht_cap)},
            {(void *)vht_cap, &numOfValidArgs[8], DWPAL_STR_PARAM, "VHT_CAP=", sizeof(vht_cap)},
            {(void *)assoc_req, &numOfValidArgs[9], DWPAL_STR_PARAM,
             "assoc_req=", sizeof(assoc_req)},
            {(void *)&msg->params.capabilities.btm_supported, &numOfValidArgs[10], DWPAL_CHAR_PARAM,
             "btm_supported=", 0},
            {(void *)&msg->params.capabilities.cell_capa, &numOfValidArgs[11], DWPAL_CHAR_PARAM,
             "cell_capa=", 0},
            {(void *)&msg->params.capabilities.band_2g_capable, &numOfValidArgs[12],
             DWPAL_CHAR_PARAM, "band_2g_capable=", 0},
            {(void *)&msg->params.capabilities.band_5g_capable, &numOfValidArgs[13],
             DWPAL_CHAR_PARAM, "band_5g_capable=", 0},
            {(void *)&msg->params.capabilities.rrm_supported, &numOfValidArgs[14], DWPAL_CHAR_PARAM,
             "rrm_supported=", 0},
            {(void *)&max_ch_width, &numOfValidArgs[15], DWPAL_CHAR_PARAM, "max_ch_width=", 0},
            {(void *)&msg->params.capabilities.max_streams, &numOfValidArgs[16], DWPAL_CHAR_PARAM,
             "max_streams=", 0},
            {(void *)&msg->params.capabilities.phy_mode, &numOfValidArgs[17], DWPAL_CHAR_PARAM,
             "phy_mode=", 0},
            {(void *)&msg->params.capabilities.max_mcs, &numOfValidArgs[18], DWPAL_CHAR_PARAM,
             "max_mcs=", 0},
            {(void *)&msg->params.capabilities.max_tx_power, &numOfValidArgs[19], DWPAL_CHAR_PARAM,
             "max_tx_power=", 0},
            {(void *)&msg->params.capabilities.mumimo_supported, &numOfValidArgs[20],
             DWPAL_CHAR_PARAM, "mu_mimo=", 0},
            {(void *)&msg->params.multi_ap_profile, &numOfValidArgs[21], DWPAL_INT_PARAM,
             "multi_ap_profile=", 0},
            /* Must be at the end */
            {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

        if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(VAP)) ==
            DWPAL_FAILURE) {
            LOG(ERROR) << "DWPAL parse error ==> Abort";
            return false;
        }
        msg->params.capabilities.max_ch_width = dwpal_ch_width_nr_to_beerocks_bw(max_ch_width);

        LOG(DEBUG) << "vap_id           : " << VAP;
        LOG(DEBUG) << "MACAddress       : " << MACAddress;
        LOG(DEBUG) << "supported_rates  : " << std::dec << supported_rates[0]
                   << " (first element only)";
        LOG(DEBUG) << "RRM_CAP          : 0x" << std::hex << RRM_CAP[0] << " (first element only)";
        LOG(DEBUG) << "HT_MCS           : 0x" << std::hex << HT_MCS[0] << " (first element only)";
        LOG(DEBUG) << "VHT_MCS          : 0x" << std::hex << VHT_MCS[0] << " (first element only)";
        LOG(DEBUG) << "HT_CAP           : " << ht_cap;
        LOG(DEBUG) << "VHT_CAP          : " << vht_cap;
        LOG(DEBUG) << "btm_supported    : " << (int)msg->params.capabilities.btm_supported;
        LOG(DEBUG) << "cell_capa        : " << (int)msg->params.capabilities.cell_capa;
        LOG(DEBUG) << "band_2g_capable  : " << (int)msg->params.capabilities.band_2g_capable;
        LOG(DEBUG) << "band_5g_capable  : " << (int)msg->params.capabilities.band_5g_capable;
        LOG(DEBUG) << "rrm_supported    : " << (int)msg->params.capabilities.rrm_supported;
        LOG(DEBUG) << "max_ch_width     : " << (int)msg->params.capabilities.max_ch_width;
        LOG(DEBUG) << "max_streams      : " << (int)msg->params.capabilities.max_streams;
        LOG(DEBUG) << "phy_mode         : " << (int)msg->params.capabilities.phy_mode;
        LOG(DEBUG) << "max_mcs          : " << (int)msg->params.capabilities.max_mcs;
        LOG(DEBUG) << "max_tx_power     : " << (int)msg->params.capabilities.max_tx_power;
        LOG(DEBUG) << "mumimo_supported : " << (int)msg->params.capabilities.mumimo_supported;
        LOG(DEBUG) << "multi_ap_profile : " << (int)msg->params.multi_ap_profile;
        LOG(DEBUG) << "assoc_req: " << assoc_req;

        for (uint8_t i = 0; i < (sizeof(numOfValidArgs) / sizeof(size_t)); i++) {
            if (numOfValidArgs[i] == 0) {
                // Skip validation on multi-ap profile since it is not mandatory.
                if (i == 21) {
                    // If the validation check fails, set the profile parameter to zero (not a Mult-AP
                    // Station), to make sure DWPAL did not put garbage there.
                    msg->params.multi_ap_profile = 0;
                    continue;
                }
                LOG(ERROR) << "Failed reading parsed parameter " << (int)i
                           << " ==> Continue with default values";
            }
        }

        msg->params.vap_id = beerocks::utils::get_ids_from_iface_string(VAP).vap_id;
        msg->params.mac    = tlvf::mac_from_string(MACAddress);

        //convert the hex string to binary
        auto binary_str                      = get_binary_association_frame(assoc_req);
        msg->params.association_frame_length = binary_str.length();

        std::copy_n(&binary_str[0], binary_str.length(), msg->params.association_frame);

        std::string ht_cap_str(ht_cap);
        get_ht_mcs_capabilities(HT_MCS, ht_cap_str, msg->params.capabilities);

        if (get_radio_info().is_5ghz) {
            std::string vht_cap_str(vht_cap);
            get_vht_mcs_capabilities(VHT_MCS, vht_cap_str, msg->params.capabilities);
        }

        get_mcs_from_supported_rates(supported_rates, msg->params.capabilities);

        parse_rrm_capabilities(RRM_CAP, msg->params.capabilities);

        son::wireless_utils::print_station_capabilities(msg->params.capabilities);

        // No need to store clients forever - may cause very big memory usage
        if (m_completed_vaps.find(msg->params.vap_id) != m_completed_vaps.end()) {
            // To prevent duplication of generation of connected event for clients,
            // need to add associated clients to the "handled_clients" set
            m_handled_clients.insert(msg->params.mac);
        }

        // Send the event to the AP manager
        event_queue_push(Event::STA_Connected, msg_buff);

        // Tunnel the association/re-association request to the controller
        if (assoc_req[0] != 0) {
            auto mgmt_frame = create_mgmt_frame_notification(assoc_req);
            if (mgmt_frame) {
                event_queue_push(Event::MGMT_Frame, mgmt_frame);
            }
        }

        break;
    }

    case Event::STA_Disconnected: {
        // TODO: Change to HAL objects
        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION));
        auto msg =
            reinterpret_cast<sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION));

        char VAP[SSID_MAX_SIZE]        = {0};
        char MACAddress[MAC_ADDR_SIZE] = {0};
        size_t numOfValidArgs[6]       = {0};
        FieldsToParse fieldsToParse[]  = {
            {NULL /*opCode*/, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, 0},
            {(void *)VAP, &numOfValidArgs[1], DWPAL_STR_PARAM, NULL, sizeof(VAP)},
            {(void *)MACAddress, &numOfValidArgs[2], DWPAL_STR_PARAM, NULL, sizeof(MACAddress)},
            {(void *)&msg->params.reason, &numOfValidArgs[3], DWPAL_CHAR_PARAM, "reason=", 0},
            {(void *)&msg->params.source, &numOfValidArgs[4], DWPAL_CHAR_PARAM, "source=", 0},
            {(void *)&msg->params.type, &numOfValidArgs[5], DWPAL_CHAR_PARAM, "type=", 0},
            /* Must be at the end */
            {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

        if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(VAP)) ==
            DWPAL_FAILURE) {
            LOG(ERROR) << "DWPAL parse error ==> Abort";
            return false;
        }

        LOG(DEBUG) << "vap_id     : " << VAP;
        LOG(DEBUG) << "MACAddress : " << MACAddress;
        LOG(DEBUG) << "reason     : " << int(msg->params.reason);
        LOG(DEBUG) << "source     : " << int(msg->params.source);
        LOG(DEBUG) << "type       : " << int(msg->params.type);

        for (uint8_t i = 0; i < (sizeof(numOfValidArgs) / sizeof(size_t)); i++) {
            if (numOfValidArgs[i] == 0) {
                LOG(ERROR) << "Failed reading parsed parameter " << (int)i << " ==> Abort";
                return false;
            }
        }

        msg->params.vap_id = beerocks::utils::get_ids_from_iface_string(VAP).vap_id;
        msg->params.mac    = tlvf::mac_from_string(MACAddress);

        event_queue_push(Event::STA_Disconnected, msg_buff); // send message to the AP manager
        break;
    }

    case Event::STA_Unassoc_RSSI: {
        // TODO: Change to HAL objects
        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE));
        auto msg = reinterpret_cast<sACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE *>(
            msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE));

        char MACAddress[MAC_ADDR_SIZE] = {0};
        char rssi[24]                  = {0};
        uint64_t rx_packets            = 0;
        size_t numOfValidArgs[5]       = {0};
        FieldsToParse fieldsToParse[]  = {
            {NULL /*opCode*/, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, 0},
            {NULL, &numOfValidArgs[1], DWPAL_STR_PARAM, NULL, 0},
            {(void *)MACAddress, &numOfValidArgs[2], DWPAL_STR_PARAM, NULL, sizeof(MACAddress)},
            {(void *)rssi, &numOfValidArgs[3], DWPAL_STR_PARAM, "rssi=", sizeof(rssi)},
            {(void *)&rx_packets, &numOfValidArgs[4], DWPAL_LONG_LONG_INT_PARAM, "rx_packets=", 0},
            /* Must be at the end */
            {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

        if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(MACAddress)) ==
            DWPAL_FAILURE) {
            LOG(ERROR) << "DWPAL parse error ==> Abort";
            return false;
        }

        /* TEMP: Traces... */
        LOG(DEBUG) << "numOfValidArgs[2]= " << numOfValidArgs[2] << " MACAddress= " << MACAddress;
        LOG(DEBUG) << "numOfValidArgs[3]= " << numOfValidArgs[3] << " rssi= " << rssi;
        LOG(DEBUG) << "numOfValidArgs[4]= " << numOfValidArgs[4] << " rx_packets= " << rx_packets;

        for (uint8_t i = 0; i < (sizeof(numOfValidArgs) / sizeof(size_t)); i++) {
            if (numOfValidArgs[i] == 0) {
                LOG(ERROR) << "Failed reading parsed parameter " << (int)i << " ==> Abort";
                return false;
            }
        }

        msg->params.rx_rssi    = beerocks::RSSI_INVALID;
        msg->params.rx_snr     = beerocks::SNR_INVALID;
        msg->params.result.mac = tlvf::mac_from_string(MACAddress);

        // Split the RSSI values
        auto rssiVec = beerocks::string_utils::str_split(rssi, ' ');
        auto snrVec  = beerocks::string_utils::str_split(rssi, ' ');

        // -128 -128 -128 -128
        int rssi_cnt    = 0;
        float rssi_watt = 0;
        float rssi_tmp  = 0;
        for (auto v : rssiVec) {
            rssi_tmp = float(beerocks::string_utils::stoi(v));
            if (rssi_tmp > beerocks::RSSI_MIN) {
                rssi_watt += pow(10, rssi_tmp / float(10));
                rssi_cnt++;
            }
        }

        msg->params.rx_packets = (rx_packets >= 127) ? 127 : rx_packets;

        // Measurement succeeded
        if ((rssi_cnt > 0) && (msg->params.rx_packets > 1)) {
            rssi_watt           = (rssi_watt / float(rssi_cnt));
            msg->params.rx_rssi = int(10 * log10(rssi_watt));
            // TODO: add snr measurements when implementing unconnected station measurement on rdkb
        } else { // Measurement failed
            auto now = std::chrono::steady_clock::now();
            auto delta =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - m_unassoc_measure_start)
                    .count();
            auto temp_sum_delay = (delta - m_unassoc_measure_delay);

            if (temp_sum_delay > m_unassoc_measure_window_size) {
                LOG(ERROR) << "event_obj->params.rx_packets = -2 , delay = " << int(temp_sum_delay);
                msg->params.rx_packets =
                    -2; // means the actual measurment started later then aspected
            }
        }

        // Measurement succeeded
        if ((rssi_cnt > 0) && (msg->params.rx_packets > 1)) {
            rssi_watt           = (rssi_watt / float(rssi_cnt));
            msg->params.rx_rssi = int(10 * log10(rssi_watt));
            // TODO: add snr measurements when implementing unconnected station measurement on rdkb
        } else { // Measurement failed
            auto now = std::chrono::steady_clock::now();
            auto delta =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - m_unassoc_measure_start)
                    .count();
            auto temp_sum_delay = (delta - m_unassoc_measure_delay);

            if (temp_sum_delay > m_unassoc_measure_window_size) {
                LOG(ERROR) << "event_obj->params.rx_packets = -2 , delay = " << int(temp_sum_delay);
                msg->params.rx_packets =
                    -2; // means the actual measurment started later then aspected
            }
        }

        // Add the message to the queue
        event_queue_push(Event::STA_Unassoc_RSSI, msg_buff);
        break;
    }

    case Event::STA_Softblock_Drop: {
        LOG(DEBUG) << buffer;

        char client_mac[MAC_ADDR_SIZE]                      = {0};
        char vap_bssid[MAC_ADDR_SIZE]                       = {0};
        char vap_name[beerocks::message::IFACE_NAME_LENGTH] = {0};
        s80211MgmtFrame::eType message_type;
        size_t numOfValidArgsForMsgType[2] = {0};

        FieldsToParse fieldsToParseForMsgType[] = {
            {NULL /*opCode*/, &numOfValidArgsForMsgType[0], DWPAL_STR_PARAM, NULL, 0},
            {(void *)&message_type, &numOfValidArgsForMsgType[1], DWPAL_UNSIGNED_CHAR_PARAM,
             "msg_type=", 0},
            /* Must be at the end */
            {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

        if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParseForMsgType,
                                         sizeof(message_type)) == DWPAL_FAILURE) {
            LOG(ERROR) << "DWPAL parse error ==> Abort";
            return false;
        }

        /* TEMP: Traces... */
        LOG(DEBUG) << "numOfValidArgs[1]=" << numOfValidArgsForMsgType[1]
                   << ", message_type=" << (uint8_t)message_type;

        if (message_type == s80211MgmtFrame::eType::PROBE_REQ) {

            auto msg_buff =
                ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION));
            auto msg = reinterpret_cast<sACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION *>(
                msg_buff.get());
            LOG_IF(!msg, FATAL) << "Memory allocation failed!";
            // Initialize the message
            memset(msg_buff.get(), 0,
                   sizeof(sACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION));

            size_t numOfValidArgs[6] = {0};

            FieldsToParse fieldsToParse[] = {
                {(void *)vap_name, &numOfValidArgs[0], DWPAL_STR_PARAM, "VAP=", sizeof(vap_name)},
                {(void *)vap_bssid, &numOfValidArgs[1], DWPAL_STR_PARAM,
                 "VAP_BSSID=", sizeof(vap_bssid)},
                {(void *)client_mac, &numOfValidArgs[2], DWPAL_STR_PARAM,
                 "addr=", sizeof(client_mac)},
                {
                    (void *)&msg->params.rx_snr,
                    &numOfValidArgs[3],
                    DWPAL_UNSIGNED_CHAR_PARAM,
                    "snr=",
                    0,
                },
                {
                    (void *)&msg->params.blocked,
                    &numOfValidArgs[4],
                    DWPAL_UNSIGNED_CHAR_PARAM,
                    "blocked=",
                    0,
                },
                {
                    (void *)&msg->params.broadcast,
                    &numOfValidArgs[5],
                    DWPAL_UNSIGNED_CHAR_PARAM,
                    "broadcast=",
                    0,
                },
                /* Must be at the end */
                {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

            if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(vap_name)) ==
                DWPAL_FAILURE) {
                LOG(ERROR) << "DWPAL parse error ==> Abort";
                return false;
            }

            for (uint8_t i = 0; i < (sizeof(numOfValidArgs) / sizeof(size_t)); i++) {
                if (numOfValidArgs[i] == 0) {
                    LOG(ERROR) << "Failed reading parsed parameter " << (int)i << " ==> Abort";
                    return false;
                }
            }

            LOG(DEBUG) << "numOfValidArgs[0]=" << numOfValidArgs[0] << ", VAP=" << vap_name;
            LOG(DEBUG) << "numOfValidArgs[1]=" << numOfValidArgs[1] << ", VAP_BSSID=" << vap_bssid;
            LOG(DEBUG) << "numOfValidArgs[2]=" << numOfValidArgs[2] << ", addr=" << client_mac;
            LOG(DEBUG) << "numOfValidArgs[3]=" << numOfValidArgs[3]
                       << ", rx_snr=" << (int)msg->params.rx_snr;
            LOG(DEBUG) << "numOfValidArgs[4]=" << numOfValidArgs[4]
                       << ", blocked=" << (int)msg->params.blocked;
            LOG(DEBUG) << "numOfValidArgs[5]=" << numOfValidArgs[5]
                       << ", broadcast=" << (int)msg->params.broadcast;

            // Check if the BSS is valid
            if (beerocks::utils::get_ids_from_iface_string(vap_name).vap_id ==
                beerocks::IFACE_ID_INVALID) {
                LOG(ERROR) << "Event received on invalid VAP ID, should handle the event!";
                return true;
            }
            // Check if the BSS is monitored
            if (!is_BSS_monitored(vap_name)) {
                LOG(DEBUG) << "Event received on non-monitored BSS, skipping!";
            }

            msg->params.client_mac = tlvf::mac_from_string(client_mac);
            //TODO need to add VAP name parsing to  this WLAN_FC_STYPE_PROBE_REQ event - WLANRTSYS-9170
            msg->params.bssid = tlvf::mac_from_string(vap_bssid);

            // Add the message to the queue
            event_queue_push(Event::STA_Steering_Probe_Req, msg_buff);

        } else if (message_type == s80211MgmtFrame::eType::AUTH) {

            auto msg_buff =
                ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION));
            auto msg = reinterpret_cast<sACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION *>(
                msg_buff.get());
            LOG_IF(!msg, FATAL) << "Memory allocation failed!";
            // Initialize the message
            memset(msg_buff.get(), 0,
                   sizeof(sACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION));

            size_t numOfValidArgs[7] = {0};

            FieldsToParse fieldsToParse[] = {
                {(void *)vap_name, &numOfValidArgs[0], DWPAL_STR_PARAM, "VAP=", sizeof(vap_name)},
                {(void *)vap_bssid, &numOfValidArgs[1], DWPAL_STR_PARAM,
                 "VAP_BSSID=", sizeof(vap_bssid)},
                {(void *)client_mac, &numOfValidArgs[2], DWPAL_STR_PARAM,
                 "addr=", sizeof(client_mac)},
                {
                    (void *)&msg->params.rx_snr,
                    &numOfValidArgs[3],
                    DWPAL_UNSIGNED_CHAR_PARAM,
                    "snr=",
                    0,
                },
                {
                    (void *)&msg->params.blocked,
                    &numOfValidArgs[4],
                    DWPAL_UNSIGNED_CHAR_PARAM,
                    "blocked=",
                    0,
                },
                {
                    (void *)&msg->params.reject,
                    &numOfValidArgs[5],
                    DWPAL_UNSIGNED_CHAR_PARAM,
                    "rejected=",
                    0,
                },
                {
                    (void *)&msg->params.reason,
                    &numOfValidArgs[6],
                    DWPAL_UNSIGNED_CHAR_PARAM,
                    "reason=",
                    0,
                },
                /* Must be at the end */
                {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

            if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(vap_name)) ==
                DWPAL_FAILURE) {
                LOG(ERROR) << "DWPAL parse error ==> Abort";
                return false;
            }

            for (uint8_t i = 0; i < (sizeof(numOfValidArgs) / sizeof(size_t)); i++) {
                if (numOfValidArgs[i] == 0) {
                    LOG(ERROR) << "Failed reading parsed parameter " << (int)i << " ==> Abort";
                    return false;
                }
            }

            LOG(DEBUG) << "numOfValidArgs[0]=" << numOfValidArgs[0] << ", VAP=" << vap_name;
            LOG(DEBUG) << "numOfValidArgs[1]=" << numOfValidArgs[1] << ", VAP_BSSID=" << vap_bssid;
            LOG(DEBUG) << "numOfValidArgs[2]=" << numOfValidArgs[2]
                       << ", client_mac=" << client_mac;
            LOG(DEBUG) << "numOfValidArgs[3]=" << numOfValidArgs[3]
                       << ", rx_snr=" << (int)msg->params.rx_snr;
            LOG(DEBUG) << "numOfValidArgs[4]=" << numOfValidArgs[4]
                       << ", blocked=" << (int)msg->params.blocked;
            LOG(DEBUG) << "numOfValidArgs[5]=" << numOfValidArgs[5]
                       << ", rejected=" << (int)msg->params.reject;
            LOG(DEBUG) << "numOfValidArgs[6]=" << numOfValidArgs[6]
                       << ", reason=" << (int)msg->params.reason;

            // Check if the BSS is valid
            if (beerocks::utils::get_ids_from_iface_string(vap_name).vap_id ==
                beerocks::IFACE_ID_INVALID) {
                LOG(ERROR) << "Event received on invalid VAP ID, should handle the event!";
                return true;
            }
            // Check if the BSS is monitored
            if (!is_BSS_monitored(vap_name)) {
                LOG(DEBUG) << "Event received on non-monitored BSS, skipping!";
            }

            msg->params.client_mac = tlvf::mac_from_string(client_mac);
            //TODO need to add VAP name parsing to  this WLAN_FC_STYPE_AUTH event - WLANRTSYS-9170
            msg->params.bssid = tlvf::mac_from_string(vap_bssid);

            // Add the message to the queue
            event_queue_push(Event::STA_Steering_Auth_Fail, msg_buff);
        } else {
            LOG(ERROR) << "Unknown message type!";
            break;
        }

        break;
    }

    case Event::BSS_TM_Query: {
        parsed_line_t parsed_obj;
        parse_event(buffer, parsed_obj);

        const char *client_mac_str;
        if (!read_param("_mac", parsed_obj, &client_mac_str)) {
            return false;
        }

        const char *vap_name;
        if (!read_param("_iface", parsed_obj, &vap_name)) {
            return false;
        }

        auto iface_ids    = beerocks::utils::get_ids_from_iface_string(vap_name);
        std::string bssid = m_radio_info.available_vaps[iface_ids.vap_id].mac;

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
        sta_bss_steer(iface_ids.vap_id, client_mac_str, bssid, op_class, m_radio_info.channel, 0, 2,
                      0);
        break;
    }

    case Event::BSS_TM_Response: {
        // TODO: Change to HAL objects
        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE));
        auto msg = reinterpret_cast<sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE));

        char MACAddress[MAC_ADDR_SIZE]                      = {0};
        int status_code                                     = 0;
        char vap_name[beerocks::message::IFACE_NAME_LENGTH] = {0};
        size_t numOfValidArgs[4]                            = {0};
        FieldsToParse fieldsToParse[]                       = {
            {NULL /*opCode*/, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, 0},
            {(void *)vap_name, &numOfValidArgs[1], DWPAL_STR_PARAM, NULL,
             beerocks::message::IFACE_NAME_LENGTH},
            {(void *)MACAddress, &numOfValidArgs[2], DWPAL_STR_PARAM, NULL, sizeof(MACAddress)},
            {(void *)&status_code, &numOfValidArgs[3], DWPAL_INT_PARAM, "status_code=", 0},
            /* Must be at the end */
            {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

        if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse,
                                         sizeof(vap_name) + sizeof(MACAddress) +
                                             sizeof(status_code)) == DWPAL_FAILURE) {
            LOG(ERROR) << "DWPAL parse error ==> Abort";
            return false;
        }

        /* TEMP: Traces... */
        LOG(DEBUG) << "numOfValidArgs[1]= " << numOfValidArgs[1] << " VAP name= " << vap_name;
        LOG(DEBUG) << "numOfValidArgs[2]= " << numOfValidArgs[2] << " MACAddress= " << MACAddress;
        LOG(DEBUG) << "numOfValidArgs[3]= " << numOfValidArgs[3]
                   << " status_code= " << int(status_code);

        for (uint8_t i = 0; i < (sizeof(numOfValidArgs) / sizeof(size_t)); i++) {
            if (numOfValidArgs[i] == 0) {
                LOG(ERROR) << "Failed reading parsed parameter " << (int)i << " ==> Abort";
                return false;
            }
        }

        msg->params.mac         = tlvf::mac_from_string(MACAddress);
        msg->params.status_code = status_code;
        std::string bssid;
        beerocks::net::network_utils::linux_iface_get_mac(vap_name, bssid);
        LOG(DEBUG) << "BTM response source BSSID: " << bssid;
        msg->params.source_bssid = tlvf::mac_from_string(bssid);

        // Add the message to the queue
        event_queue_push(Event::BSS_TM_Response, msg_buff);
        break;
    }
    case Event::DFS_CAC_Started: {
        LOG(DEBUG) << buffer;

        std::string tmp_string;

        parsed_line_t parsed_obj;
        parse_event(buffer, parsed_obj);

        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION));
        auto msg = reinterpret_cast<sACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION *>(
            msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";
        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION));

        // Channel
        if (!read_param("chan", parsed_obj, tmp_int)) {
            LOG(ERROR) << "Failed reading 'channel' parameter!";
            return false;
        }
        msg->params.channel = tmp_int;

        // Secondary Channel
        if (!read_param("sec_chan", parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading 'secondary_channel' parameter!";
            return false;
        }
        tmp_string = tmp_str;
        beerocks::string_utils::rtrim(tmp_string, ",");
        msg->params.secondary_channel = beerocks::string_utils::stoi(tmp_string);

        // Bandwidth
        if (!read_param("width", parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading 'bandwidth' parameter!";
            return false;
        }
        tmp_string = tmp_str;
        beerocks::string_utils::rtrim(tmp_string, ",");
        tmp_int               = beerocks::string_utils::stoi(tmp_string);
        msg->params.bandwidth = beerocks::eWiFiBandwidth(dwpal_bw_to_beerocks_bw(tmp_int));

        // CAC Duration
        if (!read_param("cac_time", parsed_obj, &tmp_str)) {
            LOG(ERROR) << "Failed reading 'cac_duration_sec' parameter!";
            return false;
        }
        tmp_string = tmp_str;
        beerocks::string_utils::rtrim(tmp_string, "s");
        msg->params.cac_duration_sec = beerocks::string_utils::stoi(tmp_string);

        // Add the message to the queue
        event_queue_push(Event::DFS_CAC_Started, msg_buff);
        break;
    }
    case Event::DFS_CAC_Completed: {
        LOG(DEBUG) << buffer;

        parsed_line_t parsed_obj;
        parse_event(buffer, parsed_obj);

        if (!get_radio_info().is_5ghz) {
            LOG(WARNING) << "DFS event on 2.4Ghz radio. Ignoring...";
            return true;
        }

        // TODO: Change to HAL objects
        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION));
        auto msg = reinterpret_cast<sACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION *>(
            msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";
        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION));

        if (!read_param("cac_status", parsed_obj, tmp_int)) {
            // older intel hostapd versions still use "success" parameter
            // same as the original openWrt syntax , we should support it as well.
            if (!read_param("success", parsed_obj, tmp_int)) {
                LOG(ERROR) << "Failed reading cac finished success parameter!";
                return false;
            }
        }
        msg->params.success = tmp_int;

        if (!read_param("freq", parsed_obj, tmp_int)) {
            LOG(ERROR) << "Failed reading freq parameter!";
            return false;
        }
        msg->params.frequency = tmp_int;

        if (!read_param("cf1", parsed_obj, tmp_int)) {
            LOG(ERROR) << "Failed reading 'cf1' parameter!";
            return false;
        }
        msg->params.center_frequency1 = tmp_int;

        if (!read_param("cf2", parsed_obj, tmp_int)) {
            LOG(ERROR) << "Failed reading 'cf2 parameter!";
            return false;
        }
        msg->params.center_frequency2 = tmp_int;

        msg->params.channel = son::wireless_utils::freq_to_channel(msg->params.frequency);

        if (!read_param("timeout", parsed_obj, tmp_int)) {
            LOG(ERROR) << "Failed reading timeout parameter!";
            return false;
        }
        msg->params.timeout = tmp_int;

        if (!read_param("chan_width", parsed_obj, tmp_int)) {
            LOG(ERROR) << "Failed reading chan_width parameter!";
            return false;
        }
        msg->params.bandwidth = dwpal_bw_to_beerocks_bw(tmp_int);

        // Add the message to the queue
        event_queue_push(Event::DFS_CAC_Completed, msg_buff);
        break;
    }

    case Event::DFS_NOP_Finished: {
        // TODO: Change to HAL objects
        auto msg_buff =
            ALLOC_SMART_BUFFER(sizeof(sACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION));
        auto msg = reinterpret_cast<sACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION *>(
            msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0,
               sizeof(sACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION));

        uint8_t chan_width            = 0;
        size_t numOfValidArgs[5]      = {0};
        FieldsToParse fieldsToParse[] = {
            {NULL /*opCode*/, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, 0},
            {NULL, &numOfValidArgs[1], DWPAL_STR_PARAM, NULL, 0},
            {(void *)&msg->params.frequency, &numOfValidArgs[2], DWPAL_INT_PARAM, "freq=", 0},
            {(void *)&msg->params.vht_center_frequency, &numOfValidArgs[3], DWPAL_SHORT_INT_PARAM,
             "cf1=", 0},
            {(void *)&chan_width, &numOfValidArgs[4], DWPAL_CHAR_PARAM, "chan_width=", 0},
            /* Must be at the end */
            {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

        if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(int)) ==
            DWPAL_FAILURE) {
            LOG(ERROR) << "DWPAL parse error ==> Abort";
            return false;
        }

        /* TEMP: Traces... */
        LOG(DEBUG) << "numOfValidArgs[2]= " << numOfValidArgs[2]
                   << " freq= " << (int)msg->params.frequency;
        LOG(DEBUG) << "numOfValidArgs[3]= " << numOfValidArgs[3]
                   << " cf1= " << (int)msg->params.vht_center_frequency;
        LOG(DEBUG) << "numOfValidArgs[4]= " << numOfValidArgs[4]
                   << " chan_width= " << (int)chan_width;

        for (uint8_t i = 0; i < (sizeof(numOfValidArgs) / sizeof(size_t)); i++) {
            if (numOfValidArgs[i] == 0) {
                LOG(ERROR) << "Failed reading parsed parameter " << (int)i << " ==> Abort";
                return false;
            }
        }

        msg->params.channel   = son::wireless_utils::freq_to_channel(msg->params.frequency);
        msg->params.bandwidth = dwpal_bw_to_beerocks_bw(chan_width);

        // Add the message to the queue
        event_queue_push(Event::DFS_NOP_Finished, msg_buff);
        break;
    }

    case Event::AP_Disabled: {
        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sHOSTAP_DISABLED_NOTIFICATION));
        auto msg      = reinterpret_cast<sHOSTAP_DISABLED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        memset(msg_buff.get(), 0, sizeof(sHOSTAP_DISABLED_NOTIFICATION));

        char interface[SSID_MAX_SIZE] = {0};
        size_t numOfValidArgs[2]      = {0};
        FieldsToParse fieldsToParse[] = {
            {NULL /*opCode*/, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, 0},
            {(void *)interface, &numOfValidArgs[1], DWPAL_STR_PARAM, NULL, sizeof(interface)},
            /* Must be at the end */
            {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

        if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(interface)) ==
            DWPAL_FAILURE) {
            LOG(ERROR) << "DWPAL parse error ==> Abort";
            return false;
        }

        auto iface_ids = beerocks::utils::get_ids_from_iface_string(interface);
        msg->vap_id    = iface_ids.vap_id;

        event_queue_push(Event::AP_Disabled, msg_buff); // send message to the AP manager

    } break;
    case Event::AP_Enabled: {
        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sHOSTAP_ENABLED_NOTIFICATION));
        auto msg      = reinterpret_cast<sHOSTAP_ENABLED_NOTIFICATION *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        memset(msg_buff.get(), 0, sizeof(sHOSTAP_ENABLED_NOTIFICATION));

        char interface[SSID_MAX_SIZE] = {0};
        size_t numOfValidArgs[2]      = {0};
        FieldsToParse fieldsToParse[] = {
            {NULL /*opCode*/, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, 0},
            {(void *)interface, &numOfValidArgs[1], DWPAL_STR_PARAM, NULL, sizeof(interface)},

            /* Must be at the end */
            {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

        if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(interface)) ==
            DWPAL_FAILURE) {
            LOG(ERROR) << "DWPAL parse error ==> Abort";
            return false;
        }

        auto iface_ids = beerocks::utils::get_ids_from_iface_string(interface);
        msg->vap_id    = iface_ids.vap_id;

        if (iface_ids.vap_id == beerocks::IFACE_RADIO_ID) {
            // Ignore AP-ENABLED on radio
            return true;
        }

        event_queue_push(Event::AP_Enabled, msg_buff);
    } break;

    case Event::Interface_Disabled:
    case Event::ACS_Failed: {
        LOG(DEBUG) << buffer;
        event_queue_push(event); // Forward to the AP manager
    } break;

    case Event::MGMT_Frame: {

        char vap[beerocks::message::IFACE_NAME_LENGTH] = {0};
        char frame[ASSOCIATION_FRAME_SIZE]             = {0};
        size_t numOfValidArgs[3]                       = {0};

        FieldsToParse fieldsToParse[] = {
            {NULL /*opCode*/, &numOfValidArgs[0], DWPAL_STR_PARAM, NULL, 0},
            {(void *)vap, &numOfValidArgs[1], DWPAL_STR_PARAM, NULL, sizeof(vap)},
            {(void *)frame, &numOfValidArgs[2], DWPAL_STR_PARAM, NULL, sizeof(frame)},
            /* Must be at the end */
            {NULL, NULL, DWPAL_NUM_OF_PARSING_TYPES, NULL, 0}};

        if (dwpal_string_to_struct_parse(buffer, bufLen, fieldsToParse, sizeof(vap)) ==
            DWPAL_FAILURE) {
            LOG(ERROR) << "DWPAL parse error ==> Abort";
            return false;
        }

        // Create the management frame notification event
        if (frame[0] == 0) {
            LOG(WARNING) << "Management frame received without data: " << buffer;
            return true; // Just a warning, do not fail
        }

        auto mgmt_frame = create_mgmt_frame_notification(frame);
        if (!mgmt_frame) {
            LOG(WARNING) << "Failed creating management frame notification!";
            return true; // Just a warning, do not fail
        }

        event_queue_push(Event::MGMT_Frame, mgmt_frame);
    } break;

    case Event::AP_Sta_Possible_Psk_Mismatch: {

        parsed_line_t parsed_obj;
        parse_event(buffer, parsed_obj);

        auto msg_buff = ALLOC_SMART_BUFFER(sizeof(sSTA_MISMATCH_PSK));
        auto msg      = reinterpret_cast<sSTA_MISMATCH_PSK *>(msg_buff.get());
        LOG_IF(!msg, FATAL) << "Memory allocation failed!";

        // Initialize the message
        memset(msg_buff.get(), 0, sizeof(sSTA_MISMATCH_PSK));

        const char *client_mac_str;

        if (!read_param("_mac", parsed_obj, &client_mac_str)) {
            return false;
        }
        msg->sta_mac = tlvf::mac_from_string(client_mac_str);
        LOG(DEBUG) << "sta possible psk mismatch: offending STA mac: " << msg->sta_mac;

        event_queue_push(Event::AP_Sta_Possible_Psk_Mismatch,
                         msg_buff); // send message to the AP manager
        break;
    }

    // Gracefully ignore unhandled events
    // TODO: Probably should be changed to an error once dwpal will stop
    //       sending empty or irrelevant events...
    default:
        LOG(DEBUG) << "Unhandled event received: " << opcode;
        break;
    }

    return true;
}

bool ap_wlan_hal_dwpal::process_dwpal_nl_event(struct nl_msg *msg, void *arg)
{
    LOG(ERROR) << __func__ << "isn't implemented by this derived and shouldn't be called";
    return false;
}

} // namespace dwpal

std::shared_ptr<ap_wlan_hal> ap_wlan_hal_create(std::string iface_name, hal_conf_t hal_conf,
                                                base_wlan_hal::hal_event_cb_t callback)
{
    return std::make_shared<dwpal::ap_wlan_hal_dwpal>(iface_name, callback, hal_conf);
}

} // namespace bwl
