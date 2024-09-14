/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _NETWORK_UTILS_H_
#define _NETWORK_UTILS_H_

#include "../beerocks_os_utils.h"
#include "socket.h"

#include "net_struct.h"
#include <cstdint>
#include <list>
#include <string>
#include <vector>

#include <netinet/ether.h>

#define ETH_HDRLEN 14 // Ethernet header length
#define IP4_HDRLEN 20 // IPv4 header length
#define ARP_HDRLEN 28 // ARP header length
// Define some constants.
#ifndef ARPOP_REQUEST
#define ARPOP_REQUEST 1 // Taken from <linux/if_arp.h>
#define ARPOP_REPLY 2
#endif

namespace beerocks {
namespace net {

constexpr uint16_t MIN_VLAN_ID = 1;
constexpr uint16_t MAX_VLAN_ID = 4094;

// According to 802.11-2016 convertion table (Table 9-154).

class network_utils {
public:
    static const std::string ZERO_IP_STRING;
    static const std::string ZERO_MAC_STRING;
    static const sMacAddr ZERO_MAC;
    static const std::string WILD_MAC_STRING;
    static const sMacAddr MULTICAST_1905_MAC_ADDR;

    typedef struct {
        uint16_t htype;
        uint16_t ptype;
        uint8_t hlen;
        uint8_t plen;
        uint16_t opcode;
        uint8_t sender_mac[6];
        uint8_t sender_ip[4];
        uint8_t target_mac[6];
        uint8_t target_ip[4];
    } arp_hdr;

    typedef struct {
        std::string iface;
        uint32_t iface_idx;
        std::string connection_name;
        std::string mac;
        std::string ip;
        std::string netmask;
        std::string broadcast_ip;
        std::string gw;
    } ip_info;

    typedef struct {
        std::string iface;
        std::string mac;
        std::string ip;
        std::string netmask;
        std::string ip_gw;
        std::string broadcast_ip;
    } iface_info;

    typedef struct {
        struct ether_addr hwa;
        struct in_addr ipa;
        struct in_addr bcast;
        struct in_addr nmask;
    } raw_iface_info;

    static bool is_valid_mac(std::string mac);

    static std::string ipv4_to_string(const net::sIpv4Addr &ip);
    static std::string ipv4_to_string(const uint8_t *ipv4);
    static std::string ipv4_to_string(uint32_t ip);
    static net::sIpv4Addr ipv4_from_string(const std::string &ip_str);
    static void ipv4_from_string(uint8_t *buf, const std::string &ip);
    static uint32_t uint_ipv4_from_array(void *ip);
    static uint32_t uint_ipv4_from_string(const std::string &ip_str);

    static std::vector<network_utils::ip_info> get_ip_list();
    static int get_iface_info(network_utils::iface_info &info, const std::string &iface_name);
    static bool get_raw_iface_info(const std::string &iface_name, raw_iface_info &info);

    /**
     * @brief Get the arp table as unordered map object of pairs <MAC, IP> or <IP, MAC>.
     * The key of the map can be either a MAC address or an IP address, depends on the 'mac_as_key'
     * argument value.
     * If 'mac_as_key' is 'true' then the key is a MAC address, otherwise the key is an IP address.
     *
     * @param[in] mac_as_key Decide whether the key of the returned unordered map object
     * is a MAC or an IP.
     * @return std::shared_ptr<std::unordered_map<std::string, std::string>>
     */
    static std::shared_ptr<std::unordered_map<std::string, std::string>>
    get_arp_table(bool mac_as_key = true);

    //temp
    static std::string get_mac_from_arp_table(const std::string &ipv4);

    /**
     * @brief Get list of linux interfaces.
     *
     * @return List of linux interfaces.
     */
    static std::list<std::string> linux_get_iface_list();

    static std::vector<std::string> linux_get_iface_list_from_bridge(const std::string &bridge);

    /**
     * @brief Gets the interface index corresponding to a particular name.
     *
     * @param iface_name The name of the network interface.
     * @return interface index or 0 if no interface exists with the name given.
     */
    static uint32_t linux_get_iface_index(const std::string &iface_name);

    /**
     * @brief Gets the interface name corresponding to a particular index.
     *
     * @param iface_index The index of the network interface.
     * @return interface name or empty string if no interface exists with the index given.
     */
    static std::string linux_get_iface_name(uint32_t iface_index);

    static bool linux_add_iface_to_bridge(const std::string &bridge, const std::string &iface);
    static bool linux_remove_iface_from_bridge(const std::string &bridge, const std::string &iface);
    static bool linux_iface_ctrl(const std::string &iface, bool up, std::string ip = "",
                                 const std::string &netmask = "");
    static bool linux_iface_get_mac(const std::string &iface, std::string &mac);

    /**
     * @brief Gets the interface name of the network interface with given MAC address.
     *
     * @param[in] mac MAC address of the network interface.
     * @param[out] iface On success, name of the network interface. On error, empty string.
     *
     * @return True on success and false otherwise.
     */
    static bool linux_iface_get_name(const sMacAddr &mac, std::string &iface);

    static bool linux_iface_get_ip(const std::string &iface, std::string &ip);
    static bool linux_iface_get_pci_info(const std::string &iface, std::string &pci_id);
    static bool linux_iface_exists(const std::string &iface);
    static bool linux_iface_is_up(const std::string &iface);
    static bool linux_iface_is_up_and_running(const std::string &iface);

