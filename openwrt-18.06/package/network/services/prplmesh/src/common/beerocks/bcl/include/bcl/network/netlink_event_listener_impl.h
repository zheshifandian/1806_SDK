/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_NETLINK_EVENT_LISTENER_IMPL_H_
#define BCL_NETWORK_NETLINK_EVENT_LISTENER_IMPL_H_

#include "netlink_event_listener.h"
#include "sockets_impl.h"

#include <memory>

namespace beerocks {

class EventLoop;

namespace net {

class NetlinkEventListenerImpl : public NetlinkEventListener {
public:
    /**
     * @brief Class constructor
     *
     * @param connection Netlink socket connection for kernel/user-space communication.
     * @param event_loop Event loop to wait for I/O events.
     */
    NetlinkEventListenerImpl(std::shared_ptr<Socket::Connection> connection,
                             std::shared_ptr<EventLoop> event_loop);

    /**
     * @brief Class destructor
     */
    ~NetlinkEventListenerImpl() override;

private:
    /**
     * Socket connection through which interface state information is received.
     */
    std::shared_ptr<Socket::Connection> m_connection;

    /**
     * Application event loop used by the monitor to wait for I/O events.
     */
    std::shared_ptr<EventLoop> m_event_loop;

    /**
     * @brief Parses data received through the Netlink socket connection.
     *
     * The array of bytes contains a list of Netlink messages. This method notifies a Netlink event
     * for each one of those messages. The handler functions must parse the Netlink message to
     * obtain the data they are interested in.
     *
     * @param buffer Buffer with the array of Netlink messages to parse.
     */
    void parse(const Buffer &buffer) const;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_NETLINK_EVENT_LISTENER_IMPL_H_ */
