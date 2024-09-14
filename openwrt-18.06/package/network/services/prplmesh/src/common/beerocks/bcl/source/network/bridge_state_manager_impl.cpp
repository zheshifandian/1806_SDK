/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/bridge_state_manager_impl.h>

#include <algorithm>

#include <easylogging++.h>

namespace beerocks {
namespace net {

BridgeStateManagerImpl::BridgeStateManagerImpl(
    std::unique_ptr<BridgeStateMonitor> bridge_state_monitor,
    std::unique_ptr<BridgeStateReader> bridge_state_reader)
    : m_bridge_state_monitor(std::move(bridge_state_monitor)),
      m_bridge_state_reader(std::move(bridge_state_reader))
{
    m_bridge_state_monitor->set_handler([&](const std::string &bridge_name,
                                            const std::string &iface_name, bool iface_in_bridge) {
        // Get the cached list of interfaces in the given bridge, if it exists.
        // Else, silently discard this event.
        std::set<std::string> iface_names;
        if (!get_state(bridge_name, iface_names)) {
            return;
        }

        // Kernel can send RTM_NEWLINK and RTM_DELLINK messages for various reasons, not only
        // because of an interface has been added to or removed from a bridge. In fact, when any of
        // these events happen, several messages are received in a row. To avoid notifying state
        // changes that have already been notified, we check the event against the cached list of
        // interfaces and discard duplicates.
        // If given interface has been added and it was not in the list, then add it.
        // Else, if given interface has been removed and it was in the list, then remove it
        bool iface_names_is_dirty = false;
        auto it                   = std::find(iface_names.begin(), iface_names.end(), iface_name);
        if (iface_in_bridge && (it == iface_names.end())) {
            iface_names.emplace(iface_name);
            iface_names_is_dirty = true;
        } else if ((!iface_in_bridge) && (it != iface_names.end())) {
            iface_names.erase(it);
            iface_names_is_dirty = true;
        }

        // This method is invoked to handle the Netlink bridge events that kernel sends when an
        // interface has been added to or deleted from a bridge. It should suffice then to read
        // the bridge state on application start and from then on just handle add/remove events
        // in the order they are sent by the kernel to efficiently monitor the bridge state.
        // However, in some devices like the Axepoint, when an interface is removed from a
        // bridge, we get a Netlink event with RTM_DELLINK for the interface with bridge as
        // master, **followed** by a RTM_NEWLINK additional event for the same interface and
        // bridge as master. This additional and misplaced event is misinterpreted as if the
        // interface had been added again to the bridge after its removal.
        // As a workaround, if the bridge state has allegedly changed, read the bridge state and
        // check if it has actually changed.
        if (iface_names_is_dirty) {
            std::set<std::string> actual_iface_names;
            if (!m_bridge_state_reader->read_state(bridge_name, actual_iface_names)) {
                LOG(ERROR) << "Unable to read the state of bridge " << bridge_name;
                return;
            }

            if (actual_iface_names.find(iface_name) == actual_iface_names.end()) {
                if (iface_in_bridge) {
                    iface_names_is_dirty = false;
                    LOG(DEBUG) << "Ignoring event (interface " << iface_name
                               << " has not actually been added to bridge " << bridge_name << ")";
                }
            } else if (!iface_in_bridge) {
                iface_names_is_dirty = false;
                LOG(DEBUG) << "Ignoring event (interface " << iface_name
                           << " has not actually been removed from bridge " << bridge_name << ")";
            }
        }

        // If changed, update the cached list of interfaces and notify the change.
        if (iface_names_is_dirty) {
            set_state(bridge_name, iface_names);
            notify_state_changed(bridge_name, iface_name, iface_in_bridge);
        }
    });
}

BridgeStateManagerImpl::~BridgeStateManagerImpl() { m_bridge_state_monitor->clear_handler(); }

bool BridgeStateManagerImpl::read_state(const std::string &bridge_name,
                                        std::set<std::string> &iface_names)
{
    // Return cached bridge state if available
    if (get_state(bridge_name, iface_names)) {
        return true;
    }

    // Read current bridge state (this happens the first time this method is invoked)
    if (!m_bridge_state_reader->read_state(bridge_name, iface_names)) {
        return false;
    }

    // Cache obtained bridge state (thus enabling state-change event processing for given bridge
    // from this moment on)
    set_state(bridge_name, iface_names);

    return true;
}

bool BridgeStateManagerImpl::get_state(const std::string &bridge_name,
                                       std::set<std::string> &iface_names)
{
    const auto &it = m_bridge_states.find(bridge_name);
    if (m_bridge_states.end() == it) {
        return false;
    }

    iface_names = it->second;

    return true;
}

void BridgeStateManagerImpl::set_state(const std::string &bridge_name,
                                       const std::set<std::string> &iface_names)
{
    m_bridge_states[bridge_name] = iface_names;
}

} // namespace net
} // namespace beerocks
