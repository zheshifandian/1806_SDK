/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_BRIDGE_STATE_MONITOR_IMPL_H_
#define BCL_NETWORK_BRIDGE_STATE_MONITOR_IMPL_H_

#include "bridge_state_monitor.h"
#include "netlink_event_listener.h"

#include <memory>

namespace beerocks {
namespace net {

class BridgeStateMonitorImpl : public BridgeStateMonitor {
public:
    /**
     * @brief Class constructor.
     *
     * Registers a handler in the given Netlink event listener to get notified of Netlink events
     * published by the kernel and extract information about bridge changes out of them.
     *
     * @param netlink_event_listener Netlink event listener to get notified of Netlink events.
     */
    explicit BridgeStateMonitorImpl(std::shared_ptr<NetlinkEventListener> netlink_event_listener);

    /**
     * @brief Class destructor
     */
    ~BridgeStateMonitorImpl() override;

private:
    /**
     * Netlink event listener to get notified of Netlink events.
     */
    std::shared_ptr<NetlinkEventListener> m_netlink_event_listener;

    /**
     * Handler identifier of the registered Netlink event handler function, required to remove it
     * on exit.
     */
    uint32_t m_handler_id;

    /**
     * @brief Netlink event handler function.
     *
     * Parses messages received through the Netlink socket connection to extract information about
     * bridge changes out of them.
     *
     * If the type of the Netlink message is RTM_NEWLINK or RTM_DELLINK then reads the interface
     * index and the IFLA_MASTER attribute and notifies a change in the bridge state.
     *
     * @param msg_hdr Netlink message header struct containing Netlink event.
     */
    void handle_netlink_event(const nlmsghdr *msg_hdr);
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_BRIDGE_STATE_MONITOR_IMPL_H_ */
