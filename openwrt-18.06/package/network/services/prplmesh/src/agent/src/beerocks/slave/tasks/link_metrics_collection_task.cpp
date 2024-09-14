/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "link_metrics_collection_task.h"

#include "../agent_db.h"
#include "../backhaul_manager/backhaul_manager.h"
#include "../helpers/link_metrics/ieee802_11_link_metrics_collector.h"
#include "../helpers/link_metrics/ieee802_3_link_metrics_collector.h"
#include "../helpers/media_type.h"
#include "../traffic_separation.h"

#include <beerocks/tlvf/beerocks_message_backhaul.h>

#include <tlvf/ieee_1905_1/tlvLinkMetricQuery.h>
#include <tlvf/ieee_1905_1/tlvLinkMetricResultCode.h>
#include <tlvf/ieee_1905_1/tlvReceiverLinkMetric.h>
#include <tlvf/ieee_1905_1/tlvTransmitterLinkMetric.h>
#include <tlvf/wfa_map/tlvApExtendedMetrics.h>
#include <tlvf/wfa_map/tlvApMetricQuery.h>
#include <tlvf/wfa_map/tlvAssociatedStaExtendedLinkMetrics.h>
#include <tlvf/wfa_map/tlvAssociatedStaTrafficStats.h>
#include <tlvf/wfa_map/tlvAssociatedWiFi6StaStatusReport.h>
#include <tlvf/wfa_map/tlvBeaconMetricsQuery.h>
#include <tlvf/wfa_map/tlvMetricReportingPolicy.h>
#include <tlvf/wfa_map/tlvProfile2Default802dotQSettings.h>
#include <tlvf/wfa_map/tlvProfile2RadioMetrics.h>
#include <tlvf/wfa_map/tlvProfile2TrafficSeparationPolicy.h>
#include <tlvf/wfa_map/tlvStaMacAddressType.h>

namespace beerocks {

LinkMetricsCollectionTask::LinkMetricsCollectionTask(BackhaulManager &btl_ctx,
                                                     ieee1905_1::CmduMessageTx &cmdu_tx)
    : Task(eTaskType::LINK_METRICS_COLLECTION), m_btl_ctx(btl_ctx), m_cmdu_tx(cmdu_tx)
{
}

bool LinkMetricsCollectionTask::handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx,
                                            uint32_t iface_index, const sMacAddr &dst_mac,
                                            const sMacAddr &src_mac, int fd,
                                            std::shared_ptr<beerocks_header> beerocks_header)
{
    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::LINK_METRIC_QUERY_MESSAGE: {
        handle_link_metric_query(cmdu_rx, src_mac);
        break;
    }
    case ieee1905_1::eMessageType::COMBINED_INFRASTRUCTURE_METRICS_MESSAGE: {
        handle_combined_infrastructure_metrics(cmdu_rx, src_mac);
        break;
    }
    case ieee1905_1::eMessageType::BEACON_METRICS_QUERY_MESSAGE: {
        handle_beacon_metrics_query(cmdu_rx, src_mac);
        break;
    }
    case ieee1905_1::eMessageType::ASSOCIATED_STA_LINK_METRICS_QUERY_MESSAGE: {
        handle_associated_sta_link_metrics_query(cmdu_rx, src_mac);
        break;
    }
    case ieee1905_1::eMessageType::AP_METRICS_QUERY_MESSAGE: {
        handle_ap_metrics_query(cmdu_rx, src_mac);
        break;
    }
    case ieee1905_1::eMessageType::AP_METRICS_RESPONSE_MESSAGE: {
        handle_ap_metrics_response(cmdu_rx, src_mac);
        break;
    }
    default: {
        // Message was not handled, therefore return false.
        return false;
    }
    }
    return true;
}

void LinkMetricsCollectionTask::work()
{
    auto db = AgentDB::get();

    if (!db->statuses.ap_autoconfiguration_completed) {
        return;
    }

    /**
     * Get current time. It is later used to compute elapsed time since some start time and
     * check if a timeout has expired to perform periodic actions.
     */
    auto now = std::chrono::steady_clock::now();

    /**
     * If periodic AP metrics reporting is enabled, check if time interval has elapsed and if
     * so, then report AP metrics.
     */
    if (db->link_metrics_policy.reporting_interval_sec !=
        m_ap_metrics_reporting_info.reporting_interval_s) {
        m_ap_metrics_reporting_info.reporting_interval_s =
            db->link_metrics_policy.reporting_interval_sec;
        m_ap_metrics_reporting_info.last_reporting_time_point = std::chrono::steady_clock::now();
    }
    if (0 == m_ap_metrics_reporting_info.reporting_interval_s) {
        return;
    }

    int elapsed_time_s = std::chrono::duration_cast<std::chrono::seconds>(
                             now - m_ap_metrics_reporting_info.last_reporting_time_point)
                             .count();

    if (elapsed_time_s < m_ap_metrics_reporting_info.reporting_interval_s) {
        return;
    }

    m_ap_metrics_reporting_info.last_reporting_time_point = now;

    // We must generate a new MID for the periodic AP Metrics Response messages that
    // do not correspond to an AP Metrics Query message.
    // We cannot set MID to 0 here because we must also differentiate periodic
    // AP Metrics Response messages and messages received from monitor thread
    // due to channel utilization crossed configured threshold value.
    // As a temporary solution, set MID to UINT16_MAX here.
    // TODO: to be fixed as part of #1328 - PPM-40

    // AP Metrics Queries can be originated both from periodic or other sources.
    // There is no differentiation in ap_metric_query about requested queries source.
    // m_ap_metric_query can be converted to map (mid, requested bssid vector).
    // Concurrent time based and explicit query based requests are problematic,
    // especially, if periodic is triggered right after, explicits will be dropped.
    // To fix existing problem (PPM-1203) of remained un-answered queries,
    // query can be cleared in case of query is sent to all bissids.
    m_ap_metric_query.clear();

    // Send ap_metrics query on all bssids exists on the Agent.
    send_ap_metric_query_message(UINT16_MAX);
}

