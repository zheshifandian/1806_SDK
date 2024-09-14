/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "son_actions.h"

#include "db/db_algo.h"
#include "tasks/agent_monitoring_task.h"
#include "tasks/association_handling_task.h"
#include "tasks/bml_task.h"
#include "tasks/client_steering_task.h"

#include <bcl/network/network_utils.h>
#include <bcl/network/sockets.h>
#include <bcl/son/son_wireless_utils.h>
#include <easylogging++.h>

#include <beerocks/tlvf/beerocks_message_cli.h>
#include <tlvf/ieee_1905_1/tlvAlMacAddress.h>
#include <tlvf/ieee_1905_1/tlvSupportedFreqBand.h>
#include <tlvf/ieee_1905_1/tlvSupportedRole.h>
#include <tlvf/wfa_map/tlvClientAssociationControlRequest.h>
#include <tlvf/wfa_map/tlvProfile2MultiApProfile.h>

#include "controller.h"

using namespace beerocks;
using namespace net;
using namespace son;

void son_actions::handle_completed_connection(db &database, ieee1905_1::CmduMessageTx &cmdu_tx,
                                              task_pool &tasks, std::string client_mac)
{
    LOG(INFO) << "handle_completed_connection client_mac=" << client_mac;
    if (!database.set_node_state(client_mac, beerocks::STATE_CONNECTED)) {
        LOG(ERROR) << "set node state failed";
    }

    // update bml listeners
    auto n_type = database.get_node_type(client_mac);
    if (n_type == TYPE_CLIENT) {
        LOG(DEBUG) << "BML, sending connect CONNECTION_CHANGE for mac " << client_mac << " of type "
                   << int(n_type);
        bml_task::connection_change_event new_event;
        new_event.mac = client_mac;
        tasks.push_event(database.get_bml_task_id(), bml_task::CONNECTION_CHANGE, &new_event);
    }

    auto new_hostap_mac      = database.get_node_parent(client_mac);
    auto previous_hostap_mac = database.get_node_previous_parent(client_mac);
    auto hostaps             = database.get_active_hostaps();

    hostaps.erase(new_hostap_mac); //next operations will be done only on the other APs

    if (database.is_node_wireless(client_mac)) {
        LOG(DEBUG) << "node " << client_mac << " is wireless";
        /*
         * send disassociate request to previous hostap to clear STA mac from its list
         */
        if ((!previous_hostap_mac.empty()) &&
            (previous_hostap_mac != network_utils::ZERO_MAC_STRING) &&
            (previous_hostap_mac != new_hostap_mac)) {
            disconnect_client(database, cmdu_tx, client_mac, previous_hostap_mac,
                              eDisconnect_Type_Disassoc, 0);
        }

        /*
         * launch association handling task for async actions
         * and further handling of the new connection
         */
        auto new_task =
            std::make_shared<association_handling_task>(database, cmdu_tx, tasks, client_mac);
        tasks.add_task(new_task);
    }
}

bool son_actions::add_node_to_default_location(db &database, std::string client_mac)
{
    std::string gw_lan_switch;

    auto gw = database.get_gw();
    if (!gw) {
        LOG(WARNING)
            << "add_node_to_default_location - can't get GW node, adding to default location...";
    } else {
        auto gw_mac          = tlvf::mac_to_string(gw->al_mac);
        auto gw_lan_switches = database.get_node_children(gw_mac, beerocks::TYPE_ETH_SWITCH);
        if (gw_lan_switches.empty()) {
            LOG(ERROR) << "add_node_to_default_location - GW has no LAN SWITCH node!";
            return false;
        }
        gw_lan_switch = *gw_lan_switches.begin();
    }

    if (!database.add_node_station(tlvf::mac_from_string(client_mac),
                                   tlvf::mac_from_string(gw_lan_switch))) {
        LOG(ERROR) << "add_node_to_default_location - add_node failed";
        return false;
    }

    if (!database.set_node_state(client_mac, beerocks::STATE_CONNECTING)) {
        LOG(ERROR) << "add_node_to_default_location - set_node_state failed.";
        return false;
    }

    return true;
}

