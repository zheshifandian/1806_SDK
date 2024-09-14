/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _LINK_METRICS_COLLECTION_TASK_H_
#define _LINK_METRICS_COLLECTION_TASK_H_

#include "task.h"

#include <tlvf/CmduMessageTx.h>
#include <tlvf/ieee_1905_1/eLinkMetricsType.h>
#include <tlvf/ieee_1905_1/eMediaType.h>
#include <tlvf/wfa_map/tlvApMetrics.h>
#include <tlvf/wfa_map/tlvAssociatedStaLinkMetrics.h>

#include "bcl/network/network_utils.h"

#include "../helpers/link_metrics/link_metrics.h"

namespace beerocks {

// Forward declaration for BackhaulManager context saving
class BackhaulManager;

class LinkMetricsCollectionTask : public Task {
public:
    LinkMetricsCollectionTask(BackhaulManager &btl_ctx, ieee1905_1::CmduMessageTx &cmdu_tx);

    bool handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, uint32_t iface_index,
                     const sMacAddr &dst_mac, const sMacAddr &src_mac, int fd,
                     std::shared_ptr<beerocks_header> beerocks_header) override;

    void work() override;
    void handle_event(uint8_t event_enum_value, const void *event_obj) override;

    /**
     * @brief eEvent list is used on link metrics task.
     *
     * RESET_QUERIES is sent when DEV_RESET_DEFAULT is triggered, to clean up already started processes.
     *
     */
    enum eEvent : uint8_t {
        RESET_QUERIES,
    };

private:
    BackhaulManager &m_btl_ctx;
    ieee1905_1::CmduMessageTx &m_cmdu_tx;

    void handle_link_metric_query(ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &src_mac);
    void handle_combined_infrastructure_metrics(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                const sMacAddr &src_mac);
    void handle_beacon_metrics_query(ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &src_mac);
    void handle_associated_sta_link_metrics_query(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                  const sMacAddr &src_mac);
    void handle_ap_metrics_query(ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &src_mac);
    void handle_ap_metrics_response(ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &src_mac);

    /**
     * @brief Sends an AP Metrics Query message for each bssid on 'bssid_list' to the Fronthaul.
     * If the 'bssid_list' is empty, sends a query on each bssid that exists on the Agent.
     *
     * @param mid MID of the message to be sent.
     * @param bssid_list List of bssids to send a query on.
     * @return true on success, otherwise false.
     */
    bool send_ap_metric_query_message(uint16_t mid, const std::unordered_set<sMacAddr> &bssid_list =
                                                        std::unordered_set<sMacAddr>());

    /**
     * @brief Interface in this device which connects to an interface in one or more neighbors.
     *
     * An interface is defined by its name, its MAC address and its MediaType as
     * defined in IEEE Std 1905.1, Table 6-12—Media type (intfType).
     */
    struct sLinkInterface {
        std::string iface_name; /**< The name of the interface. */
        sMacAddr iface_mac =
            beerocks::net::network_utils::ZERO_MAC; /**< The MAC address of the interface. */
        ieee1905_1::eMediaType media_type = ieee1905_1::eMediaType::
            UNKNOWN_MEDIA; /**< The underlying network technology of the connecting interface. */
        bool operator<(const sLinkInterface &rhs) const { return iface_name < rhs.iface_name; }
    };

    /**
     * @brief Neighbor 1905.1 device which connects to an interface in this device.
     *
     * A neighbor is defined by its 1905.1 AL MAC address and the MAC address of the interface in
     * the neighbor that connects to this device.
     */
    struct sLinkNeighbor {
        sMacAddr al_mac =
            beerocks::net::network_utils::ZERO_MAC; /**< The MAC address of the 1905.1 AL. */
        sMacAddr iface_mac =
            beerocks::net::network_utils::ZERO_MAC; /**< The MAC address of the interface. */
    };

    /**
     * @brief Adds link metric TLVs to response message.
     *
     * Creates a Transmitter Link Metric TLV or a Receiver Link Metric TLV or both and adds them to
     * the Link Metric Response message.
     *
     * @param[in] reporter_al_mac 1905.1 AL MAC address of the device that transmits the response
     *  message.
     * @param[in] link_interface Connecting interface in this device.
     * @param[in] link_neighbor Neighbor connected to interface.
     * @param[in] link_metrics Metrics information associated to the link between the local
     *  interface and the neighbor's interface.
     * @param[in] link_metrics_type The link metrics type requested: TX, RX or both.
     *
     * @return True on success and false otherwise.
     */
    bool add_link_metrics_tlv(const sMacAddr &reporter_al_mac, const sLinkInterface &link_interface,
                              const sLinkNeighbor &link_neighbor, const sLinkMetrics &link_metrics,
                              ieee1905_1::eLinkMetricsType link_metrics_type);

    /**
     * @brief Adds radio meteric tlv (profile2) to the cmdu-tx member
     */
    void add_radio_metrics_tlv();