void LinkMetricsCollectionTask::handle_link_metric_query(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                         const sMacAddr &src_mac)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received LINK_METRIC_QUERY_MESSAGE, mid=" << std::hex << mid;

    /**
     * The IEEE 1905.1 standard says about the Link Metric Query TLV and the neighbor type octet
     * that "If the value is 0, then the EUI48 field is not present; if the value is 1, then the
     * EUI-48 field shall be present."
     *
     * However, optional fields are not currently supported by TLVF.
     *
     * As a workaround, instead of defining a tlvLinkMetricQuery TLV with an optional field, we
     * have defined two different TLVs: tlvLinkMetricQuery with the optional field and
     * tlvLinkMetricQueryAllNeighbors without it. Application must check which of both TLVs has
     * been received inside the message.
     */
    std::shared_ptr<ieee1905_1::tlvLinkMetricQueryAllNeighbors> tlvLinkMetricQueryAllNeighbors;
    std::shared_ptr<ieee1905_1::tlvLinkMetricQuery> tlvLinkMetricQuery;

    tlvLinkMetricQueryAllNeighbors = cmdu_rx.getClass<ieee1905_1::tlvLinkMetricQueryAllNeighbors>();
    if (!tlvLinkMetricQueryAllNeighbors) {
        tlvLinkMetricQuery = cmdu_rx.getClass<ieee1905_1::tlvLinkMetricQuery>();
        if (!tlvLinkMetricQuery) {
            LOG(ERROR) << "getClass ieee1905_1::tlvLinkMetricQueryAllNeighbors and "
                          "ieee1905_1::tlvLinkMetricQuery failed";
            return;
        }
    }

    auto db = AgentDB::get();

    /**
     * 1905.1 AL MAC address of the device that transmits the response message.
     */
    sMacAddr reporter_al_mac = db->bridge.mac;

    /**
     * 1905.1 AL MAC address of a neighbor of the receiving device.
     * Query can specify a particular neighbor device or all neighbor devices.
     */
    sMacAddr neighbor_al_mac = net::network_utils::ZERO_MAC;

    /**
     * Obtain link metrics for either all neighbors or a specific neighbor
     */
    ieee1905_1::eLinkMetricNeighborType neighbor_type;

    /**
     * The link metrics type requested: TX, RX or both
     */
    ieee1905_1::eLinkMetricsType link_metrics_type;

    if (tlvLinkMetricQuery) {
        /**
    	   * If tlvLinkMetricQuery has been included in message, we will be permissive enough to
    	   * allow it specify ALL_NEIGHBORS and if so, then we will just ignore the field
    	   * containing the MAC address of neighbor.
    	   */
        neighbor_type     = tlvLinkMetricQuery->neighbor_type();
        neighbor_al_mac   = tlvLinkMetricQuery->mac_al_1905_device();
        link_metrics_type = tlvLinkMetricQuery->link_metrics_type();
    } else {
        neighbor_type = tlvLinkMetricQueryAllNeighbors->neighbor_type();
        if (ieee1905_1::eLinkMetricNeighborType::ALL_NEIGHBORS != neighbor_type) {
            LOG(ERROR) << "Unexpected neighbor type: " << std::hex << int(neighbor_type);
            return;
        }
        link_metrics_type = tlvLinkMetricQueryAllNeighbors->link_metrics_type();
    }

    /**
     * Set alias flag to true if link metrics for a specific neighbor have been requested
     */
    bool specific_neighbor =
        ieee1905_1::eLinkMetricNeighborType::SPECIFIC_NEIGHBOR == neighbor_type;

    /**
     * Create response message
     */
    auto m_cmdu_tx_header =
        m_cmdu_tx.create(mid, ieee1905_1::eMessageType::LINK_METRIC_RESPONSE_MESSAGE);
    if (!m_cmdu_tx_header) {
        LOG(ERROR) << "Failed creating LINK_METRIC_RESPONSE_MESSAGE header! mid=" << std::hex
                   << mid;
        return;
    }

    /**
     * Get the list of neighbor links from the topology database.
     * Neighbors are grouped by the interface that connects to them.
     */
    std::map<sLinkInterface, std::vector<sLinkNeighbor>> neighbor_links_map;
    if (!get_neighbor_links(neighbor_al_mac, neighbor_links_map)) {
        LOG(ERROR) << "Failed to get the list of neighbor links";
        return;
    }

    /**
     * If the specified neighbor 1905.1 AL ID does not identify a neighbor of the receiving 1905.1
     * AL, then a link metric ResultCode TLV (see Table 6-21) with a value set to “invalid
     * neighbor” shall be included in this message.
     */
    bool invalid_neighbor = specific_neighbor && neighbor_links_map.empty();
    if (invalid_neighbor) {
        auto tlvLinkMetricResultCode = m_cmdu_tx.addClass<ieee1905_1::tlvLinkMetricResultCode>();
        if (!tlvLinkMetricResultCode) {
            LOG(ERROR) << "addClass ieee1905_1::tlvLinkMetricResultCode failed, mid=" << std::hex
                       << mid;
            return;
        }

        LOG(INFO) << "Invalid neighbor 1905.1 AL ID specified: "
                  << tlvf::mac_to_string(neighbor_al_mac);

        tlvLinkMetricResultCode->value() = ieee1905_1::tlvLinkMetricResultCode::INVALID_NEIGHBOR;

        LOG(DEBUG) << "Sending LINK_METRIC_RESPONSE_MESSAGE (invalid neighbor), mid: " << std::hex
                   << mid;
        m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, src_mac, db->bridge.mac);
        return;
    }

    /**
     * Report link metrics for the link with specific neighbor or for all neighbors, as
     * obtained from topology database
     */
    for (const auto &entry : neighbor_links_map) {
        auto interface        = entry.first;
        const auto &neighbors = entry.second;

        std::unique_ptr<link_metrics_collector> collector =
            create_link_metrics_collector(interface);
        if (!collector) {
            continue;
        }

        for (const auto &neighbor : neighbors) {

            LOG(TRACE) << "Getting link metrics for interface " << interface.iface_name
                       << " (MediaType = " << std::hex << (int)interface.media_type
                       << ") and neighbor " << neighbor.iface_mac;

            sLinkMetrics link_metrics;
            if (!collector->get_link_metrics(interface.iface_name, neighbor.iface_mac,
                                             link_metrics)) {
                LOG(ERROR) << "Unable to get link metrics for interface " << interface.iface_name
                           << " and neighbor " << neighbor.iface_mac;
                return;
            }

            if (!add_link_metrics_tlv(reporter_al_mac, interface, neighbor, link_metrics,
                                      link_metrics_type)) {
                return;
            }
        }
    }

    LOG(DEBUG) << "Sending LINK_METRIC_RESPONSE_MESSAGE, mid: " << std::hex << mid;
    m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, src_mac, db->bridge.mac);
}