    /**
     * @brief Gets the current link speed and the maximum advertised speed
     * of a network interface.
     *
     * @param[in] iface Name of the network interface.
     * @param[out] link_speed On success, current link speed in Mbps of the network interface
     * as defined in SPEED_* macros included in ethtool.h.
     * It equals SPEED_UNKNOWN if the interface's link is down.
     * @param[out] max_advertised_speed On success, maximum advertised speed in Mbps of the network interface
     * as defined in SPEED_* macros included in ethtool.h.
     *
     * @return True if speed could be successfully obtained and false otherwise.
     */
    static bool linux_iface_get_speed(const std::string &iface, uint32_t &link_speed,
                                      uint32_t &max_advertised_speed);

    /**
     * @brief Gets interface statistics for the given network interface.
     *
     * @param[in] iface Name of the local network interface.
     * @param[out] iface_stats Interface statistics.
     *
     * @return True on success and false otherwise.
     */
    static bool get_iface_stats(const std::string &iface, sInterfaceStats &iface_stats);

    static bool arp_send(const std::string &iface, const std::string &dst_ip,
                         const std::string &src_ip, sMacAddr dst_mac, sMacAddr src_mac, int count,
                         int arp_socket = -1);

    static bool icmp_send(const std::string &ip, uint16_t id, int count, int icmp_socket);
    static uint16_t icmp_checksum(uint16_t *buf, int32_t len);

    /**
     * @brief Get list of BSS interfaces.
     *
     * A BSS could have more than one interface that belongs to it. Specifically, when configuring
     * a BSS as bBSS, a platform could create several virtual netdevs of which only one backhaul
     * station could connect.
     * This function returns a list of the base BSS interface name and all of its
     * extended interfaces.
     *
     * @param bss_iface BSS interface name.
     * @param bridge_iface Bridge interface name.
     * @return List of the base BSS interface name and all of its extended interfaces.
     */
    static std::vector<std::string> get_bss_ifaces(const std::string &bss_iface,
                                                   const std::string &bridge_iface);

    /**
     * @brief Create a VLAN interface.
     *
     * The function creates a new VLAN interface with ID @a vid.
     * The parrten of the interface name if the @a suffix is empty is:
     * "<iface>.<vid>"", otherwise it is "<iface>.<suffix>".
     *
     * @param iface Interface to attach the VLAN interface to.
     * @param vid VLAN ID.
     * @param suffix VLAN interface suffix.
     * @return New interface name.
     */
    static std::string create_vlan_interface(const std::string &iface, uint16_t vid,
                                             const std::string &suffix = {});

    /**
     * @brief Deletes network interface.
     *
     * @param iface Interface to delete.
     */
    static void delete_interface(const std::string &iface);

    /**
     * @brief sets interface state (up/down)
     *
     * @param iface Interface.
     * @param is_up true up false down.
     */
    static void set_interface_state(const std::string &iface, bool is_up);

    /**
     * @brief Enable or disable "vlan_filtering" on the bridge.
     *
     * @param default_vlan_id If 'default_vlan_id is not zero, turn on "vlan_filtering" and set the
     * given value as the default VLAN which will be set on all interfaces in the bridge as
     * "PVID and Egress Untagged".
     * If 'default_vlan_id is zero the "vlan_filtering" is disabled.
     * @return true on success, false otherwise.
     */
    static bool set_vlan_filtering(const std::string &bridge_iface, uint16_t default_vlan_id);

    /**
     * @brief Set a specific @a VID policy on a bridged interface - @a 'iface'.
     *
     * @param iface Bridged interface to set the VLAN ID policy on.
     * @param del If true, remove the VLAN ID policy from the given interface. Also, optional
     * arguments are irrelevant in that case.
     * @param vid VLAN ID to set/remove, if the given value is '0' apply for all possible VIDs.
     * @param is_bridge Whether the given interface is a bridge interface or not.
     * @param pvid If true, apply PVID policy on the given @a VID.
     * @param untagged If true, apply Egress Untagged policy on the given @a VID.
     * @return true on success, false otherwise.
     */
    static bool set_iface_vid_policy(const std::string &iface, bool del, uint16_t vid,
                                     bool is_bridge, bool pvid = false, bool untagged = false);

    /**
     * @brief Filter (or Remove Filter) packets containing a given VLAN ID and double-tagged packets
     * with S-Tag, by adding new rules to the nat table.
     *
     * @param set If true, set the filter, otherwise clear it.
     * @param bss_iface An interface name to apply the rule on.
     * @param vid VLAN IDs. If zero (default value), only filter double-tagged packets.
     * @return true on success, false otherwise.
     */
    static bool set_vlan_packet_filter(bool set, const std::string &bss_iface, uint16_t vid = 0);

    /**
     * @brief Generate a locally administrated mac address
     * for ethernet switch node (eth backhaul) derived from the bridge mac address.
     *
     * If the bridge mac is already a locally administrated address (having U/L bit set)
     * then swap the bridge mac bytes, before setting the U/L bit flag.
     *
     * @param bridgeMac the bridge mac address value.
     * @return generated mac address value.
     */
    static sMacAddr get_eth_sw_mac_from_bridge_mac(const sMacAddr &bridge_mac);
};
} // namespace net
} // namespace beerocks

#endif //_NETWORK_UTILS_H_
