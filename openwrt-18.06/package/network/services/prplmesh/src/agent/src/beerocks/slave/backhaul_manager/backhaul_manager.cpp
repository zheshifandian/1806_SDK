/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "backhaul_manager.h"
#include "../agent_db.h"

#include "../tasks/capability_reporting_task.h"
#include "../tasks/channel_scan_task.h"
#include "../tasks/channel_selection_task.h"
#include "../tasks/coordinated_cac_task.h"
#include "../tasks/link_metrics_collection_task.h"
#include "../tasks/switch_channel_task.h"
#include "../tasks/topology_task.h"
#include <bcl/beerocks_cmdu_client_factory_factory.h>
#include <bcl/beerocks_cmdu_server_factory.h>
#include <bcl/beerocks_timer_factory_impl.h>
#include <bcl/beerocks_timer_manager_impl.h>
#include <bcl/beerocks_ucc_server_factory.h>
#include <bcl/beerocks_utils.h>
#include <bcl/son/son_wireless_utils.h>
#include <bcl/transaction.h>
#include <btl/broker_client_factory_factory.h>
#include <easylogging++.h>

#include <beerocks/tlvf/beerocks_message.h>
#include <beerocks/tlvf/beerocks_message_backhaul.h>
#include <beerocks/tlvf/beerocks_message_control.h>
#include <beerocks/tlvf/beerocks_message_platform.h>

#include <tlvf/wfa_map/tlvAssociatedStaExtendedLinkMetrics.h>
#include <tlvf/wfa_map/tlvAssociatedStaLinkMetrics.h>
#include <tlvf/wfa_map/tlvBackhaulSteeringRequest.h>
#include <tlvf/wfa_map/tlvBackhaulSteeringResponse.h>
#include <tlvf/wfa_map/tlvErrorCode.h>
#include <tlvf/wfa_map/tlvProfile2AssociationStatusNotification.h>

// BPL Error Codes
#include <bpl/bpl_cfg.h>
#include <bpl/bpl_err.h>

#include <net/if.h> // if_nametoindex

namespace beerocks {

/**
 * Time between successive timer executions of the tasks timer
 */
// Reduced the tasks_timer_period to 90 from 500 milliseconds to fix
// the issue mentioned in PPM-1939.
// Optimizing the task wakeup time will be handled as part of PPM-1955.
constexpr auto tasks_timer_period = std::chrono::milliseconds(90);

/**
 * Time between successive timer executions of the FSM timer
 */
constexpr auto fsm_timer_period = std::chrono::milliseconds(500);

/**
 * Timeout to process a Backhaul Steering Request message.
 */
constexpr auto backhaul_steering_timeout = std::chrono::milliseconds(10000);

/**
 * Timeout to process a "dev_reset_default" WFA-CA command.
 */
constexpr auto dev_reset_default_timeout = std::chrono::seconds(UCC_REPLY_COMPLETE_TIMEOUT_SEC);

//////////////////////////////////////////////////////////////////////////////
////////////////////////// Local Module Definitions //////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define FSM_MOVE_STATE(eNewState)                                                                  \
    ({                                                                                             \
        LOG(TRACE) << "FSM: " << s_arrStates[int(m_eFSMState)] << " --> "                          \
                   << s_arrStates[int(EState::eNewState)];                                         \
        m_eFSMState = EState::eNewState;                                                           \
    })

#define FSM_IS_IN_STATE(eState) (m_eFSMState == EState::eState)
#define FSM_CURR_STATE_STR s_arrStates[int(m_eFSMState)]

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Static Members ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

const char *BackhaulManager::s_arrStates[] = {FOREACH_STATE(GENERATE_STRING)};

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Implementation ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

BackhaulManager::BackhaulManager(const config_file::sConfigSlave &config,
                                 const std::set<std::string> &slave_ap_ifaces_,
                                 const std::set<std::string> &slave_sta_ifaces_,
                                 int stop_on_failure_attempts_)
    : cmdu_tx(m_tx_buffer, sizeof(m_tx_buffer)),
      cert_cmdu_tx(m_cert_tx_buffer, sizeof(m_cert_tx_buffer)), slave_ap_ifaces(slave_ap_ifaces_),
      slave_sta_ifaces(slave_sta_ifaces_), m_beerocks_temp_path(config.temp_path),
      m_ucc_listener_port(string_utils::stoi(config.ucc_listener_port))
{
    configuration_stop_on_failure_attempts = stop_on_failure_attempts_;
    stop_on_failure_attempts               = stop_on_failure_attempts_;
    LOG(DEBUG) << "stop_on_failure_attempts=" << stop_on_failure_attempts;
    auto db                           = AgentDB::get();
    db->device_conf.ucc_listener_port = string_utils::stoi(config.ucc_listener_port);
    db->device_conf.vendor            = config.vendor;
    db->device_conf.model             = config.model;

    m_eFSMState = EState::INIT;

    std::string bridge_mac;
    if (!beerocks::net::network_utils::linux_iface_get_mac(config.bridge_iface, bridge_mac)) {
        LOG(ERROR) << "Failed getting MAC address for interface: " << config.bridge_iface;
    } else {
        db->dm_set_agent_mac(bridge_mac);
    }

    if (db->device_conf.management_mode == BPL_MGMT_MODE_MULTIAP_CONTROLLER) {

        // TODO: DHCP management is handled in ApAutoConfigurationTask for MaxLinear platforms (PPM-1777)
        LOG(INFO) << "Controller only mode is activated and agent is dummy. Do not run any tasks!";
        return;
    }

    // Agent tasks
    m_task_pool.add_task(std::make_shared<TopologyTask>(*this, cmdu_tx));
    m_task_pool.add_task(std::make_shared<ChannelSelectionTask>(*this, cmdu_tx));
    m_task_pool.add_task(std::make_shared<ChannelScanTask>(*this, cmdu_tx));
    m_task_pool.add_task(std::make_shared<CapabilityReportingTask>(*this, cmdu_tx));
    m_task_pool.add_task(std::make_shared<LinkMetricsCollectionTask>(*this, cmdu_tx));
    m_task_pool.add_task(
        std::make_shared<switch_channel::SwitchChannelTask>(m_task_pool, *this, cmdu_tx));
    m_task_pool.add_task(
        std::make_shared<coordinated_cac::CoordinatedCacTask>(m_task_pool, *this, cmdu_tx));
}

BackhaulManager::~BackhaulManager()
{
    if (m_cmdu_server) {
        m_cmdu_server->clear_handlers();
    }
}

bool BackhaulManager::thread_init()
{
    // Create UDS address where the server socket will listen for incoming connection requests.
    std::string backhaul_manager_server_uds_path =
        m_beerocks_temp_path + std::string(BEEROCKS_BACKHAUL_UDS);
    m_cmdu_server_uds_address =
        beerocks::net::UdsAddress::create_instance(backhaul_manager_server_uds_path);
    LOG_IF(!m_cmdu_server_uds_address, FATAL)
        << "Unable to create UDS server address for backhaul manager!";

    // Create server to exchange CMDU messages with clients connected through a UDS socket
    m_cmdu_server =
        beerocks::CmduServerFactory::create_instance(m_cmdu_server_uds_address, m_event_loop);
    LOG_IF(!m_cmdu_server, FATAL) << "Unable to create CMDU server for backhaul manager!";

    beerocks::CmduServer::EventHandlers cmdu_server_handlers{
        .on_client_connected    = nullptr,
        .on_client_disconnected = [&](int fd) { handle_disconnected(fd); },
        .on_cmdu_received =
            [&](int fd, uint32_t iface_index, const sMacAddr &dst_mac, const sMacAddr &src_mac,
                ieee1905_1::CmduMessageRx &cmdu_rx) {
                handle_cmdu(fd, iface_index, dst_mac, src_mac, cmdu_rx);
            },
    };
    m_cmdu_server->set_handlers(cmdu_server_handlers);

    // UCC server must be created if all the three following conditions are met:
    // - Device has been configured to work in certification mode
    // - A valid TCP port has been set
    // - The controller is not running in this device
    bool certification_mode = beerocks::bpl::cfg_get_certification_mode();
    bool local_controller   = beerocks::bpl::cfg_is_master();
    if (certification_mode && (m_ucc_listener_port != 0) && (!local_controller)) {

        LOG(INFO) << "Certification mode enabled (listening on port " << m_ucc_listener_port << ")";

        // Create server to exchange UCC commands and replies with clients connected through the
        // socket
        m_ucc_server =
            beerocks::UccServerFactory::create_instance(m_ucc_listener_port, m_event_loop);
        LOG_IF(!m_ucc_server, FATAL) << "Unable to create UCC server!";
    }

    // Create UDS address where the server socket will listen for incoming connection requests.
    std::string platform_manager_uds_path =
        m_beerocks_temp_path + std::string(BEEROCKS_PLATFORM_UDS);

    // Create CMDU client factory to create CMDU clients connected to CMDU server running in
    // platform manager when requested
    m_platform_manager_cmdu_client_factory =
        std::move(beerocks::create_cmdu_client_factory(platform_manager_uds_path, m_event_loop));
    LOG_IF(!m_platform_manager_cmdu_client_factory, FATAL)
        << "Unable to create CMDU client factory!";

    // Create broker client factory to create broker clients when requested
    std::string broker_uds_path = m_beerocks_temp_path + std::string(BEEROCKS_BROKER_UDS);
    m_broker_client_factory =
        beerocks::btl::create_broker_client_factory(broker_uds_path, m_event_loop);
    LOG_IF(!m_broker_client_factory, FATAL) << "Unable to create broker client factory!";

    // Create timer factory to create instances of timers.
    auto timer_factory = std::make_shared<beerocks::TimerFactoryImpl>();
    LOG_IF(!timer_factory, FATAL) << "Unable to create timer factory!";

    // Create timer manager to help using application timers.
    m_timer_manager = std::make_shared<beerocks::TimerManagerImpl>(timer_factory, m_event_loop);
    LOG_IF(!m_timer_manager, FATAL) << "Unable to create timer manager!";
    // In case of error in one of the steps of this method, we have to undo all the previous steps
    // (like when rolling back a database transaction, where either all steps get executed or none
    // of them gets executed)
    beerocks::Transaction transaction;

    // Create a timer to run internal tasks periodically
    m_tasks_timer = m_timer_manager->add_timer(
        "Agent Tasks", tasks_timer_period, tasks_timer_period,
        [&](int fd, beerocks::EventLoop &loop) {
            // Allow tasks to execute up to 80% of the timer period
            m_task_pool.run_tasks(int(double(tasks_timer_period.count()) * 0.8));
            return true;
        });
    if (m_tasks_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(ERROR) << "Failed to create the tasks timer";
        return false;
    }
    LOG(DEBUG) << "Tasks timer created with fd = " << m_tasks_timer;
    transaction.add_rollback_action([&]() { m_timer_manager->remove_timer(m_tasks_timer); });

    // Create a timer to run the FSM periodically
    m_fsm_timer =
        m_timer_manager->add_timer("Backhaul Manager FSM", fsm_timer_period, fsm_timer_period,
                                   [&](int fd, beerocks::EventLoop &loop) {
                                       bool continue_processing = false;
                                       do {
                                           if (!backhaul_fsm_main(continue_processing)) {
                                               return false;
                                           }
                                       } while (continue_processing);

                                       return true;
                                   });
    if (m_fsm_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(ERROR) << "Failed to create the FSM timer";
        return false;
    }
    LOG(DEBUG) << "FSM timer created with fd = " << m_fsm_timer;
    transaction.add_rollback_action([&]() { m_timer_manager->remove_timer(m_fsm_timer); });

    // Create an instance of a broker client connected to the broker server that is running in the
    // transport process
    m_broker_client = m_broker_client_factory->create_instance();
    if (!m_broker_client) {
        LOG(ERROR) << "Failed to create instance of broker client";
        return false;
    }
    transaction.add_rollback_action([&]() { m_broker_client.reset(); });

    beerocks::btl::BrokerClient::EventHandlers broker_client_handlers;
    // Install a CMDU-received event handler for CMDU messages received from the transport process.
    // These messages are actually been sent by a remote process and the broker server running in
    // the transport process just forwards them to the broker client.
    broker_client_handlers.on_cmdu_received = [&](uint32_t iface_index, const sMacAddr &dst_mac,
                                                  const sMacAddr &src_mac,
                                                  ieee1905_1::CmduMessageRx &cmdu_rx) {
        handle_cmdu_from_broker(iface_index, dst_mac, src_mac, cmdu_rx);
    };

    // Install a connection-closed event handler.
    // Currently there is no recovery mechanism if connection with broker server gets interrupted
    // (something that happens if the transport process dies). Just log a message and exit
    broker_client_handlers.on_connection_closed = [&]() {
        LOG(ERROR) << "Broker client got disconnected!";
        return false;
    };

    m_broker_client->set_handlers(broker_client_handlers);
    transaction.add_rollback_action([&]() { m_broker_client->clear_handlers(); });

    // Subscribe for the reception of CMDU messages that this process is interested in
    if (!m_broker_client->subscribe(std::set<ieee1905_1::eMessageType>{
            ieee1905_1::eMessageType::ACK_MESSAGE,
            ieee1905_1::eMessageType::AP_CAPABILITY_QUERY_MESSAGE,
            ieee1905_1::eMessageType::AP_METRICS_QUERY_MESSAGE,
            ieee1905_1::eMessageType::ASSOCIATED_STA_LINK_METRICS_QUERY_MESSAGE,
            ieee1905_1::eMessageType::BACKHAUL_STEERING_REQUEST_MESSAGE,
            ieee1905_1::eMessageType::BEACON_METRICS_QUERY_MESSAGE,
            ieee1905_1::eMessageType::CAC_REQUEST_MESSAGE,
            ieee1905_1::eMessageType::CAC_TERMINATION_MESSAGE,
            ieee1905_1::eMessageType::CHANNEL_PREFERENCE_QUERY_MESSAGE,
            ieee1905_1::eMessageType::CHANNEL_SCAN_REQUEST_MESSAGE,
            ieee1905_1::eMessageType::CHANNEL_SELECTION_REQUEST_MESSAGE,
            ieee1905_1::eMessageType::CLIENT_ASSOCIATION_CONTROL_REQUEST_MESSAGE,
            ieee1905_1::eMessageType::CLIENT_CAPABILITY_QUERY_MESSAGE,
            ieee1905_1::eMessageType::CLIENT_STEERING_REQUEST_MESSAGE,
            ieee1905_1::eMessageType::COMBINED_INFRASTRUCTURE_METRICS_MESSAGE,
            ieee1905_1::eMessageType::HIGHER_LAYER_DATA_MESSAGE,
            ieee1905_1::eMessageType::LINK_METRIC_QUERY_MESSAGE,
            ieee1905_1::eMessageType::TOPOLOGY_DISCOVERY_MESSAGE,
            ieee1905_1::eMessageType::TOPOLOGY_QUERY_MESSAGE,
            ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE,
        })) {
        LOG(ERROR) << "Failed subscribing to the Bus";
        return false;
    }

    transaction.commit();

    LOG(DEBUG) << "started";

    return true;
}

void BackhaulManager::on_thread_stop()
{
    if (m_agent_fd != beerocks::net::FileDescriptor::invalid_descriptor) {
        m_cmdu_server->disconnect(m_agent_fd);
    }

    while (m_radios_info.size() > 0) {
        auto radio_info = m_radios_info.back();
        if (radio_info) {
            LOG(DEBUG) << "Closing interface " << radio_info->sta_iface << " bwl";

            if (radio_info->sta_wlan_hal) {
                radio_info->sta_wlan_hal.reset();
            }
            if (radio_info->sta_hal_ext_events !=
                beerocks::net::FileDescriptor::invalid_descriptor) {
                m_event_loop->remove_handlers(radio_info->sta_hal_ext_events);
                radio_info->sta_hal_ext_events = beerocks::net::FileDescriptor::invalid_descriptor;
            }
            if (radio_info->sta_hal_int_events !=
                beerocks::net::FileDescriptor::invalid_descriptor) {
                m_event_loop->remove_handlers(radio_info->sta_hal_int_events);
                radio_info->sta_hal_int_events = beerocks::net::FileDescriptor::invalid_descriptor;
            }
        }
        m_radios_info.pop_back();
    }

    if (m_platform_manager_client) {
        m_platform_manager_client.reset();
    }

    if (m_broker_client) {
        m_broker_client->clear_handlers();
        m_broker_client.reset();
    }

    if (!m_timer_manager->remove_timer(m_fsm_timer)) {
        LOG(ERROR) << "Failed to remove fsm timer";
    }

    if (!m_timer_manager->remove_timer(m_tasks_timer)) {
        LOG(ERROR) << "Failed to remove tasks timer";
    }

    LOG(DEBUG) << "stopped";

    return;
}

bool BackhaulManager::send_cmdu(int fd, ieee1905_1::CmduMessageTx &cmdu_tx)
{
    return m_cmdu_server->send_cmdu(fd, cmdu_tx);
}

bool BackhaulManager::forward_cmdu_to_uds(int fd, uint32_t iface_index, const sMacAddr &dst_mac,
                                          const sMacAddr &src_mac,
                                          ieee1905_1::CmduMessageRx &cmdu_rx)
{
    return m_cmdu_server->forward_cmdu(fd, iface_index, dst_mac, src_mac, cmdu_rx);
}

bool BackhaulManager::send_cmdu_to_broker(ieee1905_1::CmduMessageTx &cmdu_tx,
                                          const sMacAddr &dst_mac, const sMacAddr &src_mac,
                                          const std::string &iface_name)
{
    if (!m_broker_client) {
        LOG(ERROR) << "Unable to send CMDU to broker server";
        return false;
    }

    uint32_t iface_index = 0;
    if (!iface_name.empty()) {
        iface_index = if_nametoindex(iface_name.c_str());
    }

    return m_broker_client->send_cmdu(cmdu_tx, dst_mac, src_mac, iface_index);
}

bool BackhaulManager::send_ack_to_controller(ieee1905_1::CmduMessageTx &cmdu_tx, uint32_t mid)
{
    // build ACK message CMDU
    auto cmdu_tx_header = cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "Failed to create ieee1905_1::eMessageType::ACK_MESSAGE";
        return false;
    }