void LinkMetricsCollectionTask::handle_combined_infrastructure_metrics(
    ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &src_mac)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received COMBINED_INFRASTRUCTURE_METRICS message, mid=" << std::hex << mid;

    if (cmdu_rx.getClass<ieee1905_1::tlvReceiverLinkMetric>())
        LOG(DEBUG) << "Received TLV_RECEIVER_LINK_METRIC";
    if (cmdu_rx.getClass<ieee1905_1::tlvTransmitterLinkMetric>())
        LOG(DEBUG) << "Received TLV_TRANSMITTER_LINK_METRIC";

    // build ACK message CMDU
    auto cmdu_tx_header = m_cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return;
    }
    LOG(DEBUG) << "sending ACK message to the originator, mid=" << std::hex << mid;
    auto db = AgentDB::get();
    m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, src_mac, db->bridge.mac);
}

void LinkMetricsCollectionTask::handle_beacon_metrics_query(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                            const sMacAddr &src_mac)
{
    LOG(DEBUG) << "now going to handle BEACON METRICS QUERY";

    // extract the desired STA mac
    auto tlvBeaconMetricsQuery = cmdu_rx.getClass<wfa_map::tlvBeaconMetricsQuery>();
    if (!tlvBeaconMetricsQuery) {
        LOG(ERROR) << "handle_1905_beacon_metrics_query should handle only tlvBeaconMetrics, but "
                      "got something else: 0x"
                   << std::hex << (uint16_t)cmdu_rx.getMessageType();
        return;
    }

    const sMacAddr &requested_sta_mac = tlvBeaconMetricsQuery->associated_sta_mac();
    LOG(DEBUG) << "the requested STA mac is: " << requested_sta_mac;

    // build ACK message CMDU
    const auto mid      = cmdu_rx.getMessageId();
    auto cmdu_tx_header = m_cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return;
    }

    auto db    = AgentDB::get();
    auto radio = db->get_radio_by_mac(requested_sta_mac, AgentDB::eMacType::CLIENT);
    if (!radio) {
        LOG(DEBUG) << "STA with MAC [" << requested_sta_mac
                   << "] is not associated with any BSS operated by the agent";

        // add an Error Code TLV
        auto error_code_tlv = m_cmdu_tx.addClass<wfa_map::tlvErrorCode>();
        if (!error_code_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvErrorCode has failed";
            return;
        }

        error_code_tlv->reason_code() =
            wfa_map::tlvErrorCode::STA_NOT_ASSOCIATED_WITH_ANY_BSS_OPERATED_BY_THE_AGENT;

        error_code_tlv->sta_mac() = requested_sta_mac;

        // report the error
        std::stringstream errorSS;
        auto error_tlv = m_cmdu_tx.getClass<wfa_map::tlvErrorCode>();
        if (error_tlv) {
            errorSS << "0x" << error_tlv->reason_code();
        } else {
            errorSS << "note: error constructing the error itself";
        }

        LOG(DEBUG) << "sending ACK message to the originator with an error, mid: " << std::hex
                   << mid << " tlv error code: " << errorSS.str();

        // send the error
        m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, src_mac, db->bridge.mac);
        return;
    }

    LOG(DEBUG) << "Found the radio that has the sation. radio: " << radio->front.iface_mac
               << "; station: " << requested_sta_mac;

    LOG(DEBUG) << "BEACON METRICS QUERY: sending ACK message to the originator mid: " << std::hex
               << mid; // USED IN TESTS

    m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, src_mac, db->bridge.mac);

    // forward message to fronthaul
    /*
     * TODO: https://jira.prplfoundation.org/browse/PPM-657
     *
     * When link metric collection task moves to agent context
     * send the message to fronthaul, not slave.
     */

    // Forward only to the desired destination
    if (!m_btl_ctx.forward_cmdu_to_uds(m_btl_ctx.get_agent_fd(), 0, db->bridge.mac, src_mac,
                                       cmdu_rx)) {
        LOG(ERROR) << "forward_cmdu_to_uds() to agent failed - fd=" << m_btl_ctx.get_agent_fd();
    }
}

void LinkMetricsCollectionTask::handle_associated_sta_link_metrics_query(
    ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &src_mac)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received ASSOCIATED_STA_LINK_METRICS_QUERY_MESSAGE , mid=" << std::hex << mid;

    if (!m_cmdu_tx.create(mid,
                          ieee1905_1::eMessageType::ASSOCIATED_STA_LINK_METRICS_RESPONSE_MESSAGE)) {
        LOG(ERROR)
            << "cmdu creation of type ASSOCIATED_STA_LINK_METRICS_RESPONSE_MESSAGE, has failed";
        return;
    }

    auto mac = cmdu_rx.getClass<wfa_map::tlvStaMacAddressType>();
    if (!mac) {
        LOG(ERROR) << "Failed to get mac address";
        return;
    }

    // adding (currently empty) an associated sta EXTENDED link metrics tlv
    auto extended = m_cmdu_tx.addClass<wfa_map::tlvAssociatedStaExtendedLinkMetrics>();
    if (!extended) {
        LOG(ERROR) << "adding wfa_map::tlvAssociatedStaExtendedLinkMetrics failed";
        return;
    }

    auto assoc_link_metrics = m_cmdu_tx.addClass<wfa_map::tlvAssociatedStaLinkMetrics>();
    if (!assoc_link_metrics) {
        LOG(ERROR) << "Failed to create tlvAssociatedStaLinkMetrics tlv";
        return;
    }

    auto db = AgentDB::get();

    // Check if it is an error scenario - if the STA specified in the STA link Query message
    // is not associated with any of the BSS operated by the Multi-AP Agent
    auto radio = db->get_radio_by_mac(mac->sta_mac(), AgentDB::eMacType::CLIENT);
    if (!radio) {
        LOG(ERROR) << "client with mac address " << mac->sta_mac() << " not found";
        //Add an Error Code TLV
        auto error_code_tlv = m_cmdu_tx.addClass<wfa_map::tlvErrorCode>();
        if (!error_code_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvErrorCode has failed";
            return;
        }
        error_code_tlv->reason_code() =
            wfa_map::tlvErrorCode::STA_NOT_ASSOCIATED_WITH_ANY_BSS_OPERATED_BY_THE_AGENT;
        error_code_tlv->sta_mac() = mac->sta_mac();

        LOG(DEBUG) << "Send a ASSOCIATED_STA_LINK_METRICS_RESPONSE_MESSAGE back to controller";
        m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, src_mac, db->bridge.mac);
        return;
    }
    auto client_it = radio->associated_clients.find(mac->sta_mac());
    if (client_it == radio->associated_clients.end()) {
        LOG(ERROR) << "Cannot find sta sta " << mac->sta_mac();
        return;
    }
    if (client_it->second.bssid == net::network_utils::ZERO_MAC) {
        LOG(ERROR) << "Cannot find sta bssid";
        return;
    }
    LOG(DEBUG) << "Client with mac address " << mac->sta_mac() << " connected to "
               << client_it->second.bssid;

    auto request_out = message_com::create_vs_message<
        beerocks_message::cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST>(m_cmdu_tx, mid);

    if (!request_out) {
        LOG(ERROR) << "Failed to build ACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST";
        return;
    }
    request_out->sync()    = true;
    request_out->sta_mac() = mac->sta_mac();

    /*
     * TODO: https://jira.prplfoundation.org/browse/PPM-657
     *
     * When link metric collection task moves to agent context
     * send the message to fronthaul, not slave.
     */
    // Filling the radio mac. This is temporary the task will be moved to the agent (PPM-1681).
    auto action_header         = message_com::get_beerocks_header(m_cmdu_tx)->actionhdr();
    action_header->radio_mac() = radio->front.iface_mac;

    m_btl_ctx.send_cmdu(m_btl_ctx.get_agent_fd(), m_cmdu_tx);
}

