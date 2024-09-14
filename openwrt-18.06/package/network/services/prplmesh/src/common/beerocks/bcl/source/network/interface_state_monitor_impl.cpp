/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/interface_state_monitor_impl.h>

#include <linux/rtnetlink.h>
#include <net/if.h>

using namespace beerocks;

namespace beerocks {
namespace net {

InterfaceStateMonitorImpl::InterfaceStateMonitorImpl(
    std::shared_ptr<NetlinkEventListener> netlink_event_listener)
    : m_netlink_event_listener(netlink_event_listener)
{
    m_handler_id = m_netlink_event_listener->register_handler(
        [this](const nlmsghdr *msg_hdr) { handle_netlink_event(msg_hdr); });
}

InterfaceStateMonitorImpl::~InterfaceStateMonitorImpl()
{
    m_netlink_event_listener->remove_handler(m_handler_id);
}

void InterfaceStateMonitorImpl::handle_netlink_event(const nlmsghdr *msg_hdr)
{
    switch (msg_hdr->nlmsg_type) {
    case RTM_NEWLINK:
    case RTM_DELLINK:
        const ifinfomsg *ifi = static_cast<const ifinfomsg *>(NLMSG_DATA(msg_hdr));

        uint32_t iface_index = ifi->ifi_index;
        bool iface_state     = (ifi->ifi_flags & IFF_UP) && (ifi->ifi_flags & IFF_RUNNING);

        char iface_name[IFNAMSIZ]{};
        if (0 != if_indextoname(iface_index, iface_name)) {
            notify_state_changed(iface_name, iface_state);
        }

        break;
    }
}

} // namespace net
} // namespace beerocks
