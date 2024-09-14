/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "link_metrics_task.h"

#include "../db/db_algo.h"
#include "../son_actions.h"

#include <beerocks/tlvf/beerocks_message.h>
#include <tlvf/ieee_1905_1/tlvLinkMetricQuery.h>

#include <easylogging++.h>

using namespace beerocks;
using namespace son;

LinkMetricsTask::LinkMetricsTask(db &database_, ieee1905_1::CmduMessageTx &cmdu_tx_,
                                 ieee1905_1::CmduMessageTx &cert_cmdu_tx_, task_pool &tasks_)
    : task("link metrics task"), database(database_), cmdu_tx(cmdu_tx_),
      cert_cmdu_tx(cert_cmdu_tx_), tasks(tasks_)
{
    last_query_request = std::chrono::steady_clock::now();
}

void LinkMetricsTask::work()
{

    // Zero setting means do not sent any periodic request.
    if (database.config.link_metrics_request_interval_seconds == std::chrono::seconds::zero()) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto last_seen_delta =
        std::chrono::duration_cast<std::chrono::seconds>(now - last_query_request);

    if (last_seen_delta > database.config.link_metrics_request_interval_seconds) {
        if (!cmdu_tx.create(0, ieee1905_1::eMessageType::LINK_METRIC_QUERY_MESSAGE)) {
            LOG(ERROR) << "Failed building message!";
            return;
        }

        auto tlvLinkMetricQueryAllNeighbors =
            cmdu_tx.addClass<ieee1905_1::tlvLinkMetricQueryAllNeighbors>();

        if (tlvLinkMetricQueryAllNeighbors) {
            tlvLinkMetricQueryAllNeighbors->link_metrics_type() =
                ieee1905_1::BOTH_TX_AND_RX_LINK_METRICS;
        } else {
            LOG(ERROR) << "addClass ieee1905_1::tlvLinkMetricQueryAllNeighbors failed";
            return;
        }

        for (const auto &agent : database.get_all_connected_agents()) {
            son_actions::send_cmdu_to_agent(agent->al_mac, cmdu_tx, database);
        }
        last_query_request = std::chrono::steady_clock::now();
    }
    return;
}

bool LinkMetricsTask::handle_ieee1905_1_msg(const sMacAddr &src_mac,
                                            ieee1905_1::CmduMessageRx &cmdu_rx)
{
    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::LINK_METRIC_RESPONSE_MESSAGE: {
        handle_cmdu_1905_link_metric_response(src_mac, cmdu_rx);
        break;
    }
    default: {
        return false;
    }
    }
    return true;
}

