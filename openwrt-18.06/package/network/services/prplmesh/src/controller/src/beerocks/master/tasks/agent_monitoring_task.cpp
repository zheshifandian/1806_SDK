/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "agent_monitoring_task.h"
#include "../db/db_algo.h"
#include "../son_actions.h"

#include <bpl/bpl_cfg.h>
#include <easylogging++.h>
#include <tlvf/ieee_1905_1/tlv1905NeighborDevice.h>
#include <tlvf/ieee_1905_1/tlvDeviceInformation.h>
#include <tlvf/wfa_map/tlvApExtendedMetrics.h>
#include <tlvf/wfa_map/tlvApOperationalBSS.h>
#include <tlvf/wfa_map/tlvClientAssociationEvent.h>
#include <tlvf/wfa_map/tlvMetricReportingPolicy.h>
#include <tlvf/wfa_map/tlvProfile2Default802dotQSettings.h>
#include <tlvf/wfa_map/tlvProfile2RadioMetrics.h>
#include <tlvf/wfa_map/tlvProfile2TrafficSeparationPolicy.h>
#include <tlvf/wfa_map/tlvProfile2UnsuccessfulAssociationPolicy.h>

using namespace beerocks;
using namespace net;
using namespace son;

agent_monitoring_task::agent_monitoring_task(db &database_, ieee1905_1::CmduMessageTx &cmdu_tx_,
                                             task_pool &tasks_, const std::string &task_name_)
    : task(task_name_), database(database_), cmdu_tx(cmdu_tx_), tasks(tasks_)
{
}

void agent_monitoring_task::work()
{
    if (!m_ap_autoconfig_renew_sent) {
        /**
         * Usually, the ap-configuration sequence is started when the agent sends the ap-config
         * search request message, to which the controller responds with the ap-config response
         * message.
         * But with the scenario presented in PPM-1390, the controller process is restarted and
         * loads without ever receiving an ap-config search request message.
         * To resolve this, we can initialize the ap-config "handshake" by sending a ap-config
         * renew message, informing the agent of a newly restarted controller process.
         * To make sure all the tasks in the controller are running correctly, we need to send
         * the ap-config renew message after the controller finished its startup function,
         * during the event loop handling.
         * Doing this will give all the controller tasks enough time to finish their
         * initialization.
         * Since the agent_monitoring_task is responsible for handling the AGENT_JOIN as well as
         * the WSC autoconfiguration, it was suggested as the best place to send the ap-config
         * renew message from.
         **/
        son_actions::send_ap_config_renew_msg(cmdu_tx, database);
        m_ap_autoconfig_renew_sent = true;
    }
}

bool agent_monitoring_task::handle_ieee1905_1_msg(const sMacAddr &src_mac,
                                                  ieee1905_1::CmduMessageRx &cmdu_rx)
{
    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_WSC_MESSAGE: {
        auto tlvWsc = cmdu_rx.getClass<ieee1905_1::tlvWsc>();
        if (!tlvWsc) {
            LOG(ERROR) << "getClass<ieee1905_1::tlvWsc> failed";
            return false;
        }

        auto m1 = WSC::m1::parse(*tlvWsc);
        if (!m1) {
            LOG(INFO) << "Not a valid M1 - Ignoring WSC CMDU";
            return false;
        }
        return start_task(src_mac, m1, cmdu_rx);
    }
    case ieee1905_1::eMessageType::TOPOLOGY_RESPONSE_MESSAGE: {
        start_agent_monitoring(src_mac, cmdu_rx);
        break;
    }
    case ieee1905_1::eMessageType::AP_METRICS_RESPONSE_MESSAGE: {
        save_bss_statistics(cmdu_rx);
        save_radio_statistics(src_mac, cmdu_rx);
        break;
    }
    default: {
        return false;
    }
    }
    return true;
}

void agent_monitoring_task::handle_event(int event_type, void *obj)
{
    switch (event_type) {
    case (STATE_DISCONNECTED): {
        std::string agent_mac = *static_cast<std::string *>(obj);

        auto agent = database.m_agents.get(tlvf::mac_from_string(agent_mac));
        if (!agent) {
            LOG(INFO) << "Agent with mac is not found in database mac=" << agent_mac;
            return;
        }

        dm_add_agent_disconnected_event(agent->al_mac);
        break;
    }
    default:
        break;
    }
}

