/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _TLVF_TYPES_H_
#define _TLVF_TYPES_H_

#include <sstream>
#include <tlvf/CmduMessageTx.h>
#include <tlvf/common/sMacAddr.h>

namespace tlvf {

constexpr size_t MAX_TLV_SIZE = MTU_SIZE - ieee1905_1::CmduMessage::kCmduHeaderLength;

/**
 * @brief convert integer to hexadecimal string
 *
 * @param integer integer to convert
 * @param number_of_digits  represent how much digits the number should have, so the function will
 *                          pad the number with zeroes from left, if necessary.
 *                          for example: int_to_hex_string(255, 4) -> "00ff"
 * @return std::string hexadecimal representation of 'integer'
 */
std::string int_to_hex_string(const unsigned int integer, const uint8_t number_of_digits);

/**
 * @brief Converts a mac address to a human-readable formatted string
 *
 * @pre The argument may not be NULL (nullptr)
 * @pre The argument must be in big endian byte order (network byte order)
 * @pre The argument should be 6 bytes long (i.e. it should be a valid MAC address)
 * @param[in] mac_address mac address to convert (uint8_t array)
 * @return The mac address in a column-separated hex format, e.g. 45:67:89:AB:CD:EF
 *
 */
std::string mac_to_string(const uint8_t *mac_address);

/**
 * @brief Converts a mac address to a human-readable formatted string
 *
 * @param mac_address mac address to convert (sMacAddr)
 * @return std::string of the format xx:xx:xx:xx:xx:xx
 */

std::string mac_to_string(const sMacAddr &mac);

/**
 * @brief Converts a string to a binary MAC address (6 bytes buffer)
 *
 * The string may be in one of the following formats (X is an upper or lower case hex digit):
 * @li XX:XX:XX:XX:XX:XX
 * @li 0xXXXXXXXXXX
 * @li XXXXXXXXXX
 * @li empty string (returns all-0 MAC address)
 *
 * @param[OUT] buf output buffer (has to be 6 bytes long)
 * @param[IN] mac string to convert
 * @return false if conversion failed - @a buf is reset to 0 in this case
 */
bool mac_from_string(uint8_t *buf, const std::string &mac);

/**
 * @brief Converts a string representing a MAC address to sMacAddr
 *
 * The string may be in one of the following formats (X is an upper or lower case hex digit):
 * @li XX:XX:XX:XX:XX:XX
 * @li 0xXXXXXXXXXX
 * @li XXXXXXXXXX
 *
 * @param mac mac std::string to convert
 * @return sMacAddr converted mac address
 */
sMacAddr mac_from_string(const std::string &mac);

/**
 * @brief Copy a MAC address from a sMacAddr object into an array.
 * @param mac sMacAddr object to copy
 * @param array Destination array
 */
void mac_to_array(const sMacAddr &mac, uint8_t array[sizeof(sMacAddr::oct)]);

/**
 * @brief Copy a MAC address from an array into a sMacAddr object.
 * @param array Array to copy
 * @param mac Destination sMacAddr object
 */
void mac_from_array(const uint8_t array[sizeof(sMacAddr::oct)], sMacAddr &mac);

/**
 * @brief Create an sMacAddr object from an array.
 * @param array Array to copy
 * @return sMacAddr object with the same MAC address
 */
sMacAddr mac_from_array(const uint8_t array[sizeof(sMacAddr::oct)]);

} // namespace tlvf

inline std::ostream &operator<<(std::ostream &os, const sMacAddr &addr)
{
    return os << tlvf::mac_to_string(addr);
}

inline el::base::MessageBuilder &operator<<(el::base::MessageBuilder &log, const sMacAddr &addr)
{
    return log << tlvf::mac_to_string(addr);
}

inline bool operator==(sMacAddr const &lhs, sMacAddr const &rhs)
{
    return (0 == std::memcmp(lhs.oct, rhs.oct, sizeof(sMacAddr)));
}

inline bool operator!=(sMacAddr const &lhs, sMacAddr const &rhs) { return !(rhs == lhs); }

namespace std {
template <> struct hash<sMacAddr> {
    size_t operator()(const sMacAddr &m) const
    {
        uint64_t value_to_hash = 0;
        for (size_t byte = 0; byte < sizeof(m.oct); byte++) {
            value_to_hash <<= 8;
            value_to_hash += m.oct[byte];
        }
        return hash<std::uint64_t>()(value_to_hash);
    }
};
} // namespace std

#endif
