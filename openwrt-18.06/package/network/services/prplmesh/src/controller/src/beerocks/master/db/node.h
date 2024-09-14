/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _NODE_H_
#define _NODE_H_

#include "../tasks/task.h"
#include "interface.h"
#include <bcl/network/network_utils.h>
#include <tlvf/common/sMacAddr.h>
#include <tlvf/ieee_1905_1/tlvReceiverLinkMetric.h>
#include <tlvf/ieee_1905_1/tlvTransmitterLinkMetric.h>
#include <tlvf/wfa_map/tlvApMetrics.h>
#include <tlvf/wfa_map/tlvChannelScanCapabilities.h>
#include <tlvf/wfa_map/tlvProfile2ChannelScanResult.h>

#include <list>
#include <map>
#include <unordered_set>

namespace son {
typedef struct {
    uint8_t channel;
    uint8_t bandwidth;
    int8_t channel_ext_above_secondary;
    std::chrono::steady_clock::time_point csa_exit_timestamp;
    std::chrono::steady_clock::time_point csa_enter_timestamp;
} sWifiChannelRadarStats;

typedef struct {
    std::string mac;
    std::string ssid;
    bool backhaul_vap;
} sVapElement;

/**
 * @brief Extended boolean parameter to support "not configured" value for configuration.
 * For persistent data, it is important to differ between configured (true/false) to unconfigured value.
 */
enum class eTriStateBool : int8_t { NOT_CONFIGURED = -1, FALSE = 0, TRUE = 1 };

std::ostream &operator<<(std::ostream &os, eTriStateBool value);

class node {
public:
    node(beerocks::eType type_, const std::string &mac_);
    void clear_node_stats_info();

    beerocks::eType get_type();
    bool set_type(beerocks::eType type_);

    int8_t vap_id = beerocks::IFACE_ID_INVALID;
    const std::string mac;           // client
    std::string parent_mac;          // hostap
    std::string dm_path;             // data model path without dot in the end
    std::string previous_parent_mac; //hostap

    int8_t link_rssi;
    std::string ipv4;
    std::string manufacturer;
    int channel = 0;
    std::string name;
    int hierarchy = -1; //redundant but more efficient
    beerocks::message::sRadioCapabilities *capabilities;
    beerocks::message::sRadioCapabilities m_sta_5ghz_capabilities;
    beerocks::message::sRadioCapabilities m_sta_24ghz_capabilities;

    beerocks::eWiFiBandwidth bandwidth = beerocks::BANDWIDTH_160;
    bool channel_ext_above_secondary   = true;

    beerocks::eNodeState state = beerocks::STATE_DISCONNECTED;

    bool supports_5ghz            = true;
    int failed_5ghz_steer_attemps = 0;

    bool supports_24ghz            = true;
    int failed_24ghz_steer_attemps = 0;
    beerocks::eBeaconMeasurementSupportLevel supports_beacon_measurement =
        beerocks::BEACON_MEAS_UNSUPPORTED;

    std::chrono::steady_clock::time_point last_state_change;

    int load_balancer_task_id             = -1;
    int dynamic_channel_selection_task_id = -1;

    std::chrono::steady_clock::time_point measurement_sent_timestamp;
    int measurement_recv_delta  = 0;
    int measurement_delay       = 0;
    int measurement_window_size = 60;

    class sta_stats_params {
    public:
        uint32_t rx_packets                             = 0;
        uint32_t tx_packets                             = 0;
        uint32_t rx_bytes                               = 0;
        uint32_t tx_bytes                               = 0;
        uint32_t retrans_count                          = 0;
        uint8_t tx_load_percent                         = 0;
        uint8_t rx_load_percent                         = 0;
        uint16_t rx_phy_rate_100kb                      = 0;
        uint16_t tx_phy_rate_100kb                      = 0;
        int8_t rx_rssi                                  = beerocks::RSSI_INVALID;
        uint16_t stats_delta_ms                         = 0;
        std::chrono::steady_clock::time_point timestamp = std::chrono::steady_clock::now();
    };
    std::shared_ptr<sta_stats_params> stats_info = std::make_shared<sta_stats_params>();

    class radio {
    public:
        bool active = false;