    auto db = AgentDB::get();

    LOG(DEBUG) << "Sending ACK message to the controller, mid=" << std::hex << mid;
    bool ret = send_cmdu_to_broker(cmdu_tx, db->controller_info.bridge_mac, db->bridge.mac);
    return ret;
}

bool BackhaulManager::forward_cmdu_to_broker(ieee1905_1::CmduMessageRx &cmdu_rx,
                                             const sMacAddr &dst_mac, const sMacAddr &src_mac,
                                             const std::string &iface_name)
{
    if (!m_broker_client) {
        LOG(ERROR) << "Unable to forward CMDU to broker server";
        return false;
    }

    uint32_t iface_index = 0;
    if (!iface_name.empty()) {
        iface_index = if_nametoindex(iface_name.c_str());
    }

    return m_broker_client->forward_cmdu(cmdu_rx, dst_mac, src_mac, iface_index);
}

void BackhaulManager::handle_disconnected(int fd)
{
    if (m_agent_fd != fd) {
        LOG(INFO) << "Unkown socket disconnected, fd=" << fd;
        return;
    }

    LOG(INFO) << "Agent socket disconnected";

    auto db = AgentDB::get();

    for (auto radio_info : m_radios_info) {

        if (!(FSM_IS_IN_STATE(OPERATIONAL) || FSM_IS_IN_STATE(CONNECTED))) {
            if (radio_info->sta_wlan_hal) {
                LOG(INFO) << "dereferencing sta_wlan_hal";
                radio_info->sta_wlan_hal.reset();
            }
            if (radio_info->sta_hal_ext_events !=
                beerocks::net::FileDescriptor::invalid_descriptor) {
                m_event_loop->remove_handlers(radio_info->sta_hal_ext_events);
                radio_info->sta_hal_ext_events = beerocks::net::FileDescriptor::invalid_descriptor;
            }
            if (radio_info->sta_hal_int_events !=
                beerocks::net::FileDescriptor::invalid_descriptor) {
                m_event_loop->remove_handlers(radio_info->sta_hal_int_events);
                radio_info->sta_hal_int_events = beerocks::net::FileDescriptor::invalid_descriptor;
            }

            if (!m_agent_ucc_listener) {
                LOG(INFO) << "sending platform_notify: Agent disconnected";
                platform_notify_error(bpl::eErrorCode::BH_SLAVE_SOCKET_DISCONNECTED,
                                      "Agent socket disconnected");
            }
            if (m_eFSMState > EState::WAIT_ENABLE) {
                FSM_MOVE_STATE(RESTART);
            }
        }
    }
    return;
}

bool BackhaulManager::handle_cmdu(int fd, uint32_t iface_index, const sMacAddr &dst_mac,
                                  const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    // Check for local handling
    if (dst_mac == beerocks::net::network_utils::ZERO_MAC) {
        if (cmdu_rx.getMessageType() == ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE) {
            return handle_slave_backhaul_message(fd, cmdu_rx);
        } else {
            return handle_slave_1905_1_message(cmdu_rx, iface_index, dst_mac, src_mac);
        }
    }

    // Forward the data (cmdu) to bus
    // LOG(DEBUG) << "forwarding slave->master message, controller_bridge_mac="
    //            << (db->controller_info.bridge_mac);

    auto db = AgentDB::get();
    return forward_cmdu_to_broker(cmdu_rx, dst_mac, db->bridge.mac);
}

bool BackhaulManager::handle_cmdu_from_broker(uint32_t iface_index, const sMacAddr &dst_mac,
                                              const sMacAddr &src_mac,
                                              ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto db = AgentDB::get();

    // Filter messages which are not destined to this agent
    if (dst_mac != beerocks::net::network_utils::MULTICAST_1905_MAC_ADDR &&
        dst_mac != db->bridge.mac) {
        LOG(DEBUG) << "handle_cmdu() - dropping msg, dst_mac=" << dst_mac
                   << ", local_bridge_mac=" << db->bridge.mac;
        return true;
    }

    // TODO: Add optimization of PID filtering for cases like the following:
    // 1. If VS message was sent by Controllers local agent to the controller, it is looped back.
    // 2. If IRE is sending message to the Controller of the Controller, it will be received in
    //    Controllers backhaul manager as well, and should ignored.

    // Handle the CMDU message. If the message was processed locally
    // (by the Backhaul Manager), this function will return 'true'.
    // Otherwise, it should be forwarded to the slaves.

    // the destination slave is used to forward the cmdu
    // only to the desired slave.
    // handle_1905_1_message has the opportunity to set it
    // to a specific slave. In this case the cmdu is forward only
    // to this slave. when dest_slave is left as invalid_descriptor
    // the cmdu is forwarded to all slaves
    if (handle_1905_1_message(cmdu_rx, iface_index, dst_mac, src_mac)) {
        //function returns true if message doesn't need to be forwarded
        return true;
    }

    ////////// If got here, message needs to be forwarded //////////

    if (m_agent_fd == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(INFO) << "No slave sockets, cmdu will not be forwarded.";
        return true;
    }
    if (!forward_cmdu_to_uds(m_agent_fd, iface_index, dst_mac, src_mac, cmdu_rx)) {
        LOG(ERROR) << "forward_cmdu_to_uds() failed - fd=" << m_agent_fd;
    }

    return true;
}

void BackhaulManager::platform_notify_error(bpl::eErrorCode code, const std::string &error_data)
{
    if (!m_platform_manager_client) {
        LOG(ERROR) << "Not connected to Platform Manager!";
        return;
    }

    auto error =
        message_com::create_vs_message<beerocks_message::cACTION_PLATFORM_ERROR_NOTIFICATION>(
            cmdu_tx);

    if (error == nullptr) {
        LOG(ERROR) << "Failed building message!";
        return;
    }

    error->code() = uint32_t(code);

    string_utils::copy_string(error->data(0), error_data.c_str(),
                              message::PLATFORM_ERROR_DATA_SIZE);

    LOG(ERROR) << "platform_notify_error: " << error_data;

    // Send the message
    m_platform_manager_client->send_cmdu(cmdu_tx);
}

