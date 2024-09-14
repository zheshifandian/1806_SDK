/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _SON_SLAVE_THREAD_H
#define _SON_SLAVE_THREAD_H

#include "tasks/task_pool.h"

#include <bcl/beerocks_backport.h>
#include <bcl/beerocks_cmdu_client.h>
#include <bcl/beerocks_cmdu_client_factory.h>
#include <bcl/beerocks_cmdu_server.h>
#include <bcl/beerocks_eventloop_thread.h>
#include <bcl/beerocks_logging.h>
#include <bcl/beerocks_timer_manager.h>
#include <bcl/network/sockets_impl.h>
#include <btl/broker_client.h>
#include <btl/broker_client_factory.h>

#include <beerocks/tlvf/beerocks_header.h>
#include <tlvf/wfa_map/tlvChannelPreference.h>

// Forward decleration
namespace beerocks_message {
class cChannelList;
}

namespace beerocks {
namespace bpl {
enum class eErrorCode;
}

class slave_thread final : public EventLoopThread {

public:
    struct sAgentConfig {
        // Common configuration from Agent configuration file.
        std::string temp_path;
        std::string vendor;
        std::string model;
        uint16_t ucc_listener_port;
        std::string bridge_iface;
        int stop_on_failure_attempts;
        std::string backhaul_preferred_bssid;

        // Radio configuration
        struct sRadioConfig {
            std::string backhaul_wireless_iface;
            int hostap_ant_gain;
            bool enable_repeater_mode;
            eIfaceType hostap_iface_type;

            // This parameter does not exist on the configuration file.
            // Need to check if it still needded. Meanwhile keep it. PPM-1550.
            bool backhaul_wireless_iface_filter_low;
        };

        // key: fronthaul interface name
        std::unordered_map<std::string, sRadioConfig> radios;
    };

    enum eSlaveState {
        // General
        STATE_WAIT_BEFORE_INIT = 0,
        STATE_INIT,
        STATE_LOAD_PLATFORM_CONFIGURATION,
        STATE_CONNECT_TO_PLATFORM_MANAGER,
        STATE_WAIT_FOR_PLATFORM_MANAGER_REGISTER_RESPONSE,
        STATE_CONNECT_TO_BACKHAUL_MANAGER,
        STATE_WAIT_RETRY_CONNECT_TO_BACKHAUL_MANAGER,
        STATE_WAIT_FOR_BACKHAUL_MANAGER_REGISTER_RESPONSE,
        STATE_JOIN_INIT,
        STATE_WAIT_FOR_FRONTHAUL_THREADS_JOINED,
        STATE_BACKHAUL_ENABLE,
        STATE_SEND_BACKHAUL_MANAGER_ENABLE,
        STATE_WAIT_FOR_BACKHAUL_MANAGER_CONNECTED_NOTIFICATION,
        STATE_BACKHAUL_MANAGER_CONNECTED,
        STATE_WAIT_FOR_AUTO_CONFIGURATION_COMPLETE,
        STATE_OPERATIONAL,
        STATE_STOPPED,
    };

    slave_thread(sAgentConfig conf, logging &logger_);
    virtual ~slave_thread();

    /**
     * @brief Initialize the Agent.
     *
     * @return true on success and false otherwise.
     */
    bool thread_init() override;

    /**
     * @brief Sends given CMDU message through the specified socket connection.
     *
     * @param fd File descriptor of the connected socket.
     * @param cmdu_tx CMDU message to send.
     * @return true on success and false otherwise.
     */
    bool send_cmdu(int fd, ieee1905_1::CmduMessageTx &cmdu_tx);

