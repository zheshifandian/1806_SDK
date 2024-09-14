/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/bridge_state_monitor_impl.h>

#include <linux/rtnetlink.h>
#include <net/if.h>

using namespace beerocks;

namespace beerocks {
namespace net {

BridgeStateMonitorImpl::BridgeStateMonitorImpl(
    std::shared_ptr<NetlinkEventListener> netlink_event_listener)
    : m_netlink_event_listener(netlink_event_listener)
{
    m_handler_id = m_netlink_event_listener->register_handler(
        [this](const nlmsghdr *msg_hdr) { handle_netlink_event(msg_hdr); });
}

BridgeStateMonitorImpl::~BridgeStateMonitorImpl()
{
    m_netlink_event_listener->remove_handler(m_handler_id);
}

void BridgeStateMonitorImpl::handle_netlink_event(const nlmsghdr *msg_hdr)
{
    switch (msg_hdr->nlmsg_type) {
    case RTM_NEWLINK:
    case RTM_DELLINK:
        const ifinfomsg *ifi = static_cast<const ifinfomsg *>(NLMSG_DATA(msg_hdr));

        if ((ifi->ifi_family != AF_BRIDGE) && (ifi->ifi_family != AF_UNSPEC)) {
            break;
        }

        int length = msg_hdr->nlmsg_len;
        length -= NLMSG_LENGTH(sizeof(ifinfomsg));
        if (length < 0) {
            break;
        }

        const rtattr *attribute = IFLA_RTA(ifi);
        while (RTA_OK(attribute, length)) {

            if (attribute->rta_type == IFLA_MASTER) {
                uint32_t bridge_index = *reinterpret_cast<uint32_t *>(RTA_DATA(attribute));
                char bridge_name[IFNAMSIZ]{};

                uint32_t iface_index = ifi->ifi_index;
                char iface_name[IFNAMSIZ]{};

                if ((0 != if_indextoname(bridge_index, bridge_name)) &&
                    (0 != if_indextoname(iface_index, iface_name))) {
                    notify_state_changed(bridge_name, iface_name,
                                         (msg_hdr->nlmsg_type == RTM_NEWLINK));
                }

                break;
            }

            attribute = RTA_NEXT(attribute, length);
        }

        break;
    }
}

} // namespace net
} // namespace beerocks
