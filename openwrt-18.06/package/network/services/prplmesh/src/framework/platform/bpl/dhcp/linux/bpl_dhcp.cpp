/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bpl/bpl_dhcp.h>
#include <tlvf/tlvftypes.h>

namespace beerocks {
namespace bpl {

struct dummy_lease {
    const char *mac;
    const char *hostname;
    const char *ipv4;
    const char *ipv6;
};

// STA informations needs to match with Boardfarm configuration.
// tests/boardfarm_plugins/boardfarm_prplmesh/prplmesh_config.json
constexpr std::array<dummy_lease, 3> dummy_leases = {
    {{"51:a1:10:20:00:01", "wifi", "192.168.100.101", "fe80::0000:0000:0000:0101"},
     {"51:a1:10:20:00:02", "wifi2", "192.168.100.102", "fe80::0000:0000:0000:0102"},
     {"51:a1:10:20:00:03", "wifi3", "192.168.100.103", "fe80::0000:0000:0000:0103"}}};

int dhcp_mon_start(dhcp_mon_cb cb) { return -2; }

int dhcp_mon_handle_event() { return 0; }

int dhcp_mon_stop() { return 0; }

bool dhcp_manual_procedure_init() { return true; }

bool dhcp_manual_procedure_destroy() { return true; }

bool dhcp_get_leases(leases_pair_t &leases_map)
{
    leases_map.first.clear();
    leases_map.second.clear();

    // It is filled with dummy data to enable Boardfarm testing.
    for (const auto &lease : dummy_leases) {

        sIPv4Lease ipv4_lease{lease.ipv4, lease.hostname};
        leases_map.first.insert(std::make_pair(tlvf::mac_from_string(lease.mac), ipv4_lease));

        // Use MAC address instead of DUID.
        sIPv6Lease ipv6_lease{tlvf::mac_from_string(lease.mac), lease.ipv6, lease.hostname};
        leases_map.second.insert(std::make_pair(lease.mac, ipv6_lease));
    }

    return true;
}

} // namespace bpl
} // namespace beerocks
