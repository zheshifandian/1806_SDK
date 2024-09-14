/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_string_utils.h>
#include <bcl/beerocks_utils.h>
#include <bcl/son/son_wireless_utils.h>
#include <easylogging++.h>
#include <iomanip>

using namespace beerocks;

int utils::write_to_file(std::string full_path, const std::string &val)
{
    std::ofstream out_file(full_path);
    if (out_file.is_open()) {
        out_file << val;
        out_file.close();
        return 0;
    } else {
        return 1;
    }
}

beerocks::eIfaceType utils::get_iface_type_from_string(std::string iface_type_name)
{
    if (!iface_type_name.compare(0, sizeof(IFACE_TYPE_STR_WIFI_INTEL), IFACE_TYPE_STR_WIFI_INTEL))
        return beerocks::IFACE_TYPE_WIFI_INTEL;
    if (!iface_type_name.compare(0, sizeof(IFACE_TYPE_STR_WIFI_UNSPECIFIED),
                                 IFACE_TYPE_STR_WIFI_UNSPECIFIED))
        return beerocks::IFACE_TYPE_WIFI_UNSPECIFIED;
    if (!iface_type_name.compare(0, sizeof(IFACE_TYPE_STR_ETHERNET), IFACE_TYPE_STR_ETHERNET))
        return beerocks::IFACE_TYPE_ETHERNET;
    if (!iface_type_name.compare(0, sizeof(IFACE_TYPE_STR_GW_BRIDGE), IFACE_TYPE_STR_GW_BRIDGE))
        return beerocks::IFACE_TYPE_GW_BRIDGE;
    if (!iface_type_name.compare(0, sizeof(IFACE_TYPE_STR_BRIDGE), IFACE_TYPE_STR_BRIDGE))
        return beerocks::IFACE_TYPE_BRIDGE;
    return beerocks::IFACE_TYPE_UNSUPPORTED;
}

std::string utils::get_iface_type_string(beerocks::eIfaceType iface_type)
{
    std::string result;
    switch (iface_type) {
    case beerocks::IFACE_TYPE_WIFI_INTEL: {
        result = std::string(IFACE_TYPE_STR_WIFI_INTEL);
        break;
    }
    case beerocks::IFACE_TYPE_WIFI_UNSPECIFIED: {
        result = std::string(IFACE_TYPE_STR_WIFI_UNSPECIFIED);
        break;
    }
    case beerocks::IFACE_TYPE_ETHERNET: {
        result = std::string(IFACE_TYPE_STR_ETHERNET);
        break;
    }
    case beerocks::IFACE_TYPE_BRIDGE: {
        result = std::string(IFACE_TYPE_STR_BRIDGE);
        break;
    }
    case beerocks::IFACE_TYPE_GW_BRIDGE: {
        result = std::string(IFACE_TYPE_STR_GW_BRIDGE);
        break;
    }
    default: {
        result = std::string(IFACE_TYPE_STR_UNSUPPORTED);
    }
    }
    return result;
}

bool utils::is_node_wireless(beerocks::eIfaceType iface_type)
{
    return ((iface_type > beerocks::IFACE_TYPE_UNSUPPORTED) &&
            (iface_type < beerocks::IFACE_TYPE_WIFI_END));
}

std::string utils::get_default_ifname_prefix() { return (beerocks::ifname_prefix_list[0]); }

char utils::get_default_ifname_separator() { return (beerocks::ifname_separators[0]); }

bool utils::is_allowed_ifname_prefix(const std::string &prefix, bool partial)
{
    if (prefix.empty()) {
        return false;
    }
    std::set<std::string> const allowed_list(std::begin(beerocks::ifname_prefix_list),
                                             std::end(beerocks::ifname_prefix_list));
    if (!partial) {
        return (allowed_list.find(prefix) != allowed_list.end());
    }
    for (auto allowed_entry : allowed_list) {
        if (prefix.compare(0, allowed_entry.size(), allowed_entry) == 0) {
            return true;
        }
    }
    return false;
}