void son_actions::unblock_sta(db &database, ieee1905_1::CmduMessageTx &cmdu_tx, std::string sta_mac)
{
    LOG(DEBUG) << "unblocking " << sta_mac << " from network";

    auto hostaps              = database.get_active_hostaps();
    const auto &current_bssid = database.get_node_parent(sta_mac);
    const auto &ssid          = database.get_hostap_ssid(tlvf::mac_from_string(current_bssid));

    for (auto &hostap : hostaps) {
        /*
         * unblock client from all hostaps to prevent it from getting locked out
         */
        const auto &hostap_vaps = database.get_hostap_vap_list(tlvf::mac_from_string(hostap));

        for (const auto &hostap_vap : hostap_vaps) {
            if (hostap_vap.second.ssid != ssid) {
                continue;
            }
            auto agent_mac = database.get_node_parent_ire(hostap);
            if (!cmdu_tx.create(
                    0, ieee1905_1::eMessageType::CLIENT_ASSOCIATION_CONTROL_REQUEST_MESSAGE)) {
                LOG(ERROR) << "cmdu creation of type CLIENT_ASSOCIATION_CONTROL_REQUEST_MESSAGE, "
                              "has failed";
                break;
            }
            auto association_control_request_tlv =
                cmdu_tx.addClass<wfa_map::tlvClientAssociationControlRequest>();
            if (!association_control_request_tlv) {
                LOG(ERROR) << "addClass wfa_map::tlvClientAssociationControlRequest failed";
                break;
            }
            association_control_request_tlv->bssid_to_block_client() =
                tlvf::mac_from_string(hostap_vap.second.mac);
            association_control_request_tlv->association_control() =
                wfa_map::tlvClientAssociationControlRequest::UNBLOCK;
            association_control_request_tlv->alloc_sta_list();
            auto sta_list         = association_control_request_tlv->sta_list(0);
            std::get<1>(sta_list) = tlvf::mac_from_string(sta_mac);

            son_actions::send_cmdu_to_agent(agent_mac, cmdu_tx, database, hostap);
            LOG(DEBUG) << "sending allow request for " << sta_mac << " to " << hostap << "bssid"
                       << association_control_request_tlv->bssid_to_block_client();
        }
    }
}

int son_actions::steer_sta(db &database, ieee1905_1::CmduMessageTx &cmdu_tx, task_pool &tasks,
                           std::string sta_mac, std::string chosen_hostap,
                           const std::string &triggered_by, const std::string &steering_type,
                           bool disassoc_imminent, int disassoc_timer_ms, bool steer_restricted)
{
    auto new_task = std::make_shared<client_steering_task>(
        database, cmdu_tx, tasks, sta_mac, chosen_hostap, triggered_by, steering_type,
        disassoc_imminent, disassoc_timer_ms, steer_restricted);

    tasks.add_task(new_task);
    return new_task->id;
}

bool son_actions::set_hostap_active(db &database, task_pool &tasks, std::string hostap_mac,
                                    bool active)
{
    bool result = database.set_hostap_active(tlvf::mac_from_string(hostap_mac), active);

    if (result) {
        bml_task::connection_change_event new_event;
        new_event.mac   = database.get_node_parent(hostap_mac);
        int bml_task_id = database.get_bml_task_id();
        tasks.push_event(bml_task_id, bml_task::CONNECTION_CHANGE, &new_event);
        LOG(TRACE) << "BML, sending hostap (" << hostap_mac
                   << ") active CONNECTION_CHANGE for IRE mac " << new_event.mac;
    }

    return result;
}

void son_actions::disconnect_client(db &database, ieee1905_1::CmduMessageTx &cmdu_tx,
                                    const std::string &client_mac, const std::string &bssid,
                                    eDisconnectType type, uint32_t reason)
{

    auto agent_mac = database.get_node_parent_ire(bssid);

    auto request =
        message_com::create_vs_message<beerocks_message::cACTION_CONTROL_CLIENT_DISCONNECT_REQUEST>(
            cmdu_tx);

    if (request == nullptr) {
        LOG(ERROR) << "Failed building ACTION_CONTROL_CLIENT_DISCONNECT_REQUEST message!";
        return;
    }
    request->mac()    = tlvf::mac_from_string(client_mac);
    request->vap_id() = database.get_hostap_vap_id(tlvf::mac_from_string(bssid));
    request->type()   = type;
    request->reason() = reason;

    const auto parent_radio = database.get_node_parent_radio(bssid);
    son_actions::send_cmdu_to_agent(agent_mac, cmdu_tx, database, parent_radio);
    LOG(DEBUG) << "sending DISASSOCIATE request, client " << client_mac << " bssid " << bssid;
}