bool LinkMetricsTask::handle_cmdu_1905_link_metric_response(const sMacAddr &src_mac,
                                                            ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(INFO) << "Received LINK_METRIC_RESPONSE_MESSAGE, mid=" << std::dec << int(mid)
              << " src_mac:" << src_mac;

    //getting reference for link metric data storage from db
    auto &link_metric_data          = database.get_link_metric_data_map();
    bool old_link_metrics_removed   = false;
    sMacAddr reporting_agent_al_mac = beerocks::net::network_utils::ZERO_MAC;

    // Neighbor related new metric data. The link metric information is collected in this structure
    // to be stored in the database link_metric_data_map.
    std::unordered_map<sMacAddr, son::node::link_metrics_data> new_link_metrics;

    // Interface related metric data. The link metric information is collected in this structure
    // per reporting device's interface, to be stored in the device interface stats.
    std::unordered_map<sMacAddr, ieee1905_1::tlvTransmitterLinkMetric::sLinkMetricInfo>
        iface_tx_link_metrics;
    std::unordered_map<sMacAddr, ieee1905_1::tlvReceiverLinkMetric::sLinkMetricInfo>
        iface_rx_link_metrics;

    auto tx_link_metrics = cmdu_rx.getClassList<ieee1905_1::tlvTransmitterLinkMetric>();

    if (tx_link_metrics.size() == 0) {
        LOG(DEBUG) << "getClassList ieee1905_1::tlvTransmitterLinkMetric contains zero metrics.";
    }

    for (const auto &tx_metric : tx_link_metrics) {

        if (tx_metric->reporter_al_mac() == beerocks::net::network_utils::ZERO_MAC) {
            LOG(ERROR) << "Zero MAC reported for Agent in tx_link_metrics";
            continue;
        }

        // Verify the MAC of Agent, if it is matching with rest of the metrics.
        // It assumes that the first one is the correct one.
        if (reporting_agent_al_mac != tx_metric->reporter_al_mac() &&
            reporting_agent_al_mac != beerocks::net::network_utils::ZERO_MAC) {
            LOG(ERROR) << "Reporting Agent MAC is different in reported tx_link_metrics "
                       << reporting_agent_al_mac << " " << tx_metric->reporter_al_mac();
            continue;
        }
        reporting_agent_al_mac = tx_metric->reporter_al_mac();

        // Clear all agent reports (neighbors) when receiving a new link metric from that agent
        if (!old_link_metrics_removed) {
            link_metric_data[reporting_agent_al_mac].clear();
            old_link_metrics_removed = true;
        }

        LOG(DEBUG) << "Received TLV_TRANSMITTER_LINK_METRIC from al_mac =" << reporting_agent_al_mac
                   << " reported neighbor al_mac =" << tx_metric->neighbor_al_mac();

        // Fill Tx data from TLV for specified Neighbor
        if (!new_link_metrics[tx_metric->neighbor_al_mac()].add_transmitter_link_metric(
                tx_metric)) {
            LOG(ERROR) << "Adding Tx Link Metric Data has failed for neighbor mac:"
                       << tx_metric->neighbor_al_mac();
        }
    }

    auto rx_link_metrics = cmdu_rx.getClassList<ieee1905_1::tlvReceiverLinkMetric>();

    if (rx_link_metrics.size() == 0) {
        LOG(DEBUG) << "getClassList ieee1905_1::tlvReceiverLinkMetric contains zero metrics.";
    }

    for (const auto &rx_metric : rx_link_metrics) {

        if (rx_metric->reporter_al_mac() == beerocks::net::network_utils::ZERO_MAC) {
            LOG(ERROR) << "Zero MAC reported for Agent in rx_link_metrics";
            continue;
        }

        // Controlling the MAC of Agent, if it is matching with rest of the metrics.
        // It assumes that the first one is the correct one.
        if (reporting_agent_al_mac != rx_metric->reporter_al_mac() &&
            reporting_agent_al_mac != beerocks::net::network_utils::ZERO_MAC) {
            LOG(ERROR) << "Reporting Agent MAC is different in reported rx_link_metrics "
                       << reporting_agent_al_mac << " " << rx_metric->reporter_al_mac();
            continue;
        }
        reporting_agent_al_mac = rx_metric->reporter_al_mac();

        // Clear agent all reports (neighbors) when receiving a new link metric from that agent
        if (!old_link_metrics_removed) {
            link_metric_data[reporting_agent_al_mac].clear();
            old_link_metrics_removed = true;
        }

        LOG(DEBUG) << "Received TLV_RECEIVER_LINK_METRIC from al_mac =" << reporting_agent_al_mac
                   << " reported neighbor al_mac =" << rx_metric->neighbor_al_mac();

        // Fill Rx data from TLV for specified Neighbor
        // Note: The Database node stats and the Datamodels' stats are not the same.
        // Therefore, client information in data model and in node DB might differ.
        if (!new_link_metrics[rx_metric->neighbor_al_mac()].add_receiver_link_metric(rx_metric)) {
            LOG(ERROR) << "Adding Rx Link Metric Data has failed for neighbor mac:"
                       << rx_metric->neighbor_al_mac();
        }
    }

    for (const auto &new_link : new_link_metrics) {

        // Add neighbor to Link Metric Data Map
        link_metric_data[reporting_agent_al_mac][new_link.first] = new_link.second;

        // Fill up Interface Link Metric to separate metrics according to Interface MACs.
        for (const auto &tx_link : new_link.second.transmitterLinkMetrics) {

            if (tx_link.rc_interface_mac == beerocks::net::network_utils::ZERO_MAC) {
                LOG(ERROR) << "Zero MAC Interface is reported for agent mac:"
                           << reporting_agent_al_mac << " and neighbor mac:" << new_link.first;
                continue;
            }

            // Check it for interface is already added or not
            auto iface = iface_tx_link_metrics.find(tx_link.rc_interface_mac);

            if (iface != iface_tx_link_metrics.end()) {
                LOG(DEBUG) << "Interface is already added with mac:" << tx_link.rc_interface_mac
                           << " so assign it as sum of rx stats.";

                iface->second.packet_errors += tx_link.link_metric_info.packet_errors;
                iface->second.transmitted_packets += tx_link.link_metric_info.transmitted_packets;
            } else {
                iface_tx_link_metrics.insert({tx_link.rc_interface_mac, tx_link.link_metric_info});
            }
        }

        for (const auto &rx_link : new_link.second.receiverLinkMetrics) {

            if (rx_link.rc_interface_mac == beerocks::net::network_utils::ZERO_MAC) {
                LOG(ERROR) << "Zero MAC Interface is reported for agent mac:"
                           << reporting_agent_al_mac << " and neighbor mac:" << new_link.first;
                continue;
            }

            // Check it for interface is already added or not
            auto iface = iface_rx_link_metrics.find(rx_link.rc_interface_mac);

            if (iface != iface_rx_link_metrics.end()) {
                LOG(DEBUG) << "Interface is already added with mac:" << rx_link.rc_interface_mac
                           << " so assign it as sum of tx stats.";

                iface->second.packet_errors += rx_link.link_metric_info.packet_errors;
                iface->second.packets_received += rx_link.link_metric_info.packets_received;
            } else {
                iface_rx_link_metrics.insert({rx_link.rc_interface_mac, rx_link.link_metric_info});
            }
        }
    }

    // Update data model of Device Interface Stats
    for (const auto &iface_tx_link : iface_tx_link_metrics) {
        database.dm_update_interface_tx_stats(reporting_agent_al_mac, iface_tx_link.first,
                                              iface_tx_link.second.transmitted_packets,
                                              iface_tx_link.second.packet_errors);
    }
    for (const auto &iface_rx_link : iface_rx_link_metrics) {
        database.dm_update_interface_rx_stats(reporting_agent_al_mac, iface_rx_link.first,
                                              iface_rx_link.second.packets_received,
                                              iface_rx_link.second.packet_errors);
    }

    LOG(DEBUG) << "Metrics data are added from al_mac = " << reporting_agent_al_mac << " with "
               << new_link_metrics.size() << " neighbors.";

    print_link_metric_map(link_metric_data);

    // TODO store the link metric response data in the DB and trigger the relevant task.
    // For now, this is only used for certification so update the certification cmdu.
    if (database.setting_certification_mode())
        construct_combined_infra_metric();

    return true;
}