void LinkMetricsCollectionTask::handle_ap_metrics_query(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                        const sMacAddr &src_mac)
{
    const auto mid           = cmdu_rx.getMessageId();
    auto ap_metric_query_tlv = cmdu_rx.getClass<wfa_map::tlvApMetricQuery>();
    if (!ap_metric_query_tlv) {
        LOG(ERROR) << "AP Metrics Query CMDU mid=" << std::hex << mid
                   << " does not have AP Metric Query TLV";
        return;
    }

    std::unordered_set<sMacAddr> bssids;
    for (size_t bssid_idx = 0; bssid_idx < ap_metric_query_tlv->bssid_list_length(); bssid_idx++) {
        auto bssid_tuple = ap_metric_query_tlv->bssid_list(bssid_idx);
        if (!std::get<0>(bssid_tuple)) {
            LOG(ERROR) << "Failed to get bssid " << bssid_idx << " from AP_METRICS_QUERY";
            return;
        }
        bssids.insert(std::get<1>(bssid_tuple));
        LOG(DEBUG) << "Received AP_METRICS_QUERY_MESSAGE, mid=" << std::hex << mid << "  bssid "
                   << std::get<1>(bssid_tuple);
    }

    if (!send_ap_metric_query_message(mid, bssids)) {
        LOG(ERROR) << "Failed to forward AP_METRICS_RESPONSE to the son_slave_thread";
        return;
    }
}

bool LinkMetricsCollectionTask::send_ap_metric_query_message(
    uint16_t mid, const std::unordered_set<sMacAddr> &bssid_list)
{
    auto db = AgentDB::get();

    for (const auto radio : db->get_radios_list()) {
        if (!radio) {
            continue;
        }

        // copy all relevant bssids to bssid_query
        std::vector<sMacAddr> bssid_query;

        if (bssid_list.empty()) {
            // we were given an empty list,
            // therefore we copy ALL non ZERO_MAC bssids
            for (const auto &bssid : radio->front.bssids) {
                if (bssid.mac != net::network_utils::ZERO_MAC) {
                    bssid_query.emplace_back(bssid.mac);
                }
            }
        } else {
            // we were given a non empty list,
            // therefore we copy only those that are both in the
            // radio and in the given list
            for (const auto &bssid : radio->front.bssids) {
                if (bssid.mac != net::network_utils::ZERO_MAC &&
                    bssid_list.find(bssid.mac) != bssid_list.end()) {
                    bssid_query.emplace_back(bssid.mac);
                }
            }
        }

        // create ap metrics query message
        if (!m_cmdu_tx.create(mid, ieee1905_1::eMessageType::AP_METRICS_QUERY_MESSAGE)) {
            LOG(ERROR) << "Failed to create AP_METRICS_QUERY_MESSAGE";
            return false;
        }

        // add ap metrics tlv
        auto query = m_cmdu_tx.addClass<wfa_map::tlvApMetricQuery>();
        if (!query) {
            LOG(ERROR) << "Failed addClass<wfa_map::tlvApMetricQuery>";
            return false;
        }

        auto bssid_query_size = bssid_query.size();

        // allocate enough bssids
        if (!query->alloc_bssid_list(bssid_query_size)) {
            LOG(ERROR) << "Failed to allocate memory for bssid_list, required size: "
                       << bssid_query_size;
            return false;
        }

        // pack all bssids in the query
        for (size_t i = 0; i < bssid_query_size; ++i) {
            auto list = query->bssid_list(i);
            if (!std::get<0>(list)) {
                LOG(ERROR) << "Failed to get element of bssid_list";
            }
            std::get<1>(list) = bssid_query[i];

            // responses are coming one by one - each bssid alone,
            // so we keep track of each bssid in the query
            m_ap_metric_query.push_back({bssid_query[i]});
        }

        /*
         * TODO: https://jira.prplfoundation.org/browse/PPM-657
         *
         * When link metric collection task moves to agent context
         * send the message to fronthaul, not slave.
         */
        if (!m_btl_ctx.send_cmdu(m_btl_ctx.get_agent_fd(), m_cmdu_tx)) {
            LOG(ERROR) << "Failed forwarding AP_METRICS_QUERY_MESSAGE message to fronthaul";
        }
    }
    return true;
}

uint32_t LinkMetricsCollectionTask::recalculate_byte_units(uint32_t bytes)
{
    const auto &byte_counter_units = AgentDB::get()->device_conf.byte_counter_units;
    if (byte_counter_units == wfa_map::tlvProfile2ApCapability::eByteCounterUnits::KIBIBYTES) {
        bytes = bytes / 1024;
    } else if (byte_counter_units ==
               wfa_map::tlvProfile2ApCapability::eByteCounterUnits::MEBIBYTES) {
        bytes = bytes / 1024 / 1024;
    }
    return bytes;
}