bool agent_monitoring_task::start_agent_monitoring(const sMacAddr &src_mac,
                                                   ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto tlvDeviceInformation = cmdu_rx.getClass<ieee1905_1::tlvDeviceInformation>();
    if (!tlvDeviceInformation) {
        LOG(ERROR) << "ieee1905_1::tlvDeviceInformation not found";
        return false;
    }

    const auto &al_mac = tlvDeviceInformation->mac();
    auto ap_op_bss_tlv = cmdu_rx.getClass<wfa_map::tlvApOperationalBSS>();
    if (!ap_op_bss_tlv) {
        LOG(ERROR) << "ieee1905_1::tlvApOperationalBSS not found";
        return false;
    }

    for (uint8_t i = 0; i < ap_op_bss_tlv->radio_list_length(); i++) {
        auto radio_entry   = std::get<1>(ap_op_bss_tlv->radio_list(i));
        auto ruid          = radio_entry.radio_uid();
        auto bsses_from_m2 = m_bss_configured[ruid];

        if (radio_entry.radio_bss_list_length() != bsses_from_m2.size()) {

            // Not all BSSes from M2 configured by Agents radio
            return false;
        }

        for (uint8_t j = 0; j < radio_entry.radio_bss_list_length(); j++) {
            auto bss_entry = std::get<1>(radio_entry.radio_bss_list(j));
            bool found     = false;

            for (const auto &bss_from_m2 : bsses_from_m2) {
                if (bss_from_m2.ssid == bss_entry.ssid_str()) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                LOG(WARNING) << "Configured BSS [" << bss_entry.ssid_str()
                             << "] for radio: " << ruid << " of device " << al_mac
                             << "came from nowhere (wasn't specified in M2)";
                return false;
            }
        }
        for (const auto &bss_from_m2 : bsses_from_m2) {
            bool found = false;

            for (uint8_t j = 0; j < radio_entry.radio_bss_list_length(); j++) {
                auto bss_entry = std::get<1>(radio_entry.radio_bss_list(j));
                if (bss_from_m2.ssid == bss_entry.ssid_str()) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                LOG(WARNING) << "BSS [" << bss_from_m2.ssid
                             << "] reported in M2 was not configured for : " << ruid
                             << " of device " << al_mac;
                return false;
            }
        }
    }

    // Delete radio entry detected as operational Agent from m_bss_configured.
    for (uint8_t i = 0; i < ap_op_bss_tlv->radio_list_length(); i++) {
        auto radio_entry = std::get<1>(ap_op_bss_tlv->radio_list(i));
        m_bss_configured.erase(radio_entry.radio_uid());
    }

    if (m_agents.count(src_mac)) {
        dm_add_agent_disconnected_event(src_mac); // Agent reconnecting.

        if (!database.dm_check_objects_limit(m_agents[src_mac], MAX_EVENT_HISTORY_SIZE)) {
            return false;
        }
    }
    auto agent_connected_event_path = dm_add_agent_connected_event(src_mac, ap_op_bss_tlv, cmdu_rx);

    if (agent_connected_event_path.empty() && NBAPI_ON) {
        LOG(ERROR) << "Failed to add AgentConnectedEvent";
        return false;
    }
    m_agents[src_mac].push(agent_connected_event_path);

    if (!dm_add_neighbor_to_agent_connected_event(agent_connected_event_path, cmdu_rx)) {
        LOG(ERROR) << "Failed to add " << agent_connected_event_path << ".Neighbor";
        return false;
    }
    if (database.get_agent_monitoring_task_id() == db::TASK_ID_NOT_FOUND) {
        database.assign_agent_monitoring_task_id(id);
    }
    return true;
}

bool agent_monitoring_task::start_task(const sMacAddr &src_mac, std::shared_ptr<WSC::m1> m1,
                                       ieee1905_1::CmduMessageRx &cmdu_rx)
{
    if (!send_tlv_metric_reporting_policy(src_mac, m1, cmdu_rx, cmdu_tx)) {
        LOG(ERROR) << "Failed to send Metric Reporting Policy to radio agent=" << src_mac;
    }
    if (!send_tlv_empty_channel_selection_request(src_mac, cmdu_tx)) {
        LOG(ERROR) << "Failed to send Channel Selection Request to radio agent=" << src_mac;
    }
    if (!database.setting_certification_mode()) {
        // trigger Topology query
        LOG(TRACE) << "Sending Topology Query to " << src_mac;
        son_actions::send_topology_query_msg(src_mac, cmdu_tx, database);

        // trigger channel selection
        if (!cmdu_tx.create(0, ieee1905_1::eMessageType::CHANNEL_PREFERENCE_QUERY_MESSAGE)) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);
    }
    if (!database.setting_certification_mode()) {
        // trigger AP capability query
        if (!cmdu_tx.create(0, ieee1905_1::eMessageType::AP_CAPABILITY_QUERY_MESSAGE)) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);
    }
    return true;
}

