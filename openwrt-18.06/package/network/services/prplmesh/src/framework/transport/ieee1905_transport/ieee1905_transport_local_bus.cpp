/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "ieee1905_transport.h"

#include <arpa/inet.h>
#include <chrono>
#include <net/if.h>

namespace beerocks {
namespace transport {

// Use transport messaging classes
using namespace beerocks::transport::messages;

void Ieee1905Transport::handle_broker_pollin_event(std::unique_ptr<messages::Message> &msg)
{
    if (auto *cmdu_tx_msg = dynamic_cast<CmduTxMessage *>(msg.get())) {
        MAPF_DBG("received CmduTxMessage message:" << std::endl << *cmdu_tx_msg);
        handle_broker_cmdu_tx_message(*cmdu_tx_msg);
    } else if (auto *interface_configuration_request_msg =
                   dynamic_cast<InterfaceConfigurationRequestMessage *>(msg.get())) {
        MAPF_DBG("received InterfaceConfigurationRequestMessage message:"
                 << std::endl
                 << *interface_configuration_request_msg);
        handle_broker_interface_configuration_request_message(*interface_configuration_request_msg);
    } else if (auto *al_mac_addr_configuration_msg =
                   dynamic_cast<AlMacAddressConfigurationMessage *>(msg.get())) {
        MAPF_DBG("received AlMacAddressConfigurationMessage message:"
                 << std::endl
                 << *al_mac_addr_configuration_msg);
        handle_al_mac_addr_configuration_message(*al_mac_addr_configuration_msg);
    } else {
        // should never receive messages which we are not subscribed to
        MAPF_WARN("received un-expected message:" << std::endl << *msg);
    }
}

void Ieee1905Transport::handle_broker_cmdu_tx_message(CmduTxMessage &msg)
{
    // parse CmduTxMessage message
    struct Packet packet;
    packet.dst_if_type  = msg.metadata()->if_type;
    packet.dst_if_index = msg.metadata()->if_index;
    packet.src_if_type  = CmduTxMessage::IF_TYPE_LOCAL_BUS;
    packet.src_if_index = 0;
    tlvf::mac_from_array(msg.metadata()->dst, packet.dst);
    tlvf::mac_from_array(msg.metadata()->src, packet.src);
    packet.ether_type       = msg.metadata()->ether_type;
    packet.header.iov_base  = 0;
    packet.header.iov_len   = 0;
    packet.payload.iov_base = msg.data();
    packet.payload.iov_len  = msg.metadata()->length;

    if (packet.ether_type == ETH_P_1905_1) {
        mapf_assert(packet.payload.iov_len >= sizeof(Ieee1905CmduHeader));

        Ieee1905CmduHeader *ch = static_cast<Ieee1905CmduHeader *>(packet.payload.iov_base);

        // update messageId field (a.k.a "MID") unless is was pre-set by the originator
        if (!msg.metadata()->preset_message_id) {
            uint16_t messageId = get_next_message_id();
            ch->messageId      = htons(messageId);
        }

        // send confirmation (with messageId value)
        if (msg.metadata()->cookie != 0) {
            CmduTxConfirmationMessage confMsg;

            confMsg.metadata()->cookie     = msg.metadata()->cookie;
            confMsg.metadata()->ether_type = msg.metadata()->ether_type;
            confMsg.metadata()->msg_type   = msg.metadata()->msg_type;
            confMsg.metadata()->msg_id     = ntohs(ch->messageId);

            MAPF_DBG("publishing CmduTxConfirmationMessage:" << std::endl << confMsg);
            if (!m_broker->publish(confMsg)) {
                MAPF_ERR("cannot publish message to broker.");
            }
        }

        if (msg.metadata()->msg_type != ntohs(ch->messageType)) {
            MAPF_WARN("CmduTxMessage messageType mismatch: cmdu: "
                      << std::hex << msg.metadata()->msg_type
                      << " packet: " << ntohs(ch->messageType));
        }
    }

    counters_[CounterId::OUTGOING_LOCAL_BUS_PACKETS]++;
    handle_packet(packet);
}

void Ieee1905Transport::handle_broker_interface_configuration_request_message(
    InterfaceConfigurationRequestMessage &msg)
{
    std::map<std::string, NetworkInterface> added_updated_network_interfaces;
    std::map<std::string, NetworkInterface> removed_network_interfaces;

    LOG(DEBUG) << "handle_broker_interface_configuration_request_message(): "
               << "iface_name=" << msg.metadata()->iface_name << ", add= " << msg.metadata()->add;

    if (msg.metadata()->is_bridge) {
        auto bridge_name = msg.metadata()->iface_name;
        MAPF_INFO("Using bridge: " << bridge_name);

        // fill a set with the interfaces that are part of the bridge:
        std::set<std::string> bridge_state{};
        auto bridge_state_ret = m_bridge_state_manager->read_state(bridge_name, bridge_state);
        LOG_IF(!bridge_state_ret, ERROR)
            << "Failed reading the bridge " << bridge_name << " state!";

        if (!bridge_state_ret || !msg.metadata()->add) {
            for (const auto &net_if_element : network_interfaces_) {
                removed_network_interfaces.insert(net_if_element);
            }
        } else {
            added_updated_network_interfaces[bridge_name].ifname      = bridge_name;
            added_updated_network_interfaces[bridge_name].bridge_name = {};
            added_updated_network_interfaces[bridge_name].is_bridge   = true;

            for (const auto &ifname : bridge_state) {
                MAPF_INFO("  Using interface: " << ifname);
                added_updated_network_interfaces[ifname].ifname      = ifname;
                added_updated_network_interfaces[ifname].bridge_name = bridge_name;
                added_updated_network_interfaces[ifname].is_bridge   = false;
            }
        }
    }
    // Interface is not a bridge
    else {
        auto iface_name  = msg.metadata()->iface_name;
        auto bridge_name = msg.metadata()->bridge_name;
        if (msg.metadata()->add) {
            added_updated_network_interfaces[iface_name].ifname      = iface_name;
            added_updated_network_interfaces[iface_name].bridge_name = bridge_name;
            added_updated_network_interfaces[iface_name].is_bridge   = false;
        } else {
            removed_network_interfaces[iface_name].ifname      = iface_name;
            removed_network_interfaces[iface_name].bridge_name = bridge_name;
            removed_network_interfaces[iface_name].is_bridge   = false;
        }
    }

    update_network_interfaces(added_updated_network_interfaces, removed_network_interfaces);
}

void Ieee1905Transport::handle_al_mac_addr_configuration_message(
    AlMacAddressConfigurationMessage &msg)
{
    auto al_mac = msg.metadata()->al_mac;
    MAPF_INFO("Using AL MAC: " << al_mac);

    set_al_mac_addr(al_mac.oct);
}

bool Ieee1905Transport::send_packet_to_broker(Packet &packet)
{
    // Create and fill an CmduRxMessage to be sent to the broker
    CmduRxMessage msg;

    tlvf::mac_to_array(packet.src, msg.metadata()->src);
    tlvf::mac_to_array(packet.dst, msg.metadata()->dst);

    msg.metadata()->ether_type    = packet.ether_type;
    msg.metadata()->if_type       = packet.src_if_type;
    msg.metadata()->if_index      = packet.src_if_index;
    msg.metadata()->length        = packet.payload.iov_len;
    msg.metadata()->received_time = std::chrono::duration_cast<std::chrono::seconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count();
    std::copy_n((uint8_t *)packet.payload.iov_base, packet.payload.iov_len, msg.data());

    if (packet.ether_type == ETH_P_1905_1) {
        Ieee1905CmduHeader *ch   = reinterpret_cast<Ieee1905CmduHeader *>(packet.payload.iov_base);
        msg.metadata()->msg_type = ntohs(ch->messageType);
        msg.metadata()->relay    = ch->GetRelayIndicator() ? 1 : 0;
    } else {
        msg.metadata()->msg_type = 0;
        msg.metadata()->relay    = 0;
    }

    counters_[CounterId::INCOMMING_LOCAL_BUS_PACKETS]++;

    MAPF_DBG("publishing CmduRxMessage:" << std::endl << msg);
    if (!m_broker->publish(msg)) {
        MAPF_ERR("failed to publish message to broker.");
        return false;
    }

    return true;
}

uint16_t Ieee1905Transport::get_next_message_id()
{
    message_id_++;

    if (message_id_ == 0) {
        MAPF_DBG("messageId wrap-around occurred.");
        counters_[CounterId::MESSAGE_ID_WRAPAROUND]++;
    }

    return message_id_;
}

} // namespace transport
} // namespace beerocks
