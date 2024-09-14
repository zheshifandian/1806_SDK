/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BPL_DHCP_H_
#define _BPL_DHCP_H_

#include "bpl.h"
#include <bcl/network/network_utils.h>
#include <tlvf/common/sMacAddr.h>

#include <chrono>
#include <string>
#include <unordered_map>

namespace beerocks {
namespace bpl {

/****************************************************************************/
/******************************* Definitions ********************************/
/****************************************************************************/

// Manual DHCP Lease Data Templates
// TODO: There is already sIpv4Addr in bcl, but not reachable in bpl.
// It can be carried under tlvf/common and used in bpl.
// Also, introduce sIpv6Addr struct.
struct sIPv4Lease {
    std::string ip_address;
    std::string host_name;
    std::chrono::milliseconds validity{0};

    sIPv4Lease() {}
    sIPv4Lease(const std::string &ip_address_, const std::string &host_name_)
        : ip_address(ip_address_), host_name(host_name_)
    {
    }
};

typedef std::unordered_map<sMacAddr, sIPv4Lease> ipv4_lease_map_t;

struct sIPv6Lease {
    sMacAddr mac{beerocks::net::network_utils::ZERO_MAC};
    std::string ip_address;
    std::string host_name;
    std::chrono::milliseconds validity{0};

    sIPv6Lease() {}
    sIPv6Lease(const sMacAddr &mac_, const std::string &ip_address_, const std::string &host_name_)
        : mac(mac_), ip_address(ip_address_), host_name(host_name_)
    {
    }
};

// Key for IPv6 leases are DUID.
typedef std::unordered_map<std::string, sIPv6Lease> ipv6_lease_map_t;
typedef std::pair<ipv4_lease_map_t, ipv6_lease_map_t> leases_pair_t;

/*
Example for sending an event using the "ubus" shell command ubus call:
  dhcp_event notify '{ "id": 1234, "op": "add", "mac": "11:22:33:44:55:66",
                       "ip": "1.1.1.1", "hostname": "test-hostname" }'
*/

/**
 * DHCP Monitor Event Callback
 *
 * @param [in] op Operation string (add, del etc.)
 * @param [in] mac Client's MAC address
 * @param [in] ip Client's IP address
 * @param [in] hostname Client's host name
 */
typedef void (*dhcp_mon_cb)(const char *op, const char *mac, const char *ip, const char *hostname);

/****************************************************************************/
/******************************** Functions *********************************/
/****************************************************************************/

/**
 * Start the DHCP monitor.
 *
 * @param [in] cb Callback function for DHCP events.
 *
 * @return File descriptor to the socket used for monitoring the UBUS.
 * @return -1 Error.
 */
int dhcp_mon_start(dhcp_mon_cb cb);

/**
 * Handle UBUS event.
 * This function should be called when there's data to be read from
 * from the UBUS socket.
 *
 * @return 0 On success of -1 on failure.
 */
int dhcp_mon_handle_event();

/**
 * Stop the DHCP monitor.
 *
 * @return 0 On success of -1 on failure.
 */
int dhcp_mon_stop();

/**
 * @brief Initialize manual procedure for reading leases.
 *
 * For example, in case of luci-rpc/openwrt platforms
 * It inits ubus context to enable manual ubus calls.
 * For failed initialization, manual ubus calls would also fail.
 *
 * @return Returns true in case of success.
 */
bool dhcp_manual_procedure_init();

/**
 * @brief Stops and free resources in manual procedure for reading leases.
 *
 * @return Returns true in case of success.
 */
bool dhcp_manual_procedure_destroy();

/**
 * @brief Get dhcp leases from lucy-rpc via ubus.
 *
 * @param [out] leases_map dhcp lease map which is pair of ipv4 and ipv6.
 * @return Returns true in case of success.
 */
bool dhcp_get_leases(leases_pair_t &leases_map);

} // namespace bpl
} // namespace beerocks

#endif // _BPL_DHCP_H_