std::string utils::get_prefix_from_iface_string(const std::string &iface)
{
    // "IFx" or IFx.x"
    std::string result("");
    std::string digits("0123456789");
    auto pos = iface.find_last_of(beerocks::ifname_separators);
    if (pos != std::string::npos) {
        auto sep = iface.at(pos);
        if (iface.substr(pos).find_first_not_of(digits + sep) != std::string::npos) {
            return result;
        }
    }
    auto prefix_num = iface.substr(0, pos);
    pos             = prefix_num.find_last_not_of(digits);
    result          = prefix_num.substr(0, pos + 1);
    return result;
}

utils::sIfaceVapIds utils::get_ids_from_iface_string(const std::string &iface)
{
    utils::sIfaceVapIds ids;
    if (iface.empty()) {
        LOG(ERROR) << "iface_is empty!";
        return ids;
    }

    auto prefix_str = get_prefix_from_iface_string(iface);
    if (!is_allowed_ifname_prefix(prefix_str)) {
        LOG(ERROR) << "iface does not contain any allowed prefix, function input string: " << iface;
        return ids;
    }

    auto iface_num_str = iface.substr(prefix_str.length(), std::string::npos);
    auto sep           = get_default_ifname_separator();
    auto pos           = iface_num_str.find_first_of(beerocks::ifname_separators);
    if (pos != std::string::npos) {
        sep = iface_num_str.at(pos);
    }
    auto iface_num_vec = string_utils::str_split(iface_num_str, sep);
    if (iface_num_vec.size() == 0) {
        LOG(ERROR) << "Invalid interface name " << iface;
        return ids;
    }

    ids.iface_prefix = prefix_str;
    ids.iface_sep    = sep;
    ids.iface_id     = string_utils::stoi(iface_num_vec[0]);
    ids.vap_id       = beerocks::IFACE_RADIO_ID;
    if (iface_num_vec.size() == 2) {
        int8_t vap_id = string_utils::stoi(iface_num_vec[1]);
        if ((vap_id < beerocks::IFACE_VAP_ID_MIN) || (vap_id > beerocks::IFACE_VAP_ID_MAX)) {
            LOG(DEBUG) << "Invalid VAP id " << vap_id << " for interface " << iface;
            ids.vap_id = beerocks::IFACE_ID_INVALID;
            return ids;
        }
        ids.vap_id = vap_id;
    }

    return ids;
}

std::string utils::get_iface_string_from_iface_vap_ids(int8_t iface_id, int8_t vap_id)
{
    std::string ifname;

    if ((iface_id < 0) || (vap_id < beerocks::IFACE_RADIO_ID) ||
        (vap_id > beerocks::IFACE_VAP_ID_MAX)) {
        LOG(ERROR) << "function input is not valid! iface_id=" << int(iface_id)
                   << ", vap_id=" << int(vap_id);
    } else {
        ifname = get_default_ifname_prefix() + std::to_string(iface_id);
        if (vap_id >= beerocks::IFACE_VAP_ID_MIN) {
            ifname += get_default_ifname_separator() + std::to_string(vap_id);
        }
    }

    return ifname;
}

std::string utils::get_iface_string_from_iface_vap_ids(const std::string &iface, int8_t vap_id)
{
    std::string ifname;
    auto iface_ids = beerocks::utils::get_ids_from_iface_string(iface);
    if ((iface_ids.iface_id < 0) || (vap_id < beerocks::IFACE_RADIO_ID) ||
        (vap_id > beerocks::IFACE_VAP_ID_MAX)) {
        LOG(ERROR) << "function input is not valid! iface=" << iface << ", vap_id=" << int(vap_id);
    } else if (vap_id == iface_ids.vap_id) {
        ifname = iface;
    } else {
        ifname = iface_ids.iface_prefix + std::to_string(iface_ids.iface_id);
        if (vap_id >= beerocks::IFACE_VAP_ID_MIN) {
            ifname += iface_ids.iface_sep + std::to_string(vap_id);
        }
    }

    return ifname;
}

beerocks::eWiFiBandwidth utils::convert_bandwidth_to_enum(int bandwidth_int)
{
    beerocks::eWiFiBandwidth bw;
    switch (bandwidth_int) {
    case 20:
        bw = beerocks::BANDWIDTH_20;
        break;
    case 40:
        bw = beerocks::BANDWIDTH_40;
        break;
    case 80:
        bw = beerocks::BANDWIDTH_80;
        break;
    case 160:
        bw = beerocks::BANDWIDTH_160;
        break;
    default:
        bw = beerocks::BANDWIDTH_80;
        break;
    }
    return bw;
}