void son_actions::send_cli_debug_message(db &database, ieee1905_1::CmduMessageTx &cmdu_tx,
                                         std::stringstream &ss)
{
    auto controller_ctx = database.get_controller_ctx();
    if (!controller_ctx) {
        LOG(ERROR) << "controller_ctx == nullptr";
        return;
    }

    auto response =
        message_com::create_vs_message<beerocks_message::cACTION_CLI_RESPONSE_STR>(cmdu_tx);

    if (response == nullptr) {
        LOG(ERROR) << "Failed building cACTION_CLI_RESPONSE_STR message!";
        return;
    }

    //In case we don't have enough space for node length, reserve 1 byte for '\0'
    size_t reserved_size =
        (message_com::get_vs_cmdu_size_on_buffer<beerocks_message::cACTION_CLI_RESPONSE_STR>() - 1);
    size_t max_size = cmdu_tx.getMessageBuffLength() - reserved_size;
    size_t size     = (ss.tellp() > int(max_size)) ? max_size : size_t(ss.tellp());

    if (!response->alloc_buffer(size + 1)) {
        LOG(ERROR) << "Failed buffer allocation";
        return;
    }

    auto buf = response->buffer(0);
    if (!buf) {
        LOG(ERROR) << "Failed buffer allocation";
        return;
    }
    std::copy_n(ss.str().c_str(), size, buf);
    (buf)[size] = 0;

    for (int idx = 0;; idx++) {
        int fd = database.get_cli_socket_at(idx);
        if (beerocks::net::FileDescriptor::invalid_descriptor != fd) {
            controller_ctx->send_cmdu(fd, cmdu_tx);
        } else {
            break;
        }
    }
}

