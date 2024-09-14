/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _PLATFORM_MANAGER_H
#define _PLATFORM_MANAGER_H

#include <bcl/beerocks_async_work_queue.h>
#include <bcl/beerocks_cmdu_server.h>
#include <bcl/beerocks_config_file.h>
#include <bcl/beerocks_event_loop.h>
#include <bcl/beerocks_logging.h>
#include <bcl/beerocks_timer_manager.h>
#include <bcl/network/buffer_impl.h>
#include <bcl/network/sockets.h>

#include "beerocks/tlvf/beerocks_message_common.h"

#include <bpl/bpl_arp.h>
#include <bpl/bpl_cfg.h>

#include <atomic>
#include <unordered_map>
#include <unordered_set>

namespace beerocks {

class PlatformManager {

public:
    /**
     * @brief Queries platform for a given parameter value.
     *
     * Platform configuration is read through BPL.
     *
     * @param parameter Parameter name. Possible values are "is_master", "is_gateway" and
     * "is_onboarding".
     * @return Requested parameter value on success and error message on error (i.e.: if unable to
     * initialize BPL or if parameter name is invalid).
     */
    static std::string query_db(const std::string &parameter);

    PlatformManager(config_file::sConfigSlave &config_,
                    const std::unordered_map<int, std::string> &interfaces_map, logging &logger_,
                    std::unique_ptr<beerocks::CmduServer> cmdu_server,
                    std::shared_ptr<beerocks::TimerManager> timer_manager,
                    std::shared_ptr<beerocks::EventLoop> event_loop);

    /**
     * @brief Class destructor.
     */
    ~PlatformManager();

    /**
     * @brief Starts platform manager.
     *
     * @return true on success and false otherwise.
     */
    bool start();

    /**
     * @brief Stops platform manager.
     *
     * @return true on success and false otherwise.
     */
    bool stop();

protected:
    bool handle_arp_monitor();
    bool handle_arp_raw();

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
     * @param fd File descriptor of the socket.
     */
    void handle_disconnected(int fd);

    /**
     * @brief Handles received CMDU message.
     *
     * @param fd File descriptor of the socket that the CMDU was received through.
     * @param cmdu_rx Received CMDU to be handled.
     * @return true on success and false otherwise.
     */
    bool handle_cmdu(int fd, ieee1905_1::CmduMessageRx &cmdu_rx);

    bool send_cmdu_to_agent_safe(ieee1905_1::CmduMessageTx &cmdu_tx);
    bool send_cmdu(int fd, ieee1905_1::CmduMessageTx &cmdu_tx);
    bool socket_disconnected(int fd);

    int get_agent_socket();
    void load_iface_params(const std::string &strIface, beerocks::eArpSource eType);
    std::string bridge_iface_from_mac(const sMacAddr &sMac);
    void send_dhcp_notification(const std::string &op, const std::string &mac,
                                const std::string &ip, const std::string &hostname);

    /**
     * @brief Clean old ARP table entries.
     *
     * Iterates over the list of ARP entries and removes those that have been inactive for a period
     * longer than the ARP notification interval.
     */
    void clean_old_arp_entries();
    bool init_dhcp_monitor();
    void stop_dhcp_monitor();
    bool init_arp_monitor();
    void stop_arp_monitor();
    bool restart_arp_monitor();

    /**
     * @brief Checks if WLAN parameters have changed.
     *
     * For each of the slaves, performs a check on the `band_enabled` and `channel` parameters and,
     * if changed, sends a ACTION_PLATFORM_WLAN_PARAMS_CHANGED_NOTIFICATION message to the slave
     * including the new settings.
     *
     * @return true if WLAN parameters have changed for any of the slaves.
     */
    bool check_wlan_params_changed();

private:
    const int PLATFORM_READ_CONF_RETRY_SEC    = 5;
    const int PLATFORM_READ_CONF_MAX_ATTEMPTS = 10;

    /**
     * Buffer to hold CMDU to be transmitted.
     */
    uint8_t m_tx_buffer[message::MESSAGE_BUFFER_LENGTH];

    /**
     * CMDU to be transmitted.
     */
    ieee1905_1::CmduMessageTx m_cmdu_tx;

    /**
     * Flag to ask asynchronous code (running in a background worker thread) to stop.
     */
    std::atomic<bool> m_should_stop{false};

    config_file::sConfigSlave &config;
    const std::unordered_map<int, std::string> interfaces_map;

    struct SIfaceParams {
        beerocks::eArpSource eType;
    };

    struct SArpEntry {
        uint32_t ip;
        int iface_index;
        std::chrono::steady_clock::time_point last_seen;
    };

    // Connected Agent file descriptor
    int m_agent_fd = beerocks::net::FileDescriptor::invalid_descriptor;
    std::mutex m_mtxSlaves;

    // Interfaces
    std::unordered_map<std::string, SIfaceParams> m_mapIfaces; // key=iface_name

    // ARP entries by source MAC (uint64_t) address
    std::unordered_map<sMacAddr, std::shared_ptr<SArpEntry>> m_mapArpEntries;
    std::chrono::steady_clock::time_point m_tpArpEntriesCleanup;

    bpl::BPL_ARP_MON_CTX m_ctxArpMon = nullptr;

    uint32_t m_uiArpMonIP;
    uint32_t m_uiArpMonMask;

    logging &logger;
    bool enable_arp_monitor;
    bool is_onboarding_on_init;

    std::string master_version;
    std::string slave_version;

    beerocks::async_work_queue work_queue;

    std::unordered_map<std::string, std::shared_ptr<beerocks_message::sWlanSettings>>
        bpl_iface_wlan_params_map;
    std::unordered_set<std::string> ap_ifaces;

    /**
     * Timer to periodically clean old ARP table entries.
     */
    int m_clean_old_arp_entries_timer = beerocks::net::FileDescriptor::invalid_descriptor;

    /**
     * Timer to periodically check if WLAN parameters have changed.
     */
    int m_check_wlan_params_changed_timer = beerocks::net::FileDescriptor::invalid_descriptor;

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
    std::shared_ptr<EventLoop> m_event_loop;

    /**
     * Connection over the ARP raw socket
     */
    std::unique_ptr<beerocks::net::Socket::Connection> m_arp_raw_socket_connection;

    /**
     * Connection over the ARP monitor socket
     */
    std::unique_ptr<beerocks::net::Socket::Connection> m_arp_mon_socket_connection;

    /**
     * Connection over the DHCP monitor socket
     */
    std::unique_ptr<beerocks::net::Socket::Connection> m_dhcp_mon_socket_connection;
};

} // namespace beerocks

#endif // _PLATFORM_MANAGER_H