int utils::convert_bandwidth_to_int(beerocks::eWiFiBandwidth bandwidth)
{
    switch (bandwidth) {
    case beerocks::BANDWIDTH_20:
        return 20;
    case beerocks::BANDWIDTH_40:
        return 40;
    case beerocks::BANDWIDTH_80:
        return 80;
    case beerocks::BANDWIDTH_80_80:
    case beerocks::BANDWIDTH_160:
        return 160;
    default:
        LOG(ERROR) << "Failed to convert eWiFiBandwidth: " << bandwidth << " to integer";
        return bandwidth;
    }
}

std::string utils::convert_channel_ext_above_to_string(bool channel_ext_above_secondary,
                                                       beerocks::eWiFiBandwidth bandwidth)
{
    switch (bandwidth) {
    case beerocks::BANDWIDTH_20:
        return std::string();
    case beerocks::BANDWIDTH_40:
    case beerocks::BANDWIDTH_80:
    case beerocks::BANDWIDTH_80_80:
    case beerocks::BANDWIDTH_160:
        if (channel_ext_above_secondary) {
            return "H";
        } else {
            return "L";
        }
    default:
        return std::string();
    }
}

std::string utils::dump_buffer(const uint8_t *buffer, size_t len)
{
    std::ostringstream hexdump;
    for (size_t i = 0; i < len; i += 16) {
        for (size_t j = i; j < len && j < i + 16; j++)
            hexdump << std::hex << std::setw(2) << std::setfill('0') << (unsigned)buffer[j] << " ";
        hexdump << std::endl;
    }
    return hexdump.str();
}

void utils::hex_dump(const std::string &description, uint8_t *addr, int len,
                     const char *calling_file, int calling_line)
{
    int16_t i;
    char ascii_chars[17]        = {};
    uint8_t *pc                 = addr;
    auto calling_file_str       = std::string(calling_file);
    const auto caller_file_name = calling_file_str.substr(calling_file_str.rfind('/') + 1);
    std::stringstream print_stream;

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0) {
                print_stream << "   " << std::string(ascii_chars) << std::endl;
            }

            print_stream << string_utils::int_to_hex_string(i, 4);
        }

        // Now the hex code for the specific character.
        print_stream << " " << string_utils::int_to_hex_string(i, 2);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
            ascii_chars[i % 16] = '.';
        } else {
            ascii_chars[i % 16] = pc[i];
        }
        ascii_chars[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        print_stream << "   ";
        i++;
    }

    // And print the final ASCII bit.
    print_stream << "   " << ascii_chars << std::endl;

    std::cout << description << std::endl << print_stream.str();

    LOG(DEBUG) << caller_file_name << "[" << (int)calling_line << "] " << description << std::endl
               << print_stream.str();
}
std::string utils::get_ISO_8601_timestamp_string(std::chrono::system_clock::time_point timestamp)
{
    auto seconds_since_epoch =
        std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch());

    // Construct time_t using 'seconds_since_epoch' rather than 'stamp' since it is
    // implementation-defined whether the value is rounded or truncated.
    std::time_t stamp_t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::time_point(seconds_since_epoch));

    // std::strftime() can convert the "now" timestamp into a string,
    // but it only supports up to a resolution of a second.
    // generating the first part of the data-time string:
    char buff[40];
    if (!std::strftime(buff, 40, "%Y-%m-%dT%H:%M:%S.", std::localtime(&stamp_t))) {
        return "";
    }

    // The subtraction bellow is used to get the fractional value of the second into the string.
    // Note: the "Z" at the end means zolo time (UTC+0). This function assume locale to always be UTC.
    // Unless we have a way to know our local, in which case, "Z" might be replaced with
    // the time delta (+03:00 for Israel, as an example).
    return std::string(buff) +
           std::to_string((timestamp.time_since_epoch() - seconds_since_epoch).count()) + "Z";
}
