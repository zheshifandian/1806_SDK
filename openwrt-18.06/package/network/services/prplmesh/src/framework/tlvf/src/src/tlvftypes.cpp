/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <climits>
#include <iomanip>
#include <linux/if_ether.h>
#include <tlvf/tlvftypes.h>

namespace tlvf {

static constexpr const char mac_bytes_separator = ':';
static constexpr const char zero_mac_string[]   = "00:00:00:00:00:00";

std::string int_to_hex_string(const unsigned int integer, const uint8_t number_of_digits)
{
    std::stringstream ss_hex_string;

    // convert to hex
    ss_hex_string << std::setw(number_of_digits) << std::setfill('0') << std::hex << integer;

    return ss_hex_string.str();
};

std::string mac_to_string(const uint8_t *mac_address)
{
    if (!mac_address) {
        LOG(FATAL) << "nullptr mac address!";
        return std::string{zero_mac_string};
    }

    std::string mac_addr_string;

    mac_addr_string = int_to_hex_string((uint32_t)mac_address[0], 2) + mac_bytes_separator +
                      int_to_hex_string((uint32_t)mac_address[1], 2) + mac_bytes_separator +
                      int_to_hex_string((uint32_t)mac_address[2], 2) + mac_bytes_separator +
                      int_to_hex_string((uint32_t)mac_address[3], 2) + mac_bytes_separator +
                      int_to_hex_string((uint32_t)mac_address[4], 2) + mac_bytes_separator +
                      int_to_hex_string((uint32_t)mac_address[5], 2);

    return mac_addr_string;
}

std::string mac_to_string(const sMacAddr &mac) { return mac_to_string((const uint8_t *)mac.oct); }

// Converts uint64_t mac address to string format
std::string mac_to_string(const uint64_t mac)
{
    uint8_t mac_address[ETH_ALEN];
    int8_t i;
    uint8_t *p = mac_address;
    for (i = 5; i >= 0; i--) {
        *p++ = mac >> (CHAR_BIT * i);
    }
    return mac_to_string(mac_address);
}

bool mac_from_string(uint8_t *buf, const std::string &mac)
{
    if (!buf) {
        return false;
    }
    std::fill_n(buf, ETH_ALEN, 0);
    std::string stripped_mac;
    if (mac.empty()) {
        return true;
    } else if (mac.size() == 3 * ETH_ALEN - 1) { // Assume XX:XX:XX:XX:XX:XX format
        stripped_mac = mac;
        stripped_mac.erase(std::remove_if(stripped_mac.begin(), stripped_mac.end(),
                                          [](char c) { return c == mac_bytes_separator; }),
                           stripped_mac.end());
    } else if (mac.size() == 2 * ETH_ALEN + 2 &&
               mac.substr(0, 2) == "0x") { // Assume 0xXXXXXXXXXX format
        stripped_mac = mac.substr(2);
    } else {
        stripped_mac = mac;
    }

    if (stripped_mac.size() != 2 * ETH_ALEN) {
        return false;
    }
    char *end        = nullptr;
    uint64_t mac_int = std::strtoull(stripped_mac.c_str(), &end, 16);
    if (end != stripped_mac.c_str() + 2 * ETH_ALEN) { // Should be exactly 12 hex digits
        return false;
    }
    for (int i = 0; i < ETH_ALEN; i++) {
        buf[i] = mac_int >> ((ETH_ALEN - i - 1) * 8);
    }
    return true;
}

sMacAddr mac_from_string(const std::string &mac)
{
    sMacAddr ret = {};

    mac_from_string(ret.oct, mac);

    return ret;
}

void mac_to_array(const sMacAddr &mac, uint8_t array[sizeof(sMacAddr::oct)])
{
    std::copy_n(mac.oct, sizeof(sMacAddr::oct), array);
}

void mac_from_array(const uint8_t array[sizeof(sMacAddr::oct)], sMacAddr &mac)
{
    std::copy_n(array, sizeof(sMacAddr::oct), mac.oct);
}

sMacAddr mac_from_array(const uint8_t array[sizeof(sMacAddr::oct)])
{
    sMacAddr ret;
    mac_from_array(array, ret);
    return ret;
}

} // namespace tlvf
