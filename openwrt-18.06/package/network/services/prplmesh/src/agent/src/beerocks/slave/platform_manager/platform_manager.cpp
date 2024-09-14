/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "platform_manager.h"

#include "../agent_db.h"

#include <bcl/network/network_utils.h>
#include <bcl/network/sockets_impl.h>
#include <bcl/transaction.h>
#include <easylogging++.h>

#include <beerocks/tlvf/beerocks_message.h>
#include <beerocks/tlvf/beerocks_message_platform.h>

#include <bpl/bpl_dhcp.h>

#include <net/if.h> // if_nametoindex

namespace beerocks {

//////////////////////////////////////////////////////////////////////////////
////////////////////////// Local Module Definitions //////////////////////////
//////////////////////////////////////////////////////////////////////////////

static const uint8_t s_arrZeroMac[]  = {0, 0, 0, 0, 0, 0};
static const uint8_t s_arrBCastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#define ARP_NOTIF_INTERVAL (90000)                  // 1.5 minutes
#define ARP_CLEAN_INTERVAL (ARP_NOTIF_INTERVAL * 2) // 2 notif. intervals

/**
 * Time interval to periodically clean old ARP table entries.
 */
constexpr std::chrono::milliseconds clean_old_arp_entries_timer_interval(ARP_CLEAN_INTERVAL);

/**
 * Time interval to periodically check if WLAN parameters have changed.
 */
constexpr std::chrono::milliseconds check_wlan_params_changed_timer_interval(5000);

//////////////////////////////////////////////////////////////////////////////
/////////////////////////// Local Module Functions ///////////////////////////
//////////////////////////////////////////////////////////////////////////////
static std::string get_sta_iface(const std::string &hostap_iface)
{
    char sta_iface_str[BPL_IFNAME_LEN];
    if (bpl::cfg_get_sta_iface(hostap_iface.c_str(), sta_iface_str) < 0) {
        LOG(DEBUG) << "failed to read sta_iface for slave ";
        return std::string();
    }
    auto sta_iface = std::string(sta_iface_str);
    if (!beerocks::net::network_utils::linux_iface_exists(sta_iface)) {
        LOG(DEBUG) << "sta iface " << sta_iface << " does not exist, clearing it from config";
        return std::string();
    }
    return sta_iface;
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Implementation ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

std::string PlatformManager::query_db(const std::string &parameter)
{
    std::string ret;
    if (bpl::bpl_init() < 0) {
        ret = "Failed to initialize BPL!";
    } else {
        if (parameter == "is_master") {
            ret = (bpl::cfg_is_master() > 0 ? "true" : "false");
        } else if (parameter == "is_gateway") {
            auto operating_mode = bpl::cfg_get_operating_mode();
            ret                 = (operating_mode == BPL_OPER_MODE_GATEWAY ||
                           operating_mode == BPL_OPER_MODE_GATEWAY_WISP
                       ? "true"
                       : "false");
        } else if (parameter == "is_onboarding") {
            ret = (bpl::cfg_is_onboarding() > 0 ? "true" : "false");
        } else {
            ret = "Error, bad parameter.\n"
                  "Allowed parameters: \n"
                  " is_master \n"
                  " is_gateway \n"
                  " is_onboarding \n";
        }
    }
    return ret;
}

PlatformManager::PlatformManager(config_file::sConfigSlave &config_,
                                 const std::unordered_map<int, std::string> &interfaces_map_,
                                 logging &logger_,
                                 std::unique_ptr<beerocks::CmduServer> cmdu_server,
                                 std::shared_ptr<beerocks::TimerManager> timer_manager,
                                 std::shared_ptr<beerocks::EventLoop> event_loop)
    : m_cmdu_tx(m_tx_buffer, sizeof(m_tx_buffer)), config(config_), interfaces_map(interfaces_map_),
      logger(logger_), m_cmdu_server(std::move(cmdu_server)), m_timer_manager(timer_manager),
      m_event_loop(event_loop)
{
    LOG_IF(!m_cmdu_server, FATAL) << "CMDU server is a null pointer!";
    LOG_IF(!m_timer_manager, FATAL) << "Timer manager is a null pointer!";
    LOG_IF(!m_event_loop, FATAL) << "Event loop is a null pointer!";

    enable_arp_monitor = (config.enable_arp_monitor == "1");

    int i = 0;
    for (int j = 0; j < IRE_MAX_SLAVES && i < BPL_NUM_OF_INTERFACES; ++j) {
        auto hostap_iface_elm = interfaces_map.find(j);
        if (hostap_iface_elm == interfaces_map.end() || hostap_iface_elm->second.empty())
            continue;

        auto hostap_iface = hostap_iface_elm->second;
        ap_ifaces.insert(hostap_iface);
        i++;
    }

    beerocks::CmduServer::EventHandlers handlers{
        .on_client_connected    = [&](int fd) { handle_connected(fd); },
        .on_client_disconnected = [&](int fd) { handle_disconnected(fd); },
        .on_cmdu_received       = [&](int fd, uint32_t iface_index, const sMacAddr &dst_mac,
                                const sMacAddr &src_mac,
                                ieee1905_1::CmduMessageRx &cmdu_rx) { handle_cmdu(fd, cmdu_rx); },
    };
    m_cmdu_server->set_handlers(handlers);
}

PlatformManager::~PlatformManager() { m_cmdu_server->clear_handlers(); }

bool PlatformManager::start()
{
    // In case of error in one of the steps of this method, we have to undo all the previous
    // steps (like when rolling back a database transaction, where either all steps get executed
    // or none of them gets executed)
    beerocks::Transaction transaction;

    // Create a timer to periodically check if WLAN parameters have changed
    m_check_wlan_params_changed_timer = m_timer_manager->add_timer(
        "Platform Manager Periodic WLAN Check", check_wlan_params_changed_timer_interval,
        check_wlan_params_changed_timer_interval, [&](int fd, beerocks::EventLoop &loop) {
            check_wlan_params_changed();
            return true;
        });
    if (m_check_wlan_params_changed_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(ERROR) << "Failed to create the check-WLAN-parameters-changed timer";
        return false;
    }
    LOG(DEBUG) << "Check-WLAN-parameters-changed timer created with fd = "
               << m_check_wlan_params_changed_timer;
    transaction.add_rollback_action(
        [&]() { m_timer_manager->remove_timer(m_check_wlan_params_changed_timer); });

    // Initialize the BPL (Beerocks Platform Library)
    if (bpl::bpl_init() < 0) {
        LOG(ERROR) << "Failed to initialize BPL!";
        return false;
    }
    transaction.add_rollback_action([&]() { bpl::bpl_close(); });

    int i = 0;
    for (int slave_num = 0; slave_num < IRE_MAX_SLAVES && i < BPL_NUM_OF_INTERFACES; ++slave_num) {

        auto hostap_iface_elm = interfaces_map.find(slave_num);
        if (hostap_iface_elm == interfaces_map.end() || hostap_iface_elm->second.empty())
            continue;

        config.sta_iface[slave_num] = get_sta_iface(hostap_iface_elm->second);

        if (config.sta_iface[slave_num].empty())
            continue;

        i++;
    }

    // Bridge & Backhaul interface
    is_onboarding_on_init = bpl::cfg_is_onboarding();
    if (is_onboarding_on_init) {
        LOG(DEBUG) << "Onboarding state.";
        for (int slave_num = 0; slave_num < IRE_MAX_SLAVES; slave_num++) {
            if (!config.sta_iface[slave_num].empty()) {
                load_iface_params(config.sta_iface[slave_num], beerocks::ARP_SRC_WIRELESS_BACK);
            }
        }
    } else {
        LOG(DEBUG) << "Non-onboarding state.";

        auto db = AgentDB::get();

        if (config.bridge_iface.empty()) {
            LOG(ERROR) << "bridge_iface is empty!";
            return false;
        }
        load_iface_params(config.bridge_iface, beerocks::ARP_SRC_ETH_FRONT);

        if (db->ethernet.wan.iface_name.empty()) {
            LOG(ERROR) << "wan.iface_name is empty!";
            return false;
        }
        load_iface_params(db->ethernet.wan.iface_name, beerocks::ARP_SRC_ETH_BACK);

        for (int slave_num = 0; slave_num < IRE_MAX_SLAVES; slave_num++) {
            if (!config.sta_iface[slave_num].empty()) {
                load_iface_params(config.sta_iface[slave_num], beerocks::ARP_SRC_WIRELESS_BACK);
            }
        }
    }

    // Start the async work queue
    m_should_stop = false;
    if (!work_queue.start()) {
        LOG(ERROR) << "Failed starting asynchronous work queue";
        return false;
    }

    transaction.commit();

    return true;
}

bool PlatformManager::stop()
{
    bool result = true;

    LOG(DEBUG) << "Stopping asynchronous work queue...";
    m_should_stop = true;
    work_queue.stop(true);

    // Stop the DHCP Monitor
    stop_dhcp_monitor();

    // Stop the ARP Monitor
    stop_arp_monitor();

    bpl::bpl_close();
    LOG(DEBUG) << "Closed BPL.";

    bpl_iface_wlan_params_map.clear();

    // Cancel and remove the check-WLAN-parameters-changed timer
    if (m_check_wlan_params_changed_timer != beerocks::net::FileDescriptor::invalid_descriptor) {
        if (!m_timer_manager->remove_timer(m_check_wlan_params_changed_timer)) {
            result = false;
        }
    }

    return result;
}

bool PlatformManager::send_cmdu_to_agent_safe(ieee1905_1::CmduMessageTx &cmdu_tx)
{
    // Lock the slaves socket map
    std::unique_lock<std::mutex> lock(m_mtxSlaves);
    if (m_agent_fd == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(ERROR) << "Agent fd is invalid";
        return false;
    }
    return send_cmdu(m_agent_fd, cmdu_tx);
}

bool PlatformManager::send_cmdu(int fd, ieee1905_1::CmduMessageTx &cmdu_tx)
{
    return m_cmdu_server->send_cmdu(fd, cmdu_tx);
}

int PlatformManager::get_agent_socket()
{
    // Lock the slaves socket map
    std::unique_lock<std::mutex> lock(m_mtxSlaves);
    return m_agent_fd;
}

void PlatformManager::load_iface_params(const std::string &strIface, beerocks::eArpSource eType)
{
    // Ignore empty interfaces
    if (strIface.empty()) {
        LOG(ERROR) << "strIface is empty!";
        return;
    }

    LOG(DEBUG) << "load_iface_params(), Interface " << strIface;

    m_mapIfaces[strIface] = {eType}; // Struct initialization
}

std::string PlatformManager::bridge_iface_from_mac(const sMacAddr &sMac)
{
    char iface_name[BPL_ARP_IFACE_NAME_LEN];

    // Read the interface name using BPL
    if (bpl::arp_get_bridge_iface(config.bridge_iface.c_str(), sMac.oct, iface_name) == -1) {
        return {};
    }

    return std::string(iface_name);
}

void PlatformManager::send_dhcp_notification(const std::string &op, const std::string &mac,
                                             const std::string &ip, const std::string &hostname)
{
    LOG(DEBUG) << "DHCP Event: " << op << ", mac: " << mac << ", ip: " << ip
               << ", hostname: " << hostname;
    auto dhcp_notif = message_com::create_vs_message<
        beerocks_message::cACTION_PLATFORM_DHCP_MONITOR_NOTIFICATION>(m_cmdu_tx);

    if (dhcp_notif == nullptr) {
        LOG(ERROR) << "Failed building ACTION_PLATFORM_DHCP_MONITOR_NOTIFICATION message!";
        return;
    }

    // Build the DHCP notification message
    if (op == "add")
        dhcp_notif->op() = beerocks_message::eDHCPOp_Add;
    else if (op == "del")
        dhcp_notif->op() = beerocks_message::eDHCPOp_Del;
    else if (op == "old")
        dhcp_notif->op() = beerocks_message::eDHCPOp_Old;

    dhcp_notif->mac()  = tlvf::mac_from_string(mac);
    dhcp_notif->ipv4() = beerocks::net::network_utils::ipv4_from_string(ip);
    string_utils::copy_string(dhcp_notif->hostname(0), hostname.c_str(), message::NODE_NAME_LENGTH);

    send_cmdu_to_agent_safe(m_cmdu_tx);
}

bool PlatformManager::check_wlan_params_changed()
{
    bool any_slave_changed = false;
    for (auto &elm : bpl_iface_wlan_params_map) {
        if (elm.second == nullptr) {
            LOG(ERROR) << "invalid map - pointer to NULL";
            return false;
        }
        auto &iface              = elm.first;
        bool wlan_params_changed = false;
        bpl::BPL_WLAN_PARAMS params;
        if (bpl::cfg_get_wifi_params(iface.c_str(), &params) < 0) {
            LOG(ERROR) << "Failed reading '" << iface << "' parameters!";
            return false;
        }

        if (elm.second->band_enabled != params.enabled) {
            elm.second->band_enabled = params.enabled;
            LOG(DEBUG) << "band_enabled changed";
            wlan_params_changed = true;
        }
        if (elm.second->channel != params.channel) {
            elm.second->channel = params.channel;
            LOG(DEBUG) << "channel changed";
            wlan_params_changed = true;
        }

        if (wlan_params_changed) {
            any_slave_changed = true;
            auto notification = message_com::create_vs_message<
                beerocks_message::cACTION_PLATFORM_WLAN_PARAMS_CHANGED_NOTIFICATION>(m_cmdu_tx);
            if (!notification) {
                LOG(ERROR) << "Failed building message!";
                return false;
            }
            notification->set_iface_name(iface);
            notification->wlan_settings().band_enabled = elm.second->band_enabled;
            notification->wlan_settings().channel      = elm.second->channel;

            send_cmdu_to_agent_safe(m_cmdu_tx);
            LOG(DEBUG) << "wlan_params_changed - cmdu msg sent, iface=" << elm.first
                       << " cmdu msg sent";
        }
    }
    return any_slave_changed;
}

void PlatformManager::handle_connected(int fd) { LOG(INFO) << "UDS socket connected, fd = " << fd; }

void PlatformManager::handle_disconnected(int fd)
{
    std::unique_lock<std::mutex> lock(m_mtxSlaves);
    if (m_agent_fd == fd) {
        LOG(INFO) << "Agent socket disconnected! fd = " << fd;
        m_agent_fd = beerocks::net::FileDescriptor::invalid_descriptor;
        bpl_iface_wlan_params_map.clear();
    } else {
        LOG(ERROR) << "Unkown socket disconnected! fd = " << fd;
        return;
    }
}

bool PlatformManager::handle_cmdu(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto beerocks_header = message_com::parse_intel_vs_message(cmdu_rx);
    if (!beerocks_header) {
        LOG(ERROR) << "Not a vendor specific message";
        return false;
    }

    if (beerocks_header->action() != beerocks_message::ACTION_PLATFORM) {
        LOG(ERROR) << "Unknown message, action: " << int(beerocks_header->action());
        return true;
    }

    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_PLATFORM_SON_SLAVE_REGISTER_REQUEST: {
        LOG(TRACE) << "ACTION_PLATFORM_SON_SLAVE_REGISTER_REQUEST";
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_PLATFORM_SON_SLAVE_REGISTER_REQUEST>();
        if (!request) {
            LOG(ERROR) << "addClass cACTION_PLATFORM_SON_SLAVE_REGISTER_REQUEST failed";
            return false;
        }

        auto agent_name = std::move(std::string("agent socket"));
        LOG(DEBUG) << "Assigning FD (" << fd << ") to " << agent_name;
        m_cmdu_server->set_client_name(fd, agent_name);

        // Lock the Agent socket mutex to be able to work in parallel with the work queue.
        {
            std::unique_lock<std::mutex> lock(m_mtxSlaves);
            m_agent_fd = fd;
        }

        // Response message
        if (!message_com::create_vs_message<
                beerocks_message::cACTION_PLATFORM_SON_SLAVE_REGISTER_RESPONSE>(m_cmdu_tx)) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        send_cmdu_to_agent_safe(m_cmdu_tx);

    } break;

    case beerocks_message::ACTION_PLATFORM_CHANGE_MODULE_LOGGING_LEVEL: {
        LOG(TRACE) << "ACTION_PLATFORM_CHANGE_MODULE_LOGGING_LEVEL";
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_PLATFORM_CHANGE_MODULE_LOGGING_LEVEL>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass ACTION_PLATFORM_CHANGE_MODULE_LOGGING_LEVEL failed";
            return false;
        }
        logger.set_log_level_state((beerocks::eLogLevel)request->params().log_level,
                                   request->params().enable);

    } break;

    case beerocks_message::ACTION_PLATFORM_ARP_QUERY_REQUEST: {
        LOG(TRACE) << "ACTION_PLATFORM_ARP_QUERY_REQUEST";

        auto request =
            beerocks_header->addClass<beerocks_message::cACTION_PLATFORM_ARP_QUERY_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_PLATFORM_ARP_QUERY_REQUEST failed";
            return false;
        }

        // Send a probe request to the ARP monitor
        if (!m_ctxArpMon) {
            LOG(WARNING) << "ARP monitor NOT active...";
            break;
        }

        if (bpl::arp_mon_probe(m_ctxArpMon, request->params().mac.oct, request->params().ipv4.oct,
                               beerocks_header->id()) != 0) {
            LOG(DEBUG) << "ARP probe failed!";
            break;
        }

        // Add the MAC to the arp entries map
        if (m_mapArpEntries.find(request->params().mac) == m_mapArpEntries.end()) {
            auto pArpEntry = std::make_shared<SArpEntry>();

            // Only the IP address is initialized at this point
            pArpEntry->ip =
                beerocks::net::network_utils::uint_ipv4_from_array(&request->params().ipv4.oct);
            pArpEntry->iface_index = -1;
            pArpEntry->last_seen   = std::chrono::steady_clock::now();

            LOG(DEBUG) << "Adding MAC " << request->params().mac << " to the ARP list...";

            m_mapArpEntries[request->params().mac] = pArpEntry;
        }

    } break;