void LinkMetricsTask::print_link_metric_map(
    std::unordered_map<sMacAddr, std::unordered_map<sMacAddr, son::node::link_metrics_data>> const
        &link_metric_data)
{
    LOG(DEBUG) << "Printing Link Metrics data map";
    for (auto const &pair_agent : link_metric_data) {
        LOG(DEBUG) << "  sent from al_mac= " << pair_agent.first;

        for (auto const &pair_neighbor : pair_agent.second) {
            LOG(DEBUG) << "  reporting neighbor al_mac= " << pair_neighbor.first;

            auto &vrx = pair_neighbor.second.receiverLinkMetrics;
            for (unsigned int i = 0; i < vrx.size(); ++i) {
                LOG(DEBUG) << "  rx interface metric data # " << i
                           << "  neighbor interface MAC=" << vrx[i].neighbor_interface_mac
                           << "  interface MAC=" << vrx[i].rc_interface_mac
                           << "  rssi= " << std::hex << int(vrx[i].link_metric_info.rssi_db)
                           << "  packets received= " << vrx[i].link_metric_info.packets_received;
            }

            auto &vtx = pair_neighbor.second.transmitterLinkMetrics;
            for (unsigned int i = 0; i < vtx.size(); i++) {
                LOG(DEBUG) << "  tx interface metric data # " << i
                           << "  neighbor interface MAC=" << vtx[i].neighbor_interface_mac
                           << "  interface MAC=" << vtx[i].rc_interface_mac
                           << "  phy_rate= " << std::hex << int(vtx[i].link_metric_info.phy_rate)
                           << "  packets transmitted= "
                           << vtx[i].link_metric_info.transmitted_packets;
            }
        }
    }
}

