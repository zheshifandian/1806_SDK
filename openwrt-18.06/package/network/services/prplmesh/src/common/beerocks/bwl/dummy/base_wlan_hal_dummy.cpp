/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "base_wlan_hal_dummy.h"

#include <bcl/beerocks_string_utils.h>
#include <bcl/beerocks_utils.h>
#include <bcl/network/network_utils.h>
#include <bcl/son/son_wireless_utils.h>

#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <easylogging++.h>

namespace bwl {
namespace dummy {

//////////////////////////////////////////////////////////////////////////////
///////////////////////// Local Module Definitions ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Name for the dummy events FIFO file
static constexpr char EVENT_FILE_NAME[] = "EVENT";

//////////////////////////////////////////////////////////////////////////////
////////////////////////// Local Module Functions ////////////////////////////
//////////////////////////////////////////////////////////////////////////////

std::ostream &operator<<(std::ostream &out, const dummy_fsm_state &value)
{
    switch (value) {
    case dummy_fsm_state::Delay:
        out << "Delay";
        break;
    case dummy_fsm_state::Init:
        out << "Init";
        break;
    case dummy_fsm_state::GetRadioInfo:
        out << "GetRadioInfo";
        break;
    case dummy_fsm_state::Attach:
        out << "Attach";
        break;
    case dummy_fsm_state::Operational:
        out << "Operational";
        break;
    case dummy_fsm_state::Detach:
        out << "Detach";
        break;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const dummy_fsm_event &value)
{
    switch (value) {
    case dummy_fsm_event::Attach:
        out << "Attach";
        break;
    case dummy_fsm_event::Detach:
        out << "Detach";
        break;
    }
    return out;
}

static void map_obj_parser(std::stringstream &ss_in, std::list<char> delimiter_list,
                           parsed_obj_map_t &map_obj)
{
    if (delimiter_list.empty())
        return;

    std::string str_storage, key;
    bool kv = true; // '1'=key, '0'=val;
    while (std::getline(ss_in, str_storage, delimiter_list.front())) {
        if (delimiter_list.size() == 1) {
            if (kv) {
                key = str_storage; // save key
            } else {
                map_obj[key] = str_storage; // save val
            }
            kv = !kv;

        } else {
            auto delimiter_list_out(delimiter_list);
            delimiter_list_out.erase(delimiter_list_out.begin()); // delete first delimiter
            std::stringstream ss_out(str_storage);
            map_obj_parser(ss_out, delimiter_list_out, map_obj);
        }
    }
}

static std::string::size_type find_first_of_delimiter_pair(std::string &str,
                                                           std::string::size_type pos,
                                                           char delim_near, char delim_far)
{
    // finds first occurrence on string 'str' of <'delim_near'><any_characters except delimiters><'delim_far'>
    auto idx = str.find_first_of(delim_far, pos);
    if (idx == std::string::npos) {
        return idx;
    }
    idx = str.rfind(delim_near, idx);
    return idx;
}

static void map_event_obj_parser(std::string event_str, parsed_obj_map_t &map_obj)
{
    // eliminate event log level from the begining of the event string : "<3>"
    size_t idx_start = 0;

    // find params without key end index
    auto idx = find_first_of_delimiter_pair(event_str, idx_start, ' ', '=');

    // put null terminator at the end of our key=val, for ss construction
    if (idx != std::string::npos) {
        event_str[idx] = '\0';
    }

    // insert to map known prams without key
    std::stringstream ss(event_str.c_str() + idx_start);
    std::string str_storage;
    bool opcode = true;
    bool type   = true;
    while (std::getline(ss, str_storage, ' ')) {
        if (type) {
            // assume that the first param is type - DATA or EVENT
            map_obj[DUMMY_EVENT_KEYLESS_PARAM_TYPE] = str_storage;
            type                                    = false;
        } else if (opcode) {
            // assume that the second param is data or event name
            map_obj[DUMMY_EVENT_KEYLESS_PARAM_OPCODE] = str_storage;
            opcode                                    = false;
        } else if (beerocks::net::network_utils::is_valid_mac(str_storage)) {
            map_obj[DUMMY_EVENT_KEYLESS_PARAM_MAC] = str_storage;
        } else if (beerocks::utils::is_allowed_ifname_prefix(str_storage, true)) {
            map_obj[DUMMY_EVENT_KEYLESS_PARAM_IFACE] = str_storage;
        }
    }

    // fill the map with the rest of event data
    while (idx != std::string::npos) {

        idx_start = ++idx;

        // find first '=' to skip on it
        idx = event_str.find_first_of('=', idx_start);

        // find the next pair of delimiters index
        idx = find_first_of_delimiter_pair(event_str, ++idx, ' ', '=');

        if (idx != std::string::npos) {
            // put null terminator at the end of our key=val, for ss_in construction
            event_str[idx] = '\0';
        }

        // parse key=val
        std::stringstream ss_in(event_str.c_str() + idx_start);
        map_obj_parser(ss_in, {'='}, map_obj);
    }
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Implementation ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

bool base_wlan_hal_dummy::dummy_obj_read_int(const std::string &key, parsed_obj_map_t &obj,
                                             int64_t &value, bool ignore_unknown)
{
    auto val_iter = obj.find(key);
    if (val_iter == obj.end()) {
        LOG(ERROR) << "param :" << key << " is not exist";
        return false;
    }

    static const std::string unknown_string = "UNKNOWN";

    if (ignore_unknown && !unknown_string.compare(val_iter->second)) {
        value = 0;
        return true;
    }

    value = beerocks::string_utils::stoi(val_iter->second);

    return (true);
}

bool base_wlan_hal_dummy::dummy_obj_read_str(const std::string &key, parsed_obj_map_t &obj,
                                             char **value)
{
    auto val_iter = obj.find(key);
    if (val_iter == obj.end()) {
        LOG(ERROR) << "param :" << key << " does not exist";
        return false;
    }

    *value = (char *)((val_iter->second).c_str());
    return (true);
}

void base_wlan_hal_dummy::parsed_obj_debug(parsed_obj_map_t &obj)
{
    LOG(TRACE) << "parsed_obj_debug:";
    std::stringstream ss_obj;
    ss_obj << std::endl << "parsed_obj_debug: " << std::endl;
    for (auto element : obj) {
        LOG(TRACE) << "key: " << element.first << ", value: " << element.second;
        ss_obj << "key: " << element.first << ", value: " << element.second << std::endl;
    }

    LOG(DEBUG) << ss_obj.str();
}

void base_wlan_hal_dummy::parsed_obj_debug(parsed_obj_listed_map_t &obj)
{
    LOG(TRACE) << "parsed_obj_debug:";
    std::stringstream ss_obj;
    ss_obj << std::endl << "parsed_obj_debug: " << std::endl;
    int element_num = 0;
    for (auto list_element : obj) {
        LOG(TRACE) << "vector element: " << element_num;
        ss_obj << "vector element: " << element_num << std::endl;
        for (auto map_element : list_element) {
            LOG(TRACE) << "key: " << map_element.first << ", value: " << map_element.second;
            ss_obj << "key: " << map_element.first << ", value: " << map_element.second
                   << std::endl;
        }
        element_num++;
    }
    LOG(DEBUG) << ss_obj.str();
}

base_wlan_hal_dummy::base_wlan_hal_dummy(HALType type, const std::string &iface_name,
                                         hal_event_cb_t callback, const hal_conf_t &hal_conf)
    : base_wlan_hal(type, iface_name, IfaceType::Intel, callback, hal_conf),
      beerocks::beerocks_fsm<dummy_fsm_state, dummy_fsm_event>(dummy_fsm_state::Delay)
{
    // Create events directory
    if (mkdir(get_status_dir().c_str(), S_IRWXU | S_IRWXG | S_IWOTH | S_IXOTH) == -1) {
        if (errno != EEXIST) { // Do NOT fail if the directory already exists
            LOG(FATAL) << "Failed creating events directory: " << strerror(errno);
        }
    }

    // Generate dummy events filename:
    // Since FIFOs allow a single reader only (well, not only, but when multiple
    // readers listen to the same FIFO, only one of the readers is randomly selected
    // to process the incoming data), generate separate FIFO filenames for every HAL type.
    m_dummy_event_file = get_status_dir(EVENT_FILE_NAME);

    switch (type) {
    case HALType::AccessPoint: {
        m_dummy_event_file += "_AP";
    } break;
    case HALType::Monitor: {
        m_dummy_event_file += "_MON";
    } break;
    case HALType::Station: {
        m_dummy_event_file += "_STA";
    } break;
    default: {
    } break;
    }

    // Remove previously created FIFO for the events file and create a new one
    if (unlink(m_dummy_event_file.c_str()) == -1) {
        LOG_IF(errno != ENOENT, FATAL)
            << "Failed removing previous events FIFO: " << strerror(errno);
    }

    // Create the dummy events FIFO file, with Read-Write permissions to Owner/Group and
    // Write-Only permissions to Others.
    if (mkfifo(m_dummy_event_file.c_str(), S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP | S_IWOTH) ==
        -1) {
        LOG(FATAL) << "Failed creating events FIFO: " << strerror(errno);
    }

    // Open the events FIFO for Read-Write. This is necessary to ensure that the FIFO file
    // always has at-least one "writer". Otherwise the FIFO is automatically closed by the
    // OS when writing data externally using "echo".
    if ((m_fds_ext_events[0] = open(m_dummy_event_file.c_str(), O_RDWR | O_NONBLOCK)) == -1) {
        LOG(FATAL) << "Failed opening events file: " << strerror(errno);
    }

    // Initialize the FSM
    fsm_setup();
}

base_wlan_hal_dummy::~base_wlan_hal_dummy()
{
    // Close the dummy events FIFO
    if (m_fds_ext_events[0] != -1) {
        close(m_fds_ext_events[0]);
        m_fds_ext_events[0] = -1;

        // Remove the FIFO file
        unlink(m_dummy_event_file.c_str());
    }

    base_wlan_hal_dummy::detach();
}

bool base_wlan_hal_dummy::fsm_setup() { return true; }

HALState base_wlan_hal_dummy::attach(bool block)
{
    m_radio_info.radio_state = eRadioState::ENABLED;
    refresh_radio_info();
    return (m_hal_state = HALState::Operational);
}

bool base_wlan_hal_dummy::detach() { return true; }

bool base_wlan_hal_dummy::set(const std::string &param, const std::string &value, int vap_id)
{
    return true;
}

bool base_wlan_hal_dummy::ping() { return true; }

bool base_wlan_hal_dummy::write_status_file(const std::string &filename,
                                            const std::string &value) const
{
    // To make sure the file is written atomically, use the following procedure.
    // 1. Write to a temporary file on the same filesystem. To make sure it's the same filesystem,
    //    put it in the same directory. We don't need to use tempfile() here because there can be
    //    only one bwl running in parallel (otherwise we have bigger problems) and we do want to
    //    overwrite/remove any dangling file from a previous run.
    // 2. Write the temporary file.
    // 3. Close it (making sure it is flushed to the filesystem).
    // At this point, we can check for failure (which covers steps 1-3).
    // 4. Rename to the final filename.
    // To be really fully atomic, between steps 3 and 4 we should also do an fsync of the file and
    // of the directory. However, that is only needed for atomicity over reboots; for atomicity
    // between processes, the rename after close is sufficient.
    auto full_path{get_status_dir() + "/" + filename};
    auto full_path_tmp{full_path + ".tmp"};
    std::ofstream statusfile(full_path_tmp);
    statusfile << value;
    statusfile.close();
    if (!statusfile) {
        LOG(ERROR) << "Failed writing to " << full_path;
        return false;
    }
    if (rename(full_path_tmp.c_str(), full_path.c_str()) < 0) {
        LOG(ERROR) << "Failed to rename " << full_path_tmp;
        return false;
    }
    return true;
}

bool base_wlan_hal_dummy::process_nl_events()
{
    LOG(ERROR) << "not implemented";
    return false;
}

bool base_wlan_hal_dummy::refresh_radio_info()
{
    m_radio_info.max_bandwidth = beerocks::eWiFiBandwidth::BANDWIDTH_40;

    if (get_iface_name() == "wlan2") {
        m_radio_info.is_5ghz        = true;
        m_radio_info.frequency_band = beerocks::eFreqType::FREQ_5G;
        for (uint16_t ch = 36; ch <= 64; ch += 4) {
            auto &channel_info = m_radio_info.channels_list[ch];
            channel_info.dfs_state =
                (ch > 48) ? beerocks::eDfsState::AVAILABLE : beerocks::eDfsState::DFS_STATE_MAX;
            // Set all ranking to highest rank (1)
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_20]  = 1;
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_40]  = 1;
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_80]  = 1;
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_160] = 1;
        }
        for (uint16_t ch = 100; ch <= 144; ch += 4) {
            auto &channel_info     = m_radio_info.channels_list[ch];
            channel_info.dfs_state = beerocks::eDfsState::AVAILABLE;
            // Set all ranking to highest rank (1)
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_20]  = 1;
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_40]  = 1;
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_80]  = 1;
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_160] = 1;
        }
        for (uint16_t ch = 149; ch <= 165; ch += 4) {
            auto &channel_info     = m_radio_info.channels_list[ch];
            channel_info.dfs_state = beerocks::eDfsState::DFS_STATE_MAX;
            // Set all ranking to highest rank (1)
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_20]  = 1;
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_40]  = 1;
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_80]  = 1;
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_160] = 1;
        }
    } else {
        m_radio_info.frequency_band = beerocks::eFreqType::FREQ_24G;
        for (uint16_t ch = 1; ch <= 11; ch++) {
            auto &channel_info     = m_radio_info.channels_list[ch];
            channel_info.dfs_state = beerocks::eDfsState::DFS_STATE_MAX;
            // Set all ranking to highest rank (1)
            channel_info.bw_info_list[beerocks::eWiFiBandwidth::BANDWIDTH_20] = 1;
        }
    }

    m_radio_info.ht_supported   = true;
    m_radio_info.ht_capability  = 0;
    m_radio_info.ht_mcs_set     = {};
    m_radio_info.vht_supported  = true;
    m_radio_info.vht_capability = 0;
    m_radio_info.vht_mcs_set    = {};

    std::string radio_mac;
    beerocks::net::network_utils::linux_iface_get_mac(m_radio_info.iface_name, radio_mac);
    for (int vap_id = 0; vap_id < predefined_vaps_num; vap_id++) {
        auto mac = tlvf::mac_from_string(radio_mac);
        mac.oct[5] += vap_id;
        m_radio_info.available_vaps[vap_id].mac = tlvf::mac_to_string(mac);
    }
    return true;
}