    /**
     * @brief Forwards given received CMDU message through the specified socket connection.
     *
     * @param fd File descriptor of the connected socket.
     * @param cmdu_rx Received CMDU message to forward.
     * @return true on success and false otherwise.
     */
    bool forward_cmdu_to_uds(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Sends ACK CMDU to controller
     *
     * @param cmdu CMDU message to send.
     * @param mid The message id to attach to the ACK
     * @return true on success and false otherwise.
     */
    bool send_ack_to_controller(ieee1905_1::CmduMessageTx &cmdu_tx, uint32_t mid);

private:
    /**
     * @brief Handles received CMDU message.
     *
     * @param fd File descriptor of the socket connection the CMDU was received through.
     * @param cmdu_rx Received CMDU to be handled.
     * @return true on success and false otherwise.
     */
    bool handle_cmdu(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * @brief Sends CMDU to transport for dispatching.
     *
     * @param cmdu CMDU message to send.
     * @param dst_mac Destination MAC address.
     * @param src_mac Source MAC address.
     * @param iface_name Name of the network interface to use (set to empty string to send on all
     * available interfaces).
     * @return true on success and false otherwise.
     */
    bool send_cmdu_to_broker(ieee1905_1::CmduMessageTx &cmdu, const sMacAddr &dst_mac,
                             const sMacAddr &src_mac, const std::string &iface_name = "");

    /**
     * @brief Forwards given received CMDU message to the broker server for dispatching which
     * eventually will be sent to the Controller.
     *
     * @param cmdu_rx Received CMDU message to forward.
     * @return true on success and false otherwise.
     */
    bool forward_cmdu_to_controller(ieee1905_1::CmduMessageRx &cmdu_rx);

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

    void on_thread_stop() override;

    /**
     * @brief Handles the client-disconnected event in the CMDU server.
     *
     * @param fd File descriptor of the socket that got disconnected.
     */
    void handle_client_disconnected(int fd);

    bool handle_cmdu_control_message(int fd, std::shared_ptr<beerocks_header> beerocks_header);
    bool handle_cmdu_backhaul_manager_message(int fd,
                                              std::shared_ptr<beerocks_header> beerocks_header);
    bool handle_cmdu_platform_manager_message(int fd,
                                              std::shared_ptr<beerocks_header> beerocks_header);
    bool handle_cmdu_ap_manager_message(const std::string &fronthaul_iface, int fd,
                                        std::shared_ptr<beerocks_header> beerocks_header);
    bool handle_cmdu_monitor_message(const std::string &fronthaul_iface, int fd,
                                     std::shared_ptr<beerocks_header> beerocks_header);
    bool handle_cmdu_control_ieee1905_1_message(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_ap_manager_ieee1905_1_message(const std::string &fronthaul_iface, int fd,
                                                   ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_cmdu_monitor_ieee1905_1_message(const std::string &fronthaul_iface, int fd,
                                                ieee1905_1::CmduMessageRx &cmdu_rx);

    bool fsm_all();
    bool agent_fsm();
    void agent_reset();
    void stop_slave_thread();
    void fronthaul_start(const std::string &fronthaul_iface);
    void fronthaul_stop(const std::string &fronthaul_iface);
    void log_son_config(const std::string &fronthaul_iface);
    void platform_notify_error(bpl::eErrorCode code, const std::string &error_data);
    bool monitor_heartbeat_check(const std::string &fronthaul_iface);
    bool ap_manager_heartbeat_check(const std::string &fronthaul_iface);

public:
    /**
     * @brief Checks if there is a link to the Multi-AP Controller.
     * 
     * @return true if there is a link to the Controller, otherwise false.
     */
    bool link_to_controller();
    bool send_cmdu_to_controller(const std::string &fronthaul_iface,
                                 ieee1905_1::CmduMessageTx &cmdu_tx);

    inline int get_ap_manager_fd(const std::string &fronthaul_iface)
    {
        return m_radio_managers[fronthaul_iface].ap_manager_fd;
    }

    inline int get_monitor_fd(const std::string &fronthaul_iface)
    {
        return m_radio_managers[fronthaul_iface].monitor_fd;
    }

    inline const CmduClient *get_backhaul_manager_cmdu_client(const std::string &fronthaul_iface)
    {
        return m_backhaul_manager_client.get();
    }

    inline const CmduClient *get_platform_manager_cmdu_client(const std::string &fronthaul_iface)
    {
        return m_platform_manager_client.get();
    }

    inline const sAgentConfig &get_agent_conf() { return config; }

    inline void fsm_stop() { m_agent_state = STATE_STOPPED; }

private:
    /**
     * Buffer to hold CMDU to be transmitted.
     */
    uint8_t m_tx_buffer[message::MESSAGE_BUFFER_LENGTH];

    /**
     * CMDU to be transmitted.
     */
    ieee1905_1::CmduMessageTx cmdu_tx;

    /**
     * Timer manager to help using application timers.
     */
    std::shared_ptr<TimerManager> m_timer_manager;

    /**
     * File descriptor of the timer to run the Finite State Machine.
     */
    int m_fsm_timer = net::FileDescriptor::invalid_descriptor;

    /**
     * File descriptor of the timer to run internal tasks periodically.
     */
    int m_tasks_timer = net::FileDescriptor::invalid_descriptor;

    /**
     * Factory to create broker client instances connected to broker server.
     * Broker client instances are used to exchange CMDU messages with remote processes running in
     * other devices in the network via the broker server running in the transport process.
     */
    std::unique_ptr<btl::BrokerClientFactory> m_broker_client_factory;

    /**
     * Factory to create CMDU client instances connected to CMDU server running in platform manager.
     */
    std::unique_ptr<CmduClientFactory> m_platform_manager_cmdu_client_factory;

    /**
     * Factory to create CMDU client instances connected to CMDU server running in backhaul manager.
     */
    std::unique_ptr<CmduClientFactory> m_backhaul_manager_cmdu_client_factory;

    /**
     * CMDU server address used by CmduServer `m_cmdu_server`.
     */
    std::shared_ptr<net::UdsAddress> m_cmdu_server_uds_address;

    /**
     * @note There is a difference in the use of a client socket the Agent holds to another server
     * (Broker/Backhaul Manager/Platform Manager) and clients that are connected to the Agent CMDU
     * server (AP Manager/Monitor).
     * On a client we hold, we need to use the internal client object function send()/forward(),
     * whereas, for clients that are connected to the Agent, the Agent only hold their file
     * descriptor so for sending them a CMDU, need to use m_cmdu_server send_cmdu()/forward_cmdu()
     * which this class have wrapper function for.
     */

    /**
     * CMDU server to exchange CMDU messages with clients through socket connections.
     */
    std::unique_ptr<CmduServer> m_cmdu_server;

    /**
     * Broker client to exchange CMDU messages with broker server running in transport process.
     */
    std::unique_ptr<btl::BrokerClient> m_broker_client;

    /**
     * CMDU client connected to the the CMDU server running in platform manager.
     * This object is dynamically created using the CMDU client factory for the platform manager
     * provided in class constructor.
     */
    std::unique_ptr<CmduClient> m_platform_manager_client;

    /**
     * CMDU client connected to the the CMDU server running in backhaul manager.
     * This object is dynamically created using the CMDU client factory for the backhaul manager
     * provided in class constructor.
     */
    std::unique_ptr<CmduClient> m_backhaul_manager_client;

    bool m_is_backhaul_disconnected = false;
    int m_agent_resets_counter      = 0;

    TaskPool m_task_pool;

    // Global FSM members:
    eSlaveState m_agent_state;
    std::chrono::steady_clock::time_point m_agent_state_timer_sec =
        std::chrono::steady_clock::time_point::min();

    bool m_stopped = false;

    struct sManagedRadio {
        int stop_on_failure_attempts;
        bool configuration_in_progress = false;
        bool fronthaul_started         = false;
        int monitor_fd                 = net::FileDescriptor::invalid_descriptor;
        int ap_manager_fd              = net::FileDescriptor::invalid_descriptor;
        std::chrono::steady_clock::time_point monitor_last_seen;
        std::chrono::steady_clock::time_point ap_manager_last_seen;
        int monitor_retries_counter    = 0;
        int ap_manager_retries_counter = 0;

        int last_reported_backhaul_rssi = RSSI_INVALID;
    };

    class cRadioManagers {
        // Regular interfaces list.
        // Key: fronthaul iface name
        std::map<std::string, sManagedRadio> m_radio_managers;
        std::unique_ptr<std::pair<std::string, sManagedRadio>> m_zwdfs_radio_manager;

        sManagedRadio &get_radio_context(const std::string &fronthaul_iface)
        {
            auto zwdfs = m_zwdfs_radio_manager && m_zwdfs_radio_manager->first == fronthaul_iface;

            auto it = m_radio_managers.find(fronthaul_iface);
            if (!zwdfs && it == m_radio_managers.end()) {
                LOG(DEBUG) << "Added new interface to radio managers: " << fronthaul_iface;
                // Insert empty new element
                it = m_radio_managers.emplace(fronthaul_iface, sManagedRadio{}).first;
            }

            return zwdfs ? m_zwdfs_radio_manager->second : it->second;
        }

    public:
        sManagedRadio &operator[](const std::string &fronthaul_iface)
        {
            return get_radio_context(fronthaul_iface);
        }

        std::map<std::string, sManagedRadio> &get() { return m_radio_managers; }
        std::unique_ptr<std::pair<std::string, sManagedRadio>> &get_zwdfs()
        {
            return m_zwdfs_radio_manager;
        }

        void set_zwdfs(std::map<std::string, sManagedRadio>::iterator &it)
        {
            m_zwdfs_radio_manager  = std::make_unique<std::pair<std::string, sManagedRadio>>();
            *m_zwdfs_radio_manager = std::move(*it);
            m_radio_managers.erase(it);
        }

        /**
         * @brief Preform an operation in context of each radio.
         *
         * @param operation An operation function that receives as input arguments the radio
         * manager context and its fronthaul interface name.
         * @return true if the operation succeeds in all radios, otherwise false.
         */
        bool do_on_each_radio_manager(
            std::function<bool(sManagedRadio &radio_manager, const std::string &fronthaul_iface)>
                operation)
        {
            auto success = true;
            for (auto &radio : get()) {
                success &= operation(radio.second, radio.first);
            }
            auto &zwdfs_radio = get_zwdfs();
            if (!zwdfs_radio) {
                return success;
            }
            success &= operation(zwdfs_radio->second, zwdfs_radio->first);
            return success;
        };
    } m_radio_managers;

    /**
     * @brief check if there was an error in the constructor
     *
     * @return false if no errors occurred in the constructor, true otherwise
     */
    bool m_constructor_failed = false;

    sAgentConfig config;

    logging &logger;

    /**
     * @brief Reset a Fronthaul by disconnecting a socket to one of its threads.
     * 
     * @details When disconnecting a socket to one of the Fronthaul threads, the disconnect handler
     * will disconnect the other one too, and call to agent_reset(). The choice which one to
     * disconnect is arbitrary.
     * 
     * @param parameter A radio manager struct containing the fronthaul threads socket file
     * descriptors.
     */
    inline void fronthaul_reset(const sManagedRadio &radio_manager) const
    {
        m_cmdu_server->disconnect(radio_manager.ap_manager_fd);
    }

    bool send_operating_channel_report(const std::string &fronthaul_iface);
    bool handle_ap_metrics_query(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_monitor_ap_metrics_response(const std::string &fronthaul_iface, int fd,
                                            ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_channel_preference_query(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_channel_selection_request(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool get_controller_channel_preference(const std::string &fronthaul_iface,
                                           ieee1905_1::CmduMessageRx &cmdu_rx);
    bool channel_selection_get_transmit_power_limit(const std::string &fronthaul_iface,
                                                    ieee1905_1::CmduMessageRx &cmdu_rx,
                                                    int &power_limit);
    bool channel_selection_current_channel_restricted(const std::string &fronthaul_iface);
    message::sWifiChannel channel_selection_select_channel(const std::string &fronthaul_iface);
    bool handle_client_association_request(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_1905_higher_layer_data_message(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_client_steering_request(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_beacon_metrics_query(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);
    bool handle_ack_message(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);

    bool read_platform_configuration();

    /**
     * @brief Save channel list into AgentDB from beerocks_message::cChannelList class.
     *
     * @param channel_list_class A shared pointer to channel_list_class.
     */
    void fill_channel_list_to_agent_db(
        const std::string &fronthaul_iface,
        const std::shared_ptr<beerocks_message::cChannelList> &channel_list_class);

    /**
     * @brief save channel switch parameters in the agent DB
     *
     * Save majority of sApChannelSwitch parameters in the agent DB.
     * Discard `switch_reason` and `is_dfs_channel` because they are not used in unified agent.
     */
    void save_channel_params_to_db(const std::string &fronthaul_iface,
                                   beerocks_message::sApChannelSwitch params);

    /**
     * @brief save cac capabilities in the agent DB
     */
    void save_cac_capabilities_params_to_db(const std::string &fronthaul_iface);

    struct sChannelPreference {
        sChannelPreference(uint8_t oper_class,
                           wfa_map::cPreferenceOperatingClasses::ePreference preference,
                           wfa_map::cPreferenceOperatingClasses::eReasonCode reason_code)
            : operating_class(oper_class)
        {
            flags.reason_code = reason_code;
            flags.preference  = preference;
        }
        sChannelPreference(uint8_t _operating_class,
                           wfa_map::cPreferenceOperatingClasses::sFlags _flags)
            : operating_class(_operating_class), flags(_flags)
        {
        }

        uint8_t operating_class;
        wfa_map::cPreferenceOperatingClasses::sFlags flags;

        bool operator==(const sChannelPreference &rhs) const
        {
            return operating_class == rhs.operating_class &&
                   flags.preference == rhs.flags.preference &&
                   flags.reason_code == rhs.flags.reason_code;
        }

        bool operator<(const sChannelPreference &rhs) const
        {
            if (operating_class != rhs.operating_class) {
                return operating_class < rhs.operating_class;
            }
            if (flags.preference != rhs.flags.preference) {
                return flags.preference < rhs.flags.preference;
            }
            if (flags.reason_code != rhs.flags.reason_code) {
                return flags.reason_code < rhs.flags.reason_code;
            }
            return false;
        }
    };
    std::map<sChannelPreference, std::set<uint8_t>> m_controller_channel_preferences;

    /**
     * @brief Get a std::map of channels preferences organized in a way it will be easy to fill
     * WFA Channel Preference Report.
     *
     * @return std::map of channels preferences.
     */
    std::map<sChannelPreference, std::set<uint8_t>>
    get_channel_preferences_from_channels_list(const std::string &fronthaul_iface);

    /**
     * @brief Get the channel preference.
     *
     * @pre The channel operating class and the preference operating class have to match.
     * @param channel A channel to check.
     * @param preference The preference of the channel.
     * @param preference_channels_list The preference channels list given by the Controller.
     * @return NON_OPERABLE if channel is restricted, channel preference otherwise.
     */
    wfa_map::cPreferenceOperatingClasses::ePreference
    get_channel_preference(message::sWifiChannel channel, const sChannelPreference &preference,
                           const std::set<uint8_t> &preference_channels_list);

    /**
     * @brief Update the vaps in the Agent DB.
     * @param iface The interface to use to find the radio in the DB.
     * @param vaps the array of VAPs to use for the update.
     * @return true on success, false on failure.
     */
    bool update_vaps_info(const std::string &iface, const beerocks_message::sVapInfo vaps[]);
};

} // namespace beerocks

#endif