    /**
     * @brief Creates a new link metrics collector for given media type.
     *
     * Creates a new link metrics collector suitable for the underlying network technology of the
     * connecting interface.
     * Collector choice depends on bits 15 to 8 of media type, that is, the media type group.
     *
     * @param[in] iface_mac MAC address of the connecting interface.
     * @param[in] media_type The underlying network technology of the connecting interface.
     *
     * @return Link metrics collector on success and nullptr otherwise.
     */
    std::unique_ptr<link_metrics_collector>
    create_link_metrics_collector(const sLinkInterface &link_interface) const;

    /**
     * @brief Gets the list of neighbors connected to this device (from topology database).
     *
     * The keys of the returned map are interfaces in this device which connect to one or more
     * neighbor device. The values are the list of neighbors connected to that interface.
     *
     * @param[in] neighbor_mac_filter Optional 1905.1 AL MAC address to filter the links to be
     * returned. A value of network_utils::ZERO_MAC means no filter has to be applied. A specific
     * MAC address means that only links to that neighbor device must be included.
     * @param[in, out] neighbor_links_map Map of neighbor links (interfaces x neighbors).
     *
     * @return True on success and false otherwise.
     */
    bool
    get_neighbor_links(const sMacAddr &neighbor_mac_filter,
                       std::map<sLinkInterface, std::vector<sLinkNeighbor>> &neighbor_links_map);

    /**
     * @brief Recalculate single value of byte units to support R2 spec
     * 
     * R2 specification require associated station traffic stats to be in KB units while R1 only
     * support Byte units. The monitor produces the value in Byte units so it need to be recalculated
     * according to the controller expectation.
     *
     * @param[in] bytes Number of bytes to recalculate
     * 
     * @return Recalculated value of the bytes
     */
    uint32_t recalculate_byte_units(uint32_t bytes);

    /**
     * @brief Recalculate all byte units of TLVs to support R2 spec
     * 
     * R2 specification require associated station traffic stats to be in KB units while R1 only
     * support Byte units. The monitor produces the value in Byte units so it need to be recalculated
     * according to the controller expectation.
     *
     * @param[in] cmdu_rx CMDU message containing associated station traffic stats TLVs
     * 
     * @return None
     */
    void recalculate_byte_units(ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * AP Metrics Reporting configuration and status information type.
     */
    struct sApMetricsReportingInfo {
        /**
         * AP Metrics Reporting Interval in seconds (0: Do not report AP Metrics periodically).
         * This value is set by the controller through a Multi-AP Policy Config Request message,
         * inside the Metric Reporting Policy TLV.
         */
        uint8_t reporting_interval_s = 0;

        /**
         * Time point at which AP metrics were reported for the last time.
         */
        std::chrono::steady_clock::time_point last_reporting_time_point;
    };

    /**
     * AP Metrics Reporting configuration and status information.
     */
    sApMetricsReportingInfo m_ap_metrics_reporting_info;

    struct sApMetricsQuery {
        sMacAddr bssid;
    };

    std::vector<sApMetricsQuery> m_ap_metric_query;

    struct sStaTrafficStats {
        sMacAddr sta_mac;
        uint32_t byte_sent;
        uint32_t byte_received;
        uint32_t packets_sent;
        uint32_t packets_received;
        uint32_t tx_packets_error;
        uint32_t rx_packets_error;
        uint32_t retransmission_count;
    };

    struct sStaLinkMetrics {
        sMacAddr sta_mac;
        wfa_map::tlvAssociatedStaLinkMetrics::sBssidInfo bssid_info;
    };

    struct sStaQosCtrlParams {
        sMacAddr sta_mac;
        int8_t tid_queue_size[IEEE80211_QOS_TID_MAX_UP];
    };

    struct sApMetrics {
        sMacAddr bssid;
        uint8_t channel_utilization;
        uint16_t number_of_stas_currently_associated;
        wfa_map::tlvApMetrics::sEstimatedService estimated_service_parameters;
        std::vector<uint8_t> estimated_service_info_field;
    };

    struct sApExtendedMetrics {
        sMacAddr bssid;
        uint32_t unicast_bytes_sent;
        uint32_t unicast_bytes_received;
        uint32_t broadcast_bytes_sent;
        uint32_t broadcast_bytes_received;
        uint32_t multicast_bytes_sent;
        uint32_t multicast_bytes_received;
    };

    struct sApMetricsResponse {
        sApMetrics metric;
        sApExtendedMetrics extended_metric;
        std::vector<sStaTrafficStats> sta_traffic_stats;
        std::vector<sStaLinkMetrics> sta_link_metrics;
        std::vector<sStaQosCtrlParams> sta_wifi_6_status;
    };

    std::vector<sApMetricsResponse> m_ap_metric_response;

    struct sRadioMetrics {
        sMacAddr radio_uid;
        uint8_t noise;
        uint8_t transmit;
        uint8_t receive_self;
        uint8_t receive_other;
    };
    std::vector<sRadioMetrics> m_radio_ap_metric_response;
};

} // namespace beerocks

#endif // _LINK_METRICS_COLLECTION_TASK_H_