bool agent_monitoring_task::send_tlv_metric_reporting_policy(const sMacAddr &dst_mac,
                                                             std::shared_ptr<WSC::m1> m1,
                                                             ieee1905_1::CmduMessageRx &cmdu_rx,
                                                             ieee1905_1::CmduMessageTx &cmdu_tx)
{
    auto radio_basic_caps = cmdu_rx.getClass<wfa_map::tlvApRadioBasicCapabilities>();
    if (!radio_basic_caps) {
        LOG(ERROR) << "getClass<wfa_map::tlvApRadioBasicCapabilities> failed";
        return false;
    }

    auto ruid                  = radio_basic_caps->radio_uid();
    auto al_mac                = m1->mac_addr();
    const auto &bss_info_confs = database.get_bss_info_configuration(m1->mac_addr());
    uint8_t num_bsss           = 0;

    for (const auto &bss_info_conf : bss_info_confs) {
        // Check if the radio supports it
        if (!son_actions::has_matching_operating_class(*radio_basic_caps, bss_info_conf)) {
            continue;
        }
        if (num_bsss >= radio_basic_caps->maximum_number_of_bsss_supported()) {
            LOG(INFO) << "Configured BSSes exceed maximum for " << al_mac << " radio " << ruid;
            break;
        }
        m_bss_configured[ruid].push_back(bss_info_conf);
        num_bsss++;
    }

    if (!cmdu_tx.create(0, ieee1905_1::eMessageType::MULTI_AP_POLICY_CONFIG_REQUEST_MESSAGE)) {
        LOG(ERROR) << "Failed building MULTI_AP_POLICY_CONFIG_REQUEST_MESSAGE ! ";
        return false;
    }

    if (num_bsss) {
        add_traffic_policy_tlv(database, cmdu_tx, m1);
        add_profile_2default_802q_settings_tlv(database, cmdu_tx, m1);
    }

    auto metric_reporting_policy_tlv = cmdu_tx.addClass<wfa_map::tlvMetricReportingPolicy>();
    if (!metric_reporting_policy_tlv) {
        LOG(ERROR) << "addClass wfa_map::tlvMetricReportingPolicy has failed";
        return false;
    }

    metric_reporting_policy_tlv->metrics_reporting_interval_sec() =
        database.config.link_metrics_request_interval_seconds.count();

    // Add one radio configuration to list
    // TODO Multiple radio can be implemented within one message (PPM-1139)
    if (!metric_reporting_policy_tlv->alloc_metrics_reporting_conf_list()) {
        LOG(ERROR) << "Failed to add metrics_reporting_conf to tlvMetricReportingPolicy";
        return false;
    }

    auto agent = database.m_agents.get(dst_mac);
    if (!agent) {
        LOG(ERROR) << "Agent with mac is not found in database mac=" << dst_mac;
        return false;
    }

    if (agent->profile > wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_1) {
        auto unsuccessful_association_policy_tlv =
            cmdu_tx.addClass<wfa_map::tlvProfile2UnsuccessfulAssociationPolicy>();

        if (!unsuccessful_association_policy_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvProfile2UnsuccessfulAssociationPolicy has failed";
            return false;
        }

        unsuccessful_association_policy_tlv->report_unsuccessful_associations().report =
            database.config.unsuccessful_assoc_report_policy;

        unsuccessful_association_policy_tlv->maximum_reporting_rate() =
            database.config.unsuccessful_assoc_max_reporting_rate;
    }

    auto tuple = metric_reporting_policy_tlv->metrics_reporting_conf_list(0);
    if (!std::get<0>(tuple)) {
        LOG(ERROR) << "Failed to get metrics_reporting_conf[0"
                   << "] from TLV_METRIC_REPORTING_POLICY";
        return false;
    }

    auto &reporting_conf     = std::get<1>(tuple);
    reporting_conf.radio_uid = ruid;
    reporting_conf.policy.include_associated_sta_link_metrics_tlv_in_ap_metrics_response  = 1;
    reporting_conf.policy.include_associated_sta_traffic_stats_tlv_in_ap_metrics_response = 1;
    reporting_conf.policy.include_associated_wifi_6_sta_status_report_tlv_in_ap_metrics_response =
        1;

    reporting_conf.sta_metrics_reporting_rcpi_threshold                  = 0;
    reporting_conf.sta_metrics_reporting_rcpi_hysteresis_margin_override = 0;
    reporting_conf.ap_channel_utilization_reporting_threshold            = 0;

    return son_actions::send_cmdu_to_agent(dst_mac, cmdu_tx, database);
}