void LinkMetricsCollectionTask::recalculate_byte_units(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    for (auto &sta_traffic : cmdu_rx.getClassList<wfa_map::tlvAssociatedStaTrafficStats>()) {
        if (!sta_traffic) {
            LOG(ERROR) << "Failed to get class list for tlvAssociatedStaTrafficStats";
            continue;
        }
        sta_traffic->byte_sent()     = recalculate_byte_units(sta_traffic->byte_sent());
        sta_traffic->byte_received() = recalculate_byte_units(sta_traffic->byte_received());
    }

    for (auto &extended_metric : cmdu_rx.getClassList<wfa_map::tlvApExtendedMetrics>()) {
        if (!extended_metric) {
            LOG(ERROR) << "Failed to get class list for tlvApExtendedMetrics";
            continue;
        }
        extended_metric->broadcast_bytes_sent() =
            recalculate_byte_units(extended_metric->broadcast_bytes_sent());
        extended_metric->broadcast_bytes_received() =
            recalculate_byte_units(extended_metric->broadcast_bytes_received());
        extended_metric->multicast_bytes_sent() =
            recalculate_byte_units(extended_metric->multicast_bytes_sent());
        extended_metric->multicast_bytes_received() =
            recalculate_byte_units(extended_metric->multicast_bytes_received());
        extended_metric->unicast_bytes_sent() =
            recalculate_byte_units(extended_metric->unicast_bytes_sent());
        extended_metric->unicast_bytes_received() =
            recalculate_byte_units(extended_metric->unicast_bytes_received());
    }
}