    case beerocks_message::ACTION_PLATFORM_ADMIN_CREDENTIALS_GET_REQUEST: {
        // Request message
        LOG(TRACE) << "ACTION_PLATFORM_ADMIN_CREDENTIALS_GET_REQUEST";

        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_PLATFORM_ADMIN_CREDENTIALS_GET_RESPONSE>(m_cmdu_tx);

        if (response == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        memset(response->params().user_password, 0, message::USER_PASS_LEN);

        char pass[BPL_USER_PASS_LEN];

        if (bpl::cfg_get_administrator_credentials(pass) < 0) {
            LOG(ERROR) << "Failed reading administrator credentials!";
            response->result() = 0;
        } else {
            response->result() = 1;
        }

        string_utils::copy_string(response->params().user_password, pass, message::USER_PASS_LEN);

        // Sent with unsafe because BML is reachable only on platform thread
        send_cmdu(fd, m_cmdu_tx);

        // clear the pwd in the memory
        memset(&pass, 0, sizeof(pass));

        // deepcode ignore CopyPasteError: <memset might be optimized and compiler might not set
        // it 0 if its not used after memset>
        *(volatile char *)pass = *(volatile char *)pass;

    } break;
    case beerocks_message::ACTION_PLATFORM_GET_MASTER_SLAVE_VERSIONS_REQUEST: {
        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_PLATFORM_GET_MASTER_SLAVE_VERSIONS_RESPONSE>(m_cmdu_tx);
        if (response == nullptr) {
            LOG(ERROR) << "addClass cACTION_PLATFORM_GET_MASTER_SLAVE_VERSIONS_RESPONSE failed";
            return false;
        }
        if (!master_version.empty() && !slave_version.empty()) {
            string_utils::copy_string(response->versions().master_version, master_version.c_str(),
                                      message::VERSION_LENGTH);
            string_utils::copy_string(response->versions().slave_version, slave_version.c_str(),
                                      message::VERSION_LENGTH);
            response->result() = 1;
        } else {
            response->result() = 0;
        }

        send_cmdu(fd, m_cmdu_tx);

    } break;
    case beerocks_message::ACTION_PLATFORM_MASTER_SLAVE_VERSIONS_NOTIFICATION: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_PLATFORM_MASTER_SLAVE_VERSIONS_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass ACTION_PLATFORM_MASTER_SLAVE_VERSIONS_NOTIFICATION failed";
            return false;
        }
        master_version.assign(notification->versions().master_version);
        slave_version.assign(notification->versions().slave_version);
    } break;

    case beerocks_message::ACTION_PLATFORM_SON_SLAVE_BACKHAUL_CONNECTION_COMPLETE_NOTIFICATION: {
        LOG(DEBUG) << "ACTION_PLATFORM_SON_SLAVE_BACKHAUL_CONNECTION_COMPLETE_NOTIFICATION";
        auto notification = beerocks_header->addClass<
            beerocks_message::
                cACTION_PLATFORM_SON_SLAVE_BACKHAUL_CONNECTION_COMPLETE_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass "
                          "cACTION_PLATFORM_SON_SLAVE_BACKHAUL_CONNECTION_COMPLETE_NOTIFICATION "
                          "failed";
            return false;
        }
        // Start ARP monitor
        if (enable_arp_monitor) {
            if (!init_arp_monitor()) {
                LOG(ERROR) << "can't start ARP monitor";
                return false;
            }
        }

        // Start DHCP monitor
        if (AgentDB::get()->device_conf.local_gw) {
            if (!init_dhcp_monitor()) {
                LOG(ERROR) << "can't start DHCP monitor";
                return false;
            }
        }
    } break;

    case beerocks_message::ACTION_PLATFORM_ONBOARD_QUERY_REQUEST: {
        LOG(TRACE) << "ACTION_PLATFORM_ONBOARD_QUERY_REQUEST";
        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_PLATFORM_ONBOARD_QUERY_RESPONSE>(m_cmdu_tx);
        if (response == nullptr) {
            LOG(ERROR) << "Failed building ONBOARD RESPONSE message!";
            return false;
        }
        response->params().onboarding = bpl::cfg_is_onboarding();

        // Sent with unsafe because BML is reachable only on platform thread
        send_cmdu(fd, m_cmdu_tx);

    } break;

    case beerocks_message::ACTION_PLATFORM_LOCAL_MASTER_GET_REQUEST: {
        LOG(TRACE) << "ACTION_PLATFORM_LOCAL_MASTER_GET_REQUEST";
        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_PLATFORM_LOCAL_MASTER_GET_RESPONSE>(m_cmdu_tx);
        if (response == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        // Read the value from CAL
        response->local_master() = bpl::cfg_is_master();

        // Sent with unsafe because BML is reachable only on platform thread
        send_cmdu(fd, m_cmdu_tx);

    } break;

    case beerocks_message::ACTION_PLATFORM_WIFI_CREDENTIALS_GET_REQUEST: {
        // Request message
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_PLATFORM_WIFI_CREDENTIALS_GET_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_PLATFORM_WIFI_CREDENTIALS_GET_REQUEST failed";
            break;
        }

        // TODO use vap_id, for now assume vap_id == MAIN_VAP
        LOG(TRACE) << "ACTION_PLATFORM_WIFI_CREDENTIALS_GET_REQUEST, vap_id="
                   << int(request->vap_id());

        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_PLATFORM_WIFI_CREDENTIALS_GET_RESPONSE>(m_cmdu_tx);

        if (response == nullptr) {
            LOG(ERROR) << "Failed building message!";
            break;
        }

        auto fill_credentials_msg = [](const int radio_dir, char *msg_ssid, char *msg_pass,
                                       uint8_t &msg_sec) -> bool {
            memset(msg_ssid, 0, message::WIFI_SSID_MAX_LENGTH);
            memset(msg_pass, 0, message::WIFI_PASS_MAX_LENGTH);

            char ssid[BPL_SSID_LEN];
            char pass[BPL_PASS_LEN];
            char sec[BPL_SEC_LEN];

            if (bpl::cfg_get_beerocks_credentials(radio_dir, ssid, pass, sec) < 0) {
                LOG(ERROR) << "Failed reading Wi-Fi credentials!";
                return false;
            } else {
                std::string sec_string(sec);
                if (sec_string == BPL_WLAN_SEC_NONE_STR) {
                    msg_sec = beerocks_message::eWiFiSec_None;
                } else if (sec_string == BPL_WLAN_SEC_WEP64_STR) {
                    msg_sec = beerocks_message::eWiFiSec_WEP64;
                } else if (sec_string == BPL_WLAN_SEC_WEP128_STR) {
                    msg_sec = beerocks_message::eWiFiSec_WEP128;
                } else if (sec_string == BPL_WLAN_SEC_WPA_PSK_STR) {
                    msg_sec = beerocks_message::eWiFiSec_WPA_PSK;
                } else if (sec_string == BPL_WLAN_SEC_WPA2_PSK_STR) {
                    msg_sec = beerocks_message::eWiFiSec_WPA2_PSK;
                } else if (sec_string == BPL_WLAN_SEC_WPA_WPA2_PSK_STR) {
                    msg_sec = beerocks_message::eWiFiSec_WPA_WPA2_PSK;
                } else {
                    msg_sec = beerocks_message::eWiFiSec_None;
                    LOG(WARNING) << "Unsupported Wi-Fi Security: " << sec_string;
                    return (false);
                }
            }

            string_utils::copy_string(msg_ssid, ssid, message::WIFI_SSID_MAX_LENGTH);
            string_utils::copy_string(msg_pass, pass, message::WIFI_PASS_MAX_LENGTH);

            // clear the pwd in the memory
            memset(&pass, 0, sizeof(pass));

            // deepcode ignore CopyPasteError: <memset might be optimized and compiler might not
            // set it 0 if its not used after memset>
            *(volatile char *)pass = *(volatile char *)pass;

            return true;
        };

        response->result() =
            fill_credentials_msg(BPL_RADIO_FRONT, response->front_params().ssid,
                                 response->front_params().pass, response->front_params().sec);
        response->result() =
            fill_credentials_msg(BPL_RADIO_BACK, response->front_params().ssid,
                                 response->front_params().pass, response->front_params().sec);

        LOG(INFO) << "fSSID: " << response->front_params().ssid
                  << " fPASS: " << response->front_params().pass
                  << " fSEC: " << response->front_params().sec;
        LOG(INFO) << "bSSID: " << response->front_params().ssid
                  << " bPASS: " << response->front_params().pass
                  << " bSEC: " << response->front_params().sec;

        // Sent with unsafe because BML is reachable only on platform thread
        send_cmdu(fd, m_cmdu_tx);

    } break;

    case beerocks_message::ACTION_PLATFORM_ONBOARD_SET_REQUEST: {
        LOG(TRACE) << "ACTION_PLATFORM_ONBOARD_SET_REQUEST";
        // Request message
        auto request =
            beerocks_header->addClass<beerocks_message::cACTION_PLATFORM_ONBOARD_SET_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_PLATFORM_ONBOARD_SET_REQUEST failed";
            break;
        }

        LOG(INFO) << "Set onboarding to " << std::to_string(request->params().onboarding);
        bpl::cfg_set_onboarding(int(request->params().onboarding));

        LOG(INFO) << "Success onboarding " << std::to_string(bpl::cfg_is_onboarding());
        // No response message is needed
    } break;

    case beerocks_message::ACTION_PLATFORM_WPS_ONBOARDING_REQUEST: {
        LOG(TRACE) << "ACTION_PLATFORM_WPS_ONBOARDING_REQUEST";

        auto request =
            beerocks_header->addClass<beerocks_message::cACTION_PLATFORM_WPS_ONBOARDING_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass ACTION_PLATFORM_WPS_ONBOARDING_REQUEST failed";
            break;
        }

        std::string iface = request->iface_name(message::IFACE_NAME_LENGTH);

        work_queue.enqueue<void>(
            [this](std::string iface) {
                bpl::cfg_notify_onboarding_completed("SSID", "PASSWORD", "SECURITY", iface.c_str(),
                                                     0);
            },
            iface);
    } break;

    case beerocks_message::ACTION_PLATFORM_ERROR_NOTIFICATION: {
        auto error =
            beerocks_header->addClass<beerocks_message::cACTION_PLATFORM_ERROR_NOTIFICATION>();
        if (error == nullptr) {
            LOG(ERROR) << "addClass failed";
            break;
        }

        uint32_t error_code = error->code();
        std::string error_data(error->data(0));

        // Notify the SL asynchronously
        work_queue.enqueue<void>([error_code, error_data]() {
            LOG(DEBUG) << "PLATFORM ERROR NOTIFICATION - Code: " << error_code
                       << ", Data: " << error_data;

            bpl::cfg_notify_error(error_code, error_data.c_str());
        });

    } break;
    default: {
        LOG(ERROR) << "Unknown PLATFORM_MANAGER message, action_op: "
                   << int(beerocks_header->action_op());
        return (false);
    }
    }

    return true;
}