bool agent_monitoring_task::send_tlv_empty_channel_selection_request(
    const sMacAddr &dst_mac, ieee1905_1::CmduMessageTx &cmdu_tx)
{
    if (!cmdu_tx.create(0, ieee1905_1::eMessageType::CHANNEL_SELECTION_REQUEST_MESSAGE)) {
        LOG(ERROR) << "Failed building CHANNEL_SELECTION_REQUEST_MESSAGE ! ";
        return false;
    }
    return son_actions::send_cmdu_to_agent(dst_mac, cmdu_tx, database);
}

bool agent_monitoring_task::add_profile_2default_802q_settings_tlv(
    db &database, ieee1905_1::CmduMessageTx &cmdu_tx, std::shared_ptr<WSC::m1> m1)
{
    auto default_8021q_config = database.get_default_8021q_setting(m1->mac_addr());
    if (default_8021q_config.primary_vlan_id > 0) {
        auto tlv_default_8021q_settings =
            cmdu_tx.addClass<wfa_map::tlvProfile2Default802dotQSettings>();
        if (!tlv_default_8021q_settings) {
            LOG(ERROR) << "Failed adding tlvProfile2Default802dotQSettings";
            return false;
        }
        tlv_default_8021q_settings->primary_vlan_id() = default_8021q_config.primary_vlan_id;
        tlv_default_8021q_settings->default_pcp()     = default_8021q_config.default_pcp;
    }
    return true;
}

bool agent_monitoring_task::add_traffic_policy_tlv(db &database, ieee1905_1::CmduMessageTx &cmdu_tx,
                                                   std::shared_ptr<WSC::m1> m1)
{
    auto traffic_separation_configs =
        database.get_traffic_separataion_configuration(m1->mac_addr());
    if (!traffic_separation_configs.empty()) {
        auto tlv_traffic_policy = cmdu_tx.addClass<wfa_map::tlvProfile2TrafficSeparationPolicy>();
        if (!tlv_traffic_policy) {
            LOG(ERROR) << "Failed adding tlvProfile2TrafficSeparationPolicy";
            return false;
        }
        for (auto &config : traffic_separation_configs) {
            auto ssid_vlan_id_entry = tlv_traffic_policy->create_ssids_vlan_id_list();
            if (!ssid_vlan_id_entry) {
                LOG(ERROR) << "Failed creating ssid_vlan_id entry";
                return false;
            }
            if (!ssid_vlan_id_entry->set_ssid_name(config.ssid)) {
                LOG(ERROR) << "Failed setting ssid";
                return false;
            }
            ssid_vlan_id_entry->vlan_id() = config.vlan_id;
            if (!tlv_traffic_policy->add_ssids_vlan_id_list(ssid_vlan_id_entry)) {
                LOG(ERROR) << "Failed adding ssid_vlan_entry";
                return false;
            }
        }
    }
    return true;
}

