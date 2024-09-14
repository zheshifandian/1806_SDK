/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_NETLINK_EVENT_LISTENER_H_
#define BCL_NETWORK_NETLINK_EVENT_LISTENER_H_

#include <linux/netlink.h>

#include <functional>
#include <unordered_map>

namespace beerocks {
namespace net {

class NetlinkEventListener {
public:
    /**
     * Netlink event handler function.
     *
     * @param msg_hdr Netlink message header struct containing Netlink event.
     */
    using NetlinkEventHandler = std::function<void(const nlmsghdr *msg_hdr)>;

    /**
     * @brief Class destructor
     */
    virtual ~NetlinkEventListener() = default;

    /**
     * @brief Sets the Netlink event handler function.
     *
     * The handler function will be called back whenever a Netlink event occurs. It must parse the
     * Netlink message passed in as parameter to obtain the data it is interested in.
     *
     * @param handler Netlink event handler function.
     * @return Handler unique identifier (required to remove handler later, when not needed any
     * more).
     */
    uint32_t register_handler(const NetlinkEventHandler &handler)
    {
        uint32_t handler_id = m_next_handler_id;
        m_next_handler_id++;

        m_handlers[handler_id] = handler;

        return handler_id;
    }

    /**
     * @brief Remove previously registered Netlink event event handler function.
     *
     * @param handler_id Handler identifier of the handler to remove and that was obtained when
     * handler was registered.
     */
    bool remove_handler(uint32_t handler_id)
    {
        auto it = m_handlers.find(handler_id);
        if (it == m_handlers.end()) {
            return false;
        }

        m_handlers.erase(it);

        return true;
    }

protected:
    /**
     * @brief Notifies a Netlink event by invoking all registered handlers.
     *
     * @param msg_hdr Netlink message header struct containing Netlink event.
     */
    void notify_netlink_event(const nlmsghdr *msg_hdr) const
    {
        for (const auto &entry : m_handlers) {
            auto handler = entry.second;

            if (handler) {
                handler(msg_hdr);
            }
        }
    }

private:
    /**
     * Map containing the Netlink event handler functions that are called back whenever a Netlink
     * event occurs.
     * The map key is the handler unique identifier and the value is the handler function.
     */
    std::unordered_map<uint32_t, NetlinkEventHandler> m_handlers;

    /**
     * Next handler identifier to be used. Its value gets incremented each time a new handler is
     * registered.
     */
    uint32_t m_next_handler_id = 0;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_NETLINK_EVENT_LISTENER_H_ */