void son_actions::handle_dead_node(std::string mac, bool reported_by_parent, db &database,
                                   ieee1905_1::CmduMessageTx &cmdu_tx, task_pool &tasks)
{
    beerocks::eType mac_type = database.get_node_type(mac);
    auto node_state          = database.get_node_state(mac);

    LOG(DEBUG) << "NOTICE: handling dead node " << mac << " type enum " << int(mac_type)
               << " reported by parent " << reported_by_parent;

    if ((mac_type == beerocks::TYPE_IRE_BACKHAUL || mac_type == beerocks::TYPE_CLIENT) &&
        database.is_node_wireless(mac)) {
        auto station = database.get_station(tlvf::mac_from_string(mac));
        if (!station) {
            LOG(ERROR) << "Station " << mac << " not found";
            return;
        }
        // If there is running association handleing task already, terminate it.
        int prev_task_id = station->association_handling_task_id;
        if (tasks.is_task_running(prev_task_id)) {
            tasks.kill_task(prev_task_id);
        }
    }

    if (reported_by_parent) {
        if (mac_type == beerocks::TYPE_IRE_BACKHAUL || mac_type == beerocks::TYPE_CLIENT) {
            database.set_node_state(mac, beerocks::STATE_DISCONNECTED);

            auto station = database.get_station(tlvf::mac_from_string(mac));
            if (!station) {
                LOG(ERROR) << "Station " << mac << " not found";
                return;
            }

            // Clear node ipv4
            database.set_node_ipv4(mac);

            // Notify steering task, if any, of disconnect.
            int steering_task = station->steering_task_id;
            if (tasks.is_task_running(steering_task))
                tasks.push_event(steering_task, client_steering_task::STA_DISCONNECTED);

            if (database.get_node_handoff_flag(*station)) {
                LOG(DEBUG) << "handoff_flag == true, mac " << mac;
                // We're in the middle of steering, don't mark as disconnected (yet).
                return;
            } else {
                LOG(DEBUG) << "handoff_flag == false, mac " << mac;

                // If we're not in the middle of steering, kill roaming task
                int prev_task_id = station->roaming_task_id;
                if (tasks.is_task_running(prev_task_id)) {
                    tasks.kill_task(prev_task_id);
                }
            }

            // If there is an instance of association handling task, kill it
            int association_handling_task_id = station->association_handling_task_id;
            if (tasks.is_task_running(association_handling_task_id)) {
                tasks.kill_task(association_handling_task_id);
            }
        }

        // close slave socket
        if (mac_type == beerocks::TYPE_SLAVE) {
            database.set_node_state(mac, beerocks::STATE_DISCONNECTED);
            set_hostap_active(database, tasks, mac, false);
        }

        /*
         * set all nodes in the subtree as disconnected
         */
        if (mac_type != beerocks::TYPE_CLIENT) {
            int agent_monitoring_task_id = database.get_agent_monitoring_task_id();
            auto nodes                   = database.get_node_subtree(mac);
            for (auto &node_mac : nodes) {
                if (database.get_node_type(node_mac) == beerocks::TYPE_IRE) {
                    std::string ire_mac = node_mac;
                    tasks.push_event(agent_monitoring_task_id, STATE_DISCONNECTED, &ire_mac);
                    // get in here when handling dead node on IRE backhaul
                    // set all platform bridges as non operational
                    LOG(DEBUG) << "setting platform with bridge mac " << node_mac
                               << " as non operational";

                    auto agent = database.m_agents.get(tlvf::mac_from_string(node_mac));
                    if (!agent) {
                        LOG(ERROR) << "agent " << node_mac << " not found";
                        return;
                    }
                    agent->state = STATE_DISCONNECTED;
                } else if (database.get_node_type(node_mac) == beerocks::TYPE_IRE_BACKHAUL ||
                           database.get_node_type(node_mac) == beerocks::TYPE_CLIENT) {

                    auto station = database.get_station(tlvf::mac_from_string(node_mac));
                    if (!station) {
                        LOG(ERROR) << "station " << node_mac << " not found";
                        return;
                    }

                    // kill old roaming task
                    int prev_task_id = station->roaming_task_id;
                    if (tasks.is_task_running(prev_task_id)) {
                        tasks.kill_task(prev_task_id);
                    }
                }

                database.set_node_state(node_mac, beerocks::STATE_DISCONNECTED);
                set_hostap_active(database, tasks, node_mac,
                                  false); //implementation checks for hostap node type

                if (database.get_node_type(node_mac) == beerocks::TYPE_IRE ||
                    database.get_node_type(node_mac) == beerocks::TYPE_CLIENT) {
                    tasks.push_event(agent_monitoring_task_id, STATE_DISCONNECTED, &mac);
                    bml_task::connection_change_event new_event;
                    new_event.mac = node_mac;
                    tasks.push_event(database.get_bml_task_id(), bml_task::CONNECTION_CHANGE,
                                     &new_event);
                    LOG(DEBUG) << "BML, sending client disconnect CONNECTION_CHANGE for mac "
                               << new_event.mac;
                }
            }
        }
    }

    // update bml listeners
    if (node_state == beerocks::STATE_CONNECTED) {
        if (mac_type == beerocks::TYPE_CLIENT) {
            bml_task::connection_change_event new_event;
            new_event.mac = mac;
            tasks.push_event(database.get_bml_task_id(), bml_task::CONNECTION_CHANGE, &new_event);
            LOG(DEBUG) << "BML, sending client disconnect CONNECTION_CHANGE for mac "
                       << new_event.mac;
        } else if (mac_type == beerocks::TYPE_IRE_BACKHAUL) {
            auto backhauls_bridge = database.get_node_children(mac, beerocks::TYPE_IRE);
            if (backhauls_bridge.empty()) {
                LOG(ERROR) << "backhaul has no bridge node under it!";
            } else {
                for (auto it = backhauls_bridge.begin(); it != backhauls_bridge.end(); it++) {
                    bml_task::connection_change_event new_event;
                    new_event.mac = *it;
                    LOG(DEBUG) << "BML, sending IRE disconnect CONNECTION_CHANGE for mac "
                               << new_event.mac;
                    tasks.push_event(database.get_bml_task_id(), bml_task::CONNECTION_CHANGE,
                                     &new_event);
                }
            }
        }
    }

    LOG(DEBUG) << "handling dead node, done for mac " << mac;
}

bool son_actions::validate_beacon_measurement_report(beerocks_message::sBeaconResponse11k report,
                                                     const std::string &sta_mac,
                                                     const std::string &bssid)
{
    if (report.rcpi > RCPI_MAX) {
        LOG(WARNING) << "RCPI Measurement is in reserved value range rcpi=" << report.rcpi;
    }

    return (report.rep_mode == 0) &&
           //      (report.rsni                                  >  0          ) &&
           (report.rcpi != RCPI_INVALID) &&
           //      (report.start_time                            >  0          ) &&
           //      (report.duration                              >  0          ) &&
           (report.channel > 0) && (tlvf::mac_to_string(report.sta_mac) == sta_mac) &&
           (tlvf::mac_to_string(report.bssid) == bssid);
}

/**
 * @brief Check if the operating classes of @a radio_basic_caps matches any of the operating classes
 *        in @a bss_info_conf
 *
 * @param radio_basic_caps The AP Radio Basic Capabilities TLV of the radio
 * @param bss_info_conf The BSS Info we try to configure
 * @return true if one of the operating classes overlaps, false if they are disjoint
 */