void LinkMetricsCollectionTask::handle_ap_metrics_response(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                           const sMacAddr &src_mac)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received AP_METRICS_RESPONSE_MESSAGE, mid=" << std::hex << mid;

    auto db = AgentDB::get();

    if (db->controller_info.bridge_mac == net::network_utils::ZERO_MAC) {
        LOG(ERROR) << "controller_info.bridge_mac == ZERO_MAC. Skip AP_METRICS_RESPONSE_MESSAGE";
        return;
    }

    /**
     * If AP Metrics Response message does not correspond to a previously received and forwarded
     * AP Metrics Query message (which we know because message id is not set), then forward message
     * to controller.
     * This might happen when channel utilization value has crossed configured threshold or when
     * periodic metrics reporting interval has elapsed.
     */
    if (0 == mid) {
        recalculate_byte_units(cmdu_rx);
        m_btl_ctx.forward_cmdu_to_broker(cmdu_rx, db->controller_info.bridge_mac, db->bridge.mac);
        return;
    }

    /**
     * When periodic metrics reporting interval has elapsed, we emulate that we have received an
     * AP Metrics Query message from controller. To differentiate real queries from emulated ones,
     * we use a "special" mid value.
     * Note that this design is flaw as a real query might also have this special mid value. This
     * is just a quick and dirty fix to pass 4.7.5 and 4.7.6 for M1
     * TODO: to be fixed as part of #1328
     */
    if (UINT16_MAX == mid) {
        mid = 0;
    }

    // radio metrics
    auto in_radio_metrics_tlv = cmdu_rx.getClass<wfa_map::tlvProfile2RadioMetrics>();
    if (!in_radio_metrics_tlv) {
        LOG(ERROR) << "Failed to get class tlvProfile2RadioMetrics";
        return;
    }
    sRadioMetrics radio_metrics;
    radio_metrics.radio_uid     = in_radio_metrics_tlv->radio_uid();
    radio_metrics.noise         = in_radio_metrics_tlv->noise();
    radio_metrics.transmit      = in_radio_metrics_tlv->transmit();
    radio_metrics.receive_self  = in_radio_metrics_tlv->receive_self();
    radio_metrics.receive_other = in_radio_metrics_tlv->receive_other();
    m_radio_ap_metric_response.push_back(radio_metrics);

    // bss metrics
    auto ap_metrics_tlv_list = cmdu_rx.getClassList<wfa_map::tlvApMetrics>();
    if (ap_metrics_tlv_list.empty()) {
        LOG(WARNING) << "got empty ap metrics response for mid=" << std::hex << mid;
    }

    for (auto ap_metrics_tlv : ap_metrics_tlv_list) {

        if (!ap_metrics_tlv) {
            LOG(WARNING) << "found null ap_metrics_tlv in response, skipping. mid=" << std::hex
                         << mid;
            continue;
        }

        auto bssid_tlv = ap_metrics_tlv->bssid();
        auto mac       = std::find_if(
            m_ap_metric_query.begin(), m_ap_metric_query.end(),
            [&bssid_tlv](sApMetricsQuery const &query) { return query.bssid == bssid_tlv; });

        if (mac == m_ap_metric_query.end()) {
            LOG(ERROR) << "Failed search in ap_metric_query for bssid: " << bssid_tlv
                       << " from mid=" << std::hex << mid;
            return;
        }

        sApExtendedMetrics extended_metrics;

        //TO DO: PPM-1388 Set correct values for this params.
        extended_metrics.bssid                    = ap_metrics_tlv->bssid();
        extended_metrics.broadcast_bytes_sent     = 0;
        extended_metrics.broadcast_bytes_received = 0;
        extended_metrics.multicast_bytes_sent     = 0;
        extended_metrics.multicast_bytes_received = 0;
        extended_metrics.unicast_bytes_sent       = 0;
        extended_metrics.unicast_bytes_received   = 0;

        sApMetrics metric;
        // Copy data to the response vector
        metric.bssid               = ap_metrics_tlv->bssid();
        metric.channel_utilization = ap_metrics_tlv->channel_utilization();
        metric.number_of_stas_currently_associated =
            ap_metrics_tlv->number_of_stas_currently_associated();
        metric.estimated_service_parameters = ap_metrics_tlv->estimated_service_parameters();
        auto info                           = ap_metrics_tlv->estimated_service_info_field();
        for (size_t i = 0; i < ap_metrics_tlv->estimated_service_info_field_length(); i++) {
            metric.estimated_service_info_field.push_back(info[i]);
        }
        std::vector<sStaTrafficStats> traffic_stats_response;

        for (auto &sta_traffic : cmdu_rx.getClassList<wfa_map::tlvAssociatedStaTrafficStats>()) {
            if (!sta_traffic) {
                LOG(ERROR) << "Failed to get class list for tlvAssociatedStaTrafficStats";
                continue;
            }

            traffic_stats_response.push_back(
                {sta_traffic->sta_mac(), recalculate_byte_units(sta_traffic->byte_sent()),
                 recalculate_byte_units(sta_traffic->byte_received()), sta_traffic->packets_sent(),
                 sta_traffic->packets_received(), sta_traffic->tx_packets_error(),
                 sta_traffic->rx_packets_error(), sta_traffic->retransmission_count()});
        }

        std::vector<sStaLinkMetrics> link_metrics_response;
        for (auto &sta_link_metric : cmdu_rx.getClassList<wfa_map::tlvAssociatedStaLinkMetrics>()) {
            if (!sta_link_metric) {
                LOG(ERROR) << "Failed getClassList<wfa_map::tlvAssociatedStaLinkMetrics>";
                continue;
            }
            if (sta_link_metric->bssid_info_list_length() != 1) {
                LOG(ERROR) << "sta_link_metric->bssid_info_list_length() should be equal to 1";
                continue;
            }
            auto response_list = sta_link_metric->bssid_info_list(0);
            link_metrics_response.push_back(
                {sta_link_metric->sta_mac(), std::get<1>(response_list)});
        }

        std::vector<sStaQosCtrlParams> qos_ctrl_response;
        for (auto &sta_qos_ctrl_params :
             cmdu_rx.getClassList<wfa_map::tlvAssociatedWiFi6StaStatusReport>()) {
            if (!sta_qos_ctrl_params) {
                LOG(ERROR) << "Failed getClassList<wfa_map::tlvAssociatedWiFi6StaStatusReport>";
                return;
            }
            uint8_t tid_list_length = sta_qos_ctrl_params->tid_queue_size_list_length();

            sStaQosCtrlParams sta_qos_params;
            sta_qos_params.sta_mac = sta_qos_ctrl_params->sta_mac();
            for (uint8_t tid_index = 0; tid_index < tid_list_length; tid_index++) {
                auto tid_tuple        = sta_qos_ctrl_params->tid_queue_size_list(tid_index);
                auto &qos_ctrl_params = std::get<1>(tid_tuple);
                sta_qos_params.tid_queue_size[tid_index] = qos_ctrl_params.queue_size;
            }

            qos_ctrl_response.push_back({sta_qos_params});
        }

        // Fill a response vector
        m_ap_metric_response.push_back({metric, extended_metrics, traffic_stats_response,
                                        link_metrics_response, qos_ctrl_response});

        // Remove an entry from the processed query
        m_ap_metric_query.erase(
            std::remove_if(m_ap_metric_query.begin(), m_ap_metric_query.end(),
                           [&](sApMetricsQuery const &query) { return mac->bssid == query.bssid; }),
            m_ap_metric_query.end());
    }

    if (!m_ap_metric_query.empty()) {
        LOG(DEBUG) << "Still expecting " << m_ap_metric_query.size() << " ap metric responses.";
        return;
    }

    // We received all responses - prepare and send response message to the controller
    auto cmdu_header = m_cmdu_tx.create(mid, ieee1905_1::eMessageType::AP_METRICS_RESPONSE_MESSAGE);

    if (!cmdu_header) {
        LOG(ERROR) << "Failed building IEEE1905 AP_METRICS_RESPONSE_MESSAGE";
        return;
    }

    // Prepare tlvApMetrics for each processed query
    for (const auto &response : m_ap_metric_response) {
        auto ap_metrics_response_tlv = m_cmdu_tx.addClass<wfa_map::tlvApMetrics>();
        if (!ap_metrics_response_tlv) {
            LOG(ERROR) << "Failed addClass<wfa_map::tlvApMetrics>";
            return;
        }

        ap_metrics_response_tlv->bssid()               = response.metric.bssid;
        ap_metrics_response_tlv->channel_utilization() = response.metric.channel_utilization;
        ap_metrics_response_tlv->number_of_stas_currently_associated() =
            response.metric.number_of_stas_currently_associated;
        ap_metrics_response_tlv->estimated_service_parameters() =
            response.metric.estimated_service_parameters;
        if (!ap_metrics_response_tlv->alloc_estimated_service_info_field(
                response.metric.estimated_service_info_field.size())) {
            LOG(ERROR) << "Couldn't allocate "
                          "ap_metrics_response_tlv->alloc_estimated_service_info_field";
            return;
        }
        std::copy_n(response.metric.estimated_service_info_field.begin(),
                    response.metric.estimated_service_info_field.size(),
                    ap_metrics_response_tlv->estimated_service_info_field());

        auto ap_extended_metrics_tlv = m_cmdu_tx.addClass<wfa_map::tlvApExtendedMetrics>();

        if (!ap_extended_metrics_tlv) {
            LOG(ERROR) << "Failed addClass<wfa_map::tlvApExtendedMetrics>";
            return;
        }

        ap_extended_metrics_tlv->bssid()              = response.extended_metric.bssid;
        ap_extended_metrics_tlv->unicast_bytes_sent() = response.extended_metric.unicast_bytes_sent;
        ap_extended_metrics_tlv->unicast_bytes_received() =
            response.extended_metric.unicast_bytes_received;
        ap_extended_metrics_tlv->broadcast_bytes_sent() =
            response.extended_metric.broadcast_bytes_sent;
        ap_extended_metrics_tlv->broadcast_bytes_received() =
            response.extended_metric.broadcast_bytes_received;
        ap_extended_metrics_tlv->multicast_bytes_sent() =
            response.extended_metric.multicast_bytes_sent;
        ap_extended_metrics_tlv->multicast_bytes_received() =
            response.extended_metric.multicast_bytes_received;

        for (auto &stat : response.sta_traffic_stats) {
            auto sta_traffic_response_tlv =
                m_cmdu_tx.addClass<wfa_map::tlvAssociatedStaTrafficStats>();

            if (!sta_traffic_response_tlv) {
                LOG(ERROR) << "Failed addClass<wfa_map::tlvAssociatedStaTrafficStats>";
                continue;
            }

            sta_traffic_response_tlv->sta_mac()              = stat.sta_mac;
            sta_traffic_response_tlv->byte_sent()            = stat.byte_sent;
            sta_traffic_response_tlv->byte_received()        = stat.byte_received;
            sta_traffic_response_tlv->packets_sent()         = stat.packets_sent;
            sta_traffic_response_tlv->packets_received()     = stat.packets_received;
            sta_traffic_response_tlv->tx_packets_error()     = stat.tx_packets_error;
            sta_traffic_response_tlv->rx_packets_error()     = stat.rx_packets_error;
            sta_traffic_response_tlv->retransmission_count() = stat.retransmission_count;

            // adding, currently only with sta-mac set, an associated sta EXTENDED link metrics tlv
            auto extended = m_cmdu_tx.addClass<wfa_map::tlvAssociatedStaExtendedLinkMetrics>();
            if (!extended) {
                LOG(ERROR) << "adding wfa_map::tlvAssociatedStaExtendedLinkMetrics failed";
                continue;
            }
            extended->associated_sta() = stat.sta_mac;
        }

        for (auto &link_metric : response.sta_link_metrics) {
            auto sta_link_metric_response_tlv =
                m_cmdu_tx.addClass<wfa_map::tlvAssociatedStaLinkMetrics>();

            if (!sta_link_metric_response_tlv) {
                LOG(ERROR) << "Failed addClass<wfa_map::tlvAssociatedStaLinkMetrics>";
                continue;
            }

            sta_link_metric_response_tlv->sta_mac() = link_metric.sta_mac;
            if (!sta_link_metric_response_tlv->alloc_bssid_info_list(1)) {
                LOG(ERROR) << "Failed alloc_bssid_info_list";
                continue;
            }
            auto &sta_link_metric_response =
                std::get<1>(sta_link_metric_response_tlv->bssid_info_list(0));
            sta_link_metric_response = link_metric.bssid_info;
        }

        // For each station one "Associated Wifi 6 Sta Status tlv" is added to
        // "AP Metrics Response Message". And value of the tlv fields are populated
        // with values from reponse vector.
        for (auto &qos_control_params : response.sta_wifi_6_status) {
            auto sta_wifi6_status_report_response_tlv =
                m_cmdu_tx.addClass<wfa_map::tlvAssociatedWiFi6StaStatusReport>();

            if (!sta_wifi6_status_report_response_tlv) {
                LOG(ERROR) << "Failed addClass<wfa_map::tlvAssociatedWiFi6StaStatusReport>";
                return;
            }

            sta_wifi6_status_report_response_tlv->sta_mac() = qos_control_params.sta_mac;
            if (!sta_wifi6_status_report_response_tlv->alloc_tid_queue_size_list(
                    IEEE80211_QOS_TID_MAX_UP)) {
                LOG(ERROR) << "alloc_tid_queue_size_list() has failed!";
                return;
            }
            for (uint8_t n = 0; n < IEEE80211_QOS_TID_MAX_UP; n++) {
                auto &ap_assoc_wifi_6_sta_status_report =
                    std::get<1>(sta_wifi6_status_report_response_tlv->tid_queue_size_list(n));
                ap_assoc_wifi_6_sta_status_report.tid        = n;
                ap_assoc_wifi_6_sta_status_report.queue_size = qos_control_params.tid_queue_size[n];
            }
        }
    }

    // add radios tlv
    for (auto &response : m_radio_ap_metric_response) {
        auto out_radio_metrics_tlv = m_cmdu_tx.addClass<wfa_map::tlvProfile2RadioMetrics>();
        if (!out_radio_metrics_tlv) {
            LOG(ERROR) << "Failed to add class tlvProfile2RadioMetrics";
            continue;
        }
        // just copy the radio metrics
        out_radio_metrics_tlv->radio_uid()     = response.radio_uid;
        out_radio_metrics_tlv->noise()         = response.noise;
        out_radio_metrics_tlv->transmit()      = response.transmit;
        out_radio_metrics_tlv->receive_self()  = response.receive_self;
        out_radio_metrics_tlv->receive_other() = response.receive_other;
    }

    // Clear m_radio_ap_metric_response and m_ap_metric_response vectors
    // after preparing response to the controller
    m_ap_metric_response.clear();
    m_radio_ap_metric_response.clear();

    LOG(DEBUG) << "Sending AP_METRICS_RESPONSE_MESSAGE, mid=" << std::hex << mid;
    m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, db->controller_info.bridge_mac, db->bridge.mac);
}