        std::string iface_name;
        beerocks::eIfaceType iface_type;
        std::vector<beerocks::message::sWifiChannel> supported_channels;
        uint8_t operating_class = 0;
        int ant_gain            = 0;
        int tx_power            = 0;
        std::string ssid;
        beerocks::eRadioBandCapability capability = beerocks::SUBBAND_CAPABILITY_UNKNOWN;
        uint16_t vht_center_frequency             = 0;
        int8_t channel_ext_above_primary          = 1;
        bool is_dfs                               = false;
        bool cac_completed                        = false;
        bool on_dfs_reentry                       = false;
        std::set<std::string> dfs_reentry_clients;
        beerocks::eApActiveMode ap_activity_mode = beerocks::AP_ACTIVE_MODE;

        std::list<sWifiChannelRadarStats> Radar_stats;
        std::vector<uint8_t> conf_restricted_channels;

        class ap_stats_params {
        public:
            int active_sta_count                 = 0;
            uint32_t rx_packets                  = 0;
            uint32_t tx_packets                  = 0;
            uint32_t rx_bytes                    = 0;
            uint32_t tx_bytes                    = 0;
            uint32_t errors_sent                 = 0;
            uint32_t errors_received             = 0;
            uint32_t retrans_count               = 0;
            int8_t noise                         = 0;
            uint8_t channel_load_percent         = 0;
            uint8_t total_client_tx_load_percent = 0;
            uint8_t total_client_rx_load_percent = 0;
            uint16_t stats_delta_ms              = 0;
            std::chrono::steady_clock::time_point timestamp;
        };
        std::shared_ptr<ap_stats_params> stats_info = std::make_shared<ap_stats_params>();
        std::unordered_map<int8_t, sVapElement> vaps_info;

        class channel_scan_capabilities {
        public:
            // 1: True (Agent can only perform scan on boot), 0: False (Agent can perform Requested scans)
            uint8_t on_boot_only = 1;

            // 0x00: No impact (independent radio is available for scanning that is not used for Fronthaul or backhaul)
            // 0x01: Reduced number of spatial streams
            // 0x02: Time slicing impairment (Radio may go off channel for a series of short intervals)
            // 0x03: Radio unavailable for >= 2 seconds)
            uint8_t scan_impact = 0x02;

            // The minimum interval in seconds between the start of two consecutive channel scans on this radio
            uint32_t minimum_scan_interval = 0;

            // An unordered-map of operating classes the radio can scan on
            // Key: operating-class id, Value: operating
            std::unordered_map<uint8_t, std::vector<beerocks::message::sWifiChannel>>
                operating_classes;
        };
        channel_scan_capabilities scan_capabilities;

        /* The channel_scan_report structure holds channel scan report information
         * The channel_scan_report_key is comprised of an operating-class & channel-number pair
         * The channel_scan_report_hash is the hash function that resolves the pair's hash for the mapping function
         */
        class channel_scan_report {
        public:
            typedef std::pair<uint8_t, uint8_t> channel_scan_report_key;
            struct channel_scan_report_hash {
                std::size_t operator()(const std::pair<uint8_t, uint8_t> &pair) const
                {
                    return std::hash<uint8_t>()(pair.first) ^ std::hash<uint8_t>()(pair.second);
                }
            };
            std::vector<beerocks_message::sChannelScanResults> neighbors;
        };
        typedef std::set<channel_scan_report::channel_scan_report_key> channel_scan_report_index;
        std::unordered_map<channel_scan_report::channel_scan_report_key, channel_scan_report,
                           channel_scan_report::channel_scan_report_hash>
            scan_report;
        // Key:     std::string ISO-8601-timestamp
        // Value:   std::set<std::pair> Report index
        std::unordered_map<std::string, channel_scan_report_index> channel_scan_report_records;

        struct channel_scan_config {
            bool is_enabled = false;
            std::unordered_set<uint8_t> channel_pool; // default value: empty list
            int interval_sec    = -1;                 //-1 (invalid)
            int dwell_time_msec = -1;                 //-1 (invalid)
        };

        struct channel_scan_status {
            // The scan is pending flag will be used to indicate when a single scan was requested
            bool scan_is_pending  = false;
            bool scan_in_progress = false;
            beerocks::eChannelScanStatusCode last_scan_error_code =
                beerocks::eChannelScanStatusCode::SUCCESS;
        };

