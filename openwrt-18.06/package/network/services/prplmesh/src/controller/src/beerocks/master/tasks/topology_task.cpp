
/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "topology_task.h"
#include "../db/db_algo.h"
#include "../son_actions.h"
#include "bml_task.h"
#include "client_steering_task.h"
#include "dhcp_task.h"

#include <beerocks/tlvf/beerocks_message_1905_vs.h>
#include <easylogging++.h>
#include <tlvf/ieee_1905_1/s802_11SpecificInformation.h>
#include <tlvf/ieee_1905_1/tlv1905NeighborDevice.h>
#include <tlvf/ieee_1905_1/tlvDeviceInformation.h>
#include <tlvf/ieee_1905_1/tlvNon1905neighborDeviceList.h>
#include <tlvf/wfa_map/tlvApOperationalBSS.h>
#include <tlvf/wfa_map/tlvClientAssociationEvent.h>
#include <tlvf/wfa_map/tlvProfile2ReasonCode.h>

#ifdef BEEROCKS_RDKB
#include "rdkb/rdkb_wlan_task.h"
#endif

using namespace beerocks;
using namespace net;
using namespace son;

topology_task::topology_task(db &database_, ieee1905_1::CmduMessageTx &cmdu_tx_, task_pool &tasks_)
    : task("topology_task"), database(database_), cmdu_tx(cmdu_tx_), tasks(tasks_)
{
}

void topology_task::work() {}

bool topology_task::handle_ieee1905_1_msg(const sMacAddr &src_mac,
                                          ieee1905_1::CmduMessageRx &cmdu_rx)
{
    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::TOPOLOGY_RESPONSE_MESSAGE: {
        handle_topology_response(src_mac, cmdu_rx);
        break;
    }
    case ieee1905_1::eMessageType::TOPOLOGY_NOTIFICATION_MESSAGE: {
        handle_topology_notification(src_mac, cmdu_rx);
        break;
    }
    default: {
        return false;
    }
    }
    return true;
}