bool LinkMetricsCollectionTask::add_link_metrics_tlv(const sMacAddr &reporter_al_mac,
                                                     const sLinkInterface &link_interface,
                                                     const sLinkNeighbor &link_neighbor,
                                                     const sLinkMetrics &link_metrics,
                                                     ieee1905_1::eLinkMetricsType link_metrics_type)
{
    /**
     * Add Transmitter Link Metric TLV if specifically requested or both requested
     */
    if ((ieee1905_1::eLinkMetricsType::TX_LINK_METRICS_ONLY == link_metrics_type) ||
        (ieee1905_1::eLinkMetricsType::BOTH_TX_AND_RX_LINK_METRICS == link_metrics_type)) {
        auto tlvTransmitterLinkMetric = m_cmdu_tx.addClass<ieee1905_1::tlvTransmitterLinkMetric>();
        if (!tlvTransmitterLinkMetric) {
            LOG(ERROR) << "addClass ieee1905_1::tlvTransmitterLinkMetric failed";
            return false;
        }

        tlvTransmitterLinkMetric->reporter_al_mac() = reporter_al_mac;
        tlvTransmitterLinkMetric->neighbor_al_mac() = link_neighbor.al_mac;

        if (!tlvTransmitterLinkMetric->alloc_interface_pair_info()) {
            LOG(ERROR) << "alloc_interface_pair_info failed";
            return false;
        }
        auto interface_pair_info = tlvTransmitterLinkMetric->interface_pair_info(0);
        if (!std::get<0>(interface_pair_info)) {
            LOG(ERROR) << "Failed accessing interface_pair_info";
            return false;
        }
        auto &interfacePairInfo                     = std::get<1>(interface_pair_info);
        interfacePairInfo.rc_interface_mac          = link_interface.iface_mac;
        interfacePairInfo.neighbor_interface_mac    = link_neighbor.iface_mac;
        interfacePairInfo.link_metric_info.intfType = link_interface.media_type;
        // TODO
        //Indicates whether or not the 1905.1 link includes one or more IEEE 802.1 bridges
        interfacePairInfo.link_metric_info.IEEE802_1BridgeFlag =
            ieee1905_1::tlvTransmitterLinkMetric::LINK_DOES_NOT_INCLUDE_BRIDGE;
        interfacePairInfo.link_metric_info.packet_errors = link_metrics.transmitter.packet_errors;
        interfacePairInfo.link_metric_info.transmitted_packets =
            link_metrics.transmitter.transmitted_packets;
        interfacePairInfo.link_metric_info.mac_throughput_capacity =
            std::min(link_metrics.transmitter.mac_throughput_capacity_mbps,
                     static_cast<uint32_t>(UINT16_MAX));
        interfacePairInfo.link_metric_info.link_availability =
            link_metrics.transmitter.link_availability;
        interfacePairInfo.link_metric_info.phy_rate =
            std::min(link_metrics.transmitter.phy_rate_mbps, static_cast<uint32_t>(UINT16_MAX));
    }

    /**
     * Add Receiver Link Metric TLV if specifically requested or both requested
     */
    if ((ieee1905_1::eLinkMetricsType::RX_LINK_METRICS_ONLY == link_metrics_type) ||
        (ieee1905_1::eLinkMetricsType::BOTH_TX_AND_RX_LINK_METRICS == link_metrics_type)) {
        auto tlvReceiverLinkMetric = m_cmdu_tx.addClass<ieee1905_1::tlvReceiverLinkMetric>();
        if (!tlvReceiverLinkMetric) {
            LOG(ERROR) << "addClass ieee1905_1::tlvReceiverLinkMetric failed";
            return false;
        }

        tlvReceiverLinkMetric->reporter_al_mac() = reporter_al_mac;
        tlvReceiverLinkMetric->neighbor_al_mac() = link_neighbor.al_mac;

        if (!tlvReceiverLinkMetric->alloc_interface_pair_info()) {
            LOG(ERROR) << "alloc_interface_pair_info failed";
            return false;
        }
        auto interface_pair_info = tlvReceiverLinkMetric->interface_pair_info(0);
        if (!std::get<0>(interface_pair_info)) {
            LOG(ERROR) << "Failed accessing interface_pair_info";
            return false;
        }
        auto &interfacePairInfo                          = std::get<1>(interface_pair_info);
        interfacePairInfo.rc_interface_mac               = link_interface.iface_mac;
        interfacePairInfo.neighbor_interface_mac         = link_neighbor.iface_mac;
        interfacePairInfo.link_metric_info.intfType      = link_interface.media_type;
        interfacePairInfo.link_metric_info.packet_errors = link_metrics.receiver.packet_errors;
        interfacePairInfo.link_metric_info.packets_received =
            link_metrics.receiver.packets_received;
        interfacePairInfo.link_metric_info.rssi_db = link_metrics.receiver.rssi;
    }

    return true;
}

