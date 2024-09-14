/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef MONITOR_THREAD_H
#define MONITOR_THREAD_H

#include "monitor_rssi.h"
#ifdef BEEROCKS_RDKB
#include "rdkb/monitor_rdkb_hal.h"
#endif
#include "monitor_db.h"
#include "monitor_stats.h"

#include <bcl/beerocks_cmdu_client_factory.h>
#include <bcl/beerocks_eventloop_thread.h>
#include <bcl/beerocks_logging.h>
#include <bcl/beerocks_timer_manager.h>
#include <bcl/network/file_descriptor.h>
#include <beerocks/tlvf/beerocks_message_monitor.h>

// Monitor HAL
#include <bwl/mon_wlan_hal.h>

namespace son {
class Monitor : public beerocks::EventLoopThread {
public:
    Monitor(const std::string &monitor_iface_,
            beerocks::config_file::sConfigSlave &beerocks_slave_conf_, beerocks::logging &logger_);

    /**
     * @brief Initialize monitor.
     *
     * @return true on success and false otherwise.
     */
    virtual bool thread_init() override;

    /**
     * @brief Stops monitor.
     */
    virtual void on_thread_stop() override;

private:
    /**
     * @brief Sends given CMDU message to the slave.
     *
     * @param cmdu_tx CMDU message to send.
     * @return true on success and false otherwise.
     */
    bool send_cmdu(ieee1905_1::CmduMessageTx &cmdu_tx);

    /**
     * @brief Handles CMDU message received from slave.
     *
     * @param cmdu_rx Received CMDU to be handled.
     */
    void handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Runs the Finite State Machine of the AP manager.
     * 
     * @return true on success and false otherwise.
     */
    bool monitor_fsm();

    bool hal_event_handler(bwl::base_wlan_hal::hal_event_ptr_t event_ptr);

    /**
     * @brief Executes when channel utilization measurement period has elapsed.
     *
     * This method is periodically invoked if AP Metrics Channel Utilization Reporting Threshold
     * has been set to a non-zero value in last received Metric Reporting Policy TLV.
     * Measurement period is set to an implementation-specific value.
     * On invocation, method shall measure current channel utilization on the radio. If difference
     * with respect to the previous measurement has crossed the reporting threshold, it shall send
     * an AP Metrics Response message to the controller.
     */
    void on_channel_utilization_measurement_period_elapsed();

    /**
     * @brief Creates AP Metrics Response message
     *
     * @param mid Message ID.
     * @param bssid_list list of BSSID of BSS operated by the Multi-AP Agent to include in the AP
     * Metrics Response message.
     * @return True on success and false otherwise.
     */
    bool create_ap_metrics_response(uint16_t mid, const std::vector<sMacAddr> &bssid_list);

    bool update_ap_stats();
    bool update_sta_qos_ctrl_params();
    bool update_sta_stats(const std::chrono::steady_clock::time_point &timeout);

    void debug_channel_load_11k_request(beerocks_message::sStaChannelLoadRequest11k &request);
    void debug_channel_load_11k_response(beerocks_message::sStaChannelLoadResponse11k &event);
    void debug_beacon_11k_request(beerocks_message::sBeaconRequest11k &request);
    void debug_beacon_11k_response(beerocks_message::sBeaconResponse11k &event);

    void send_heartbeat();
    void update_vaps_in_db();
#ifdef BEEROCKS_RDKB
    void send_steering_return_status(beerocks_message::eActionOp_MONITOR ActionOp, int32_t status);
#endif
    beerocks_message::sSonConfig son_config;

    /**
     * MAC address of the radio interface that this monitor instance operates on.
     * It is obtained by getting the MAC address of the monitor interface provided as parameter in
     * constructor.
     */
    sMacAddr m_radio_mac;

    std::string monitor_iface;
    beerocks::config_file::sConfigSlave &beerocks_slave_conf;
    std::string bridge_iface;

    /**
     * Buffer to hold CMDU to be transmitted.
     */
    uint8_t m_tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];

    /**
     * CMDU to be transmitted.
     */
    ieee1905_1::CmduMessageTx cmdu_tx;

    /** 
     * File descriptor of the ARP socket. 
     */
    int m_arp_fd = beerocks::net::FileDescriptor::invalid_descriptor;

    /**
     * File descriptor to the external events queue.
     */
    std::vector<int> m_mon_hal_ext_events = {beerocks::net::FileDescriptor::invalid_descriptor};

    /**
     * File descriptor to the internal events queue.
     */
    int m_mon_hal_int_events = beerocks::net::FileDescriptor::invalid_descriptor;

    /**
     * File descriptor to the Netlink events queue.
     */
    int m_mon_hal_nl_events = beerocks::net::FileDescriptor::invalid_descriptor;

    beerocks::logging &logger;
    bool m_logger_configured = false;

    typedef struct {
        std::string sta_mac;
        int dialog_token;
        std::chrono::steady_clock::time_point timestamp;
        int id;
    } sEvent11k;
    std::unordered_multimap<std::string, sEvent11k> pending_11k_events;

    monitor_db mon_db;
    monitor_rssi mon_rssi;
#ifdef BEEROCKS_RDKB
    monitor_rdkb_hal mon_rdkb_hal;
#endif
    monitor_stats mon_stats;

    int hal_command_failures_count = 0;

    std::shared_ptr<bwl::mon_wlan_hal> mon_wlan_hal;
    bool mon_hal_attached           = false;
    bwl::HALState last_attach_state = bwl::HALState::Uninitialized;

    std::chrono::steady_clock::time_point m_sta_stats_polling_start_timestamp;
    bool m_sta_stats_polling_completed = true;

    const uint8_t GENERATE_CONNECTED_EVENTS_WORK_TIME_LIMIT_MSEC = 5;
    const uint16_t GENERATE_CONNECTED_EVENTS_DELAY_MSEC          = 500;
    bool m_generate_connected_clients_events                     = false;
    std::chrono::steady_clock::time_point m_next_generate_connected_events_time =
        std::chrono::steady_clock::time_point::min();

    void handle_cmdu_vs_message(ieee1905_1::CmduMessageRx &cmdu_rx);
    void handle_cmdu_ieee1905_1_message(ieee1905_1::CmduMessageRx &cmdu_rx);

    void handle_multi_ap_policy_config_request(ieee1905_1::CmduMessageRx &cmdu_rx);
    void handle_ap_metrics_query(ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * Factory to create CMDU client instances connected to CMDU server running in slave.
     */
    std::shared_ptr<beerocks::CmduClientFactory> m_slave_cmdu_client_factory;

    /**
     * Timer manager to help using application timers.
     */
    std::shared_ptr<beerocks::TimerManager> m_timer_manager;

    /**
     * File descriptor of the timer to run the Finite State Machine.
     */
    int m_fsm_timer = beerocks::net::FileDescriptor::invalid_descriptor;

    /**
     * CMDU client connected to the CMDU server running in slave.
     * This object is dynamically created using the CMDU client factory for the slave provided in 
     * class constructor.
     */
    std::shared_ptr<beerocks::CmduClient> m_slave_client;
};
} // namespace son

#endif