bool PlatformManager::handle_arp_monitor()
{
    auto arp_notif =
        message_com::create_vs_message<beerocks_message::cACTION_PLATFORM_ARP_MONITOR_NOTIFICATION>(
            m_cmdu_tx);
    if (arp_notif == nullptr) {
        LOG(ERROR) << "Failed building message!";
        return false;
    }

    // Process the message
    bpl::BPL_ARP_MON_ENTRY entry;
    if (bpl::arp_mon_process(m_ctxArpMon, &entry) != 0) {
        LOG(ERROR) << "Failed processing ARP monitor message!";
        return (false);
    }

    // Ignore IPs outside the monitored network, zeroed MACs or invalid state
    if (((beerocks::net::network_utils::uint_ipv4_from_array(entry.ip) & m_uiArpMonMask) !=
         (m_uiArpMonIP & m_uiArpMonMask)) ||
        (!memcmp(entry.mac, s_arrZeroMac, sizeof(entry.mac))) ||
        (!memcmp(entry.mac, s_arrBCastMac, sizeof(entry.mac))) ||
        (entry.state == beerocks::ARP_NUD_FAILED)) {

        // LOG(DEBUG) << "Ignoring ARP from: "
        //            << beerocks::net::network_utils::ipv4_to_string(entry.ip) << " ("
        //            << entry.mac << ")"
        //            << ", state: " << int(entry.state)
        //            << ", type: " << int(entry.type);

        return true;
    }

    // Copy entry values
    tlvf::mac_from_array(entry.mac, arp_notif->params().mac);
    std::copy_n(entry.ip, sizeof(beerocks::net::sIpv4Addr::oct), arp_notif->params().ipv4.oct);
    arp_notif->params().iface_idx = entry.iface_idx;
    arp_notif->params().state     = entry.state;
    arp_notif->params().source    = entry.source;
    arp_notif->params().type      = entry.type;

    // After processing the message, copy the ipv4 and mac as strings
    std::string client_ipv4 =
        beerocks::net::network_utils::ipv4_to_string(arp_notif->params().ipv4);
    std::string client_mac = tlvf::mac_to_string(arp_notif->params().mac);

    auto iIfaceIndex = arp_notif->params().iface_idx;

    auto iface_name = beerocks::net::network_utils::linux_get_iface_name(iIfaceIndex);

    if (iface_name.empty()) {
        LOG(ERROR) << "Failed to find iface of iface_index" << int(iIfaceIndex);
        return false;
    }

    auto it_iface = m_mapIfaces.find(iface_name);
    if (it_iface != m_mapIfaces.end()) {
        auto &pIfaceParams         = it_iface->second;
        arp_notif->params().source = pIfaceParams.eType;

    } else if (entry.type != ARP_TYPE_DELNEIGH) {

        std::string mac  = tlvf::mac_to_string(arp_notif->params().mac);
        std::string ipv4 = beerocks::net::network_utils::ipv4_to_string(arp_notif->params().ipv4);
        LOG(WARNING) << "Interface index " << int(iIfaceIndex) << " not found! mac=" << mac
                     << ", ipv4=" << ipv4;
        return (false);
    }

    std::string source = ((arp_notif->params().source == beerocks::ARP_SRC_ETH_BACK) ||
                          (arp_notif->params().source == beerocks::ARP_SRC_WIRELESS_BACK))
                             ? "BACK"
                             : "FRONT";

    // Send the message to the Slave
    LOG(INFO) << "ARP - Interface: " << iface_name << ", State: " << int(arp_notif->params().state)
              << ", IP: " << client_ipv4 << " (" << client_mac << ")"
              << ", Source: " << source << ", Type: " << int(arp_notif->params().type);

    // Check if the master should be notified
    bool fSendNotif = true;
    if (entry.type != ARP_TYPE_DELNEIGH) {
        auto pArpEntry = m_mapArpEntries.find(arp_notif->params().mac);

        if (pArpEntry != m_mapArpEntries.end()) {

            auto now                = std::chrono::steady_clock::now();
            auto last_seen_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                                          now - pArpEntry->second->last_seen)
                                          .count();

            // Check for IP/Inteface changes
            if ((pArpEntry->second->ip != beerocks::net::network_utils::uint_ipv4_from_array(
                                              arp_notif->params().ipv4.oct)) ||
                (pArpEntry->second->iface_index != int(arp_notif->params().iface_idx)) ||
                (last_seen_duration >= ARP_NOTIF_INTERVAL)) {

                // Update the entry
                pArpEntry->second->ip = beerocks::net::network_utils::uint_ipv4_from_array(
                    arp_notif->params().ipv4.oct);
                pArpEntry->second->iface_index = arp_notif->params().iface_idx;
                pArpEntry->second->last_seen   = now;

                LOG(DEBUG) << "Client last seen " << last_seen_duration
                           << " milliseconds ago. Sending notification!";

            } else {
                // Do NOT send notification
                fSendNotif = false;

                LOG(DEBUG) << "Client last seen " << last_seen_duration
                           << " milliseconds ago. Skipping notification...";
            }
        }
    }

    // Send the message to the slave
    if (fSendNotif) {
        send_cmdu_to_agent_safe(m_cmdu_tx);
    }

    return (true);
}