std::unique_ptr<link_metrics_collector>
LinkMetricsCollectionTask::create_link_metrics_collector(const sLinkInterface &link_interface) const
{
    ieee1905_1::eMediaType media_type = link_interface.media_type;
    ieee1905_1::eMediaTypeGroup media_type_group =
        static_cast<ieee1905_1::eMediaTypeGroup>(media_type >> 8);

    if (ieee1905_1::eMediaTypeGroup::IEEE_802_3 == media_type_group) {
        return std::make_unique<ieee802_3_link_metrics_collector>();
    }

    if (ieee1905_1::eMediaTypeGroup::IEEE_802_11 == media_type_group) {
        return std::make_unique<ieee802_11_link_metrics_collector>();
    }

    LOG(ERROR) << "Unable to create link metrics collector for interface '"
               << link_interface.iface_name << "' (unsupported media type " << std::hex
               << (int)media_type << ")";

    return nullptr;
}

bool LinkMetricsCollectionTask::get_neighbor_links(
    const sMacAddr &neighbor_mac_filter,
    std::map<sLinkInterface, std::vector<sLinkNeighbor>> &neighbor_links_map)
{
    // TODO: Topology Database is required to implement this method.

    // TODO: this is not accurate as we have made the assumption that there is a single interface.
    // Note that when processing Topology Discovery message we must store the IEEE 1905.1 AL MAC
    // address of the transmitting device together with the interface that such message is
    // received through.
    auto db = AgentDB::get();

    auto add_eth_neighbor = [&](const std::string &iface_name, const sMacAddr &iface_mac) {
        sLinkInterface wired_interface;
        wired_interface.iface_name = iface_name;
        wired_interface.iface_mac  = iface_mac;

        if (!MediaType::get_media_type(wired_interface.iface_name,
                                       ieee1905_1::eMediaTypeGroup::IEEE_802_3,
                                       wired_interface.media_type)) {
            LOG(ERROR) << "Unable to compute media type for interface "
                       << wired_interface.iface_name;
            return false;
        }

        for (const auto &neighbors_on_local_iface : db->neighbor_devices) {
            auto &neighbors = neighbors_on_local_iface.second;
            for (const auto &neighbor_entry : neighbors) {
                if (neighbor_entry.second.receiving_iface_name == iface_name) {
                    sLinkNeighbor neighbor;
                    neighbor.al_mac    = neighbor_entry.first;
                    neighbor.iface_mac = neighbor_entry.second.transmitting_iface_mac;
                    if ((neighbor_mac_filter == net::network_utils::ZERO_MAC) ||
                        (neighbor_mac_filter == neighbor.al_mac)) {
                        neighbor_links_map[wired_interface].push_back(neighbor);
                    }
                }
            }
        }
        return true;
    };

    // Add WAN interface
    if (!db->device_conf.local_gw && !db->ethernet.wan.iface_name.empty()) {
        if (!add_eth_neighbor(db->ethernet.wan.iface_name, db->ethernet.wan.mac)) {
            // Error message inside the lambda function.
            return false;
        }
    }

    // Add LAN interfaces
    for (const auto &lan_iface_info : db->ethernet.lan) {
        if (!add_eth_neighbor(lan_iface_info.iface_name, lan_iface_info.mac)) {
            // Error message inside the lambda function.
            return false;
        }
    }

    // Also include a link for each associated client
    for (const auto radio : db->get_radios_list()) {
        if (!radio) {
            continue;
        }

        for (const auto &associated_client : radio->associated_clients) {
            auto &bssid = associated_client.second.bssid;

            sLinkInterface interface;

            interface.iface_name = radio->front.iface_name;
            interface.iface_mac  = bssid;
            interface.media_type =
                MediaType::get_802_11_media_type(radio->freq_type, radio->max_supported_bw);

            if (ieee1905_1::eMediaType::UNKNOWN_MEDIA == interface.media_type) {
                LOG(ERROR) << "Unknown media type for interface " << interface.iface_name;
                return false;
            }

            LOG(TRACE) << "Getting neighbors connected to interface " << interface.iface_name
                       << " with BSSID " << bssid;

            // TODO: This is not correct... We actually have to get this from the topology
            // discovery message, which will give us the neighbor interface and AL MAC addresses.
            sLinkNeighbor neighbor;
            neighbor.iface_mac = associated_client.first;
            neighbor.al_mac    = neighbor.iface_mac;

            if ((neighbor_mac_filter == net::network_utils::ZERO_MAC) ||
                (neighbor_mac_filter == neighbor.al_mac)) {
                neighbor_links_map[interface].push_back(neighbor);
            }
        }
    }

    return true;
}

void LinkMetricsCollectionTask::handle_event(uint8_t event_enum_value, const void *event_obj)
{
    switch (eEvent(event_enum_value)) {
    case RESET_QUERIES: {
        LOG(DEBUG) << "Received RESET_QUERIES event.";
        m_ap_metric_query.clear();
        m_ap_metrics_reporting_info.reporting_interval_s = 0;
        break;
    }
    default: {
        LOG(DEBUG) << "Message handler doesn't exist for event type " << event_enum_value;
        break;
    }
    }
}

} // namespace beerocks