void agent_monitoring_task::dm_add_sta_to_agent_connected_event(
    const std::string &obj_path, const sMacAddr &bssid,
    std::shared_ptr<wfa_map::tlvAssociatedClients> &assoc_client_tlv)
{
    auto ambiorix_dm = database.get_ambiorix_obj();

    if (!ambiorix_dm) {
        LOG(ERROR) << "Failed to get Ambiorix datamodel";
        return;
    }
    for (int i = 0; i < assoc_client_tlv->bss_list_length(); i++) {
        auto bss = std::get<1>(assoc_client_tlv->bss_list(i));

        if (bssid == bss.bssid()) {
            for (int j = 0; j < bss.clients_associated_list_length(); j++) {
                auto sta      = std::get<1>(bss.clients_associated_list(j));
                auto sta_path = ambiorix_dm->add_instance(obj_path + ".STA");

                if (sta_path.empty() && NBAPI_ON) {
                    LOG(ERROR) << "Failed to add " << obj_path << ".STA, mac: " << sta.mac();
                    return;
                }
                ambiorix_dm->set_current_time(sta_path);
                ambiorix_dm->set(sta_path, "MACAddress", sta.mac());
                ambiorix_dm->set(sta_path, "LastConnectTime",
                                 sta.time_since_last_association_sec());
            }
            return;
        }
    }
}

std::string agent_monitoring_task::dm_add_agent_connected_event(
    const sMacAddr &agent_mac, std::shared_ptr<wfa_map::tlvApOperationalBSS> &ap_op_bss_tlv,
    ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto ambiorix_dm = database.get_ambiorix_obj();

    if (!ambiorix_dm) {
        LOG(ERROR) << "Failed to get Ambiorix datamodel";
        return {};
    }

    auto tlv_assoc_client = cmdu_rx.getClass<wfa_map::tlvAssociatedClients>();

    if (!tlv_assoc_client) {
        LOG(DEBUG) << "getClass tlvAssociatedClients failed";
    }

    std::string agent_connected_event_path =
        "Device.WiFi.DataElements.AgentConnectedEvent.AgentConnected";
    std::string agent_connected_path = ambiorix_dm->add_instance(agent_connected_event_path);

    if (agent_connected_path.empty() && NBAPI_ON) {
        LOG(ERROR) << "Failed to add " << agent_connected_event_path << ", mac: " << agent_mac;
        return {};
    }

    ambiorix_dm->set(agent_connected_path, "ID", agent_mac);
    ambiorix_dm->set_current_time(agent_connected_path);
    for (int i = 0; i < ap_op_bss_tlv->radio_list_length(); i++) {
        auto radio      = std::get<1>(ap_op_bss_tlv->radio_list(i));
        auto radio_path = ambiorix_dm->add_instance(agent_connected_path + ".Radio");

        if (radio_path.empty() && NBAPI_ON) {
            LOG(ERROR) << "Failed to add " << agent_connected_path
                       << ".Radio, mac: " << radio.radio_uid();
            return agent_connected_path;
        }
        ambiorix_dm->set(radio_path, "ID", radio.radio_uid());
        for (int j = 0; j < radio.radio_bss_list_length(); j++) {
            auto bss      = std::get<1>(radio.radio_bss_list(j));
            auto bss_path = ambiorix_dm->add_instance(radio_path + ".BSS");

            if (bss_path.empty() && NBAPI_ON) {
                LOG(ERROR) << "Failed to add " << radio_path << ".BSS BSSID: " << bss.radio_bssid();
                return agent_connected_path;
            }
            ambiorix_dm->set(bss_path, "BSSID", bss.radio_bssid());
            ambiorix_dm->set(bss_path, "SSID", bss.ssid_str());
            if (tlv_assoc_client) {
                dm_add_sta_to_agent_connected_event(bss_path, bss.radio_bssid(), tlv_assoc_client);
            }
        }
    }
    return agent_connected_path;
}

bool agent_monitoring_task::dm_add_neighbor_to_agent_connected_event(
    const std::string &event_path, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto ambiorix_dm = database.get_ambiorix_obj();

    if (!ambiorix_dm) {
        LOG(ERROR) << "Failed to get Ambiorix datamodel";
        return {};
    }

    auto tlv1905NeighborDeviceList = cmdu_rx.getClassList<ieee1905_1::tlv1905NeighborDevice>();
    for (const auto &tlv1905NeighborDevice : tlv1905NeighborDeviceList) {
        if (!tlv1905NeighborDevice) {
            LOG(ERROR) << "getClassList<ieee1905_1::tlv1905NeighborDevice> failed";
            return false;
        }
        auto device_count = tlv1905NeighborDevice->mac_al_1905_device_length() /
                            sizeof(ieee1905_1::tlv1905NeighborDevice::sMacAl1905Device);
        for (size_t i = 0; i < device_count; i++) {
            const auto neighbor_al_mac_tuple = tlv1905NeighborDevice->mac_al_1905_device(i);

            if (!std::get<0>(neighbor_al_mac_tuple)) {
                LOG(ERROR) << "Failed to get al_mac element.";
                return false;
            }

            auto &neighbor_mac = std::get<1>(neighbor_al_mac_tuple).mac;
            auto neighbor_path = ambiorix_dm->add_instance(event_path + ".Neighbor");

            if (neighbor_path.empty() && NBAPI_ON) {
                LOG(ERROR) << "Failed to add " << event_path << ".Neighbor";
                return false;
            }
            if (!ambiorix_dm->set(neighbor_path, "ID", neighbor_mac)) {
                LOG(ERROR) << "Failed to set ID for : " << neighbor_path;
                return false;
            }
        }
    }
    return true;
}

