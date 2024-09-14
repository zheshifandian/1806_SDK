/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include "controller_ucc_listener.h"
#include "db/db.h"
#include "periodic/periodic_operation_pool.h"
#include "tasks/link_metrics_task.h"
#include "tasks/task_pool.h"

#include "../../../common/beerocks/bwl/include/bwl/base_wlan_hal.h"
#include <bcl/beerocks_cmdu_server.h>
#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_event_loop.h>
#include <bcl/beerocks_logging.h>
#include <bcl/beerocks_message_structs.h>
#include <bcl/beerocks_timer_manager.h>
#include <bcl/beerocks_ucc_server.h>
#include <bcl/network/file_descriptor.h>
#include <btl/broker_client_factory.h>

#include <mapf/common/encryption.h>
#include <tlvf/WSC/configData.h>
#include <tlvf/WSC/m1.h>
#include <tlvf/WSC/m2.h>
#include <tlvf/ieee_1905_1/tlvWsc.h>
#include <tlvf/wfa_map/tlvApRadioBasicCapabilities.h>

#include <cstddef>
#include <ctime>
#include <stdint.h>

namespace son {
class Controller {

public:
    Controller(db &database_,
               std::unique_ptr<beerocks::btl::BrokerClientFactory> broker_client_factory,
               std::unique_ptr<beerocks::UccServer> ucc_server,
               std::unique_ptr<beerocks::CmduServer> cmdu_server,
               std::shared_ptr<beerocks::TimerManager> timer_manager,
               std::shared_ptr<beerocks::EventLoop> event_loop);
    ~Controller();

    /**
     * @brief Starts controller.
     *
     * @return true on success and false otherwise.
     */
    bool start();

    /**
     * @brief Stops controller.
     *
     * @return true on success and false otherwise.
     */
    bool stop();

    /**
     * @brief Sends given CMDU message through the specified socket connection.
     *
     * @param fd File descriptor of the connected socket.
     * @param cmdu_tx CMDU message to send.
     * @return true on success and false otherwise.
     */
    bool send_cmdu(int fd, ieee1905_1::CmduMessageTx &cmdu_tx);

    /**
     * @brief Sends given CMDU message to the broker server running in the transport process to be
     * forwarded to another device or multicast.
     *
     * @param cmdu_tx CMDU message to send.
     * @param dst_mac Destination MAC address (must not be empty).
     * @param src_mac Source MAC address (must not be empty).
     * @param iface_name Name of the network interface to use (set to empty string to send on all
     * available interfaces).
     * @return true on success and false otherwise.
     */
    bool send_cmdu_to_broker(ieee1905_1::CmduMessageTx &cmdu_tx, const sMacAddr &dst_mac,
                             const sMacAddr &src_mac, const std::string &iface_name = "");

    /**
     * @brief Start client steering initiated by NBAPI.
     *
     * @param sta_mac Mac address of client.
     * @param target_bssid Target BSSID.
     * @return True if client steering started successfully, false otherwise.
     */
    bool start_client_steering(const std::string &sta_mac, const std::string &target_bssid);

    /**
     * @brief Trigger channel scan initiated by NBAPI.
     *
     * @param ruid ruid of radio for wich scan requested.
     * @param channel_pool List of channels.
     * @param pool_size Channel pool size.
     * @param dwell_time Channel scan dwell time in milliseconds.
     * @return True if channel scan tiggered, false otherwise.
     */
    bool
    trigger_scan(const sMacAddr &ruid,
                 std::array<uint8_t, beerocks::message::SUPPORTED_CHANNELS_LENGTH> channel_pool,
                 uint8_t pool_size, int dwell_time);

private:
    /**
     * @brief Handles the client-connected event in the CMDU server.
     *
     * @param fd File descriptor of the socket that got connected.
     */
    void handle_connected(int fd);

    /**
     * @brief Handles the client-disconnected event in the CMDU server.
     *
     * @param fd File descriptor of the socket that got disconnected.
     */
    void handle_disconnected(int fd);