bool BackhaulManager::finalize_slaves_connect_state(bool fConnected)
{
    LOG(TRACE) << __func__ << ": fConnected=" << fConnected;
    // Backhaul Connected Notification
    if (fConnected) {

        // Build the notification message
        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CONNECTED_NOTIFICATION>(cmdu_tx);

        if (notification == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        auto db = AgentDB::get();

        beerocks::net::network_utils::iface_info iface_info;

        LOG_IF(!db->device_conf.local_controller && db->backhaul.selected_iface_name.empty(), FATAL)
            << "selected_iface_name is empty";

        // Update the backhaul BSSID on the AgentDB and detach from any sta_bwl not used for
        // by wireless connection:

        // Initialize backhaul bssid to empty in case in the connecten type is wired. If it is not,
        // will be overriden in the loop below.
        db->backhaul.backhaul_bssid = {};

        for (auto &radio_info : m_radios_info) {
            if (db->backhaul.connection_type == AgentDB::sBackhaul::eConnectionType::Wireless &&
                radio_info->sta_iface == db->backhaul.selected_iface_name) {
                LOG_IF(!radio_info->sta_wlan_hal, FATAL) << "selected sta_hal is nullptr";
                db->backhaul.backhaul_bssid =
                    tlvf::mac_from_string(radio_info->sta_wlan_hal->get_bssid());
            } else { // Detach from unused stations

                if (radio_info->sta_wlan_hal) {
                    radio_info->sta_wlan_hal.reset();
                }
                if (radio_info->sta_hal_ext_events !=
                    beerocks::net::FileDescriptor::invalid_descriptor) {
                    m_event_loop->remove_handlers(radio_info->sta_hal_ext_events);
                    radio_info->sta_hal_ext_events =
                        beerocks::net::FileDescriptor::invalid_descriptor;
                }
                if (radio_info->sta_hal_int_events !=
                    beerocks::net::FileDescriptor::invalid_descriptor) {
                    m_event_loop->remove_handlers(radio_info->sta_hal_int_events);
                    radio_info->sta_hal_int_events =
                        beerocks::net::FileDescriptor::invalid_descriptor;
                }
            }
        }

        // Send the message
        send_cmdu(m_agent_fd, cmdu_tx);

        // Backhaul Disconnected Notification
    } else {

        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION>(cmdu_tx);
        if (notification == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        notification->stopped() =
            (uint8_t)(configuration_stop_on_failure_attempts && !stop_on_failure_attempts);
        LOG(DEBUG) << "Sending DISCONNECTED notification";
        send_cmdu(m_agent_fd, cmdu_tx);
    }

    return true;
}

bool BackhaulManager::backhaul_fsm_main(bool &skip_select)
{
    skip_select = false;

    // Process internal FSMs before the main one, to prevent
    // falling into the "default" case...

    // UCC FSM. If UCC is in RESET, we have to stay in (or move to) ENABLED state.
    if (m_is_in_reset_state) {
        if (m_eFSMState == EState::ENABLED) {
            if (!m_dev_reset_default_completed) {
                // The "dev_reset_default" asynchronous command processing is complete.
                m_dev_reset_default_completed = true;

                // Tear down all VAPs in all radios to make sure no station can connect until APs
                // are given a fresh configuration.
                send_slaves_tear_down();

                if (m_dev_reset_default_timer !=
                    beerocks::net::FileDescriptor::invalid_descriptor) {
                    // Send back second reply to UCC client.
                    m_agent_ucc_listener->send_reply(m_dev_reset_default_fd);

                    // Cancel timer to check if a "dev_reset_default" command handling timed out.
                    m_timer_manager->remove_timer(m_dev_reset_default_timer);
                }
            }

            // Stay in ENABLE state until onboarding_state will change
            return true;
        } else if (m_eFSMState > EState::ENABLED) {
            FSM_MOVE_STATE(RESTART);
        }
    }

    // Wireless FSM
    if (m_eFSMState > EState::_WIRELESS_START_ && m_eFSMState < EState::_WIRELESS_END_) {
        return backhaul_fsm_wireless(skip_select);
    }

    switch (m_eFSMState) {
    // Initialize the module
    case EState::INIT: {
        state_time_stamp_timeout = std::chrono::steady_clock::now() +
                                   std::chrono::seconds(STATE_WAIT_ENABLE_TIMEOUT_SECONDS);
        auto db                             = AgentDB::get();
        db->backhaul.connection_type        = AgentDB::sBackhaul::eConnectionType::Invalid;
        db->backhaul.bssid_multi_ap_profile = 0;
        db->controller_info.bridge_mac      = beerocks::net::network_utils::ZERO_MAC;

        FSM_MOVE_STATE(WAIT_ENABLE);
        break;
    }
    // Wait for Enable command
    case EState::WAIT_ENABLE: {
        break;
    }
    // Received Backhaul Enable command
    case EState::ENABLED: {

        // Connect/Reconnect to the platform manager
        if (!m_platform_manager_client) {
            m_platform_manager_client = m_platform_manager_cmdu_client_factory->create_instance();

            if (m_platform_manager_client) {
                beerocks::CmduClient::EventHandlers handlers;
                handlers.on_connection_closed = [&]() {
                    LOG(ERROR) << "Client to Platform Manager disconnected, restarting "
                                  "Backhaul Manager";
                    m_platform_manager_client.reset();
                    FSM_MOVE_STATE(RESTART);
                    return true;
                };
                m_platform_manager_client->set_handlers(handlers);
            } else {
                LOG(ERROR) << "Failed connecting to Platform Manager!";
            }
        } else {
            LOG(DEBUG) << "Using existing client to Platform Manager";
        }

        auto db = AgentDB::get();

        // Ignore 'selected_backhaul' since this case is not covered by certification flows
        if (db->device_conf.local_controller && db->device_conf.local_gw) {
            LOG(DEBUG) << "local controller && local gw";
            FSM_MOVE_STATE(CONNECTED);
            db->backhaul.connection_type = AgentDB::sBackhaul::eConnectionType::Invalid;
            db->backhaul.selected_iface_name.clear();
            break;
        }

        // link establish
        auto ifaces =
            beerocks::net::network_utils::linux_get_iface_list_from_bridge(db->bridge.iface_name);

        // If a wired (WAN) interface was provided, try it first, check if the interface is UP
        wan_monitor::ELinkState wired_link_state = wan_monitor::ELinkState::eInvalid;
        if (!db->device_conf.local_gw && !db->ethernet.wan.iface_name.empty()) {
            wired_link_state = wan_mon.initialize(db->ethernet.wan.iface_name);
            // Failure might be due to insufficient permissions, datailed error message is being
            // printed inside.
            if (wired_link_state == wan_monitor::ELinkState::eInvalid) {
                LOG(WARNING) << "wan_mon.initialize() failed, skip wired link establishment";
            }
        }
        if ((wired_link_state == wan_monitor::ELinkState::eUp) &&
            (m_selected_backhaul.empty() || m_selected_backhaul == DEV_SET_ETH)) {

            auto it = std::find(ifaces.begin(), ifaces.end(), db->ethernet.wan.iface_name);
            if (it == ifaces.end()) {
                LOG(ERROR) << "wire iface " << db->ethernet.wan.iface_name
                           << " is not on the bridge";
                FSM_MOVE_STATE(RESTART);
                break;
            }

            // Mark the connection as WIRED
            db->backhaul.connection_type     = AgentDB::sBackhaul::eConnectionType::Wired;
            db->backhaul.selected_iface_name = db->ethernet.wan.iface_name;

        } else {
            // If no wired backhaul is configured, or it is down, we get into this else branch.

            // If selected backhaul is not empty, it's because we are in certification mode and
            // it was given with "dev_set_config".
            // If the RUID of the selected backhaul is null, then restart instead of continuing
            // with the preferred backhaul.
            if (!m_selected_backhaul.empty()) {
                auto selected_ruid = db->get_radio_by_mac(
                    tlvf::mac_from_string(m_selected_backhaul), AgentDB::eMacType::RADIO);

                if (!selected_ruid) {
                    LOG(ERROR) << "UCC configured backhaul RUID which is not enabled";
                    // Restart state will update the onboarding status to failure.
                    FSM_MOVE_STATE(RESTART);
                    break;
                }

                // Override backhaul_preferred_radio_band if UCC set it
                db->device_conf.back_radio.backhaul_preferred_radio_band = selected_ruid->freq_type;
            }

            // Mark the connection as WIRELESS
            db->backhaul.connection_type = AgentDB::sBackhaul::eConnectionType::Wireless;
        }

        // Move to the next state immediately
        if (db->backhaul.connection_type == AgentDB::sBackhaul::eConnectionType::Wireless) {
            FSM_MOVE_STATE(INIT_HAL);
        } else { // EType::Wired
            FSM_MOVE_STATE(CONNECTED);
        }

        skip_select = true;
        break;
    }
    // Backhaul Link exists
    case EState::CONNECTED: {
        auto db = AgentDB::get();

        bool wired_backhaul =
            db->backhaul.connection_type == AgentDB::sBackhaul::eConnectionType::Wired;

        // In certification mode we want to wait till dev_set_config is received (wired backhaul)
        // or start_wps_registration (wireless backhaul).
        if (db->device_conf.certification_mode && wired_backhaul &&
            !db->device_conf.local_controller) {
            if (m_is_in_reset_state && m_selected_backhaul.empty()) {
                break;
            }
        }

        finalize_slaves_connect_state(true);
        stop_on_failure_attempts = configuration_stop_on_failure_attempts;

        LOG(DEBUG) << "clearing blacklist";
        ap_blacklist.clear();

        /**
         * According to the 1905.1 specification section 8.2.1.1 - A 1905.1 management entity shall
         * transmit a topology discovery message every 60 seconds or if an "implementation-specific"
         * event occurs (e.g., device initialized or an interface is connected).
         * Sending "AGENT_DEVICE_INITIALIZED" event will trigger sending of topology discovery
         * message.
         */
        m_task_pool.send_event(eTaskType::TOPOLOGY, TopologyTask::eEvent::AGENT_DEVICE_INITIALIZED);

        // This snippet is commented out since the only place that use it, is also commented out.
        // An event-driven solution will be implemented as part of the task:
        // [TASK] Dynamic switching between wired and wireless
        // https://github.com/prplfoundation/prplMesh/issues/866
        // auto db = AgentDB::get();

        // if (!db->device_conf.local_gw()) {
        //     if (db->ethernet.wan.iface_name.empty()) {
        //         LOG(WARNING) << "WAN interface is empty on Repeater platform configuration!";
        //     }
        //     eth_link_poll_timer = std::chrono::steady_clock::now();
        //     m_eth_link_up       = beerocks::net::network_utils::linux_iface_is_up_and_running(
        //         db->ethernet.wan.iface_name);
        // }
        FSM_MOVE_STATE(OPERATIONAL);
        break;
    }
    // Backhaul manager is OPERATIONAL!
    case EState::OPERATIONAL: {
        /*
        * TODO
        * This code segment is commented out since wireless-backhaul is not yet supported and
        * the current implementation causes high CPU load on steady-state.
        * The high CPU load is due to a call to linux_iface_is_up_and_running() performed every
        * second to check if the wired interface changed its state. The implementation of the above
        * polls the interface flags using ioctl() which is very costly (~120 milliseconds).
        *
        * An event-driven solution will be implemented as part of the task:
        * [TASK] Dynamic switching between wired and wireless
        * https://github.com/prplfoundation/prplMesh/issues/866
        */
        // /**
        //  * Get current time. It is later used to compute elapsed time since some start time and
        //  * check if a timeout has expired to perform periodic actions.
        //  */
        // auto db = AgentDB::get();
        //
        // auto now = std::chrono::steady_clock::now();
        //
        // if (!db->device_conf.local_gw()) {
        //     if (db->ethernet.wan.iface_name.empty()) {
        //         LOG(WARNING) << "WAN interface is empty on Repeater platform configuration!";
        //     }
        // int time_elapsed_ms =
        //     std::chrono::duration_cast<std::chrono::milliseconds>(now - eth_link_poll_timer)
        //         .count();
        // //pooling eth link status every second to notice if there been a change.
        // if (time_elapsed_ms > POLL_TIMER_TIMEOUT_MS) {

        //     eth_link_poll_timer = now;
        //     bool eth_link_up = beerocks::net::network_utils::linux_iface_is_up_and_running(db->ethernet.wan.iface_name);
        //     if (eth_link_up != m_eth_link_up) {
        //         m_eth_link_up = beerocks::net::network_utils::linux_iface_is_up_and_running(db->ethernet.wan.iface_name);
        //         FSM_MOVE_STATE(RESTART);
        //     }
        // }
        // }
        break;
    }
    case EState::RESTART: {

        LOG(DEBUG) << "Restarting ...";

        auto db = AgentDB::get();

        for (auto &radio_info : m_radios_info) {
            auto radio = db->radio(radio_info->sta_iface);
            if (!radio) {
                continue;
            }
            // Clear the backhaul interface mac.
            radio->back.iface_mac = beerocks::net::network_utils::ZERO_MAC;

            if (radio_info->sta_wlan_hal) {
                radio_info->sta_wlan_hal.reset();
            }
            if (radio_info->sta_hal_ext_events !=
                beerocks::net::FileDescriptor::invalid_descriptor) {
                m_event_loop->remove_handlers(radio_info->sta_hal_ext_events);
                radio_info->sta_hal_ext_events = beerocks::net::FileDescriptor::invalid_descriptor;
            }
            if (radio_info->sta_hal_int_events !=
                beerocks::net::FileDescriptor::invalid_descriptor) {
                m_event_loop->remove_handlers(radio_info->sta_hal_int_events);
                radio_info->sta_hal_int_events = beerocks::net::FileDescriptor::invalid_descriptor;
            }
        }

        finalize_slaves_connect_state(false); //send disconnect to all connected slaves

        // wait again for enable from each slave before proceeding to attach
        pending_slave_sta_ifaces.clear();
        pending_slave_sta_ifaces = slave_sta_ifaces;

        if (configuration_stop_on_failure_attempts && !stop_on_failure_attempts) {
            LOG(ERROR) << "Reached to max stop on failure attempts!";
            platform_notify_error(bpl::eErrorCode::BH_STOPPED, "backhaul manager stopped");
            FSM_MOVE_STATE(STOPPED);
        } else {
            FSM_MOVE_STATE(INIT);
        }

        m_task_pool.send_event(eTaskType::LINK_METRICS_COLLECTION,
                               LinkMetricsCollectionTask::eEvent::RESET_QUERIES);

        ap_blacklist.clear();

        break;
    }
    case EState::STOPPED: {
        break;
    }
    default: {
        LOG(ERROR) << "Undefined state: " << int(m_eFSMState);
        return false;
    }
    }

    return (true);
}

bool BackhaulManager::backhaul_fsm_wireless(bool &skip_select)
{
    switch (m_eFSMState) {
    case EState::INIT_HAL: {
        skip_select = true;
        state_time_stamp_timeout =
            std::chrono::steady_clock::now() + std::chrono::seconds(WPA_ATTACH_TIMEOUT_SECONDS);
        FSM_MOVE_STATE(WPA_ATTACH);
        break;
    }
    case EState::WPA_ATTACH: {

        bool success = true;

        auto db = AgentDB::get();

        for (auto &radio_info : m_radios_info) {
            std::string iface = radio_info->sta_iface;
            if (radio_info->sta_iface.empty())
                continue;

            LOG(DEBUG) << FSM_CURR_STATE_STR << " iface: " << radio_info->sta_iface;

            // Create a HAL instance if doesn't exists
            if (!radio_info->sta_wlan_hal) {

                bwl::hal_conf_t hal_conf;

                if (!beerocks::bpl::bpl_cfg_get_wpa_supplicant_ctrl_path(radio_info->sta_iface,
                                                                         hal_conf.wpa_ctrl_path)) {
                    LOG(ERROR) << "Couldn't get hostapd control path";
                    return false;
                }

                using namespace std::placeholders; // for `_1`
                radio_info->sta_wlan_hal = bwl::sta_wlan_hal_create(
                    radio_info->sta_iface,
                    std::bind(&BackhaulManager::hal_event_handler, this, _1, radio_info->sta_iface),
                    hal_conf);
                LOG_IF(!radio_info->sta_wlan_hal, FATAL) << "Failed creating HAL instance!";
            } else {
                LOG(DEBUG) << "STA HAL exists...";
            }

            // Attach in BLOCKING mode
            auto attach_state = radio_info->sta_wlan_hal->attach(true);
            if (attach_state == bwl::HALState::Operational) {

                // Events
                int ext_events_fd = radio_info->sta_wlan_hal->get_ext_events_fd();
                int int_events_fd = radio_info->sta_wlan_hal->get_int_events_fd();
                if (ext_events_fd >= 0 && int_events_fd) {
                    beerocks::EventLoop::EventHandlers ext_events_handlers{
                        .name = "sta_hal_ext_events",
                        .on_read =
                            [radio_info](int fd, EventLoop &loop) {
                                radio_info->sta_wlan_hal->process_ext_events(fd);
                                return true;
                            },
                    };
                    if (!m_event_loop->register_handlers(ext_events_fd, ext_events_handlers)) {
                        LOG(ERROR) << "Unable to register handlers for external events queue!";
                        return false;
                    }

                    LOG(DEBUG) << "External events queue with fd = " << ext_events_fd;
                    radio_info->sta_hal_ext_events = ext_events_fd;

                    beerocks::EventLoop::EventHandlers int_events_handlers{
                        .name = "sta_hal_int_events",
                        .on_read =
                            [radio_info](int fd, EventLoop &loop) {
                                radio_info->sta_wlan_hal->process_int_events();
                                return true;
                            },
                    };
                    if (!m_event_loop->register_handlers(int_events_fd, int_events_handlers)) {
                        LOG(ERROR) << "Unable to register handlers for internal events queue!";
                        return false;
                    }

                    LOG(DEBUG) << "Internal events queue with fd = " << int_events_fd;
                    radio_info->sta_hal_int_events = int_events_fd;
                } else {
                    LOG(ERROR) << "Invalid event file descriptors - "
                               << "External = " << ext_events_fd
                               << ", Internal = " << int_events_fd;

                    success = false;
                    break;
                }

                /**
                 * This code was disabled as part of the effort to pass certification flow
                 * (PR #1469), and broke wireless backhaul flow.
                 * If a connected backhaul interface has been discovered, the backhaul fsm was set
                 * to MASTER_DISCOVERY state, otherwise to INITIATE_SCAN.
                 */

                // if (!roam_flag && soc->sta_wlan_hal->is_connected()) {
                //     if (!soc->sta_wlan_hal->update_status()) {
                //         LOG(ERROR) << "failed to update sta status";
                //         success = false;
                //         break;
                //     }
                //     connected                        = true;
                //     db->backhaul.selected_iface_name = iface;
                //     db->backhaul.connection_type   = AgentDB::sBackhaul::eConnectionType::Wireless;
                //     selected_bssid                 = soc->sta_wlan_hal->get_bssid();
                //     selected_bssid_channel         = soc->sta_wlan_hal->get_channel();
                //     soc->slave_is_backhaul_manager = true;
                //     break;
                // }

                auto radio = db->radio(radio_info->sta_iface);
                if (!radio) {
                    LOG(DEBUG) << "Radio of iface " << radio_info->sta_iface
                               << " does not exist on the db";
                    continue;
                }
                // Update the backhaul interface mac.
                radio->back.iface_mac =
                    tlvf::mac_from_string(radio_info->sta_wlan_hal->get_radio_mac());

            } else if (attach_state == bwl::HALState::Failed) {
                // Delete the HAL instance
                radio_info->sta_wlan_hal.reset();
                success = false;
                break;
            }
        }

        if (!success) {
            if (std::chrono::steady_clock::now() > state_time_stamp_timeout) {
                LOG(ERROR) << "attach wpa timeout";
                platform_notify_error(bpl::eErrorCode::BH_TIMEOUT_ATTACHING_TO_WPA_SUPPLICANT, "");
                stop_on_failure_attempts--;
                FSM_MOVE_STATE(RESTART);
            } else {
                UTILS_SLEEP_MSEC(1000);
            }
            break;
        }

        state_attempts = 0; // for next state

        state_time_stamp_timeout =
            std::chrono::steady_clock::now() + std::chrono::seconds(STATE_WAIT_WPS_TIMEOUT_SECONDS);
        FSM_MOVE_STATE(WAIT_WPS);
        break;
    }
    // Wait for WPS command
    case EState::WAIT_WPS: {
        auto db = AgentDB::get();
        if (!db->device_conf.local_gw &&
            std::chrono::steady_clock::now() > state_time_stamp_timeout) {
            LOG(ERROR) << STATE_WAIT_WPS_TIMEOUT_SECONDS
                       << " seconds has passed on state WAIT_WPS, move state to RESTART!";
            FSM_MOVE_STATE(RESTART);
        }
        break;
    }
    case EState::INITIATE_SCAN: {

        hidden_ssid            = false;
        selected_bssid_channel = 0;
        selected_bssid.clear();

        if (state_attempts > MAX_FAILED_SCAN_ATTEMPTS && !roam_flag) {
            LOG(DEBUG)
                << "exceeded maximum failed scan attempts, attempting hidden ssid connection";
            hidden_ssid              = true;
            pending_slave_sta_ifaces = slave_sta_ifaces;

            FSM_MOVE_STATE(WIRELESS_CONFIG_4ADDR_MODE);
            break;
        }

        if ((state_attempts > MAX_FAILED_ROAM_SCAN_ATTEMPTS) && roam_flag) {
            LOG(DEBUG) << "exceeded MAX_FAILED_ROAM_SCAN_ATTEMPTS";
            roam_flag                   = false;
            roam_selected_bssid_channel = 0;
            roam_selected_bssid.clear();
            state_attempts = 0;
            FSM_MOVE_STATE(RESTART);
            break;
        }
        auto db = AgentDB::get();

        bool preferred_band_is_available = false;

        // Check if backhaul preferred band is supported (supporting radio is available)
        if (db->device_conf.back_radio.backhaul_preferred_radio_band ==
            beerocks::eFreqType::FREQ_AUTO) {
            preferred_band_is_available = true;
        } else {
            for (const auto &radios_info : m_radios_info) {
                if (radios_info->sta_iface.empty())
                    continue;
                if (!radios_info->sta_wlan_hal) {
                    LOG(WARNING) << "Sta_hal of " << radios_info->sta_iface << " is null";
                    continue;
                }
                auto radio = db->radio(radios_info->hostap_iface);
                if (!radio) {
                    continue;
                }
                if (db->device_conf.back_radio.backhaul_preferred_radio_band == radio->freq_type) {
                    preferred_band_is_available = true;
                }
            }
        }

        LOG_IF(!preferred_band_is_available, DEBUG) << "Preferred backhaul band is not available";

        bool success        = true;
        bool scan_triggered = false;

        for (const auto &radios_info : m_radios_info) {
            if (radios_info->sta_iface.empty())
                continue;

            if (!radios_info->sta_wlan_hal) {
                LOG(WARNING) << "Sta_hal of " << radios_info->sta_iface << " is null";
                continue;
            }

            auto radio = db->radio(radios_info->hostap_iface);
            if (!radio) {
                continue;
            }

            if (preferred_band_is_available &&
                db->device_conf.back_radio.backhaul_preferred_radio_band !=
                    beerocks::eFreqType::FREQ_AUTO &&
                db->device_conf.back_radio.backhaul_preferred_radio_band != radio->freq_type) {
                LOG(DEBUG) << "slave iface=" << radios_info->sta_iface
                           << " is not of the preferred backhaul band";
                continue;
            }

            pending_slave_sta_ifaces.insert(radios_info->sta_iface);

            if (!radios_info->sta_wlan_hal->initiate_scan()) {
                LOG(ERROR) << "initiate_scan for iface " << radios_info->sta_iface << " failed!";
                platform_notify_error(bpl::eErrorCode::BH_SCAN_FAILED_TO_INITIATE_SCAN,
                                      "iface='" + radios_info->sta_iface + "'");
                success = false;
                break;
            }
            scan_triggered = true;
            LOG(INFO) << "wait for scan results on iface " << radios_info->sta_iface;
        }

        if (!success || !scan_triggered) {
            LOG_IF(!scan_triggered, DEBUG) << "no sta hal is available for scan";
            FSM_MOVE_STATE(RESTART);
        } else {
            FSM_MOVE_STATE(WAIT_FOR_SCAN_RESULTS);
            skip_select              = true;
            state_time_stamp_timeout = std::chrono::steady_clock::now() +
                                       std::chrono::seconds(WAIT_FOR_SCAN_RESULTS_TIMEOUT_SECONDS);
        }
        break;
    }
    case EState::WAIT_FOR_SCAN_RESULTS: {
        if (std::chrono::steady_clock::now() > state_time_stamp_timeout) {
            LOG(DEBUG) << "scan timed out";
            auto db = AgentDB::get();
            platform_notify_error(bpl::eErrorCode::BH_SCAN_TIMEOUT,
                                  "SSID='" + db->device_conf.back_radio.ssid + "'");

            state_attempts++;
            FSM_MOVE_STATE(INITIATE_SCAN);
            break;
        }

        skip_select = false;
        break;
    }
    case EState::WIRELESS_CONFIG_4ADDR_MODE: {

        // Disconnect is necessary before changing 4addr mode, to make sure wpa_supplicant is not using the iface
        if (hidden_ssid) {
            for (auto &radios_info : m_radios_info) {
                if (!radios_info->sta_wlan_hal || radios_info->sta_iface.empty())
                    continue;
                std::string iface = radios_info->sta_iface;
                radios_info->sta_wlan_hal->disconnect();
                radios_info->sta_wlan_hal->set_4addr_mode(true);
            }
        } else {
            auto active_hal = get_wireless_hal();
            active_hal->disconnect();
            active_hal->set_4addr_mode(true);
        }
        FSM_MOVE_STATE(WIRELESS_ASSOCIATE_4ADDR);
        skip_select = true;
        break;
    }
    case EState::WIRELESS_ASSOCIATE_4ADDR: {

        // Get the HAL for the connected interface
        auto active_hal = get_wireless_hal();

        if (roam_flag) {
            selected_bssid         = roam_selected_bssid;
            selected_bssid_channel = roam_selected_bssid_channel;
            if (!active_hal->roam(tlvf::mac_from_string(selected_bssid), selected_bssid_channel)) {
                platform_notify_error(bpl::eErrorCode::BH_ROAMING,
                                      "BSSID='" + selected_bssid + "'");
                stop_on_failure_attempts--;
                FSM_MOVE_STATE(RESTART);
                break;
            }
        }

        auto db = AgentDB::get();

        if (hidden_ssid) {
            std::string iface;

            std::shared_ptr<bwl::sta_wlan_hal> selected_hal;
            for (auto it = pending_slave_sta_ifaces.cbegin();
                 it != pending_slave_sta_ifaces.end();) {
                iface          = *it;
                auto iface_hal = get_wireless_hal(iface);

                if (!iface_hal) {
                    LOG(ERROR) << "Slave for iface " << iface << " not found!";
                    break;
                }

                iface_hal->refresh_radio_info();

                if (son::wireless_utils::which_freq(iface_hal->get_radio_info().channel) ==
                        beerocks::FREQ_24G &&
                    pending_slave_sta_ifaces.size() > 1) {
                    ++it;
                    LOG(DEBUG) << "skipping 2.4GHz iface " << iface
                               << " while other ifaces are available";
                    continue;
                }

                it = pending_slave_sta_ifaces.erase(it);
                break;
            }

            db->backhaul.selected_iface_name = iface;
            active_hal                       = get_wireless_hal();
        }

        if (active_hal->connect(db->device_conf.back_radio.ssid, db->device_conf.back_radio.pass,
                                db->device_conf.back_radio.security_type,
                                db->device_conf.back_radio.mem_only_psk, selected_bssid,
                                selected_bssid_channel, hidden_ssid)) {
            LOG(DEBUG) << "successful call to active_hal->connect(), bssid=" << selected_bssid
                       << ", channel=" << selected_bssid_channel
                       << ", iface=" << db->backhaul.selected_iface_name;
        } else {
            LOG(ERROR) << "connect command failed for iface " << db->backhaul.selected_iface_name;
            FSM_MOVE_STATE(INITIATE_SCAN);
            break;
        }

        FSM_MOVE_STATE(WIRELESS_ASSOCIATE_4ADDR_WAIT);
        state_attempts           = 0;
        skip_select              = true;
        state_time_stamp_timeout = std::chrono::steady_clock::now() +
                                   std::chrono::seconds(MAX_WIRELESS_ASSOCIATE_TIMEOUT_SECONDS);
        break;
    }
    case EState::WIRELESS_ASSOCIATE_4ADDR_WAIT: {

        auto db  = AgentDB::get();
        auto now = std::chrono::steady_clock::now();
        if (now > state_time_stamp_timeout) {
            LOG(ERROR) << "associate wait timeout";
            if (hidden_ssid) {
                if (pending_slave_sta_ifaces.empty()) {
                    LOG(ERROR) << "hidden ssid association failed for all ifaces";
                    platform_notify_error(
                        bpl::eErrorCode::BH_SCAN_EXCEEDED_MAXIMUM_FAILED_SCAN_ATTEMPTS,
                        "attempts=" + std::to_string(MAX_FAILED_SCAN_ATTEMPTS) + ", SSID='" +
                            db->device_conf.back_radio.ssid + "'");
                } else {
                    FSM_MOVE_STATE(WIRELESS_ASSOCIATE_4ADDR);
                    break;
                }
            } else {

                if (roam_flag) {
                    FSM_MOVE_STATE(RESTART);
                    roam_flag = false;
                    break;
                }

                stop_on_failure_attempts--;
                platform_notify_error(bpl::eErrorCode::BH_ASSOCIATE_4ADDR_TIMEOUT,
                                      "SSID='" + db->device_conf.back_radio.ssid + "', iface='" +
                                          db->backhaul.selected_iface_name + "'");

                if (!selected_bssid.empty()) {
                    ap_blacklist_entry &entry = ap_blacklist[selected_bssid];
                    entry.timestamp           = now;
                    entry.attempts++;
                    LOG(DEBUG) << "updating bssid " << selected_bssid
                               << " blacklist entry, attempts=" << entry.attempts;
                }
                roam_flag = false;
            }
            FSM_MOVE_STATE(INITIATE_SCAN);
        }
        break;
    }
    case EState::WIRELESS_WAIT_FOR_RECONNECT: {
        auto now = std::chrono::steady_clock::now();
        if (now > state_time_stamp_timeout) {
            LOG(DEBUG) << "reconnect wait timed out";

            // increment attempts count in blacklist
            if (!selected_bssid.empty()) {
                auto &entry     = ap_blacklist[selected_bssid];
                entry.timestamp = now;
                entry.attempts++;
                LOG(DEBUG) << "updating bssid " << selected_bssid
                           << " blacklist entry, attempts=" << entry.attempts
                           << ", max_allowed attempts=" << AP_BLACK_LIST_FAILED_ATTEMPTS_THRESHOLD;
            }

            FSM_MOVE_STATE(INITIATE_SCAN);
        }
        break;
    }
    default: {
        LOG(ERROR) << "backhaul_fsm_wireless() Invalid state: " << int(m_eFSMState);
        return false;
    }
    }
    return (true);
}

bool BackhaulManager::handle_slave_backhaul_message(int fd, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto beerocks_header = message_com::parse_intel_vs_message(cmdu_rx);
    if (!beerocks_header) {
        LOG(WARNING) << "Not a beerocks vendor specific message";
        return true;
    }

    // Validate BACKHAUL action
    if (beerocks_header->action() != beerocks_message::ACTION_BACKHAUL) {
        LOG(ERROR) << "Invalid message action received: action=" << int(beerocks_header->action())
                   << ", action_op=" << int(beerocks_header->action_op());
        return false;
    }

    // Handle messages
    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_BACKHAUL_REGISTER_REQUEST: {

        m_agent_fd      = fd;
        auto agent_name = std::move(std::string("agent"));
        LOG(DEBUG) << "Assigning FD (" << fd << ") to " << agent_name;
        m_cmdu_server->set_client_name(fd, agent_name);

        if (!m_agent_ucc_listener && m_ucc_server) {
            m_agent_ucc_listener =
                std::make_unique<agent_ucc_listener>(*this, cert_cmdu_tx, std::move(m_ucc_server));
            if (!m_agent_ucc_listener) {
                LOG(ERROR) << "failed creating agent_ucc_listener";
                return false;
            }

            // Install handlers for WFA-CA commands
            beerocks::beerocks_ucc_listener::CommandHandlers handlers;
            handlers.on_dev_reset_default =
                [&](int fd, const std::unordered_map<std::string, std::string> &params) {
                    handle_dev_reset_default(fd, params);
                };
            handlers.on_dev_set_config =
                [&](const std::unordered_map<std::string, std::string> &params,
                    std::string &err_string) { return handle_dev_set_config(params, err_string); };
            m_agent_ucc_listener->set_handlers(handlers);
        }

        LOG(DEBUG) << "Received ACTION_BACKHAUL_REGISTER_REQUEST";

        auto register_response =
            message_com::create_vs_message<beerocks_message::cACTION_BACKHAUL_REGISTER_RESPONSE>(
                cmdu_tx);

        if (register_response == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        send_cmdu(fd, cmdu_tx);
        LOG(DEBUG) << "Sent ACTION_BACKHAUL_REGISTER_RESPONSE";
        break;
    }

    case beerocks_message::ACTION_BACKHAUL_ENABLE: {

        LOG(DEBUG) << "Received ACTION_BACKHAUL_ENABLE";

        auto db = AgentDB::get();

        // Adding only radios which does not exist in the backhaul m_radios_info list.
        // This allows to keep the backhaul connection if exists and report "connected" to the Agent
        // immediately.
        for (auto radio : db->get_radios_list()) {
            if (!radio) {
                continue;
            }

            auto found =
                std::find_if(m_radios_info.begin(), m_radios_info.end(),
                             [&](const std::shared_ptr<sRadioInfo> &radio_info) {
                                 if ((!radio->front.iface_name.empty() &&
                                      radio_info->hostap_iface == radio->front.iface_name) ||
                                     (!radio_info->sta_iface.empty() &&
                                      radio_info->sta_iface == radio->back.iface_name)) {
                                     return true;
                                 }
                                 return false;
                             });

            LOG(DEBUG) << "Radio found=" << (found != m_radios_info.end());

            auto radio_info = std::shared_ptr<sRadioInfo>();
            if (found == m_radios_info.end()) {
                // If a radio hasn't been found in the radios_info lists then add it.
                radio_info               = std::make_shared<sRadioInfo>();
                radio_info->hostap_iface = radio->front.iface_name;
                radio_info->sta_iface    = radio->back.iface_name;
                LOG(DEBUG) << "Pushing new Radio";
                m_radios_info.push_back(radio_info);
            } else {
                radio_info = *found;
                LOG(DEBUG) << "Updating new Radio";
            }

            radio_info->radio_mac = radio->front.iface_mac;
        }

        if (m_eFSMState >= EState::CONNECTED) {
        }

        // If we're already connected, send a notification to the slave
        if (FSM_IS_IN_STATE(OPERATIONAL)) {

            // Send a connection notification to the Agent right away.
            FSM_MOVE_STATE(CONNECTED);
            break;
        }

        if (db->device_conf.local_gw) {
            FSM_MOVE_STATE(ENABLED);
            break;
        }

        if (db->device_conf.back_radio.backhaul_preferred_radio_band ==
            beerocks::eFreqType::FREQ_UNKNOWN) {
            LOG(DEBUG) << "Unknown backhaul preferred radio band, setting to auto";
            m_sConfig.backhaul_preferred_radio_band = beerocks::eFreqType::FREQ_AUTO;
        } else {
            m_sConfig.backhaul_preferred_radio_band =
                db->device_conf.back_radio.backhaul_preferred_radio_band;
        }

        // Change mixed state to WPA2
        if (db->device_conf.back_radio.security_type == bwl::WiFiSec::WPA_WPA2_PSK) {
            db->device_conf.back_radio.security_type = bwl::WiFiSec::WPA2_PSK;
        }

        LOG(DEBUG) << "Agent is ready, proceeding" << std::endl
                   << "SSID: " << db->device_conf.back_radio.ssid << ", Pass: ****"
                   << ", Security: " << db->device_conf.back_radio.security_type
                   << ", Bridge: " << db->bridge.iface_name
                   << ", Wired: " << db->ethernet.wan.iface_name;

        FSM_MOVE_STATE(ENABLED);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST: {
        LOG(DEBUG) << "Received ACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST";
        auto request_in = beerocks_header->addClass<
            beerocks_message::cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST>();
        if (!request_in) {
            LOG(ERROR)
                << "addClass cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST failed";
            return false;
        }
        configuration_stop_on_failure_attempts = request_in->attempts();
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST: {
        LOG(DEBUG) << "ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST";

        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST>();
        if (!request) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST failed";
            return false;
        }
        std::string sta_mac = tlvf::mac_to_string(request->params().mac);
        bool ap_busy        = false;
        bool bwl_error      = false;
        if (unassociated_rssi_measurement_header_id == -1) {
            auto response = message_com::create_vs_message<
                beerocks_message::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE>(
                cmdu_tx, beerocks_header->id());
            if (response == nullptr) {
                LOG(ERROR) << "Failed building "
                              "ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE message!";
                break;
            }
            response->mac() = tlvf::mac_from_string(sta_mac);
            send_cmdu(m_agent_fd, cmdu_tx);
            LOG(DEBUG) << "send ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE, sta_mac = "
                       << sta_mac;
            int bandwidth = beerocks::utils::convert_bandwidth_to_int(
                (beerocks::eWiFiBandwidth)request->params().bandwidth);
            if (get_wireless_hal()->unassoc_rssi_measurement(
                    sta_mac, request->params().channel, bandwidth,
                    request->params().vht_center_frequency, request->params().measurement_delay,
                    request->params().mon_ping_burst_pkt_num)) {
            } else {
                bwl_error = true;
                LOG(ERROR) << "unassociated_sta_rssi_measurement failed!";
            }

            unassociated_rssi_measurement_header_id = beerocks_header->id();
            LOG(DEBUG) << "CLIENT_RX_RSSI_MEASUREMENT_REQUEST, mac = " << sta_mac
                       << " channel = " << int(request->params().channel) << " bandwidth="
                       << beerocks::utils::convert_bandwidth_to_int(
                              (beerocks::eWiFiBandwidth)request->params().bandwidth);
        } else {
            ap_busy = true;
            LOG(WARNING)
                << "busy!, send response to retry CLIENT_RX_RSSI_MEASUREMENT_REQUEST, mac = "
                << sta_mac;
        }

        if (ap_busy || bwl_error) {
            auto response = message_com::create_vs_message<
                beerocks_message::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>(
                cmdu_tx, beerocks_header->id());
            if (response == nullptr) {
                LOG(ERROR) << "Failed building ACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE "
                              "message!";
                break;
            }
            response->params().result.mac = request->params().mac;
            response->params().rx_rssi    = beerocks::RSSI_INVALID;
            response->params().rx_snr     = beerocks::SNR_INVALID;
            response->params().rx_packets = -1;
            send_cmdu(m_agent_fd, cmdu_tx);
        }
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE: {
        auto response_in = beerocks_header->addClass<
            beerocks_message::cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE>();
        if (!response_in) {
            LOG(ERROR) << "addClass ACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE failed";
            return false;
        }

        auto mid = beerocks_header->id();

        if (!cmdu_tx.create(
                mid, ieee1905_1::eMessageType::ASSOCIATED_STA_LINK_METRICS_RESPONSE_MESSAGE)) {
            LOG(ERROR) << "cmdu creation of type ASSOCIATED_STA_LINK_METRICS_RESPONSE_MESSAGE "
                          "has failed";
            return false;
        }

        auto response_out = cmdu_tx.addClass<wfa_map::tlvAssociatedStaLinkMetrics>();
        if (!response_out) {
            LOG(ERROR) << "adding wfa_map::tlvAssociatedStaLinkMetrics failed";
            return false;
        }

        response_out->sta_mac() = response_in->sta_mac();

        if (!response_out->alloc_bssid_info_list(response_in->bssid_info_list_length())) {
            LOG(ERROR) << "alloc_per_bss_sta_link_metrics failed";
            return false;
        }

        // adding (currently empty) an associated sta EXTENDED link metrics tlv.
        // The values will be filled part of PPM-1259
        auto extended = cmdu_tx.addClass<wfa_map::tlvAssociatedStaExtendedLinkMetrics>();
        if (!extended) {
            LOG(ERROR) << "adding wfa_map::tlvAssociatedStaExtendedLinkMetrics failed";
            return false;
        }

        extended->associated_sta() = response_in->sta_mac();

        if (!extended->alloc_metrics_list(response_in->bssid_info_list_length())) {
            LOG(ERROR) << "allocation of per BSS STA metrics failed";
            return false;
        }

        auto db = AgentDB::get();

        for (size_t i = 0; i < response_out->bssid_info_list_length(); ++i) {
            auto &bss_in  = std::get<1>(response_in->bssid_info_list(i));
            auto &bss_out = std::get<1>(response_out->bssid_info_list(i));

            auto &client_mac = response_out->sta_mac();

            auto radio = db->get_radio_by_mac(client_mac, AgentDB::eMacType::CLIENT);
            if (!radio) {
                LOG(ERROR) << "radio for client mac " << client_mac << " not found";
                return false;
            }

            // If get_radio_by_mac() found the radio, it means that 'client_mac' is on the radio
            // 'associated_clients' list.
            bss_out.bssid = radio->associated_clients.at(client_mac).bssid;
            if (bss_out.bssid == beerocks::net::network_utils::ZERO_MAC) {
                LOG(ERROR) << "bssid is ZERO_MAC";
                return false;
            }

            bss_out.earliest_measurement_delta = bss_in.earliest_measurement_delta;
            bss_out.downlink_estimated_mac_data_rate_mbps =
                bss_in.downlink_estimated_mac_data_rate_mbps;
            bss_out.uplink_estimated_mac_data_rate_mbps =
                bss_in.uplink_estimated_mac_data_rate_mbps;
            bss_out.sta_measured_uplink_rcpi_dbm_enc = bss_in.sta_measured_uplink_rcpi_dbm_enc;
        }

        LOG(DEBUG) << "Send AssociatedStaLinkMetrics to controller, mid = " << mid;
        send_cmdu_to_broker(cmdu_tx, db->controller_info.bridge_mac, db->bridge.mac);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_ZWDFS_RADIO_DETECTED: {
        auto msg_in =
            beerocks_header->addClass<beerocks_message::cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED>();
        if (!msg_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED failed";
            return false;
        }

        auto front_iface_name = msg_in->front_iface_name_str();

        LOG(DEBUG) << "Received ACTION_BACKHAUL_ZWDFS_RADIO_DETECTED from front_radio="
                   << front_iface_name;

        auto removed_it = std::remove_if(m_radios_info.begin(), m_radios_info.end(),
                                         [&](const std::shared_ptr<sRadioInfo> &radio_info) {
                                             return radio_info->hostap_iface == front_iface_name;
                                         });

        m_radios_info.erase(removed_it, m_radios_info.end());

        // Notify channel selection task on zwdfs radio re-connect
        m_task_pool.send_event(eTaskType::CHANNEL_SELECTION,
                               ChannelSelectionTask::eEvent::AP_ENABLED, &front_iface_name);

        break;
    }
    case beerocks_message::ACTION_BACKHAUL_AP_DISABLED_NOTIFICATION: {
        auto msg_in = beerocks_header
                          ->addClass<beerocks_message::cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION>();
        if (!msg_in) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION failed";
            return false;
        }

        // TODO: Remove when moving channel selection task to Agent context as part of PPM-1680.
        auto front_iface_name = msg_in->iface_str();
        // notify channel selection task on radio disconnect
        m_task_pool.send_event(eTaskType::CHANNEL_SELECTION,
                               ChannelSelectionTask::eEvent::AP_DISABLED, &front_iface_name);

        break;
    }
    default: {
        bool handled =
            m_task_pool.handle_cmdu(cmdu_rx, 0, sMacAddr(), sMacAddr(), fd, beerocks_header);
        if (!handled) {
            LOG(ERROR) << "Unhandled message received from the Agent: "
                       << int(beerocks_header->action_op());
            return false;
        }
        return true;
    }
    }

    return true;
}

bool BackhaulManager::handle_1905_1_message(ieee1905_1::CmduMessageRx &cmdu_rx,
                                            uint32_t iface_index, const sMacAddr &dst_mac,
                                            const sMacAddr &src_mac)
{
    /*
     * return values:
     * true if the message was handled by the backhaul manager
     * false if the message needs to be forwarded by the calling function
     */
    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_RENEW_MESSAGE: {
        auto db = AgentDB::get();
        if (src_mac != db->controller_info.bridge_mac) {
            LOG(INFO) << "current controller_bridge_mac=" << db->controller_info.bridge_mac
                      << " but renew came from src_mac=" << src_mac << ", ignoring";
            return true;
        }
        // According to IEEE 1905.1, there should be a separate renew per frequency band. However,
        // Multi-AP overrides this and says that all radios have to restart WSC when a renew is
        // received. The actual handling is done in the slaves, so forward it to the slaves by
        // returning false.
        return false;
    }
    case ieee1905_1::eMessageType::BACKHAUL_STEERING_REQUEST_MESSAGE: {
        return handle_backhaul_steering_request(cmdu_rx, src_mac);
    }
    case ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE: {
        // We should not handle vendor specific messages here, return false so the message will
        // be forwarded and will not be passed to the task_pool.
        return false;
    }
    default: {
        // TODO add a warning once all vendor specific flows are replaced with EasyMesh
        // flows, since we won't expect a 1905 message not handled in this function
        return m_task_pool.handle_cmdu(cmdu_rx, iface_index, dst_mac, src_mac,
                                       beerocks::net::FileDescriptor::invalid_descriptor);
    }
    }
}

bool BackhaulManager::handle_slave_1905_1_message(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                  uint32_t iface_index, const sMacAddr &dst_mac,
                                                  const sMacAddr &src_mac)
{
    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::FAILED_CONNECTION_MESSAGE: {
        return handle_slave_failed_connection_message(cmdu_rx, src_mac);
    }
    default: {
        bool handled = m_task_pool.handle_cmdu(cmdu_rx, iface_index, dst_mac, src_mac,
                                               beerocks::net::FileDescriptor::invalid_descriptor);
        if (!handled) {
            LOG(DEBUG) << "Unhandled 1905 message " << std::hex << int(cmdu_rx.getMessageType())
                       << ", forwarding to controller...";

            auto db = AgentDB::get();
            if (db->controller_info.bridge_mac == beerocks::net::network_utils::ZERO_MAC) {
                LOG(DEBUG) << "Controller MAC unknown. Dropping message.";
                return false;
            }

            // Send the CMDU to the broker
            return forward_cmdu_to_broker(cmdu_rx, db->controller_info.bridge_mac, db->bridge.mac,
                                          db->bridge.iface_name);
        }

        return true;
    }
    }
}

bool BackhaulManager::send_slaves_enable()
{
    auto iface_hal = get_wireless_hal();

    auto db = AgentDB::get();
    for (const auto &radio_info : m_radios_info) {
        auto notification =
            message_com::create_vs_message<beerocks_message::cACTION_BACKHAUL_ENABLE_APS_REQUEST>(
                cmdu_tx);

        if (notification == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        notification->set_iface(radio_info->hostap_iface);

        // enable wireless backhaul interface on the selected channel
        if (radio_info->sta_iface == db->backhaul.selected_iface_name) {
            notification->channel() = iface_hal->get_channel();
            // Set default bw 0 (20Mhz) to cover most cases.
            // Since channel operates in 20Mhz center_channel is the same as the main channel.
            // Need to figure out how to get bw parameter of the selected channel (PPM-643).
            notification->bandwidth()      = eWiFiBandwidth::BANDWIDTH_20;
            notification->center_channel() = notification->channel();
        }

        // Uninitialized CMDU parameters remain zero.
        // Channel zero is used to trigger ACS on radio which does not have BH link
        // TODO: This flow is valid only for MxL platforms (PPM-1928)

        LOG(DEBUG) << "Send enable to radio " << radio_info->hostap_iface
                   << ", channel = " << int(notification->channel())
                   << ", center_channel = " << int(notification->center_channel());

        send_cmdu(m_agent_fd, cmdu_tx);
    }

    return true;
}

bool BackhaulManager::send_slaves_tear_down()
{
    auto msg =
        message_com::create_vs_message<beerocks_message::cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST>(
            cmdu_tx);
    if (!msg) {
        LOG(ERROR) << "Failed building cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST";
        return false;
    }
    LOG(DEBUG) << "Request agent to tear down";
    if (!send_cmdu(m_agent_fd, cmdu_tx)) {
        LOG(ERROR) << "Failed to send cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST";
        return false;
    }

    return true;
}

bool BackhaulManager::hal_event_handler(bwl::base_wlan_hal::hal_event_ptr_t event_ptr,
                                        std::string iface)
{
    if (!event_ptr) {
        LOG(ERROR) << "Invalid event!";
        return false;
    }

    // TODO: TEMP!
    LOG(DEBUG) << "Got event " << int(event_ptr->first) << " from iface " << iface;

    // AP Event & Data
    typedef bwl::sta_wlan_hal::Event Event;
    auto event = (Event)(event_ptr->first);
    auto data  = event_ptr->second.get();

    switch (event) {

    case Event::Connected: {

        auto iface_hal = get_wireless_hal(iface);
        auto bssid     = tlvf::mac_from_string(iface_hal->get_bssid());

        LOG(DEBUG) << "WPA EVENT_CONNECTED on iface=" << iface;
        LOG(DEBUG) << "successfully connected to bssid=" << bssid
                   << " on channel=" << (iface_hal->get_channel()) << " on iface=" << iface;

        auto db = AgentDB::get();

        if (iface == db->backhaul.selected_iface_name && !hidden_ssid) {
            //this is generally not supposed to happen
            LOG(WARNING) << "event iface=" << iface
                         << ", selected iface=" << db->backhaul.selected_iface_name
                         << ", hidden_ssid=" << hidden_ssid;
        }

        // Try adding wireless backhaul STA interface to the bridge in case there is no
        // entity (hostadp) that adds it automaticaly.
        auto bridge        = db->bridge.iface_name;
        auto bridge_ifaces = beerocks::net::network_utils::linux_get_iface_list_from_bridge(bridge);
        if (!beerocks::net::network_utils::linux_add_iface_to_bridge(bridge, iface)) {
            LOG(INFO) << "The wireless interface " << iface << " is already in the bridge";
        }

        // This event may come as a result of enabling the backhaul, but also as a result
        // of steering. *Only* in case it was the result of steering, we need to send a steering
        // response.
        if (m_backhaul_steering_bssid == bssid) {
            m_backhaul_steering_bssid = beerocks::net::network_utils::ZERO_MAC;
            m_timer_manager->remove_timer(m_backhaul_steering_timer);

            create_backhaul_steering_response(wfa_map::tlvErrorCode::eReasonCode::RESERVED, bssid);

            LOG(DEBUG) << "Sending BACKHAUL_STA_STEERING_RESPONSE_MESSAGE";
            send_cmdu_to_broker(cmdu_tx, db->controller_info.bridge_mac, db->bridge.mac);
        }

        // TODO: Need to unite WAIT_WPS and WIRELESS_ASSOCIATE_4ADDR_WAIT handling
        if (FSM_IS_IN_STATE(WAIT_WPS) || FSM_IS_IN_STATE(WIRELESS_ASSOCIATE_4ADDR_WAIT)) {
            auto msg = static_cast<bwl::sACTION_BACKHAUL_CONNECTED_NOTIFICATION *>(data);
            if (!msg) {
                LOG(ERROR) << "ACTION_BACKHAUL_CONNECTED_NOTIFICATION not found on Connected event";
                return false;
            }
            LOG(INFO) << "Multi-AP-Profile: " << msg->multi_ap_profile
                      << ", Multi-AP Primary VLAN ID: " << msg->multi_ap_primary_vlan_id;

            db->traffic_separation.primary_vlan_id = msg->multi_ap_primary_vlan_id;
            db->backhaul.bssid_multi_ap_profile    = msg->multi_ap_profile;

            auto request = message_com::create_vs_message<
                beerocks_message::cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST>(cmdu_tx);

            // Send the message to one of the son_slaves.
            send_cmdu(m_agent_fd, cmdu_tx);
        }

        if (FSM_IS_IN_STATE(WAIT_WPS)) {
            db->backhaul.selected_iface_name = iface;
            db->backhaul.connection_type     = AgentDB::sBackhaul::eConnectionType::Wireless;
            LOG(DEBUG) << "WPS scan completed successfully on iface = " << iface
                       << ", enabling all APs";

            // Send slave enable the AP's
            send_slaves_enable();
            FSM_MOVE_STATE(CONNECTED);
        }
        if (FSM_IS_IN_STATE(WIRELESS_ASSOCIATE_4ADDR_WAIT)) {
            LOG(DEBUG) << "successful connect on iface=" << iface;
            if (hidden_ssid) {
                iface_hal->refresh_radio_info();
                const auto &bwl_radio_info = iface_hal->get_radio_info();
                for (const auto &radio_info : m_radios_info) {
                    if (radio_info->sta_iface == iface) {
                        auto radio = db->radio(iface);
                        if (!radio) {
                            continue;
                        }
                        /* prevent low filter radio from connecting to high band in any case */
                        if (son::wireless_utils::which_freq(bwl_radio_info.channel) ==
                                beerocks::FREQ_5G &&
                            radio->sta_iface_filter_low &&
                            !son::wireless_utils::is_low_subband(bwl_radio_info.channel)) {
                            LOG(DEBUG) << "iface " << iface
                                       << " is connected on low 5G band with filter, aborting";
                            FSM_MOVE_STATE(WIRELESS_CONFIG_4ADDR_MODE);
                            return true;
                        }
                        /* prevent unfiltered ("high") radio from connecting to low band, unless we have only 2 radios */
                        int sta_iface_count_5ghz = 0;
                        for (const auto &sta_iface : slave_sta_ifaces) {
                            auto sta_iface_hal = get_wireless_hal(sta_iface);
                            if (!sta_iface_hal)
                                break;

                            sta_iface_hal->refresh_radio_info();
                            if (son::wireless_utils::which_freq(
                                    sta_iface_hal->get_radio_info().channel) == beerocks::FREQ_5G) {
                                sta_iface_count_5ghz++;
                            }
                        }
                        if (son::wireless_utils::which_freq(bwl_radio_info.channel) ==
                                beerocks::FREQ_5G &&
                            !radio->sta_iface_filter_low &&
                            son::wireless_utils::is_low_subband(bwl_radio_info.channel) &&
                            sta_iface_count_5ghz > 1) {
                            LOG(DEBUG) << "iface " << iface
                                       << " is connected on low 5G band with filter, aborting";
                            FSM_MOVE_STATE(WIRELESS_CONFIG_4ADDR_MODE);
                            return true;
                        }
                    }
                }
            }
            roam_flag      = false;
            state_attempts = 0;

            // Send slaves to enable the AP's
            send_slaves_enable();

            FSM_MOVE_STATE(CONNECTED);
        } else if (FSM_IS_IN_STATE(WIRELESS_WAIT_FOR_RECONNECT)) {
            LOG(DEBUG) << "reconnected successfully, continuing";

            // IRE running controller
            if (db->device_conf.local_controller && !db->device_conf.local_gw) {
                FSM_MOVE_STATE(CONNECTED);
            } else {
                FSM_MOVE_STATE(OPERATIONAL);
            }
        }
    } break;

    case Event::Disconnected: {
        if (FSM_IS_IN_STATE(WAIT_WPS)) {
            return true;
        }
        auto db = AgentDB::get();
        if (iface == db->backhaul.selected_iface_name) {
            if (FSM_IS_IN_STATE(OPERATIONAL) || FSM_IS_IN_STATE(CONNECTED)) {

                // If this event comes as a result of a steering request, then do not consider it
                // as an error.
                if (m_backhaul_steering_bssid == beerocks::net::network_utils::ZERO_MAC) {
                    platform_notify_error(bpl::eErrorCode::BH_DISCONNECTED,
                                          "Backhaul disconnected on operational state");
                    stop_on_failure_attempts--;
                }

                state_time_stamp_timeout =
                    std::chrono::steady_clock::now() +
                    std::chrono::seconds(WIRELESS_WAIT_FOR_RECONNECT_TIMEOUT);
                FSM_MOVE_STATE(WIRELESS_WAIT_FOR_RECONNECT);
            } else if (FSM_IS_IN_STATE(WIRELESS_ASSOCIATE_4ADDR_WAIT)) {
                if (!data) {
                    LOG(ERROR) << "Disconnected event without data!";
                    return false;
                }
                roam_flag = false;
                auto msg =
                    static_cast<bwl::sACTION_BACKHAUL_DISCONNECT_REASON_NOTIFICATION *>(data);
                if (msg->disconnect_reason == uint32_t(DEAUTH_REASON_PASSPHRASE_MISMACH)) {
                    //enter bssid to black_list trigger timer
                    auto local_time_stamp = std::chrono::steady_clock::now();
                    auto local_bssid      = tlvf::mac_to_string(msg->bssid);
                    LOG(DEBUG) << "insert bssid = " << local_bssid << " to backhaul blacklist";
                    ap_blacklist_entry entry;
                    entry.timestamp           = local_time_stamp;
                    entry.attempts            = AP_BLACK_LIST_FAILED_ATTEMPTS_THRESHOLD;
                    ap_blacklist[local_bssid] = entry;
                    platform_notify_error(bpl::eErrorCode::BH_ASSOCIATE_4ADDR_FAILURE,
                                          "SSID='" + db->device_conf.back_radio.ssid +
                                              "', BSSID='" + local_bssid + "', DEAUTH_REASON='" +
                                              std::to_string(msg->disconnect_reason));
                    stop_on_failure_attempts--;
                    FSM_MOVE_STATE(INITIATE_SCAN);
                }

            } else {
                platform_notify_error(bpl::eErrorCode::BH_DISCONNECTED,
                                      "Backhaul disconnected non operational state");
                stop_on_failure_attempts--;
                FSM_MOVE_STATE(RESTART);
            }
        }

    } break;

    case Event::Terminating: {

        LOG(DEBUG) << "wpa_supplicant terminated, restarting";
        platform_notify_error(bpl::eErrorCode::BH_WPA_SUPPLICANT_TERMINATED,
                              "wpa_supplicant terminated");
        stop_on_failure_attempts--;
        FSM_MOVE_STATE(RESTART);

    } break;

    case Event::ScanResults: {
        if (FSM_IS_IN_STATE(WAIT_WPS)) {
            return true;
        }
        if (FSM_IS_IN_STATE(OPERATIONAL) &&
            m_backhaul_steering_bssid != beerocks::net::network_utils::ZERO_MAC) {

            LOG(DEBUG) << "Received scan results while a steering bssid is set.";

            auto active_hal = get_wireless_hal();
            if (!active_hal) {
                LOG(ERROR) << "Couldn't get active HAL";
                return false;
            }

            LOG(DEBUG) << "Steering to BSSID " << m_backhaul_steering_bssid
                       << ", channel=" << m_backhaul_steering_channel;
            auto associate =
                active_hal->roam(m_backhaul_steering_bssid, m_backhaul_steering_channel);
            if (!associate) {
                LOG(ERROR) << "Couldn't associate active HAL with bssid: "
                           << m_backhaul_steering_bssid;

                auto response = create_backhaul_steering_response(
                    wfa_map::tlvErrorCode::eReasonCode::
                        BACKHAUL_STEERING_REQUEST_REJECTED_TARGET_BSS_SIGNAL_NOT_SUITABLE,
                    m_backhaul_steering_bssid);

                if (!response) {
                    LOG(ERROR) << "Failed to build Backhaul Steering Response message.";
                    return false;
                }

                auto db = AgentDB::get();
                send_cmdu_to_broker(cmdu_tx, db->controller_info.bridge_mac, db->bridge.mac);

                // Steering operation has failed so cancel it to avoid sending a second reply when
                // timer expires.
                cancel_backhaul_steering_operation();

                return false;
            }
            // Resetting m_backhaul_steering_bssid is done by a timer.
            // Sending the steering response is done when receiving
            // the CONNECTED event.
            return true;
        }
        if (!FSM_IS_IN_STATE(WAIT_FOR_SCAN_RESULTS)) {
            LOG(DEBUG) << "not waiting for scan results, ignoring event";
            return true;
        }

        LOG(DEBUG) << "scan results available for iface " << iface;
        pending_slave_sta_ifaces.erase(iface);

        if (pending_slave_sta_ifaces.empty()) {
            LOG(DEBUG) << "scan results ready";
            get_scan_measurement();
            if (!select_bssid()) {
                LOG(DEBUG) << "couldn't find a suitable BSSID";
                FSM_MOVE_STATE(INITIATE_SCAN);
                state_attempts++;
                return false;
            } else {
                FSM_MOVE_STATE(WIRELESS_CONFIG_4ADDR_MODE);
            }
        }

    } break;

    case Event::ChannelSwitch: {

    } break;

    case Event::STA_Unassoc_RSSI: {

        if (!data) {
            LOG(ERROR) << "STA_Unassoc_RSSI without data!";
            return false;
        }

        auto msg = static_cast<bwl::sACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE *>(data);

        LOG(DEBUG) << "ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE for mac "
                   << msg->params.result.mac << " id = " << unassociated_rssi_measurement_header_id;

        if (unassociated_rssi_measurement_header_id > -1) {
            auto response = message_com::create_vs_message<
                beerocks_message::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>(cmdu_tx);

            if (response == nullptr) {
                LOG(ERROR) << "Failed building message!";
                break;
            }
            response->params().result.mac        = msg->params.result.mac;
            response->params().result.channel    = msg->params.result.channel;
            response->params().result.rssi       = msg->params.result.rssi;
            response->params().rx_phy_rate_100kb = msg->params.rx_phy_rate_100kb;
            response->params().tx_phy_rate_100kb = msg->params.tx_phy_rate_100kb;
            response->params().rx_rssi           = msg->params.rx_rssi;
            response->params().rx_snr            = msg->params.rx_snr;
            response->params().rx_packets        = msg->params.rx_packets;
            response->params().src_module        = msg->params.src_module;

            send_cmdu(m_agent_fd, cmdu_tx);
        } else {
            LOG(ERROR) << "sta_unassociated_rssi_measurement_header_id == -1";
        }
    } break;

    // Unhandled events
    default: {
        LOG(ERROR) << "Unhandled event: " << int(event);
        return false;
    }
    }

    return true;
} // namespace beerocks

bool BackhaulManager::select_bssid()
{
    int max_rssi_24     = beerocks::RSSI_INVALID;
    int max_rssi_5_best = beerocks::RSSI_INVALID;
    int max_rssi_5_high = beerocks::RSSI_INVALID;
    int max_rssi_5_low  = beerocks::RSSI_INVALID;
    std::string best_bssid_5, best_bssid_5_high, best_bssid_5_low, best_bssid_24;
    int best_bssid_channel_5 = 0, best_bssid_channel_5_high = 0, best_bssid_channel_5_low = 0,
        best_bssid_channel_24 = 0;
    std::string best_24_sta_iface, best_5_high_sta_iface, best_5_low_sta_iface, best_5_sta_iface;

    // Support up to 256 scan results
    std::vector<bwl::SScanResult> scan_results;

    auto db = AgentDB::get();

    LOG(DEBUG) << "select_bssid: SSID = " << db->device_conf.back_radio.ssid;

    for (const auto &radio_info : m_radios_info) {

        if (radio_info->sta_iface.empty() || !radio_info->sta_wlan_hal) {
            LOG(DEBUG) << "skipping empty iface";
            continue;
        }

        std::string &iface = radio_info->sta_iface;

        LOG(DEBUG) << "select_bssid: iface  = " << iface;
        int num_of_results = radio_info->sta_wlan_hal->get_scan_results(
            db->device_conf.back_radio.ssid, scan_results);
        LOG(DEBUG) << "Scan Results: " << num_of_results;

        for (auto &scan_result : scan_results) {

            auto bssid = tlvf::mac_to_string(scan_result.bssid);
            LOG(DEBUG) << "select_bssid: bssid = " << bssid
                       << ", channel = " << int(scan_result.channel) << " iface = " << iface
                       << ", rssi=" << int(scan_result.rssi);

            auto ap_blacklist_it = ap_blacklist.find(bssid);
            if (ap_blacklist_it != ap_blacklist.end()) {
                ap_blacklist_entry &entry = ap_blacklist_it->second;
                if (std::chrono::steady_clock::now() >
                    (entry.timestamp + std::chrono::seconds(AP_BLACK_LIST_TIMEOUT_SECONDS))) {
                    LOG(DEBUG) << " bssid = " << bssid
                               << " aged and removed from backhaul blacklist";
                    ap_blacklist.erase(bssid);
                } else if (entry.attempts >= AP_BLACK_LIST_FAILED_ATTEMPTS_THRESHOLD) {
                    LOG(DEBUG) << " bssid = " << bssid << " is blacklisted, skipping";
                    continue;
                }
            }
            if (roam_flag) {
                if ((bssid == roam_selected_bssid) &&
                    (scan_result.channel == roam_selected_bssid_channel)) {
                    LOG(DEBUG) << "roaming flag on  - found bssid match = " << roam_selected_bssid
                               << " roam_selected_bssid_channel = "
                               << int(roam_selected_bssid_channel);
                    db->backhaul.selected_iface_name = iface;
                    return true;
                }
            } else if ((db->backhaul.preferred_bssid != beerocks::net::network_utils::ZERO_MAC) &&
                       (tlvf::mac_from_string(bssid) == db->backhaul.preferred_bssid)) {
                LOG(DEBUG) << "preferred bssid - found bssid match = " << bssid;
                selected_bssid_channel           = scan_result.channel;
                selected_bssid                   = bssid;
                db->backhaul.selected_iface_name = iface;
                return true;
            } else if (son::wireless_utils::which_freq(scan_result.channel) == eFreqType::FREQ_5G) {
                auto radio = db->radio(radio_info->sta_iface);
                if (!radio) {
                    return false;
                }
                if (radio->sta_iface_filter_low &&
                    son::wireless_utils::which_subband(scan_result.channel) ==
                        beerocks::LOW_SUBBAND) {
                    // iface with low filter - best low
                    if (scan_result.rssi > max_rssi_5_low) {
                        max_rssi_5_low           = scan_result.rssi;
                        best_bssid_5_low         = bssid;
                        best_bssid_channel_5_low = scan_result.channel;
                        best_5_low_sta_iface     = iface;
                    }

                } else if (!radio->sta_iface_filter_low &&
                           son::wireless_utils::which_subband(scan_result.channel) ==
                               beerocks::HIGH_SUBBAND) {
                    // iface without low filter (high filter or bypass) - best high
                    if (scan_result.rssi > max_rssi_5_high) {
                        max_rssi_5_high           = scan_result.rssi;
                        best_bssid_5_high         = bssid;
                        best_bssid_channel_5_high = scan_result.channel;
                        best_5_high_sta_iface     = iface;
                    }
                }

                if (scan_result.rssi > max_rssi_5_best) {
                    // best 5G (low/high)
                    max_rssi_5_best      = scan_result.rssi;
                    best_bssid_5         = bssid;
                    best_bssid_channel_5 = scan_result.channel;
                    best_5_sta_iface     = iface;
                }

            } else {
                // best 2.4G
                if (scan_result.rssi > max_rssi_24) {
                    max_rssi_24           = scan_result.rssi;
                    best_bssid_24         = bssid;
                    best_bssid_channel_24 = scan_result.channel;
                    best_24_sta_iface     = iface;
                }
            }
        }
    }

    if (!best_bssid_24.empty()) {
        LOG(DEBUG) << "BEST - 2.4Ghz          - " << best_24_sta_iface
                   << " - BSSID: " << best_bssid_24 << ", Channel: " << int(best_bssid_channel_24)
                   << ", RSSI: " << int(max_rssi_24);
    } else {
        LOG(DEBUG) << "BEST - 2.4Ghz          - Not Found!";
    }

    if (!best_bssid_5_low.empty()) {
        LOG(DEBUG) << "BEST - 5Ghz (Low)      - " << best_5_low_sta_iface
                   << " - BSSID: " << best_bssid_5_low
                   << ", Channel: " << int(best_bssid_channel_5_low)
                   << ", RSSI: " << int(max_rssi_5_low);
    } else {
        LOG(DEBUG) << "BEST - 5Ghz (Low)      - Not Found!";
    }

    if (!best_bssid_5_high.empty()) {
        LOG(DEBUG) << "BEST - 5Ghz (High)     - " << best_5_high_sta_iface
                   << " - BSSID: " << best_bssid_5_high
                   << ", Channel: " << int(best_bssid_channel_5_high)
                   << ", RSSI: " << int(max_rssi_5_high);
    } else {
        LOG(DEBUG) << "BEST - 5Ghz (High)     - Not Found!";
    }

    if (!best_bssid_5.empty()) {
        LOG(DEBUG) << "BEST - 5Ghz (Absolute) - " << best_5_sta_iface
                   << " - BSSID: " << best_bssid_5 << ", Channel: " << int(best_bssid_channel_5)
                   << ", RSSI: " << int(max_rssi_5_best);
    } else {
        LOG(DEBUG) << "BEST - 5Ghz (Absolute) - Not Found!";
    }

    if (max_rssi_5_high != beerocks::RSSI_INVALID &&
        (best_5_sta_iface == best_5_low_sta_iface || best_5_low_sta_iface.empty()) &&
        son::wireless_utils::which_subband(best_bssid_channel_5) == beerocks::HIGH_SUBBAND) {

        max_rssi_5_best      = max_rssi_5_high;
        best_bssid_5         = best_bssid_5_high;
        best_bssid_channel_5 = best_bssid_channel_5_high;
        best_5_sta_iface     = best_5_high_sta_iface;

    } else if (max_rssi_5_low != beerocks::RSSI_INVALID &&
               (best_5_sta_iface == best_5_high_sta_iface || best_5_high_sta_iface.empty()) &&
               son::wireless_utils::which_subband(best_bssid_channel_5) == beerocks::LOW_SUBBAND) {

        max_rssi_5_best      = max_rssi_5_low;
        best_bssid_5         = best_bssid_5_low;
        best_bssid_channel_5 = best_bssid_channel_5_low;
        best_5_sta_iface     = best_5_low_sta_iface;
    }

    if (!best_bssid_5.empty()) {
        LOG(DEBUG) << "Selected 5Ghz - " << best_5_sta_iface << " - BSSID: " << best_bssid_5
                   << ", Channel: " << int(best_bssid_channel_5)
                   << ", RSSI: " << int(max_rssi_5_best);
    } else {
        LOG(DEBUG) << "Selected 5Ghz - Not Found!";
    }

    // Select the base backhaul interface
    if (((max_rssi_24 == beerocks::RSSI_INVALID) && (max_rssi_5_best == beerocks::RSSI_INVALID)) ||
        roam_flag) {
        // TODO: ???
        return false;
    } else if (max_rssi_24 == beerocks::RSSI_INVALID) {
        selected_bssid                   = best_bssid_5;
        selected_bssid_channel           = best_bssid_channel_5;
        db->backhaul.selected_iface_name = best_5_sta_iface;
    } else if (max_rssi_5_best == beerocks::RSSI_INVALID) {
        selected_bssid                   = best_bssid_24;
        selected_bssid_channel           = best_bssid_channel_24;
        db->backhaul.selected_iface_name = best_24_sta_iface;
    } else if ((max_rssi_5_best > RSSI_THRESHOLD_5GHZ)) {
        selected_bssid                   = best_bssid_5;
        selected_bssid_channel           = best_bssid_channel_5;
        db->backhaul.selected_iface_name = best_5_sta_iface;
    } else if (max_rssi_24 < max_rssi_5_best + RSSI_BAND_DELTA_THRESHOLD) {
        selected_bssid                   = best_bssid_5;
        selected_bssid_channel           = best_bssid_channel_5;
        db->backhaul.selected_iface_name = best_5_sta_iface;
    } else {
        selected_bssid                   = best_bssid_24;
        selected_bssid_channel           = best_bssid_channel_24;
        db->backhaul.selected_iface_name = best_24_sta_iface;
    }

    if (!get_wireless_hal()) {
        LOG(ERROR) << "Slave for interface " << db->backhaul.selected_iface_name << " NOT found!";
        return false;
    }

    return true;
}

void BackhaulManager::get_scan_measurement()
{
    // Support up to 256 scan results
    std::vector<bwl::SScanResult> scan_results;
    auto db = AgentDB::get();

    LOG(DEBUG) << "get_scan_measurement: SSID = " << db->device_conf.back_radio.ssid;
    scan_measurement_list.clear();
    for (auto &radio_info : m_radios_info) {

        if (radio_info->sta_iface.empty()) {
            LOG(DEBUG) << "skipping empty iface";
            continue;
        }
        if (!radio_info->sta_wlan_hal) {
            continue;
        }

        std::string &iface = radio_info->sta_iface;
        LOG(DEBUG) << "get_scan_measurement: iface  = " << iface;
        int num_of_results = radio_info->sta_wlan_hal->get_scan_results(
            db->device_conf.back_radio.ssid, scan_results);
        LOG(DEBUG) << "Scan Results: " << int(num_of_results);
        if (num_of_results < 0) {
            LOG(ERROR) << "get_scan_results failed!";
            return;
        } else if (num_of_results == 0) {
            continue;
        }

        for (auto &scan_result : scan_results) {

            auto bssid = tlvf::mac_to_string(scan_result.bssid);
            LOG(DEBUG) << "get_scan_measurement: bssid = " << bssid
                       << ", channel = " << int(scan_result.channel) << " iface = " << iface;

            auto it = scan_measurement_list.find(bssid);
            if (it != scan_measurement_list.end()) {
                //updating rssi if stronger
                if (scan_result.rssi > it->second.rssi) {
                    LOG(DEBUG) << "updating scan rssi for bssid = " << bssid
                               << " channel = " << int(scan_result.channel)
                               << " rssi = " << int(it->second.rssi) << " to -> "
                               << int(scan_result.rssi);
                    it->second.rssi = scan_result.rssi;
                }
            } else {
                //insert new entry
                beerocks::net::sScanResult scan_measurement;

                scan_measurement.mac         = scan_result.bssid;
                scan_measurement.channel     = scan_result.channel;
                scan_measurement.rssi        = scan_result.rssi;
                scan_measurement_list[bssid] = scan_measurement;
                LOG(DEBUG) << "insert scan to list bssid = " << bssid
                           << " channel = " << int(scan_result.channel)
                           << " rssi = " << int(scan_result.rssi);
            }
        }
    }
}

std::shared_ptr<bwl::sta_wlan_hal> BackhaulManager::get_wireless_hal(std::string iface)
{
    // If the iface argument is empty, use the selected wireless interface
    auto db = AgentDB::get();
    if (iface.empty()) {
        iface = db->backhaul.selected_iface_name;
    }

    for (auto &radio_info : m_radios_info) {
        if (radio_info->sta_iface == iface) {
            return radio_info->sta_wlan_hal;
        }
    }
    return {};
}

bool BackhaulManager::handle_slave_failed_connection_message(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                             const sMacAddr &src_mac)
{
    // TODO: need to move this function to the link metrics task after the task will be moved to the
    // Agent context. PPM-1681.
    auto db = AgentDB::get();
    if (!db->link_metrics_policy.report_unsuccessful_associations) {
        // do nothing, no need to report
        return true;
    }

    // Calculate if reporting is needed
    auto now = std::chrono::steady_clock::now();
    auto elapsed_time_m =
        std::chrono::duration_cast<std::chrono::minutes>(
            now - db->link_metrics_policy.failed_association_last_reporting_time_point)
            .count();

    // Start the counting from the beginning if the last report was more then a minute ago also sets
    // the last reporting time to now.
    if (elapsed_time_m > 1) {
        db->link_metrics_policy.number_of_reports_in_last_minute             = 0;
        db->link_metrics_policy.failed_association_last_reporting_time_point = now;
    }

    if (db->link_metrics_policy.number_of_reports_in_last_minute >
        db->link_metrics_policy.failed_associations_maximum_reporting_rate) {
        // we exceeded the maximum reports allowed
        // do nothing, no need to report
        LOG(WARNING)
            << "received failed connection, but exceeded the maximum number of reports in a minute:"
            << db->link_metrics_policy.failed_associations_maximum_reporting_rate;
        return true;
    }

    // report
    db->link_metrics_policy.number_of_reports_in_last_minute++;

    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Sending FAILED_CONNECTION_MESSAGE, mid=" << std::hex << int(mid);

    return forward_cmdu_to_broker(cmdu_rx, db->controller_info.bridge_mac, db->bridge.mac,
                                  db->bridge.iface_name);
}

bool BackhaulManager::handle_backhaul_steering_request(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                       const sMacAddr &src_mac)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received BACKHAUL_STA_STEERING message, mid=" << std::hex << mid;

    auto bh_sta_steering_req = cmdu_rx.getClass<wfa_map::tlvBackhaulSteeringRequest>();
    if (!bh_sta_steering_req) {
        LOG(WARNING) << "Failed cmdu_rx.getClass<wfa_map::tlvBackhaulSteeringRequest>(), mid="
                     << std::hex << mid;
        return false;
    }

    // build ACK message CMDU
    auto cmdu_tx_header = cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }

    auto db = AgentDB::get();

    LOG(DEBUG) << "Sending ACK message to the originator, mid=" << std::hex << mid;
    send_cmdu_to_broker(cmdu_tx, db->controller_info.bridge_mac, db->bridge.mac);

    auto channel    = bh_sta_steering_req->target_channel_number();
    auto oper_class = bh_sta_steering_req->operating_class();
    auto bssid      = bh_sta_steering_req->target_bssid();

    auto is_valid_channel = son::wireless_utils::is_channel_in_operating_class(oper_class, channel);

    if (!is_valid_channel) {
        LOG(WARNING) << "Unable to steer to BSSID " << bssid
                     << ": Invalid channel number (oper_class=" << oper_class
                     << ", channel=" << channel << ")";

        auto response = create_backhaul_steering_response(
            wfa_map::tlvErrorCode::eReasonCode::
                BACKHAUL_STEERING_REQUEST_REJECTED_CANNOT_OPERATE_ON_CHANNEL_SPECIFIED,
            bssid);

        if (!response) {
            LOG(ERROR) << "Failed to build Backhaul Steering Response message.";
            return false;
        }

        send_cmdu_to_broker(cmdu_tx, db->controller_info.bridge_mac, db->bridge.mac);

        return false;
    }

    /*
        TODO: BACKHAUL_STA_STEERING can be accepted in wired backhaul too.
              Code below is incorrect in that case.
    */
    auto active_hal = get_wireless_hal();
    if (!active_hal) {
        LOG(ERROR) << "Couldn't get active HAL";
        return false;
    }

    // If current BSSID is the same as the specified target BSSID, then do not steer and send a
    // response immediately.
    if (tlvf::mac_from_string(active_hal->get_bssid()) == bssid) {
        LOG(WARNING) << "Current BSSID matches target BSSID " << bssid << ". No steering required.";

        auto response =
            create_backhaul_steering_response(wfa_map::tlvErrorCode::eReasonCode::RESERVED, bssid);

        if (!response) {
            LOG(ERROR) << "Failed to build Backhaul Steering Response message.";
            return false;
        }

        send_cmdu_to_broker(cmdu_tx, db->controller_info.bridge_mac, db->bridge.mac);

        return true;
    }

    // Trigger (asynchronously) a scan of the target BSSID on the target channel.
    // The steering itself will be done when the scan results are received.
    // If this function call fails for some reason (for example because a scan was already in
    // progress - We don't know if the supplicant returns a failure or not in that case), there is
    // still the possibility that we receive scan results and do the steering. Thus do not send a
    // Backhaul Steering Response message with "error" result code if this function fails nor return
    // false, just log a warning and let the execution continue. If we do not steer after the
    // timeout elapses, a response will anyway be sent to the controller.
    auto scan_result = active_hal->scan_bss(bssid, channel);
    if (!scan_result) {
        LOG(WARNING) << "Failed to scan for the target BSSID: " << bssid << " on channel "
                     << channel << ".";
    }

    // If a timer exists already, it means that there is a steering on-going. What are we supposed
    // to do in such a case? Send the response for the first request immediately? Or send only a
    // response for the second request?
    // This doesn't seem to be specified. Thus, what we do here is OK: only send a response for the
    // second request.
    if (m_backhaul_steering_timer != beerocks::net::FileDescriptor::invalid_descriptor) {
        cancel_backhaul_steering_operation();
    }

    // We should only send a Backhaul Steering Response message with "success" result code if we
    // succeed to associate with the specified BSSID within 10 seconds.
    // Set the channel and BSSID of the target BSS so we can use them later.
    m_backhaul_steering_bssid   = bssid;
    m_backhaul_steering_channel = channel;

    // Create a timer to check if this Backhaul Steering Request times out.
    m_backhaul_steering_timer = m_timer_manager->add_timer(
        "Backhaul Steering Timeout", backhaul_steering_timeout, std::chrono::milliseconds::zero(),
        [&](int fd, beerocks::EventLoop &loop) {
            cancel_backhaul_steering_operation();

            // We'll end up in this situation only if an attempt to scan and associate was made, but
            // we didn't manage to actually connect within 10 seconds, so that's probably indicative
            // of bad reception.
            // There is no suitable reason code for "timeout" so "target BSS signal not suitable" is
            // used as it seems to be the more appropriate of all the reason codes available.
            create_backhaul_steering_response(
                wfa_map::tlvErrorCode::eReasonCode::
                    BACKHAUL_STEERING_REQUEST_REJECTED_TARGET_BSS_SIGNAL_NOT_SUITABLE,
                bssid);

            LOG(DEBUG)
                << "Steering request timed out. Sending BACKHAUL_STA_STEERING_RESPONSE_MESSAGE";
            auto db = AgentDB::get();
            send_cmdu_to_broker(cmdu_tx, db->controller_info.bridge_mac, db->bridge.mac);
            return true;
        });
    if (m_backhaul_steering_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(ERROR) << "Failed to create the backhaul steering request timeout timer";
        return false;
    }
    LOG(DEBUG) << "Backhaul steering request timeout timer created with fd = "
               << m_backhaul_steering_timer;
    return true;
}

bool BackhaulManager::create_backhaul_steering_response(
    wfa_map::tlvErrorCode::eReasonCode error_code, const sMacAddr &target_bssid)
{
    auto cmdu_tx_header =
        cmdu_tx.create(0, ieee1905_1::eMessageType::BACKHAUL_STEERING_RESPONSE_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "Failed to create Backhaul Steering Response message";
        return false;
    }

    auto bh_steering_resp_tlv = cmdu_tx.addClass<wfa_map::tlvBackhaulSteeringResponse>();
    if (!bh_steering_resp_tlv) {
        LOG(ERROR) << "Couldn't addClass<wfa_map::tlvBackhaulSteeringResponse>";
        return false;
    }

    auto active_hal = get_wireless_hal();
    if (!active_hal) {
        LOG(ERROR) << "Couldn't get active HAL";
        return false;
    }

    auto interface = active_hal->get_iface_name();

    auto db = AgentDB::get();

    auto radio = db->radio(interface);
    if (!radio) {
        return false;
    }

    sMacAddr sta_mac = radio->back.iface_mac;

    LOG(DEBUG) << "Interface: " << interface << " MAC: " << sta_mac
               << " Target BSSID: " << target_bssid;

    bh_steering_resp_tlv->target_bssid()         = target_bssid;
    bh_steering_resp_tlv->backhaul_station_mac() = sta_mac;

    if (!error_code) {
        bh_steering_resp_tlv->result_code() =
            wfa_map::tlvBackhaulSteeringResponse::eResultCode::SUCCESS;
    } else {
        bh_steering_resp_tlv->result_code() =
            wfa_map::tlvBackhaulSteeringResponse::eResultCode::FAILURE;

        auto error_code_tlv = cmdu_tx.addClass<wfa_map::tlvErrorCode>();
        if (!bh_steering_resp_tlv) {
            LOG(ERROR) << "Couldn't addClass<wfa_map::tlvErrorCode>";
            return false;
        }

        error_code_tlv->reason_code() = error_code;
    }

    return true;
}

void BackhaulManager::cancel_backhaul_steering_operation()
{
    m_backhaul_steering_bssid   = beerocks::net::network_utils::ZERO_MAC;
    m_backhaul_steering_channel = 0;

    m_timer_manager->remove_timer(m_backhaul_steering_timer);
}

std::string BackhaulManager::freq_to_radio_mac(eFreqType freq) const
{
    auto db = AgentDB::get();
    for (const auto radio : db->get_radios_list()) {
        if (!radio) {
            continue;
        }
        if (radio->freq_type == freq) {
            return tlvf::mac_to_string(radio->front.iface_mac);
        }
    }

    LOG(ERROR) << "Radio not found for freq " << int(freq);
    return {};
}

bool BackhaulManager::start_wps_pbc(const sMacAddr &radio_mac)
{
    if ((m_eFSMState == EState::OPERATIONAL)) {
        auto it = std::find_if(m_radios_info.begin(), m_radios_info.end(),
                               [&](std::shared_ptr<sRadioInfo> radio_info) {
                                   return radio_info->radio_mac == radio_mac;
                               });
        if (it == m_radios_info.end()) {
            LOG(ERROR) << "couldn't find slave for radio mac " << radio_mac;
            return false;
        }

        // Store the socket to the slave managing the requested radio
        auto &radio_info = *it;
        // WPS PBC registration on AP interface
        auto msg = message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_START_WPS_PBC_REQUEST>(cmdu_tx);
        if (!msg) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        msg->set_iface(radio_info->hostap_iface);
        LOG(DEBUG) << "Start WPS PBC registration on interface " << radio_info->hostap_iface;
        return send_cmdu(m_agent_fd, cmdu_tx);
    } else {
        // WPS PBC registration on STA interface
        auto sta_wlan_hal = get_selected_backhaul_sta_wlan_hal();
        if (!sta_wlan_hal) {
            LOG(ERROR) << "Failed to get backhaul STA hal";
            return false;
        }

        // Disable radio interface to make sure its not beaconing along while the supplicant is
        // scanning.Disable rest of radio interfaces to prevent stations from connecting (there is
        // no BH link anyway).
        // This is a temporary solution for axepoint (prplwrt) in order to pass wbh easymesh
        // certification tests (Need to be removed once PPM-643 or PPM-1580 are solved)
        for (const auto &radio_info : m_radios_info) {
            auto msg = message_com::create_vs_message<
                beerocks_message::cACTION_BACKHAUL_RADIO_DISABLE_REQUEST>(cmdu_tx);
            if (!msg) {
                LOG(ERROR) << "Failed building cACTION_BACKHAUL_RADIO_DISABLE_REQUEST";
                return false;
            }

            msg->set_iface(radio_info->hostap_iface);
            LOG(DEBUG) << "Request Agent to disable the radio interface "
                       << radio_info->hostap_iface << " before WPS starts";
            if (!send_cmdu(m_agent_fd, cmdu_tx)) {
                LOG(ERROR) << "Failed to send cACTION_BACKHAUL_RADIO_DISABLE_REQUEST";
                return false;
            }
            UTILS_SLEEP_MSEC(3000);
        }
        if (!sta_wlan_hal->start_wps_pbc()) {
            LOG(ERROR) << "Failed to start wps";
            return false;
        }
        return true;
    }
}

bool BackhaulManager::set_mbo_assoc_disallow(const sMacAddr &radio_mac, const sMacAddr &bssid,
                                             bool enable)
{
    auto it = std::find_if(
        m_radios_info.begin(), m_radios_info.end(),
        [&](std::shared_ptr<sRadioInfo> radio_info) { return radio_info->radio_mac == radio_mac; });
    if (it == m_radios_info.end()) {
        LOG(ERROR) << "couldn't find slave for radio mac " << radio_mac;
        return false;
    }

    // Store the socket to the slave managing the requested radio
    const auto &radio_info = *it;

    auto msg = message_com::create_vs_message<
        beerocks_message::cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST>(cmdu_tx);
    if (!msg) {
        LOG(ERROR) << "Failed building message!";
        return false;
    }

    msg->enable() = enable;
    msg->bssid()  = bssid;

    // Filling the radio mac. This is temporary until UCC listener will be moved to agent (PPM-1678)
    auto action_header         = message_com::get_beerocks_header(cmdu_tx)->actionhdr();
    action_header->radio_mac() = radio_mac;

    LOG(DEBUG) << "Set MBO ASSOC_DISALLOW on interface " << radio_info->hostap_iface << " to "
               << enable;
    send_cmdu(m_agent_fd, cmdu_tx);

    if (!cmdu_tx.create(0, ieee1905_1::eMessageType::ASSOCIATION_STATUS_NOTIFICATION_MESSAGE)) {
        LOG(ERROR) << "Failed building message!";
        return false;
    }

    auto profile2_association_status_notification_tlv =
        cmdu_tx.addClass<wfa_map::tlvProfile2AssociationStatusNotification>();
    if (!profile2_association_status_notification_tlv) {
        LOG(ERROR) << "addClass failed";
        return false;
    }

    profile2_association_status_notification_tlv->alloc_bssid_status_list();
    auto bssid_status_tuple = profile2_association_status_notification_tlv->bssid_status_list(0);

    if (!std::get<0>(bssid_status_tuple)) {
        LOG(ERROR) << "getting bssid status has failed!";
        return false;
    }

    auto &bssid_status = std::get<1>(bssid_status_tuple);

    bssid_status.bssid = bssid;
    bssid_status.association_allowance_status =
        enable ? wfa_map::tlvProfile2AssociationStatusNotification::eAssociationAllowanceStatus::
                     NO_MORE_ASSOCIATIONS_ALLOWED
               : wfa_map::tlvProfile2AssociationStatusNotification::eAssociationAllowanceStatus::
                     ASSOCIATIONS_ALLOWED;

    auto db = AgentDB::get();
    send_cmdu_to_broker(cmdu_tx, db->controller_info.bridge_mac, db->bridge.mac);

    return true;
}

std::shared_ptr<bwl::sta_wlan_hal> BackhaulManager::get_selected_backhaul_sta_wlan_hal()
{
    auto selected_backhaul_it =
        std::find_if(m_radios_info.begin(), m_radios_info.end(),
                     [&](const std::shared_ptr<sRadioInfo> &radio_info) {
                         return tlvf::mac_from_string(m_selected_backhaul) == radio_info->radio_mac;
                     });
    if (selected_backhaul_it == m_radios_info.end()) {
        LOG(ERROR) << "Invalid backhaul";
        return nullptr;
    }
    return (*selected_backhaul_it)->sta_wlan_hal;
}

void BackhaulManager::handle_dev_reset_default(
    int fd, const std::unordered_map<std::string, std::string> &params)
{
    // Certification tests will do "dev_reset_default" multiple times without "dev_set_config" in
    // between. In that case, do nothing but reply.
    if (m_is_in_reset_state) {
        // Send back second reply to UCC client.
        m_agent_ucc_listener->send_reply(fd);
        return;
    }

    // Store socket descriptor to send reply to UCC client when command processing is completed.
    m_dev_reset_default_fd = fd;

    // Get the HAL for the connected wireless interface and, if any, disconnect the interface
    auto active_hal = get_wireless_hal();
    if (active_hal) {
        active_hal->disconnect();
    }

    // Add wired interface to the bridge
    // It will be removed later on (dev_set_config) in case of wireless backhaul connection is needed.
    auto db            = AgentDB::get();
    auto bridge        = db->bridge.iface_name;
    auto bridge_ifaces = beerocks::net::network_utils::linux_get_iface_list_from_bridge(bridge);
    auto eth_iface     = db->ethernet.wan.iface_name;

    //check if wired interface is enabled or try to enable it.
    if (!beerocks::net::network_utils::linux_iface_is_up_and_running(eth_iface)) {
        LOG(INFO) << "The wired interface " << eth_iface << " is not up, lets try to enable it";
        beerocks::net::network_utils::set_interface_state(eth_iface, true);

        UTILS_SLEEP_MSEC(1000);
        if (!beerocks::net::network_utils::linux_iface_is_up_and_running(eth_iface)) {
            LOG(ERROR) << "The wired interface is not yet running after 1 sec";
            m_agent_ucc_listener->send_reply(
                fd, beerocks::beerocks_ucc_listener::command_failed_error_string);
            return;
        }
    }

    if (std::find(bridge_ifaces.begin(), bridge_ifaces.end(), eth_iface) != bridge_ifaces.end()) {
        LOG(INFO) << "The wired interface is already in the bridge";
    } else {
        if (!beerocks::net::network_utils::linux_add_iface_to_bridge(bridge, eth_iface)) {
            LOG(ERROR) << "Failed to add iface '" << eth_iface << "' to bridge '" << bridge
                       << "' !";
            m_agent_ucc_listener->send_reply(
                fd, beerocks::beerocks_ucc_listener::command_failed_error_string);
            return;
        }
    }

    m_dev_reset_default_timer = m_timer_manager->add_timer(
        "Dev Reset Default",
        std::chrono::duration_cast<std::chrono::milliseconds>(dev_reset_default_timeout),
        std::chrono::milliseconds::zero(), [&](int fd, beerocks::EventLoop &loop) {
            m_timer_manager->remove_timer(m_dev_reset_default_timer);

            LOG(DEBUG) << "dev_reset_default timed out";

            m_agent_ucc_listener->send_reply(m_dev_reset_default_fd, "Timeout");

            return true;
        });
    if (m_dev_reset_default_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(ERROR) << "Failed to create the dev_reset_default timeout timer";
        m_agent_ucc_listener->send_reply(
            fd, beerocks::beerocks_ucc_listener::command_failed_error_string);
        return;
    }

    m_selected_backhaul           = "";
    m_is_in_reset_state           = true;
    m_dev_reset_default_completed = false;
}

bool BackhaulManager::handle_dev_set_config(
    const std::unordered_map<std::string, std::string> &params, std::string &err_string)
{
    if (!m_is_in_reset_state) {
        err_string = "Command not expected at this moment";
        return false;
    }

    if (params.find("bss_info") != params.end()) {
        err_string = "parameter 'bss_info' is not relevant to the agent";
        return false;
    }

    if (params.find("backhaul") == params.end()) {
        err_string = "parameter 'backhaul' is missing";
        return false;
    }

    // Get the selected backhaul specified in received command.
    auto backhaul = params.at("backhaul");
    std::transform(backhaul.begin(), backhaul.end(), backhaul.begin(), ::tolower);
    if (backhaul == DEV_SET_ETH) {
        // wired backhaul connection.
        m_selected_backhaul = DEV_SET_ETH;
    } else {
        // wireless backhaul connection.
        // backhaul param must be a radio UID, in hex, starting with 0x
        sMacAddr backhaul_radio_uid = net::network_utils::ZERO_MAC;
        const std::string hex_prefix{"0x"};
        const size_t radio_uid_size = hex_prefix.size() + 2 * sizeof(backhaul_radio_uid.oct);
        if ((backhaul.substr(0, 2) != hex_prefix) || (backhaul.size() != radio_uid_size) ||
            backhaul.find_first_not_of("0123456789abcdef", 2) != std::string::npos) {
            err_string = "parameter 'backhaul' is not 'eth' nor MAC address";
            return false;
        }
        for (size_t idx = 0; idx < sizeof(backhaul_radio_uid.oct); idx++) {
            backhaul_radio_uid.oct[idx] = std::stoul(backhaul.substr(2 + 2 * idx, 2), 0, 16);
        }
        m_selected_backhaul = tlvf::mac_to_string(backhaul_radio_uid);

        // remove wired (ethernet) interface from the bridge
        auto db            = AgentDB::get();
        auto bridge        = db->bridge.iface_name;
        auto bridge_ifaces = beerocks::net::network_utils::linux_get_iface_list_from_bridge(bridge);
        auto eth_iface     = db->ethernet.wan.iface_name;
        if (std::find(bridge_ifaces.begin(), bridge_ifaces.end(), eth_iface) !=
            bridge_ifaces.end()) {
            if (!beerocks::net::network_utils::linux_remove_iface_from_bridge(bridge, eth_iface)) {
                LOG(ERROR) << "Failed to remove interface '" << eth_iface << "' from bridge '"
                           << bridge << "' !";
                return false;
            }
        } else {
            LOG(INFO) << "Interface '" << eth_iface << "' not found in bridge '" << bridge << "' !";
        }

        // Disable wired interface for wireless backhaul connection.
        // It will be enabled back if in case we want to establish wired bh connection.
        beerocks::net::network_utils::set_interface_state(eth_iface, false);

        UTILS_SLEEP_MSEC(1000);
        if (beerocks::net::network_utils::linux_iface_is_up_and_running(eth_iface)) {
            LOG(ERROR) << "The wired interface is not yet down after 1 sec";
            return false;
        }
    }

    // Signal to backhaul manager that it can continue onboarding.
    m_is_in_reset_state = false;

    return true;
}

} // namespace beerocks