        /**
         * These members are part of the continuous channel scan.
         * The contiuous scan runs every interval_sec.
         */
        channel_scan_config continuous_scan_config; /**< continues scan configuration */
        channel_scan_status continuous_scan_status; /**< continues scan status        */
        std::list<beerocks_message::sChannelScanResults>
            continuous_scan_results; /**< continues scan results list  */

        /**
         * These members are part of the single channel scan.
         * The single scan triggered once.
         */
        channel_scan_config single_scan_config; /**< single scan configuration */
        channel_scan_status single_scan_status; /**< single scan status        */
        std::list<beerocks_message::sChannelScanResults>
            single_scan_results; /**< single scan results list  */
    };
    std::shared_ptr<radio> hostap = std::make_shared<radio>();

    class link_metrics_data {
    public:
        link_metrics_data(){};
        ~link_metrics_data(){};

        std::vector<ieee1905_1::tlvTransmitterLinkMetric::sInterfacePairInfo>
            transmitterLinkMetrics;
        std::vector<ieee1905_1::tlvReceiverLinkMetric::sInterfacePairInfo> receiverLinkMetrics;

        bool add_transmitter_link_metric(
            std::shared_ptr<ieee1905_1::tlvTransmitterLinkMetric> TxLinkMetricData);
        bool add_receiver_link_metric(
            std::shared_ptr<ieee1905_1::tlvReceiverLinkMetric> RxLinkMetricData);
    };

    class ap_metrics_data {
    public:
        ap_metrics_data(){};
        ~ap_metrics_data(){};

        sMacAddr bssid                               = beerocks::net::network_utils::ZERO_MAC;
        uint8_t channel_utilization                  = 0;
        uint16_t number_of_stas_currently_associated = 0;
        std::vector<uint8_t> estimated_service_info_fields;
        bool include_ac_vo = false;
        bool include_ac_bk = false;
        bool include_ac_vi = false;

        bool add_ap_metric_data(std::shared_ptr<wfa_map::tlvApMetrics> ApMetricData);
    };

    bool is_prplmesh                = false;
    beerocks::eIfaceType iface_type = beerocks::IFACE_TYPE_ETHERNET;
    std::chrono::steady_clock::time_point last_seen;

    /**
     * @brief Returns active interface mac addresses via loop through interface objects.
     *
     * @return active interface mac's returned as vector of sMacAddr
     */
    std::vector<sMacAddr> get_interfaces_mac();

    /**
     * @brief Get Interface with the given MAC, create it if necessary.
     *
     * @param mac interface MAC address
     * @return shared pointer of Interface Object
     */
    std::shared_ptr<prplmesh::controller::db::Interface> add_interface(const sMacAddr &mac);

    /**
     * @brief Get Interface with the given MAC, if there is one. Else returns nullptr.
     *
     * @param mac interface MAC address
     * @return shared pointer of Interface Object on success, nullptr otherwise.
     */
    std::shared_ptr<prplmesh::controller::db::Interface> get_interface(const sMacAddr &mac);

    /**
     * @brief Remove the Interface with the given MAC Address.
     */
    void remove_interface(const sMacAddr &mac);

    /**
     * @brief Get all Interfaces
     */
    const std::vector<std::shared_ptr<prplmesh::controller::db::Interface>> &get_interfaces()
    {
        return m_interfaces;
    }

    /**
     * @brief Returns unused interface mac addresses
     *
     * @param new_interfaces vector of active interface macs from topology message
     * @return unused interface mac's returned as vector of sMacAddr
     */
    std::vector<sMacAddr> get_unused_interfaces(const std::vector<sMacAddr> &new_interfaces);

    /**
     * @brief Get Neighbor with the given MAC, create it if necessary within Interface.
     *
     * @param interface_mac interface MAC address
     * @param neighbor_mac neighbor MAC address
     * @param flag_ieee1905 is IEEE1905 Flag
     * @return shared pointer of Neighbor Object
     */
    std::shared_ptr<prplmesh::controller::db::Interface::sNeighbor>
    add_neighbor(const sMacAddr &interface_mac, const sMacAddr &neighbor_mac, bool flag_ieee1905);

private:
    beerocks::eType type;

    /**
     * @brief Interfaces configured on this node.
     *
     * The Interface objects are kept alive by this list. Only active interfaces should be on this list.
     *
     */
    std::vector<std::shared_ptr<prplmesh::controller::db::Interface>> m_interfaces;
};
} // namespace son
#endif