bool PlatformManager::handle_arp_raw()
{
    // Skip invalid ARP packets
    bpl::BPL_ARP_MON_ENTRY entry;
    int task_id = bpl::arp_mon_process_raw_arp(m_ctxArpMon, &entry);
    if (task_id == 0) {
        // task-id equals to 0  means nodes list is empty
        // not an error, but no further work is required
        return true;
    } else if (task_id < 0) {
        return false;
    }

    auto arp_resp =
        message_com::create_vs_message<beerocks_message::cACTION_PLATFORM_ARP_QUERY_RESPONSE>(
            m_cmdu_tx, task_id);

    if (arp_resp == nullptr) {
        LOG(ERROR) << "Failed building cACTION_PLATFORM_ARP_QUERY_RESPONSE message!";
        return false;
    }

    // Copy entry values
    tlvf::mac_from_array(entry.mac, arp_resp->params().mac);
    std::copy_n(entry.ip, sizeof(arp_resp->params().ipv4.oct), arp_resp->params().ipv4.oct);
    arp_resp->params().iface_idx = entry.iface_idx;
    arp_resp->params().state     = entry.state;
    arp_resp->params().source    = entry.source;

    std::string strIface = bridge_iface_from_mac(arp_resp->params().mac);

    if (strIface.empty())
        return (false);

    // Find the interface index and source
    SIfaceParams *pIfaceParams = nullptr;
    auto it                    = m_mapIfaces.find(strIface);
    if (it != m_mapIfaces.end()) {
        pIfaceParams = &it->second;
    }

    int iIfaceIndex = if_nametoindex(strIface.c_str());

    if (iIfaceIndex == 0) {
        LOG(WARNING) << "Failed reading interface index for (" << strIface
                     << "): " << strerror(errno);

        return false;
    }

    // Update ARP response parameters
    arp_resp->params().iface_idx = (pIfaceParams) ? iIfaceIndex : 0;
    arp_resp->params().source =
        (pIfaceParams) ? uint8_t(pIfaceParams->eType) : uint8_t(beerocks::ARP_SRC_ETH_FRONT);

    std::string strSource =
        (pIfaceParams && ((pIfaceParams->eType == beerocks::ARP_SRC_ETH_BACK) ||
                          (pIfaceParams->eType == beerocks::ARP_SRC_WIRELESS_BACK)))
            ? "BACK"
            : "FRONT";

    LOG(DEBUG) << "Discovered IP: "
               << beerocks::net::network_utils::ipv4_to_string(arp_resp->params().ipv4) << " ("
               << arp_resp->params().mac << ") on '" << strIface << "' (" << strSource << ")";

    // Update ARP entry parameters
    auto pArpEntry = m_mapArpEntries.find(arp_resp->params().mac);

    if (pArpEntry != m_mapArpEntries.end()) {
        pArpEntry->second->ip =
            beerocks::net::network_utils::uint_ipv4_from_array(arp_resp->params().ipv4.oct);
        pArpEntry->second->iface_index = arp_resp->params().iface_idx;
        pArpEntry->second->last_seen   = std::chrono::steady_clock::now();
    } else {
        // This should not happen since the client is added to the list on query request...
        LOG(WARNING) << "MAC " << arp_resp->params().mac
                     << " was NOT found in the ARP entries list...";
    }
    LOG(TRACE) << "ACTION_PLATFORM_ARP_QUERY_RESPONSE mac=" << arp_resp->params().mac
               << " task_id=" << task_id;
    send_cmdu_to_agent_safe(m_cmdu_tx);

    return (true);
}

