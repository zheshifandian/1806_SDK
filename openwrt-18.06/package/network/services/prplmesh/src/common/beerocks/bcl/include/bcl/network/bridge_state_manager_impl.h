/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_BRIDGE_STATE_MANAGER_IMPL_H_
#define BCL_NETWORK_BRIDGE_STATE_MANAGER_IMPL_H_

#include "bridge_state_manager.h"

#include <memory>
#include <unordered_map>

namespace beerocks {
namespace net {

/**
 * This class implements the BridgeStateManager facade interface in terms of (by delegating to) the
 * BridgeStateMonitor and BridgeStateReader interfaces and performs additional functionality
 * before/after forwarding requests.
 */
class BridgeStateManagerImpl : public BridgeStateManager {
public:
    /**
     * @brief Class constructor
     *
     * This implementation delegates BridgeStateMonitor and BridgeStateReader requests to given
     * reader and monitor instances respectively.
     *
     * A bridge state is defined by the list of network interfaces currently in the bridge.
     *
     * The bridge state monitor is used to monitor changes in the state of a network bridge in an
     * event-driven way, that is, without polling. The monitor will execute a callback handler
     * function whenever either a network interface is added to the bridge or removed from the
     * bridge.
     *
     * The bridge state reader is used to read the state of a network bridge when it is not known
     * yet (i.e. the first time the bridge state is queried).
     *
     * Installs a state-changed event handler on the monitor which stores received bridge state
     * information into the list of current interfaces for each known bridge. This way, when the
     * read_state() method is called, the cached state can be quickly returned instead of having
     * to query the network bridge using the reader.
     *
     * @param bridge_state_monitor Bridge state monitor.
     * @param bridge_state_reader Bridge state reader.
     */
    BridgeStateManagerImpl(std::unique_ptr<BridgeStateMonitor> bridge_state_monitor,
                           std::unique_ptr<BridgeStateReader> bridge_state_reader);

    /**
     * @brief Class destructor
     *
     * Removes the state-changed event handler installed on the monitor.
     */
    ~BridgeStateManagerImpl() override;

    /**
     * @brief Reads current state of a network bridge (i.e.: the list of network interfaces in the
     * bridge).
     *
     * The first time this method is called and since the bridge state is yet unknown, this class
     * delegates to BridgeStateReader::read_state and caches obtained state. From then on, cached
     * state is returned and the bridge reader is not used any more. Cached state is continuously
     * updated with information about changes provided by the bridge monitor.
     *
     * This method uses the bridge reader to fill in the initial list of interfaces for the given
     * bridge. This first call also enables that from then on, updates from the monitor for that
     * bridge are processed. All updates from the monitor for a bridge unknown to this class are
     * silently discarded. Thus it is mandatory to call this method for the bridge of interest at
     * least once, at the beginning.
     *
     * @see BridgeStateReader::read_state
     */
    bool read_state(const std::string &bridge_name, std::set<std::string> &iface_names) override;

private:
    /**
     * Bridge state monitor used to monitor changes in the state of the network bridge.
     */
    std::unique_ptr<BridgeStateMonitor> m_bridge_state_monitor;

    /**
     * Bridge state reader used to read the state of a network bridge when it is not known yet
     * (i.e. the first time the state is queried).
     */
    std::unique_ptr<BridgeStateReader> m_bridge_state_reader;

    /**
     * Map containing the current state of each known network bridge.
     * The map key is the bridge name and the map value is the bridge state (i.e.: the list of
     * interfaces in the bridge).
     */
    std::unordered_map<std::string, std::set<std::string>> m_bridge_states;

    /**
     * @brief Gets last known bridge state.
     *
     * @param[in] bridge_name Bridge name.
     * @param[out] iface_names Names of the interfaces in the network bridge.
     * @return true on success and false otherwise (i.e.: bridge state is not known yet).
     */
    bool get_state(const std::string &bridge_name, std::set<std::string> &iface_names);

    /**
     * @brief Sets last known bridge state.
     *
     * @param[in] bridge_name Bridge name.
     * @param[out] iface_names Names of the interfaces in the network bridge.
     */
    void set_state(const std::string &bridge_name, const std::set<std::string> &iface_names);
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_BRIDGE_STATE_MANAGER_IMPL_H_ */