void agent_monitoring_task::save_bss_statistics(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    for (auto ap_extended_metric_tlv : cmdu_rx.getClassList<wfa_map::tlvApExtendedMetrics>()) {
        auto bss_stats = std::make_shared<sBssStats>();

        bss_stats->unicast_bytes_sent       = ap_extended_metric_tlv->unicast_bytes_sent();
        bss_stats->unicast_bytes_received   = ap_extended_metric_tlv->unicast_bytes_received();
        bss_stats->multicast_bytes_sent     = ap_extended_metric_tlv->multicast_bytes_sent();
        bss_stats->multicast_bytes_received = ap_extended_metric_tlv->multicast_bytes_received();
        bss_stats->broadcast_bytes_sent     = ap_extended_metric_tlv->broadcast_bytes_sent();
        bss_stats->broadcast_bytes_received = ap_extended_metric_tlv->broadcast_bytes_received();
        m_bss_stats[ap_extended_metric_tlv->bssid()] = bss_stats;
    }
}

void agent_monitoring_task::save_radio_statistics(const sMacAddr &src_mac,
                                                  ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto radio_metrics_tlv = cmdu_rx.getClass<wfa_map::tlvProfile2RadioMetrics>();
    auto agent             = database.m_agents.get(src_mac);

    if (!agent) {
        LOG(ERROR) << "Agent with mac is not found in database mac=" << src_mac;
        return;
    }
    if (!radio_metrics_tlv) {
        if (agent->profile >
            wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_1) {
            LOG(ERROR) << "Agent " << src_mac << " has profile " << agent->profile
                       << ", it should have included Profile2 Radio Metrics!";
        }
        return;
    }

    auto radio_stats = std::make_shared<sRadioStats>();

    radio_stats->noise                            = radio_metrics_tlv->noise();
    radio_stats->transmit                         = radio_metrics_tlv->transmit();
    radio_stats->receive_self                     = radio_metrics_tlv->receive_self();
    radio_stats->receive_other                    = radio_metrics_tlv->receive_other();
    radio_stats->utilization                      = 0; // TO DO: PPM-1136
    m_radio_stats[radio_metrics_tlv->radio_uid()] = radio_stats;
}

bool agent_monitoring_task::dm_add_agent_disconnected_event(const sMacAddr &agent_mac)
{
    if (!database.dm_check_objects_limit(m_disconnected, MAX_EVENT_HISTORY_SIZE)) {
        LOG(ERROR) << "Failed to remove overflow AgentDisconnectedEvent objects.";
        return false;
    }

    auto ambiorix_dm = database.get_ambiorix_obj();

    if (!ambiorix_dm) {
        LOG(ERROR) << "ambiorix_dm is nullptr";
        return false;
    }

    std::string agent_discon_event_path =
        "Device.WiFi.DataElements.AgentDisconnectedEvent.AgentDisconnected";
    std::string agent_discon_path = ambiorix_dm->add_instance(agent_discon_event_path);

    if (agent_discon_path.empty()) {
        LOG(ERROR) << "Failed to add " << agent_discon_event_path << " for mac: " << agent_mac;
        return false;
    }
    m_disconnected.push(agent_discon_path);
    if (!ambiorix_dm->set(agent_discon_path, "ID", agent_mac)) {
        LOG(ERROR) << "Failed to set " << agent_discon_path << "ID";
        return false;
    }
    if (!dm_set_agent_disconnected_event_params(agent_discon_path, agent_mac)) {
        return false;
    }
    return true;
}