    /**
     * @brief Handles received CMDU message.
     *
     * @param fd File descriptor of the socket connection the CMDU was received through.
     * @param iface_index Index of the network interface that the CMDU message was received on.
     * @param dst_mac Destination MAC address.
     * @param src_mac Source MAC address.
     * @param cmdu_rx Received CMDU to be handled.
     * @return true on success and false otherwise.
     */
    bool handle_cmdu(int fd, uint32_t iface_index, const sMacAddr &dst_mac, const sMacAddr &src_mac,
                     ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Handles CMDU message received from broker.
     *
     * This handler is slightly different than the handler for CMDU messages received from other
     * processes as it checks the source and destination MAC addresses set by the original sender.
     * It also filters out messages that are not addressed to the controller.
     *
     * @param iface_index Index of the network interface that the CMDU message was received on.
     * @param dst_mac Destination MAC address.
     * @param src_mac Source MAC address.
     * @param cmdu_rx Received CMDU to be handled.
     * @return true on success and false otherwise.
     */
    bool handle_cmdu_from_broker(uint32_t iface_index, const sMacAddr &dst_mac,
                                 const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);

    bool handle_cmdu_1905_1_message(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_control_message(const sMacAddr &src_mac,
                                     std::shared_ptr<beerocks::beerocks_header> beerocks_header);
    void handle_cmdu_control_ieee1905_1_message(const std::string &src_mac,
                                                ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_intel_slave_join(const sMacAddr &src_mac,
                                 std::shared_ptr<wfa_map::tlvApRadioBasicCapabilities> radio_caps,
                                 beerocks::beerocks_header &beerocks_header,
                                 ieee1905_1::CmduMessageTx &cmdu_tx,
                                 const std::shared_ptr<Agent> &agent);
    bool
    handle_non_intel_slave_join(const sMacAddr &src_mac,
                                std::shared_ptr<wfa_map::tlvApRadioBasicCapabilities> radio_caps,
                                const WSC::m1 &m1, const std::shared_ptr<Agent> &agent,
                                const sMacAddr &radio_mac, ieee1905_1::CmduMessageTx &cmdu_tx);

    // 1905 messages handlers
    bool handle_cmdu_1905_autoconfiguration_search(const sMacAddr &src_mac,
                                                   ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_autoconfiguration_WSC(const sMacAddr &src_mac,
                                                ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_ap_metric_response(const sMacAddr &src_mac,
                                             ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_ap_capability_report(const sMacAddr &src_mac,
                                               ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_channel_preference_report(const sMacAddr &src_mac,
                                                    ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_channel_selection_response(const sMacAddr &src_mac,
                                                     ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_channel_scan_report(const sMacAddr &src_mac,
                                              ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_operating_channel_report(const sMacAddr &src_mac,
                                                   ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_ack_message(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_higher_layer_data_message(const sMacAddr &src_mac,
                                                    ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_steering_completed_message(const sMacAddr &src_mac,
                                                     ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_client_steering_btm_report_message(const sMacAddr &src_mac,
                                                             ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_beacon_response(const sMacAddr &src_mac,
                                          ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_backhaul_sta_steering_response(const sMacAddr &src_mac,
                                                         ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_tunnelled_message(const sMacAddr &src_mac,
                                            ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_failed_connection_message(const sMacAddr &src_mac,
                                                    ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_1905_associated_sta_link_metrics_response_message(
        const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool autoconfig_wsc_parse_radio_caps(
        const sMacAddr &radio_mac,
        std::shared_ptr<wfa_map::tlvApRadioBasicCapabilities> radio_caps);

    /**
     * @brief Get info from 'AP HT Capabilities' TLV,
     * set data to AP HTCapabilities data element from Controller Data Model.
     *
     * @param cmdu_rx AP Capability Report message
     * @return true on success, false otherwise
    */
    bool handle_tlv_ap_ht_capabilities(ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Get info from 'AP HE Capabilities' TLV,
     * set data to AP HECapabilities data element.
     *
     * @param cmdu_rx AP Capability Report message.
     * @return True on success, false otherwise.
    */
    bool handle_tlv_ap_he_capabilities(ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Get info from 'AP VHT Capabilities' TLV,
     * set data to AP VHTCapabilities data element.
     *
     * @param cmdu_rx AP Capability Report message.
     * @return True on success, false otherwise.
    */
    bool handle_tlv_ap_vht_capabilities(ieee1905_1::CmduMessageRx &cmdu_rx);

    // Autoconfig encryption support
    bool autoconfig_wsc_add_m2(WSC::m1 &m1, const wireless_utils::sBssInfoConf *bss_info_conf);
    bool autoconfig_wsc_add_m2_encrypted_settings(WSC::m2::config &m2_cfg,
                                                  WSC::configData &config_data, uint8_t authkey[32],
                                                  uint8_t keywrapkey[16]);
    bool autoconfig_wsc_authentication(WSC::m1 &m1, WSC::m2 &m2, uint8_t authkey[32]);
    void autoconfig_wsc_calculate_keys(WSC::m1 &m1, WSC::m2::config &m2_cfg,
                                       const mapf::encryption::diffie_hellman &dh,
                                       uint8_t authkey[32], uint8_t keywrapkey[16]);

    /**
     * @brief Handles Tlv of AP Extended Metrics (tlvApExtendedMetrics).
     *
     * @param agent agent shared object.
     * @param cmdu_rx  AP Extended Metrics Response message.
     * @return True on success, false otherwise.
    */
    bool handle_tlv_ap_extended_metrics(std::shared_ptr<Agent> agent,
                                        ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Handles Tlv of STA Link Metrics (tlvAssociatedStaLinkMetrics).
     *
     * @param src_mac Source MAC address.
     * @param cmdu_rx  AP Metrics Response or Associated STA Link Metrics Response message.
     * @return True on success, false otherwise.
    */
    bool handle_tlv_associated_sta_link_metrics(const sMacAddr &src_mac,
                                                ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Handles Tlv of STA Extended Link Metrics (tlvAssociatedStaExtendedLinkMetrics).
     *
     * @param src_mac Source MAC address.
     * @param cmdu_rx  AP Metrics Response or Associated STA Link Metrics Response message.
     * @return True on success, false otherwise.
    */
    bool handle_tlv_associated_sta_extended_link_metrics(const sMacAddr &src_mac,
                                                         ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Handles Tlv of STA Traffic Stats (tlvAssociatedStaTrafficStats).
     *
     * @param src_mac Source MAC address.
     * @param cmdu_rx  AP Metrics Response message.
     * @return True on success, false otherwise.
    */
    bool handle_tlv_associated_sta_traffic_stats(const sMacAddr &src_mac,
                                                 ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Handles Tlv of Profile-2 AP Capability (tlvProfile2ApCapability).
     *
     * @param agent agent db shared object.
     * @param cmdu_rx Received CMDU as AP Capability Report message.
     * @return True on success, false otherwise.
    */
    bool handle_tlv_profile2_ap_capability(std::shared_ptr<Agent> agent,
                                           ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Handles Tlv of Profile-2 CAC Status Report (tlvProfile2CacStatusReport).
     *
     * @param radio radio db shared object.
     * @param cmdu_rx Received CMDU as CAC Status Report message.
     * @return True on success, false otherwise.
    */
    bool handle_tlv_profile2_cac_status_report(std::shared_ptr<Agent::sRadio> radio,
                                               ieee1905_1::CmduMessageRx &cmdu_rx);
    /**
     * @brief Extracts ESP value from est_service_info_field and set it to specified
     * with @param_name NBAPI EstServiceParameter.
     *
     * @param param_name Name of NBAPI EstServiceParameter parameter to be set.
     * @param reporting_agent_bssid BSSID of BSS for which parameter will be set.
     * @param est_service_info_field Array with ESP values.
     *
    */
    void set_esp(const std::string &param_name, const sMacAddr &reporting_agent_bssid,
                 uint8_t *est_service_info_field);

    /**
     * Buffer to hold CMDU to be transmitted.
     */
    uint8_t m_tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];

    /**
     * CMDU to be transmitted.
     */
    ieee1905_1::CmduMessageTx cmdu_tx;

    /**
     * Buffer to hold CMDU to be transmitted by the UCC listener (in certification mode).
     */
    uint8_t m_cert_tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];

    /**
     * CMDU to be transmitted by the UCC listener (in certification mode).
     */
    ieee1905_1::CmduMessageTx cert_cmdu_tx;

    db &database;
    task_pool tasks;
    periodic_operation_pool operations;
    beerocks::controller_ucc_listener m_controller_ucc_listener;

    // It is used only in handle_cmdu_1905_ap_metric_response() to call construct_combined_infra_metric().
    // TODO It can be removed after cert_cmdu_tx usage is removed (PPM-1130).
    std::shared_ptr<LinkMetricsTask> m_link_metrics_task;

    /**
     * Factory to create broker client instances connected to broker server.
     * Broker client instances are used to exchange CMDU messages with remote processes running in
     * other devices in the network via the broker server running in the transport process.
     */
    std::unique_ptr<beerocks::btl::BrokerClientFactory> m_broker_client_factory;

    /**
     * CMDU server to exchange CMDU messages with clients through socket connections.
     */
    std::unique_ptr<beerocks::CmduServer> m_cmdu_server;

    /**
     * Timer manager to help using application timers.
     */
    std::shared_ptr<beerocks::TimerManager> m_timer_manager;

    /**
     * Application event loop used by the process to wait for I/O events.
     */
    std::shared_ptr<beerocks::EventLoop> m_event_loop;

    /**
     * File descriptor of the timer to run internal tasks periodically.
     */
    int m_tasks_timer = beerocks::net::FileDescriptor::invalid_descriptor;

    /**
     * File descriptor of the timer to run periodic operations.
     */
    int m_operations_timer = beerocks::net::FileDescriptor::invalid_descriptor;

    /**
     * Broker client to exchange CMDU messages with broker server running in transport process.
     */
    std::unique_ptr<beerocks::btl::BrokerClient> m_broker_client;
};

} // namespace son

#endif