bool LinkMetricsTask::construct_combined_infra_metric()
{
    auto &link_metric_data = database.get_link_metric_data_map();

    if (!cert_cmdu_tx.create(0,
                             ieee1905_1::eMessageType::COMBINED_INFRASTRUCTURE_METRICS_MESSAGE)) {
        LOG(ERROR) << "cmdu creation of type COMBINED_INFRASTRUCTURE_METRICS_MESSAGE, has failed";
        return false;
    }

    for (auto &agent : link_metric_data) {

        for (auto &per_neighbor : agent.second) {

            auto &vrx = per_neighbor.second.receiverLinkMetrics;
            if (vrx.size()) {
                auto link_metric_rx_tlv =
                    cert_cmdu_tx.addClass<ieee1905_1::tlvReceiverLinkMetric>();
                if (!link_metric_rx_tlv) {
                    LOG(ERROR) << "addClass ieee1905_1::tlvReceiverLinkMetric failed";
                    return false;
                }
                link_metric_rx_tlv->reporter_al_mac() = agent.first;
                link_metric_rx_tlv->neighbor_al_mac() = per_neighbor.first;
                if (!link_metric_rx_tlv->alloc_interface_pair_info(vrx.size())) {
                    LOG(ERROR) << "alloc_interface_pair_info() has failed!";
                    return false;
                }
                size_t interface_idx = 0;
                for (auto &interface_pair_info : vrx) {
                    auto interface_tuple = link_metric_rx_tlv->interface_pair_info(interface_idx);
                    if (!std::get<0>(interface_tuple)) {
                        LOG(ERROR) << "getting interface entry has failed!";
                        return false;
                    }
                    std::get<1>(interface_tuple) = interface_pair_info;
                    interface_idx++;
                }
            }
            auto &vtx = per_neighbor.second.transmitterLinkMetrics;
            if (vtx.size()) {
                auto link_metric_tx_tlv =
                    cert_cmdu_tx.addClass<ieee1905_1::tlvTransmitterLinkMetric>();
                if (!link_metric_tx_tlv) {
                    LOG(ERROR) << "addClass ieee1905_1::tlvTransmitterLinkMetric failed";
                    return false;
                }
                link_metric_tx_tlv->reporter_al_mac() = agent.first;
                link_metric_tx_tlv->neighbor_al_mac() = per_neighbor.first;
                for (auto &interface_pair_info : vtx) {
                    if (!link_metric_tx_tlv->alloc_interface_pair_info()) {
                        LOG(ERROR) << "alloc_interface_pair_info() has failed!";
                        return false;
                    }
                    auto interface_idx =
                        link_metric_tx_tlv->interface_pair_info_length() /
                        sizeof(ieee1905_1::tlvReceiverLinkMetric::sInterfacePairInfo);
                    auto interface_tuple =
                        link_metric_tx_tlv->interface_pair_info(interface_idx - 1);
                    if (!std::get<0>(interface_tuple)) {
                        LOG(ERROR) << "getting interface entry has failed!";
                        return false;
                    }
                    std::get<1>(interface_tuple) = interface_pair_info;
                }
            }
        }
    }

    // Getting reference for ap metric data storage from db
    const auto &ap_metric_data = database.get_ap_metric_data_map();
    for (auto &it : ap_metric_data) {
        auto metric_data_per_agent = it.second;
        auto ap_metrics_tlv        = cert_cmdu_tx.addClass<wfa_map::tlvApMetrics>();
        if (!ap_metrics_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvApMetrics failed";
            return false;
        }
        ap_metrics_tlv->bssid()               = metric_data_per_agent.bssid;
        ap_metrics_tlv->channel_utilization() = metric_data_per_agent.channel_utilization;
        ap_metrics_tlv->number_of_stas_currently_associated() =
            metric_data_per_agent.number_of_stas_currently_associated;
        auto len = metric_data_per_agent.estimated_service_info_fields.size();
        if (!ap_metrics_tlv->alloc_estimated_service_info_field(len)) {
            LOG(ERROR) << "alloc_estimated_service_info_field() has failed!";
            return false;
        }
        std::copy(metric_data_per_agent.estimated_service_info_fields.begin(),
                  metric_data_per_agent.estimated_service_info_fields.end(),
                  ap_metrics_tlv->estimated_service_info_field());
        if (metric_data_per_agent.include_ac_bk) {
            ap_metrics_tlv->estimated_service_parameters().include_ac_bk = 0x1;
        }
        if (metric_data_per_agent.include_ac_vi) {
            ap_metrics_tlv->estimated_service_parameters().include_ac_vi = 0x1;
        }
        if (metric_data_per_agent.include_ac_vo) {
            ap_metrics_tlv->estimated_service_parameters().include_ac_vo = 0x1;
        }
    }

    cert_cmdu_tx.finalize();
    return true;
}