bool agent_monitoring_task::dm_set_agent_disconnected_event_params(
    const std::string &agent_discon_path, const sMacAddr &agent_mac)
{
    bool ok          = true;
    auto ambiorix_dm = database.get_ambiorix_obj();

    if (!ambiorix_dm) {
        LOG(ERROR) << "ambiorix_dm is nullptr";
        return false;
    }

    auto agent = database.m_agents.get(agent_mac);

    if (!agent) {
        LOG(ERROR) << "Agent with mac is not found in database mac=" << agent_mac;
        return false;
    }
    for (const auto &radio : agent->radios) {

        auto radio_path = ambiorix_dm->add_instance(agent_discon_path + ".Radio");

        if (radio_path.empty() && NBAPI_ON) {
            LOG(ERROR) << "Failed to add " << agent_discon_path + ".Radio"
                       << ". MAC: " << radio.first;
            return false;
        }
        ok &= ambiorix_dm->set(radio_path, "ID", radio.first);
        auto radio_stats = m_radio_stats.get(radio.first);

        if (radio_stats) {
            ok &= ambiorix_dm->set(radio_path, "Utilization", radio_stats->utilization);
            ok &= ambiorix_dm->set(radio_path, "Transmit", radio_stats->transmit);
            ok &= ambiorix_dm->set(radio_path, "ReceiveSelf", radio_stats->receive_self);
            ok &= ambiorix_dm->set(radio_path, "ReceiveOther", radio_stats->receive_other);
            ok &= ambiorix_dm->set(radio_path, "Noise", radio_stats->noise);
            if (!ok) {
                LOG(ERROR) << "Failed to set parameter for " << radio_path;
                return false;
            }
        }
        for (const auto &bss : radio.second->bsses) {

            auto bss_path = ambiorix_dm->add_instance(radio_path + ".BSS");

            if (bss_path.empty() && NBAPI_ON) {
                LOG(ERROR) << "Failed to add " << radio_path + ".BSS"
                           << ". BSSID: " << bss.first;
                return false;
            }
            ok &= ambiorix_dm->set(bss_path, "BSSID", bss.first);
            auto bss_stats = m_bss_stats.get(bss.first);

            if (bss_stats) {
                ok &= ambiorix_dm->set(bss_path, "UnicastBytesSent", bss_stats->unicast_bytes_sent);
                ok &= ambiorix_dm->set(bss_path, "UnicastBytesReceived",
                                       bss_stats->unicast_bytes_received);
                ok &= ambiorix_dm->set(bss_path, "MulticastBytesSent",
                                       bss_stats->multicast_bytes_sent);
                ok &= ambiorix_dm->set(bss_path, "MulticastBytesReceived",
                                       bss_stats->multicast_bytes_received);
                ok &= ambiorix_dm->set(bss_path, "BroadcastBytesSent",
                                       bss_stats->broadcast_bytes_sent);
                ok &= ambiorix_dm->set(bss_path, "BroadcastBytesReceived",
                                       bss_stats->broadcast_bytes_received);
                if (!ok) {
                    LOG(ERROR) << "Failed to set parameter for " << bss_path;
                    return false;
                }
            }
            for (const auto &sta : bss.second->connected_stations) {

                auto sta_path = ambiorix_dm->add_instance(bss_path + ".STA");

                if (sta_path.empty() && NBAPI_ON) {
                    LOG(ERROR) << "Failed to add " << bss_path + ".STA"
                               << ". MAC: " << sta.first;
                    return false;
                }
                if (!ambiorix_dm->set(sta_path, "MACAddress", sta.first)) {
                    LOG(ERROR) << "Failed to set MACAddress for " << sta_path;
                    return false;
                }
            }
        }
    }

    auto neighbors = database.get_1905_1_neighbors(agent_mac);

    for (const auto &neighbor : neighbors) {
        auto neighbor_path = ambiorix_dm->add_instance(agent_discon_path + ".Neighbor");

        if (neighbor_path.empty() && NBAPI_ON) {
            LOG(ERROR) << "Failed to add " << agent_discon_path + ".Neighbor"
                       << ". MAC: " << neighbor;
            return false;
        }
        ok &= ambiorix_dm->set(neighbor_path, "ID", neighbor);
    }
    if (!ok) {
        LOG(ERROR) << "Failed to set some of parameter for " << agent_discon_path;
    }
    return ok;
}