bool topology_task::handle_topology_response(const sMacAddr &src_mac,
                                             ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received TOPOLOGY_RESPONSE_MESSAGE from " << src_mac << ", mid=" << std::hex
               << int(mid);

    auto tlvDeviceInformation = cmdu_rx.getClass<ieee1905_1::tlvDeviceInformation>();
    if (!tlvDeviceInformation) {
        LOG(ERROR) << "ieee1905_1::tlvDeviceInformation not found";
        return false;
    }

    const auto &al_mac = tlvDeviceInformation->mac();

    // Get all Agent known fronthaul radios if exist, and compare it to what the Agent reports.
    // If a radio exist on the database but not the tlvDeviceInformation, this radio node needs
    // to be removed.
    // This shall work in the opposite way as well: If it doesn't exist on the database, but
    // reported in the TLV, then we need to add the node to the database. Of course this will remain
    // as "TODO" for future task. Meanwhile parse properly the TLV, leaving the unused parts in
    // comment for future implementation.
    auto fronthaul_radios_on_db = database.get_node_children(
        tlvf::mac_to_string(al_mac), beerocks::TYPE_SLAVE, beerocks::STATE_CONNECTED);

    std::unordered_set<std::string> reported_fronthaul_radios;
    std::vector<sMacAddr> interface_macs{};

    // create topology response update event for bml listeners
    bml_task::topology_response_update_event new_bml_event;
    new_bml_event.al_mac = al_mac;

    for (uint8_t i = 0; i < tlvDeviceInformation->local_interface_list_length(); i++) {
        const auto iface_info_tuple = tlvDeviceInformation->local_interface_list(i);
        if (!std::get<0>(iface_info_tuple)) {
            LOG(ERROR) << "Failed to get " << int(i) << " element of local iface info "
                       << "on Device Information TLV";
            return false;
        }

        auto &iface_info = std::get<1>(iface_info_tuple);

        const auto &iface_mac = iface_info.mac();
        auto iface_mac_str    = tlvf::mac_to_string(iface_mac);

        const auto media_type       = iface_info.media_type();
        const auto media_type_group = media_type >> 8;

        interface_macs.push_back(iface_mac);

        // TODO Name and Status of Interface should be add
        database.add_interface(al_mac, iface_mac, media_type);
        LOG(DEBUG) << "Interface is added al_mac:" << al_mac << " iface_mac:" << iface_mac;

        // For wireless interface it is defined on IEEE 1905.1 that the size of the media info
        // is n=10 octets, which the size of s802_11SpecificInformation struct.
        // For wired interface n=0.
        if ((ieee1905_1::eMediaTypeGroup::IEEE_802_11 == media_type_group) &&
            (iface_info.media_info_length() == 10)) {

            const auto media_info = reinterpret_cast<ieee1905_1::s802_11SpecificInformation *>(
                iface_info.media_info(0));
            const auto iface_role = media_info->role;

            // For future implementation
            // const auto &iface_bssid = media_info->network_membership;
            // const auto iface_bw     = media_info->ap_channel_bandwidth;
            // const auto iface_cf1    = media_info->ap_channel_center_frequency_index1;
            // const auto iface_cf2    = media_info->ap_channel_center_frequency_index2;

            if (iface_role == ieee1905_1::eRole::AP) {
                reported_fronthaul_radios.insert(iface_mac_str);
            }

            // update bml event with the new radio interface
            new_bml_event.radio_interfaces.push_back(iface_info);

            LOG(DEBUG) << "New radio interface is reported, mac=" << iface_mac
                       << ", AP=" << (iface_role == ieee1905_1::eRole::AP);

            // TODO: Add/Update the node on the database
        }
    }

    // Update active mac list of the device node
    database.dm_update_interface_elements(al_mac, interface_macs);

    tasks.push_event(database.get_bml_task_id(), bml_task::TOPOLOGY_RESPONSE_UPDATE,
                     &new_bml_event);

    // If the database has radio mac that is not reported, remove its node from the db.
    for (const auto &fronthaul_radio_on_db : fronthaul_radios_on_db) {
        if (reported_fronthaul_radios.find(fronthaul_radio_on_db) ==
            reported_fronthaul_radios.end()) {
            LOG(DEBUG) << "radio " << fronthaul_radio_on_db
                       << " is not reported on Device Information TLV, removing the radio node";
            son_actions::handle_dead_node(fronthaul_radio_on_db, true, database, cmdu_tx, tasks);
        }
    }

    auto tlvApInformation = cmdu_rx.getClass<wfa_map::tlvApOperationalBSS>();
    if (tlvApInformation) {
        for (uint8_t i = 0; i < tlvApInformation->radio_list_length(); i++) {
            auto radio_entry = std::get<1>(tlvApInformation->radio_list(i));
            LOG(DEBUG) << "Operational BSS radio " << radio_entry.radio_uid();
            if (fronthaul_radios_on_db.find(tlvf::mac_to_string(radio_entry.radio_uid())) ==
                fronthaul_radios_on_db.end()) {
                LOG(WARNING) << "OperationalBSS on unknown radio " << radio_entry.radio_uid();
                continue;
            }
            // Update BSSes in the Agent
            auto radio = database.get_radio(src_mac, radio_entry.radio_uid());
            if (!radio) {
                LOG(WARNING) << "OperationalBSS on unknown radio  " << radio_entry.radio_uid()
                             << " on " << src_mac;
                continue;
            }

            radio->bsses.keep_new_prepare();

            for (uint8_t j = 0; j < radio_entry.radio_bss_list_length(); j++) {
                auto bss_entry = std::get<1>(radio_entry.radio_bss_list(j));
                LOG(DEBUG) << "Operational BSS " << bss_entry.radio_bssid();

                // TODO "backhaul" is not set in this TLV, so just assume false
                if (!database.update_vap(radio_entry.radio_uid(), bss_entry.radio_bssid(),
                                         bss_entry.ssid_str(), false)) {
                    LOG(ERROR) << "Failed to update VAP for radio " << radio_entry.radio_uid()
                               << " BSS " << bss_entry.radio_bssid() << " SSID "
                               << bss_entry.ssid_str();
                }
                auto bss  = radio->bsses.add(bss_entry.radio_bssid(), *radio);
                bss->ssid = bss_entry.ssid_str();
                // backhaul is not reported in this message. Leave it unchanged.
            }

            auto removed = radio->bsses.keep_new_remove_old();
            for (const auto &bss : removed) {
                // Remove all clients from that vap
                auto client_list = database.get_node_children(tlvf::mac_to_string(bss->bssid),
                                                              beerocks::TYPE_CLIENT);
                for (auto &client : client_list) {
                    son_actions::handle_dead_node(client, true, database, cmdu_tx, tasks);
                }

                // Remove the vap from DB
                database.remove_vap(*radio, bss->vap_id);
            }
        }
    }

    for (const auto &iface_mac : interface_macs) {

        auto iface_node = database.get_interface_node(al_mac, iface_mac);
        if (!iface_node) {
            LOG(ERROR) << "Failed to get interface node with mac: " << iface_mac;
            continue;
        }
        iface_node->m_neighbors.keep_new_prepare();
    }

    // The reported neighbors list might not be correct since the reporting al_mac hasn't received
    // a Topology Discovery from its neighbors yet. Therefore, remove a neighbor node only if more
    // than 65 seconds (timeout according to standard + 5 seconds grace) have passed since we added
    // this node. This promise that the reported al_mac will get the Topology Discovery messages
    // from its neighbors and add them to the report.
    bool check_dead_neighbors =
        (database.get_last_state_change(tlvf::mac_to_string(src_mac)) +
             std::chrono::seconds(beerocks::ieee1905_1_consts::DISCOVERY_NOTIFICATION_TIMEOUT_SEC +
                                  5) <
         std::chrono::steady_clock::now());

    std::unordered_set<sMacAddr> reported_neighbor_al_macs;
    auto tlv1905NeighborDeviceList = cmdu_rx.getClassList<ieee1905_1::tlv1905NeighborDevice>();

    for (const auto &tlv1905NeighborDevice : tlv1905NeighborDeviceList) {
        if (!tlv1905NeighborDevice) {
            LOG(ERROR) << "ieee1905_1::tlv1905NeighborDevice has invalid pointer";
            return false;
        }

        auto device_count = tlv1905NeighborDevice->mac_al_1905_device_length() /
                            sizeof(ieee1905_1::tlv1905NeighborDevice::sMacAl1905Device);

        for (size_t i = 0; i < device_count; i++) {
            const auto neighbor_al_mac_tuple = tlv1905NeighborDevice->mac_al_1905_device(i);
            if (!std::get<0>(neighbor_al_mac_tuple)) {
                LOG(ERROR) << "Getting al_mac element has failed";
                return false;
            }

            auto &neighbor_al_mac = std::get<1>(neighbor_al_mac_tuple).mac;

            // Add neighbor to related the interface
            database.add_neighbor(al_mac, tlv1905NeighborDevice->mac_local_iface(), neighbor_al_mac,
                                  true);

            LOG(DEBUG) << "Inserting reported 1905 neighbor " << neighbor_al_mac << " to the list";
            reported_neighbor_al_macs.insert(neighbor_al_mac);
        }
    }

    recently_reported_neighbors[al_mac] = std::chrono::steady_clock::now();
    // Remove neighbors from recently_reported_neighbors map if they stay there more than 5 seconds.
    for (auto neighbor = recently_reported_neighbors.begin();
         neighbor != recently_reported_neighbors.end();) {
        if (std::chrono::seconds(5) < (std::chrono::steady_clock::now() - neighbor->second)) {
            neighbor = recently_reported_neighbors.erase(neighbor);
        } else {
            ++neighbor;
        }
    }

    for (auto reported_neighbor_mac : reported_neighbor_al_macs) {
        if (!database.has_node(reported_neighbor_mac) &&
            recently_reported_neighbors.find(reported_neighbor_mac) ==
                recently_reported_neighbors.end()) {
            // Send a Topology Query if any new neighbor was detected.
            if (!cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_QUERY_MESSAGE)) {
                LOG(ERROR) << "Failed to build TOPOLOGY_QUERY_MESSAGE message!";
            }
            // Save just reported new neighbor to avoid sending unnecessary multiple
            // Topology Query from different devices to the same new node.
            recently_reported_neighbors.insert(
                {reported_neighbor_mac, std::chrono::steady_clock::now()});
            son_actions::send_cmdu_to_agent(reported_neighbor_mac, cmdu_tx, database);
            // Send an AP-AutoConfiguration Renew message to notify the neighbor
            // that an AP-AutoConfiguration WSC message should be sent.
            son_actions::send_ap_config_renew_msg(cmdu_tx, database);
        }
    }

    auto tlvNon1905NeighborDeviceList =
        cmdu_rx.getClassList<ieee1905_1::tlvNon1905neighborDeviceList>();

    for (const auto &tlvNon1905NeighborDevice : tlvNon1905NeighborDeviceList) {
        if (!tlvNon1905NeighborDevice) {
            LOG(ERROR) << "ieee1905_1::tlvNon1905NeighborDevice has invalid pointer";
            return false;
        }

        auto device_count =
            tlvNon1905NeighborDevice->mac_non_1905_device_length() / sizeof(sMacAddr);

        for (size_t i = 0; i < device_count; i++) {
            const auto neighbor_al_mac_tuple = tlvNon1905NeighborDevice->mac_non_1905_device(i);
            if (!std::get<0>(neighbor_al_mac_tuple)) {
                LOG(ERROR) << "Getting al_mac element has failed";
                return false;
            }

            auto &neighbor_al_mac = std::get<1>(neighbor_al_mac_tuple);

            // Add neighbor to related the interface
            database.add_neighbor(al_mac, tlvNon1905NeighborDevice->mac_local_iface(),
                                  neighbor_al_mac, false);

            LOG(DEBUG) << "Inserting reported non 1905 neighbor " << neighbor_al_mac
                       << " to the list";
        }
    }

    if (check_dead_neighbors) {
        handle_dead_neighbors(src_mac, al_mac, reported_neighbor_al_macs);
    }

    // Update active neighbors mac list of the interface node
    for (const auto &iface_mac : interface_macs) {

        auto iface_node = database.get_interface_node(al_mac, iface_mac);
        if (!iface_node) {
            LOG(ERROR) << "Failed to get interface node with mac: " << iface_mac;
            continue;
        }

        auto removed_neighbors = iface_node->m_neighbors.keep_new_remove_old();

        // Removed members needs to be cleaned up from datamodel also.
        for (const auto &removed_neighbor : removed_neighbors) {
            database.dm_remove_interface_neighbor(removed_neighbor->dm_path);
        }
    }

    auto agent = database.m_agents.get(al_mac);
    if (!agent) {
        LOG(WARNING) << "Agent with mac is not found in database mac=" << al_mac;
        return false;
    }

    // Update Profile Information in Agent.
    auto tlvProfile2MultiApProfile = cmdu_rx.getClass<wfa_map::tlvProfile2MultiApProfile>();
    if (tlvProfile2MultiApProfile) {
        agent->profile = tlvProfile2MultiApProfile->profile();
    }

    return true;
}