void PlatformManager::clean_old_arp_entries()
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_mapArpEntries.begin(); it != m_mapArpEntries.end();) {

        auto last_seen_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second->last_seen)
                .count();

        // If the client wasn't seen --> erase it
        if (last_seen_duration >= ARP_NOTIF_INTERVAL) {
            LOG(INFO) << "Removing client with MAC " << it->first
                      << " due to inactivity for more than " << last_seen_duration
                      << " milliseconds.";

            it = m_mapArpEntries.erase(it);
        } else {
            it++;
        }
    }
}

bool PlatformManager::init_arp_monitor()
{
    if (!m_ctxArpMon) {
        LOG(DEBUG) << "Starting ARP Monitor...";

        // Read IP/Netmask of the monitored interface
        beerocks::net::network_utils::raw_iface_info info;
        if (!beerocks::net::network_utils::get_raw_iface_info(config.bridge_iface, info)) {
            LOG(ERROR) << "Failed reading '" + config.bridge_iface + "' information";
            return false;
        }

        int ret = bpl::arp_mon_start(&m_ctxArpMon, config.bridge_iface.c_str());
        if (ret < 0) {
            // If arp_monitor failed to start, continue without it. It might failed due to
            // insufficient permissions. Detailed error message is printed inside.
            if (ret == -int(bpl::eErrorCode::OPERATION_NOT_SUPPORTED)) {
                LOG(INFO) << "Skip starting ARP monitor (not supported)";
                return (true);
            }
            LOG(ERROR) << "Failed starting ARP monitor!";
            return (false);
        }

        m_uiArpMonIP   = info.ipa.s_addr;
        m_uiArpMonMask = info.nmask.s_addr;

        // TODO: PPM-639 Inject ARP monitor to platform manager
        if (!m_arp_raw_socket_connection) {
            m_arp_raw_socket_connection = std::make_unique<beerocks::net::SocketConnectionImpl>(
                std::make_shared<beerocks::net::ConnectedSocket>(
                    bpl::arp_mon_get_raw_arp_fd(m_ctxArpMon)));
            if (!m_arp_raw_socket_connection) {
                LOG(ERROR) << "Unable to create ARP raw connection!";
                stop_arp_monitor();
                return false;
            }

            beerocks::EventLoop::EventHandlers handlers;
            // Handler name
            handlers.name = "arp_raw";

            // Handle event
            handlers.on_read = [&](int fd, EventLoop &loop) {
                if (!handle_arp_raw()) {
                    LOG(ERROR) << "handle_arp_raw failed, restarting ARP monitor";
                    if (!restart_arp_monitor()) {
                        LOG(ERROR) << "failed to restart ARP monitor";
                    }
                }
                return true;
            };

            // Remove the socket on disconnection or error
            handlers.on_disconnect = [&](int fd, EventLoop &loop) {
                m_arp_raw_socket_connection.reset();
                stop_arp_monitor();
                return true;
            };
            handlers.on_error = handlers.on_disconnect;

            if (!m_event_loop->register_handlers(m_arp_raw_socket_connection->socket()->fd(),
                                                 handlers)) {
                LOG(ERROR) << "Unable to register handlers for ARP raw connection!";
                stop_arp_monitor();
                return false;
            }
        }

        if (!m_arp_mon_socket_connection) {
            m_arp_mon_socket_connection = std::make_unique<beerocks::net::SocketConnectionImpl>(
                std::make_shared<beerocks::net::ConnectedSocket>(bpl::arp_mon_get_fd(m_ctxArpMon)));
            if (!m_arp_mon_socket_connection) {
                LOG(ERROR) << "Unable to create ARP monitor connection!";
                stop_arp_monitor();
                return false;
            }

            beerocks::EventLoop::EventHandlers handlers;
            // Handler name
            handlers.name = "arp_monitor";

            // Handle event
            handlers.on_read = [&](int fd, EventLoop &loop) {
                if (!handle_arp_monitor()) {
                    LOG(ERROR) << "handle_arp_monitor failed, restarting ARP monitor";
                    if (!restart_arp_monitor()) {
                        LOG(ERROR) << "failed to restart ARP monitor";
                    }
                }
                return true;
            };

            // Remove the socket on disconnection or error
            handlers.on_disconnect = [&](int fd, EventLoop &loop) {
                m_arp_mon_socket_connection.reset();
                stop_arp_monitor();
                return true;
            };
            handlers.on_error = handlers.on_disconnect;

            if (!m_event_loop->register_handlers(m_arp_mon_socket_connection->socket()->fd(),
                                                 handlers)) {
                LOG(ERROR) << "Unable to register handlers for ARP monitor connection!";
                stop_arp_monitor();
                return false;
            }
        }

        LOG(DEBUG) << "ARP Monitor started on interface '" << config.bridge_iface << "' ("
                   << beerocks::net::network_utils::ipv4_to_string(m_uiArpMonIP) << "/"
                   << beerocks::net::network_utils::ipv4_to_string(m_uiArpMonMask) << ")";

        // Initialize the ARP entries cleanup timestamp
        m_tpArpEntriesCleanup = std::chrono::steady_clock::now();

        // Create a timer to periodically clean old ARP table entries
        m_clean_old_arp_entries_timer = m_timer_manager->add_timer(
            "Clean Old ARP Entries", clean_old_arp_entries_timer_interval,
            clean_old_arp_entries_timer_interval, [&](int fd, beerocks::EventLoop &loop) {
                clean_old_arp_entries();
                return true;
            });
        if (m_clean_old_arp_entries_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
            LOG(ERROR) << "Failed to create the clean-old-ARP-entries timer";
            stop_arp_monitor();
            return false;
        }
    }

    return true;
}

