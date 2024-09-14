/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_BRIDGE_STATE_MONITOR_H_
#define BCL_NETWORK_BRIDGE_STATE_MONITOR_H_

#include <functional>
#include <string>

namespace beerocks {
namespace net {

class BridgeStateMonitor {
public:
    /**
     * Network bridge state-changed handler function.
     *
     * @param bridge_name Bridge name.
     * @param iface_name Interface name.
     * @param iface_in_bridge Flag that signals with true that the interface has been added to the
     * bridge and with false if the interface has been removed from the bridge.
     */
    using StateChangedHandler = std::function<void(
        const std::string &bridge_name, const std::string &iface_name, bool iface_in_bridge)>;

    /**
     * @brief Class destructor
     */
    virtual ~BridgeStateMonitor() = default;

    /**
     * @brief Sets the state-changed event handler function.
     *
     * Sets the callback function to handle bridge state changes.
     * Use nullptr to remove previously installed callback function.
     *
     * If a handler is set, it will be called back whenever a bridge state changes, either because
     * a new interface is added to the bridge or because an interface is removed from the bridge.
     *
     * @param handler State change handler function (or nullptr).
     */
    void set_handler(const StateChangedHandler &handler) { m_handler = handler; }

    /**
     * @brief Clears previously set state-changed event handler function.
     *
     * Clears callback function previously set.
     * Behaves like set_handler(nullptr)
     */
    void clear_handler() { m_handler = nullptr; }

protected:
    /**
     * @brief Notifies a network bridge state-changed event.
     *
     * @param bridge_name Bridge name.
     * @param iface_name Interface name.
     * @param iface_in_bridge Flag that signals with true that the interface has been added to the
     * bridge and with false if the interface has been removed from the bridge.
     */
    void notify_state_changed(const std::string &bridge_name, const std::string &iface_name,
                              bool iface_in_bridge) const
    {
        if (m_handler) {
            m_handler(bridge_name, iface_name, iface_in_bridge);
        }
    }

private:
    /**
     * Network bridge state-changed handler function that is called back whenever a bridge state
     * changes.
     */
    StateChangedHandler m_handler;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_BRIDGE_STATE_MONITOR_H_ */