void topology_task::handle_dead_neighbors(const sMacAddr &src_mac, const sMacAddr &al_mac,
                                          std::unordered_set<sMacAddr> reported_neighbor_al_macs)
{
    LOG(TRACE) << "Checking if one of " << src_mac << " neighbors is no longer connected";

    auto neighbor_al_macs_on_db = database.get_1905_1_neighbors(al_mac);
    LOG(DEBUG) << "Comparing reported neighbors to neighbors on the database, neighbors_on_db="
               << neighbor_al_macs_on_db.size();

    for (const auto &neighbor_al_mac_on_db : neighbor_al_macs_on_db) {

        LOG(DEBUG) << "Checks if al_mac " << al_mac << " neighbor " << neighbor_al_mac_on_db
                   << " is reported in this message";

        // If reported al_mac is on the db skip it, otherwise remove the node.
        if (reported_neighbor_al_macs.find(neighbor_al_mac_on_db) !=
            reported_neighbor_al_macs.end()) {
            continue;
        }

        std::string neighbor_al_mac_on_db_str = tlvf::mac_to_string(neighbor_al_mac_on_db);
        auto backhhaul_mac                    = database.get_node_parent(neighbor_al_mac_on_db_str);

        // It is possible that re-routing took place, and the node is now a neighbour of some
        // other node. To filter such cases, compare the current al_mac of the neighbor to the
        // al_mac of the reporter. If they are not equal then it means than the neighbor is
        // currently under another node.
        auto current_parent_al_mac = database.get_node_parent_ire(backhhaul_mac);
        if (current_parent_al_mac != src_mac) {
            continue;
        }

        LOG(DEBUG) << "known neighbor al_mac  " << neighbor_al_mac_on_db
                   << " is not reported on 1905 Neighbor Device TLV, removing the al_mac node";
        son_actions::handle_dead_node(backhhaul_mac, true, database, cmdu_tx, tasks);
    }
}