bool base_wlan_hal_dummy::refresh_vap_info(int vap_id) { return true; }

bool base_wlan_hal_dummy::refresh_vaps_info(int id) { return true; }

/**
 * @brief process simulated events
 *        events are expected to be simulated by writing the event
 *        string to the first line in the EVENT file.
 *        For example, simulating client connected event:
 *        echo "STA_CONNECTED,11:22:33:44:55:66"
 *
 * @return true on success
 * @return false on failure
 */
bool base_wlan_hal_dummy::process_ext_events(int fd)
{
    LOG(TRACE) << "Processing external dummy event...";

    // Read the event from the FIFO
    memset(m_event_data, 0, sizeof(m_event_data));
    int read_bytes = read(m_fds_ext_events[0], m_event_data, sizeof(m_event_data));
    if (read_bytes <= 0) {
        LOG(ERROR) << "Failed reading event: " << strerror(errno);
        return false;
    }

    // Remove trailing new-line
    if (m_event_data[read_bytes - 1] == '\n') {
        m_event_data[read_bytes - 1] = 0;
        read_bytes--;
    }

    std::string event(m_event_data, 0, read_bytes);
    LOG(DEBUG) << "Received event " << event;

    parsed_obj_map_t event_obj;
    map_event_obj_parser(event, event_obj);
    // parsed_obj_debug(event_obj);

    // Process the event
    if (event_obj[DUMMY_EVENT_KEYLESS_PARAM_TYPE] == "EVENT") {
        if (!process_dummy_event(event_obj)) {
            LOG(ERROR) << "Failed processing DUMMY event: "
                       << event_obj[DUMMY_EVENT_KEYLESS_PARAM_OPCODE];
            // Do not fail the AP Manager on parsing errors
            return true;
        }
    }
    // Process data
    else if (event_obj[DUMMY_EVENT_KEYLESS_PARAM_TYPE] == "DATA") {
        if (!process_dummy_data(event_obj)) {
            LOG(ERROR) << "Failed processing DUMMY data: "
                       << event_obj[DUMMY_EVENT_KEYLESS_PARAM_OPCODE];
            // Do not fail the AP Manager on parsing errors
            return true;
        }
    } else {
        LOG(ERROR) << "Unsupported type " << event_obj[DUMMY_EVENT_KEYLESS_PARAM_TYPE];
        // Do not fail the AP Manager on unknown events
        return true;
    }

    return true;
}

std::string base_wlan_hal_dummy::get_radio_mac()
{
    std::string mac;
    if (!beerocks::net::network_utils::linux_iface_get_mac(m_radio_info.iface_name, mac)) {
        LOG(ERROR) << "Failed to get radio mac from ifname " << m_radio_info.iface_name;
    }
    return mac;
}

bool base_wlan_hal_dummy::get_channel_utilization(uint8_t &channel_utilization)
{
    const uint8_t min_value   = 0;
    const uint8_t max_value   = UINT8_MAX;
    static uint8_t last_value = max_value;

    if (max_value == last_value) {
        channel_utilization = min_value;
    } else {
        channel_utilization = last_value + 1;
    }

    last_value = channel_utilization;

    return true;
}

} // namespace dummy
} // namespace bwl