void PlatformManager::stop_arp_monitor()
{
    if (m_ctxArpMon) {
        bpl::arp_mon_stop(m_ctxArpMon);
        LOG(DEBUG) << "ARP Monitor Stopped.";
        m_ctxArpMon = nullptr;
    }

    if (m_arp_raw_socket_connection) {
        // Remove installed event handlers for the connected socket
        m_event_loop->remove_handlers(m_arp_raw_socket_connection->socket()->fd());

        m_arp_raw_socket_connection.reset();
    }

    if (m_arp_mon_socket_connection) {
        // Remove installed event handlers for the connected socket
        m_event_loop->remove_handlers(m_arp_mon_socket_connection->socket()->fd());

        m_arp_mon_socket_connection.reset();
    }

    // Cancel and remove the clean-old-ARP-entries timer
    if (m_clean_old_arp_entries_timer != beerocks::net::FileDescriptor::invalid_descriptor) {
        m_timer_manager->remove_timer(m_clean_old_arp_entries_timer);
    }
}

bool PlatformManager::restart_arp_monitor()
{
    stop_arp_monitor();

    if (!init_arp_monitor()) {
        LOG(ERROR) << "can't start ARP monitor";
        return false;
    }

    return true;
}

bool PlatformManager::init_dhcp_monitor()
{
    static auto dhcp_monitor_cb_wrapper = [&](const std::string &op, const std::string &mac,
                                              const std::string &ip, const std::string &hostname) {
        send_dhcp_notification(op, mac, ip, hostname);
    };

    // TODO: PPM-639 Inject DHCP monitor to platform manager
    if (!m_dhcp_mon_socket_connection) {
        int dhcp_mon_fd = bpl::dhcp_mon_start(
            [](const char *op, const char *mac, const char *ip, const char *hostname) {
                dhcp_monitor_cb_wrapper(op, mac, ip, hostname);
            });
        if (dhcp_mon_fd < 0) {
            if (dhcp_mon_fd == -int(bpl::eErrorCode::OPERATION_NOT_SUPPORTED)) {
                LOG(INFO) << "Skip starting DHCP monitor (not supported)";
                return (true);
            }
            LOG(ERROR) << "Failed starting DHCP monitor: " << dhcp_mon_fd;
            return (false);
        }

        m_dhcp_mon_socket_connection = std::make_unique<beerocks::net::SocketConnectionImpl>(
            std::make_shared<beerocks::net::ConnectedSocket>(dhcp_mon_fd));
        if (!m_dhcp_mon_socket_connection) {
            LOG(ERROR) << "Unable to create DHCP monitor connection!";
            stop_dhcp_monitor();
            return false;
        }

        beerocks::EventLoop::EventHandlers handlers;
        // Handler name
        handlers.name = "dhcp_monitor";

        // Handle event
        handlers.on_read = [&](int fd, EventLoop &loop) {
            bpl::dhcp_mon_handle_event();
            return true;
        };

        handlers.on_disconnect = [&](int fd, EventLoop &loop) {
            m_dhcp_mon_socket_connection.reset();
            stop_dhcp_monitor();
            return true;
        };
        handlers.on_error = handlers.on_disconnect;

        if (!m_event_loop->register_handlers(m_dhcp_mon_socket_connection->socket()->fd(),
                                             handlers)) {
            LOG(ERROR) << "Unable to register handlers for DHCP monitor connection !";
            stop_dhcp_monitor();
            return false;
        }

        LOG(DEBUG) << "DHCP Monitor started, fd = " << dhcp_mon_fd;
    }

    return true;
}

void PlatformManager::stop_dhcp_monitor()
{
    if (bpl::dhcp_mon_stop() != 0) {
        LOG(ERROR) << "Failed stopping DHCP Monitor!";
    } else {
        LOG(DEBUG) << "DHCP Monitor Stopped.";
    }

    if (m_dhcp_mon_socket_connection) {
        // Remove installed event handlers for the connected socket
        m_event_loop->remove_handlers(m_dhcp_mon_socket_connection->socket()->fd());

        m_dhcp_mon_socket_connection.reset();
    }
}

} // namespace beerocks