bool son_actions::has_matching_operating_class(
    wfa_map::tlvApRadioBasicCapabilities &radio_basic_caps,
    const wireless_utils::sBssInfoConf &bss_info_conf)
{
    for (uint8_t i = 0; i < radio_basic_caps.operating_classes_info_list_length(); i++) {
        auto operating_class_info = std::get<1>(radio_basic_caps.operating_classes_info_list(i));
        for (auto operating_class : bss_info_conf.operating_class) {
            if (operating_class == operating_class_info.operating_class()) {
                return true;
            }
        }
    }
    return false;
}

bool son_actions::send_cmdu_to_agent(const sMacAddr &dest_mac, ieee1905_1::CmduMessageTx &cmdu_tx,
                                     db &database, const std::string &radio_mac)
{
    if (cmdu_tx.getMessageType() == ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE) {
        if (!database.is_prplmesh(dest_mac)) {
            // skip non-prplmesh agents
            return false;
        }
        auto beerocks_header = message_com::get_beerocks_header(cmdu_tx);
        if (!beerocks_header) {
            LOG(ERROR) << "Failed getting beerocks_header!";
            return false;
        }

        beerocks_header->actionhdr()->radio_mac() = tlvf::mac_from_string(radio_mac);
        beerocks_header->actionhdr()->direction() = beerocks::BEEROCKS_DIRECTION_AGENT;
    }

    auto controller_ctx = database.get_controller_ctx();
    if (controller_ctx == nullptr) {
        LOG(ERROR) << "controller_ctx == nullptr";
        return false;
    }

    return controller_ctx->send_cmdu_to_broker(cmdu_tx, dest_mac, database.get_local_bridge_mac());
}

bool son_actions::send_ap_config_renew_msg(ieee1905_1::CmduMessageTx &cmdu_tx, db &database)
{
    // Create AP-Configuration renew message
    auto cmdu_header =
        cmdu_tx.create(0, ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_RENEW_MESSAGE);
    if (!cmdu_header) {
        LOG(ERROR) << "Failed building IEEE1905 AP_AUTOCONFIGURATION_RENEW_MESSAGE";
        return false;
    }

    // Add MAC address TLV
    auto tlvAlMac = cmdu_tx.addClass<ieee1905_1::tlvAlMacAddress>();
    if (!tlvAlMac) {
        LOG(ERROR) << "Failed addClass ieee1905_1::tlvAlMacAddress";
        return false;
    }
    tlvAlMac->mac() = database.get_local_bridge_mac();

    // Add Supported-Role TLV
    auto tlvSupportedRole = cmdu_tx.addClass<ieee1905_1::tlvSupportedRole>();
    if (!tlvSupportedRole) {
        LOG(ERROR) << "Failed addClass ieee1905_1::tlvSupportedRole";
        return false;
    }
    tlvSupportedRole->value() = ieee1905_1::tlvSupportedRole::REGISTRAR;

    // Add Supported-Frequency-Band TLV
    auto tlvSupportedFreqBand = cmdu_tx.addClass<ieee1905_1::tlvSupportedFreqBand>();
    if (!tlvSupportedFreqBand) {
        LOG(ERROR) << "Failed addClass ieee1905_1::tlvSupportedFreqBand";
        return false;
    }
    // According to the Multi-AP Specification Version 2.0 section 7.1
    // Ragardless of what is sent here, the Agent will handle the Renew eitherway
    tlvSupportedFreqBand->value() = ieee1905_1::tlvSupportedFreqBand::eValue(0);

    LOG(INFO) << "Send AP_AUTOCONFIGURATION_RENEW_MESSAGE";
    return son_actions::send_cmdu_to_agent(network_utils::MULTICAST_1905_MAC_ADDR, cmdu_tx,
                                           database);
}

bool son_actions::send_topology_query_msg(const sMacAddr &dest_mac,
                                          ieee1905_1::CmduMessageTx &cmdu_tx, db &database)
{
    if (!cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_QUERY_MESSAGE)) {
        LOG(ERROR) << "Failed building TOPOLOGY_QUERY_MESSAGE message!";
        return false;
    }
    auto tlvProfile2MultiApProfile = cmdu_tx.addClass<wfa_map::tlvProfile2MultiApProfile>();
    if (!tlvProfile2MultiApProfile) {
        LOG(ERROR) << "addClass wfa_map::tlvProfile2MultiApProfile failed";
        return false;
    }
    return send_cmdu_to_agent(dest_mac, cmdu_tx, database);
}
