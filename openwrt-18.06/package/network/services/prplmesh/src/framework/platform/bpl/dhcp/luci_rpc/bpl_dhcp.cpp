/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2019-2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */
#include <bcl/beerocks_os_utils.h>
#include <bcl/beerocks_string_utils.h>
#include <bpl/bpl_dhcp.h>
#include <tlvf/tlvftypes.h>

#include <fcntl.h>
#include <netinet/in.h>

extern "C" {
// Ignore some warnings from libubus
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <libubox/blobmsg.h>
#include <libubus.h>
}

const unsigned UBUS_CALL_TIMEOUT_MS = 100;
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

namespace beerocks {
namespace bpl {

static ubus_context *s_pUbusCtx = nullptr;

// Link local address map filled from "ip -6 n" system call.
static std::unordered_map<std::string, sMacAddr> ipv6_neighbors;

// lucy-rpc dhcp leases policy
enum {
    DHCP_IPV4_LEASES,
    DHCP_IPV6_LEASES,
};

static const blobmsg_policy dhcp_lease_policy[] = {
    [DHCP_IPV4_LEASES] = {.name = "dhcp_leases", .type = BLOBMSG_TYPE_ARRAY},
    [DHCP_IPV6_LEASES] = {.name = "dhcp6_leases", .type = BLOBMSG_TYPE_ARRAY},
};

// lucy-rpc dhcp ipv4 lease policy
enum {
    IPV4_EXPIRES,
    IPV4_HOSTNAME,
    IPV4_MAC_ADDRESS,
    IPV4_DUID,
    IPV4_IP_ADDRESS,
};

static const blobmsg_policy ipv4_lease_policy[] = {
    [IPV4_EXPIRES]     = {.name = "expires", .type = BLOBMSG_TYPE_INT32},
    [IPV4_HOSTNAME]    = {.name = "hostname", .type = BLOBMSG_TYPE_STRING},
    [IPV4_MAC_ADDRESS] = {.name = "macaddr", .type = BLOBMSG_TYPE_STRING},
    [IPV4_DUID]        = {.name = "duid", .type = BLOBMSG_TYPE_STRING},
    [IPV4_IP_ADDRESS]  = {.name = "ipaddr", .type = BLOBMSG_TYPE_STRING}};

// lucy-rpc dhcp ipv6 lease policy
enum {
    IPV6_EXPIRES,
    IPV6_HOSTNAME,
    IPV6_DUID,
    IPV6_IP_ADDRESS,
    IPV6_IP_ADDRESSES,
};

static const blobmsg_policy ipv6_lease_policy[] = {
    [IPV6_EXPIRES]      = {.name = "expires", .type = BLOBMSG_TYPE_INT32},
    [IPV6_HOSTNAME]     = {.name = "hostname", .type = BLOBMSG_TYPE_STRING},
    [IPV6_DUID]         = {.name = "duid", .type = BLOBMSG_TYPE_STRING},
    [IPV6_IP_ADDRESS]   = {.name = "ip6addr", .type = BLOBMSG_TYPE_STRING},
    [IPV6_IP_ADDRESSES] = {.name = "ip6addrs", .type = BLOBMSG_TYPE_ARRAY}};

int dhcp_mon_start(dhcp_mon_cb cb) { return -2; }

int dhcp_mon_handle_event() { return 0; }

int dhcp_mon_stop() { return 0; }

bool dhcp_manual_procedure_init()
{
    // Prevent multiple instances.
    if (s_pUbusCtx) {
        return true;
    }

    // Use default ubus socket
    s_pUbusCtx = ubus_connect(NULL);

    if (!s_pUbusCtx) {
        LOG(ERROR) << "ubus_connect() is failed";
        return false;
    }
    return true;
}

bool dhcp_manual_procedure_destroy()
{
    if (s_pUbusCtx) {
        ubus_free(s_pUbusCtx);
        s_pUbusCtx = nullptr;
    }

    return true;
}

/**
 * @brief IPv6 neighbor discovery handler.
 *
 * It process outputs of "ip -6 n" system call output.
 * Successfull discoveries added to ipv6_neighbors.
 *
 * Example output of the command:
 * fd44:3490:d13d:4::644 dev br-lan  used 124/35748/121 probes 6 FAILED
 * fd44:3490:d13d:4::d2e dev br-lan lladdr 18:26:49:c4:b9:d6 used 430/430/386 probes 1 STALE
 *
 * First parameter of its output is ipv6 address.
 * Parameter of one after lladdr name tag contains MAC Address.
 *
 * @return Returns true in case of non-empty neighbor list.
 */
static bool ipv6_discovery_handling()
{
    auto ret_str = beerocks::os_utils::system_call_with_output("ip -6 neigh");

    if (ret_str.empty()) {
        return false;
    }

    ipv6_neighbors.clear();
    auto ipv6_discoveries = beerocks::string_utils::str_split(ret_str, '\n');

    // Parse Lines for each discovery.
    for (const auto &discovery : ipv6_discoveries) {

        // Parse discovery members.
        auto discovery_entries = beerocks::string_utils::str_split(discovery, ' ');

        // Process each entry/line.
        std::string ipv6_addr, mac;
        for (auto itr = discovery_entries.cbegin(); itr < discovery_entries.cend(); itr++) {

            // First Member is IPv6 address.
            if (itr == discovery_entries.cbegin()) {
                ipv6_addr = *itr;
                continue;
            }

            // Link local address (MAC) name tag and one after MAC is placed.
            if (*itr == "lladdr") {
                itr++;
                if (itr != discovery_entries.cend()) {
                    mac = *itr;
                }
                break;
            }
        }

        if (!mac.empty()) {
            ipv6_neighbors.insert(std::make_pair(ipv6_addr, tlvf::mac_from_string(mac)));
        }
    }

    return (!ipv6_neighbors.empty());
}

/**
 * @brief IPv6 discovery triggering to specifed ipv6 address.
 *
 * To understand DHCP IPv6 leases and their correct link local (MAC) address,
 * there should be IPv6 communication between server and client.
 *
 * It can be either triggered with PING or simple connection request to the device.
 * In this method triggering is done via connecting a random port of the device.
 * Even if the port does not exist, discovery is started.
 * Port connection timeout is very low (100 us) to prevent time loss.
 * Select method wait until connection succeeds or timeout.
 *
 * If device is discovered, IPv6 address and corresponding MAC address appears on
 * "ip -6 n" command output. Check "ipv6_discovery_handling()" for details.
 *
 * @param [in] ipv6_address ipv6 address to be trigger for discovery.
 */
static void ipv6_discovery_trigger(const std::string &ipv6_address)
{
    sockaddr_in6 addr;
    timeval tv;
    fd_set fdset;

    auto soc         = socket(AF_INET6, SOCK_STREAM, 0);
    addr.sin6_family = AF_INET6;
    addr.sin6_port   = htons(5000); // Random port number.

    int ret_val = inet_pton(AF_INET6, ipv6_address.c_str(), &addr.sin6_addr);
    if (ret_val != 1) {
        LOG(ERROR) << "Failed network address structure (inet_pton).";
        return;
    }

    ret_val = fcntl(soc, F_SETFL, O_NONBLOCK);
    if (ret_val == -1) {
        LOG(ERROR) << "File descriptor error (fcntl).";
        return;
    }

    connect(soc, (sockaddr *)&addr, sizeof(addr));

    FD_ZERO(&fdset);
    FD_SET(soc, &fdset);

    // Short timeout to trigger neighbor discovery.
    tv.tv_sec  = 0;
    tv.tv_usec = 100;

    select(soc + 1, NULL, &fdset, NULL, &tv);
    close(soc);
}

/**
 * @brief Lease data handler which passed as function callback to ubus_invoke.
 *
 * This handler proces luci-rpc getDHCPLeases methods output.
 * IPv4 and IPv6 leases are processed within this method.
 *
 * @param [in] req ubus request contains private pointer.
 * @param [in] type message type.
 * @param [in] msg data pointer which received from ubus.
 */
static void lease_data_handler(ubus_request *req, int type, blob_attr *msg)
{
    if (!msg || !req->priv) {
        return;
    }

    // Passed private pointer to get data from function callback.
    auto leases_map = static_cast<leases_pair_t *>(req->priv);
    leases_map->first.clear();
    leases_map->second.clear();

    blob_attr *tb[ARRAY_SIZE(dhcp_lease_policy)];

    if (blobmsg_parse(dhcp_lease_policy, ARRAY_SIZE(dhcp_lease_policy), tb, blobmsg_data(msg),
                      blobmsg_data_len(msg)) != 0) {
        LOG(ERROR) << "Parse failed with dhcp_lease_policy";
        return;
    }

    if (tb[DHCP_IPV4_LEASES]) {

        blob_attr *attr;
        auto head = blobmsg_data(tb[DHCP_IPV4_LEASES]);
        auto len  = blobmsg_data_len(tb[DHCP_IPV4_LEASES]);

        // Iterate over all IPv4 Leases
        __blob_for_each_attr(attr, head, len)
        {
            blob_attr *tb_ipv4[ARRAY_SIZE(ipv4_lease_policy)];

            if (blobmsg_parse(ipv4_lease_policy, ARRAY_SIZE(ipv4_lease_policy), tb_ipv4,
                              blobmsg_data(attr), blobmsg_data_len(attr)) != 0) {

                LOG(ERROR) << "Parse failed with ipv4_lease_policy";
                continue;
            }

            if (tb_ipv4[IPV4_MAC_ADDRESS] && tb_ipv4[IPV4_IP_ADDRESS]) {

                sMacAddr mac = tlvf::mac_from_string(blobmsg_get_string(tb_ipv4[IPV4_MAC_ADDRESS]));
                sIPv4Lease lease;

                lease.ip_address.assign(blobmsg_get_string(tb_ipv4[IPV4_IP_ADDRESS]));

                if (tb_ipv4[IPV4_HOSTNAME]) {
                    lease.host_name.assign(blobmsg_get_string(tb_ipv4[IPV4_HOSTNAME]));
                }
                if (tb_ipv4[IPV4_EXPIRES]) {
                    lease.validity =
                        std::chrono::milliseconds(blobmsg_get_u32(tb_ipv4[IPV4_EXPIRES]));
                }

                // In case of older lease given for same MAC, discard data.
                if (leases_map->first[mac].validity > lease.validity) {
                    continue;
                }
                leases_map->first[mac] = lease;
            }
        }
    }

    if (tb[DHCP_IPV6_LEASES]) {

        blob_attr *attr;
        auto head = blobmsg_data(tb[DHCP_IPV6_LEASES]);
        auto len  = blobmsg_data_len(tb[DHCP_IPV6_LEASES]);

        // Iterate over all IPv6 Leases
        __blob_for_each_attr(attr, head, len)
        {
            blob_attr *tb_ipv6[ARRAY_SIZE(ipv6_lease_policy)];

            if (blobmsg_parse(ipv6_lease_policy, ARRAY_SIZE(ipv6_lease_policy), tb_ipv6,
                              blobmsg_data(attr), blobmsg_data_len(attr)) != 0) {

                LOG(ERROR) << "Parse failed with ipv6_lease_policy";
                continue;
            }

            if (tb_ipv6[IPV6_DUID] && tb_ipv6[IPV6_IP_ADDRESS]) {

                std::string duid{blobmsg_get_string(tb_ipv6[IPV6_DUID])};
                sIPv6Lease lease;

                lease.ip_address.assign(blobmsg_get_string(tb_ipv6[IPV6_IP_ADDRESS]));

                if (tb_ipv6[IPV6_HOSTNAME]) {
                    lease.host_name.assign(blobmsg_get_string(tb_ipv6[IPV6_HOSTNAME]));
                }
                if (tb_ipv6[IPV6_EXPIRES]) {
                    lease.validity =
                        std::chrono::milliseconds(blobmsg_get_u32(tb_ipv6[IPV6_EXPIRES]));
                }

                // In case of older lease given for same DUID, discard data.
                if (leases_map->second[duid].validity > lease.validity) {
                    continue;
                }

                // Check IP address is discovered before.
                auto neighbor = ipv6_neighbors.find(lease.ip_address);

                if (neighbor != ipv6_neighbors.end()) {
                    lease.mac = neighbor->second;
                }

                leases_map->second[duid] = lease;
            }
        }

        // Process each ipv6 leases to find undiscovered entries.
        bool undiscovered_devices = false;
        for (const auto &ipv6_lease : leases_map->second) {

            if (ipv6_lease.second.mac == beerocks::net::network_utils::ZERO_MAC) {
                ipv6_discovery_trigger(ipv6_lease.second.ip_address);
                undiscovered_devices = true;
            }
        }

        // Process triggered discovery results. Unless, if there is undiscovered one.
        if (undiscovered_devices) {

            // TODO: Refactor to remove need of sleep (PPM-1382).
            // Wait until discoveries registered.
            UTILS_SLEEP_MSEC(20);
            if (!ipv6_discovery_handling()) {

                // Discovery list is empty or failed operation. Skip IPv6 leases.
                leases_map->second.clear();
                return;
            }

            // Iterations are not constant map members, because it changes the content.
            for (auto itr = leases_map->second.begin(); itr != leases_map->second.end();) {

                if (itr->second.mac == beerocks::net::network_utils::ZERO_MAC) {

                    // Find neighbor with that IPv6 address.
                    auto neighbor = ipv6_neighbors.find(itr->second.ip_address);

                    if (neighbor != ipv6_neighbors.end()) {
                        itr->second.mac = neighbor->second;
                    } else {
                        itr = leases_map->second.erase(itr);
                        continue;
                    }
                }
                itr++;
            }
        }
    }
    return;
}

bool dhcp_get_leases(leases_pair_t &leases_map)
{
    int ret_val;
    uint32_t id;

    // Check for successful init.
    if (!s_pUbusCtx) {
        LOG(ERROR) << "Uninitialized ubus context.";
        return false;
    }

    if (ubus_lookup_id(s_pUbusCtx, "luci-rpc", &id)) {
        LOG(ERROR) << "Failed to look up luci-rpc";
        return false;
    }

    ret_val = ubus_invoke(s_pUbusCtx, id, "getDHCPLeases", nullptr, lease_data_handler,
                          static_cast<void *>(&leases_map), UBUS_CALL_TIMEOUT_MS);

    return (ret_val == 0);
}

} // namespace bpl
} // namespace beerocks