bool topology_task::handle_topology_notification(const sMacAddr &src_mac,
                                                 ieee1905_1::CmduMessageRx &cmdu_rx)
{

    auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received TOPOLOGY_NOTIFICATION_MESSAGE, from " << src_mac << ", mid=" << std::hex
               << int(mid);

    // IEEE 1905.1 defines that TOPOLOGY_NOTIFICATION_MESSAGE must containt one 1905.1 AL MAC
    // address type TLV, and MultiAp standard extend it with zero or one Client Association Event
    // TLV. So if we didn't receive Client Association Event TLV, we need to send
    // TOPOLOGY_QUERY_MESSAGE to figure out what has changed on the topology.
    auto client_association_event_tlv = cmdu_rx.getClass<wfa_map::tlvClientAssociationEvent>();
    if (!client_association_event_tlv) {
        LOG(INFO) << "wfa_map::tlvClientAssociationEvent not found, sending TOPOLOGY_QUERY_MESSAGE";

        return son_actions::send_topology_query_msg(src_mac, cmdu_tx, database);
    }

    std::shared_ptr<beerocks_message::tlvVsClientAssociationEvent> vs_tlv = nullptr;
    auto beerocks_header = beerocks::message_com::parse_intel_vs_message(cmdu_rx);
    if (beerocks_header) {
        vs_tlv = beerocks_header->addClass<beerocks_message::tlvVsClientAssociationEvent>();
        if (!vs_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvVsClientAssociationEvent failed";
            return false;
        }
    }

    auto &client_mac    = client_association_event_tlv->client_mac();
    auto client_mac_str = tlvf::mac_to_string(client_mac);

    auto &bssid    = client_association_event_tlv->bssid();
    auto bssid_str = tlvf::mac_to_string(bssid);

    auto association_event = client_association_event_tlv->association_event();
    bool client_connected =
        (association_event == wfa_map::tlvClientAssociationEvent::CLIENT_HAS_JOINED_THE_BSS);

    LOG(INFO) << "client " << (client_connected ? "connected" : "disconnected")
              << ", client_mac=" << client_mac_str << ", bssid=" << bssid_str;

    if (client_connected) {
        //add or update node parent
        auto client = database.add_node_station(client_mac, bssid);
        if (!client) {
            LOG(ERROR) << "client " << client_mac << " not created";
            return false;
        }

        LOG(INFO) << "client connected, mac=" << client_mac_str << ", bssid=" << bssid_str;

        auto bss_bw    = database.get_node_bw(bssid_str);
        auto client_bw = bss_bw;
        if (vs_tlv) {
            if (son::wireless_utils::get_station_max_supported_bw(vs_tlv->capabilities(),
                                                                  client_bw)) {
                client_bw = std::min(client_bw, bss_bw);
            }
        }
        database.set_node_channel_bw(client_mac, database.get_node_channel(bssid_str), client_bw,
                                     database.get_node_channel_ext_above_secondary(bssid_str), 0,
                                     database.get_hostap_vht_center_frequency(bssid));

        // Note: The Database node stats and the Datamodels' stats are not the same.
        // Therefore, client information in data model and in node DB might differ.
        database.clear_node_stats_info(client_mac);
        client->clear_cross_rssi();
        database.dm_clear_sta_stats(tlvf::mac_from_string(client_mac_str));

        if (!(database.get_node_type(client_mac_str) == beerocks::TYPE_IRE_BACKHAUL &&
              database.get_node_handoff_flag(*client))) {
            // The node is not an IRE in handoff
            database.set_node_type(client_mac_str, beerocks::TYPE_CLIENT);
        }

        database.set_node_backhaul_iface_type(client_mac_str,
                                              beerocks::IFACE_TYPE_WIFI_UNSPECIFIED);

        if (vs_tlv) {
            database.set_node_vap_id(client_mac_str, vs_tlv->vap_id());
            database.set_station_capabilities(client_mac_str, vs_tlv->capabilities());
        }

        // Notify existing steering task of completed connection
        // Check if task is running before pushing the event
        if (tasks.is_task_running(client->steering_task_id)) {
            tasks.push_event(client->steering_task_id, client_steering_task::STA_CONNECTED);
        }

        int dhcp_task = database.get_dhcp_task_id();
        tasks.push_event(dhcp_task, DhcpTask::STA_CONNECTED);

#ifdef BEEROCKS_RDKB
        //push event to rdkb_wlan_hal task
        if (vs_tlv && database.settings_rdkb_extensions()) {
            bwl::sClientAssociationParams new_event = {};

            new_event.mac          = client_mac;
            new_event.bssid        = bssid;
            new_event.vap_id       = vs_tlv->vap_id();
            new_event.capabilities = vs_tlv->capabilities();

            tasks.push_event(database.get_rdkb_wlan_task_id(),
                             rdkb_wlan_task::events::STEERING_EVENT_CLIENT_CONNECT_AVAILABLE,
                             &new_event);
        }
#endif

        son_actions::handle_completed_connection(database, cmdu_tx, tasks, client_mac_str);

    } else {
        // client disconnected

#ifdef BEEROCKS_RDKB

        // Push event to rdkb_wlan_hal task
        if (vs_tlv && database.settings_rdkb_extensions()) {
            beerocks_message::sSteeringEvDisconnect new_event = {};
            new_event.client_mac                              = client_mac;
            new_event.bssid                                   = bssid;
            new_event.reason                                  = vs_tlv->disconnect_reason();
            new_event.source = beerocks_message::eDisconnectSource(vs_tlv->disconnect_source());
            new_event.type   = beerocks_message::eDisconnectType(vs_tlv->disconnect_type());

            tasks.push_event(database.get_rdkb_wlan_task_id(),
                             rdkb_wlan_task::events::STEERING_EVENT_CLIENT_DISCONNECT_AVAILABLE,
                             &new_event);
        }
#endif

        auto client = database.get_station(tlvf::mac_from_string(client_mac_str));
        if (!client) {
            LOG(ERROR) << "Station " << client_mac_str << " not found";
            return false;
        }

        /*
            TODO: Reason code should come from Client Disassociation Stats message in
                    reason Code TLV but since we do not have this data Reason Code
                    set to 1 (UNSPECIFIED_REASON - IEEE802.11-16, Table 9.45).
                    Should be fixed after PPM-864.
            TODO: ReasonCode should be tested after PPM-1905 for nl80211 platforms.
        */
        uint16_t reason_code = (vs_tlv)
                                   ? vs_tlv->disconnect_reason()
                                   : (uint16_t)wfa_map::tlvProfile2ReasonCode::UNSPECIFIED_REASON;
        if (!database.notify_disconnection(client_mac_str, reason_code)) {
            LOG(WARNING) << "Failed to notify disconnection event.";
        }

        // STA only needs to be removed if the BSSID reported in the disconnection event matches
        // the BSSID the station is currently connected to.
        // Otherwise, the station has probably already re-connected to another BSS in the meantime
        // and we should not remove it.
        auto bss = client->get_bss();
        if (!bss) {
            LOG(INFO) << "BSS of the Station is empty mac: " << client->mac;
            return false;
        }
        bool reported_by_parent = bssid == bss->bssid;

        if (reported_by_parent && !database.dm_remove_sta(*client)) {
            LOG(ERROR) << "Failed to remove STA from data model mac:" << client_mac_str;
        }

        // TODO: Validate usages of reported_by_parent flag usages (PPM-1948)
        son_actions::handle_dead_node(client_mac_str, reported_by_parent, database, cmdu_tx, tasks);
    }

    return true;
}
