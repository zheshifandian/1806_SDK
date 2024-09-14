/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "ieee1905_transport.h"

namespace beerocks {
namespace transport {

Ieee1905Transport::Ieee1905Transport(
    std::shared_ptr<beerocks::net::InterfaceStateManager> interface_state_manager,
    std::shared_ptr<beerocks::net::BridgeStateManager> bridge_state_manager,
    std::shared_ptr<broker::BrokerServer> broker, std::shared_ptr<EventLoop> event_loop)
    : m_interface_state_manager(interface_state_manager),
      m_bridge_state_manager(bridge_state_manager), m_broker(broker), m_event_loop(event_loop)
{
    LOG_IF(!m_interface_state_manager, FATAL) << "Interface state manager is a null pointer!";
    LOG_IF(!m_bridge_state_manager, FATAL) << "Bridge state manager is a null pointer!";
    LOG_IF(!m_broker, FATAL) << "Broker server is a null pointer!";
    LOG_IF(!m_event_loop, FATAL) << "Event loop is a null pointer!";
}

bool Ieee1905Transport::start()
{
    LOG(INFO) << "Starting 1905 transport...";

    // Register broker handlers for internal and external messages
    m_broker->register_external_message_handler(
        [&](std::unique_ptr<messages::Message> &msg, broker::BrokerServer &broker) -> bool {
            LOG(DEBUG) << "Processing external message type: " << uint32_t(msg->type());
            handle_broker_pollin_event(msg);
            return true;
        });

    m_broker->register_internal_message_handler(
        [&](std::unique_ptr<messages::Message> &msg, broker::BrokerServer &broker) -> bool {
            LOG(DEBUG) << "Processing internal message type: " << uint32_t(msg->type());
            handle_broker_pollin_event(msg);
            return true;
        });

    m_interface_state_manager->set_handler([&](const std::string &iface_name, bool iface_state) {
        handle_interface_state_change(iface_name, iface_state);
    });

    m_bridge_state_manager->set_handler(
        [&](const std::string &bridge_name, const std::string &iface_name, bool iface_added) {
            handle_bridge_state_change(bridge_name, iface_name, iface_added);
        });

    return true;
}

bool Ieee1905Transport::stop()
{
    m_interface_state_manager->clear_handler();
    m_bridge_state_manager->clear_handler();

    return true;
}

} // namespace transport
} // namespace beerocks
