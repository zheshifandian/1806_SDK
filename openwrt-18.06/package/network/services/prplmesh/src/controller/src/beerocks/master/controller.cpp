/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "controller.h"
#include "periodic/persistent_data_commit_operation.h"
#include "periodic/persistent_database_aging.h"
#include "son_actions.h"
#include "son_management.h"
#include "tasks/agent_monitoring_task.h"
#include "tasks/bml_task.h"
#include "tasks/channel_selection_task.h"
#include "tasks/client_association_task.h"
#include "tasks/client_steering_task.h"
#include "tasks/dhcp_task.h"
#include "tasks/load_balancer_task.h"
#include "tasks/optimal_path_task.h"
#include "tasks/statistics_polling_task.h"
#include "tasks/topology_task.h"
#ifdef BEEROCKS_RDKB
#include "tasks/rdkb/rdkb_wlan_task.h"
#endif
#include "db/db_algo.h"
#include "db/network_map.h"
#include "tasks/client_locating_task.h"
#include "tasks/dynamic_channel_selection_r2_task.h"
#include "tasks/dynamic_channel_selection_task.h"
#include "tasks/network_health_check_task.h"

#include <bcl/beerocks_backport.h>
#include <bcl/beerocks_utils.h>
#include <bcl/beerocks_version.h>
#include <bcl/network/sockets.h>
#include <bcl/son/son_wireless_utils.h>
#include <bcl/transaction.h>
#include <bpl/bpl_board.h>
#include <bpl/bpl_cfg.h>

#include <easylogging++.h>

#include <beerocks/tlvf/beerocks_message_1905_vs.h>
#include <beerocks/tlvf/beerocks_message_control.h>

#include <tlvf/AssociationRequestFrame/AssocReqFrame.h>
#include <tlvf/ieee_1905_1/eMessageType.h>
#include <tlvf/ieee_1905_1/tlvAlMacAddress.h>
#include <tlvf/ieee_1905_1/tlvAutoconfigFreqBand.h>
#include <tlvf/ieee_1905_1/tlvEndOfMessage.h>
#include <tlvf/ieee_1905_1/tlvSearchedRole.h>
#include <tlvf/ieee_1905_1/tlvSupportedFreqBand.h>
#include <tlvf/ieee_1905_1/tlvSupportedRole.h>
#include <tlvf/wfa_map/tlvApExtendedMetrics.h>
#include <tlvf/wfa_map/tlvApMetrics.h>
#include <tlvf/wfa_map/tlvApRadioIdentifier.h>
#include <tlvf/wfa_map/tlvApVhtCapabilities.h>
#include <tlvf/wfa_map/tlvAssociatedStaLinkMetrics.h>
#include <tlvf/wfa_map/tlvAssociatedStaTrafficStats.h>
#include <tlvf/wfa_map/tlvBackhaulSteeringResponse.h>
#include <tlvf/wfa_map/tlvChannelPreference.h>
#include <tlvf/wfa_map/tlvChannelScanCapabilities.h>
#include <tlvf/wfa_map/tlvChannelSelectionResponse.h>
#include <tlvf/wfa_map/tlvClientCapabilityReport.h>
#include <tlvf/wfa_map/tlvClientInfo.h>
#include <tlvf/wfa_map/tlvErrorCode.h>
#include <tlvf/wfa_map/tlvHigherLayerData.h>
#include <tlvf/wfa_map/tlvOperatingChannelReport.h>
#include <tlvf/wfa_map/tlvProfile2ApCapability.h>
#include <tlvf/wfa_map/tlvProfile2CacStatusReport.h>
#include <tlvf/wfa_map/tlvProfile2ChannelScanResult.h>
#include <tlvf/wfa_map/tlvProfile2MultiApProfile.h>
#include <tlvf/wfa_map/tlvProfile2RadioMetrics.h>
#include <tlvf/wfa_map/tlvProfile2ReasonCode.h>
#include <tlvf/wfa_map/tlvProfile2StatusCode.h>
#include <tlvf/wfa_map/tlvRadioOperationRestriction.h>
#include <tlvf/wfa_map/tlvSearchedService.h>
#include <tlvf/wfa_map/tlvStaMacAddressType.h>
#include <tlvf/wfa_map/tlvSteeringBTMReport.h>
#include <tlvf/wfa_map/tlvSupportedService.h>
#include <tlvf/wfa_map/tlvTimestamp.h>
#include <tlvf/wfa_map/tlvTunnelledData.h>
#include <tlvf/wfa_map/tlvTunnelledProtocolType.h>
#include <tlvf/wfa_map/tlvTunnelledSourceInfo.h>

#include <net/if.h> // if_nametoindex

extern "C" {
#include <uci.h>
}

namespace son {

/**
 * Time between successive timer executions of the tasks timer
 */
constexpr auto tasks_timer_period = std::chrono::milliseconds(250);

/**
 * Time between successive timer executions of the operations timer
 */
constexpr auto operations_timer_period = std::chrono::milliseconds(1000);

static bool uci_get_option_value_controller(char *conf_path, std::string &value)
{
    struct uci_context *ctx = uci_alloc_context();
    if (!ctx) {
        return false;
    }

    uci_ptr ptr;
    if (uci_lookup_ptr(ctx, &ptr, conf_path, true) != UCI_OK || !ptr.o) {
        LOG(ERROR) << "UCI failed to lookup ptr for path: " << conf_path << std::endl;
        return false;
    }
    value = ptr.o->v.string;

    return true;
}

Controller::Controller(db &database_,
                       std::unique_ptr<beerocks::btl::BrokerClientFactory> broker_client_factory,
                       std::unique_ptr<beerocks::UccServer> ucc_server,
                       std::unique_ptr<beerocks::CmduServer> cmdu_server,
                       std::shared_ptr<beerocks::TimerManager> timer_manager,
                       std::shared_ptr<beerocks::EventLoop> event_loop)
    : cmdu_tx(m_tx_buffer, sizeof(m_tx_buffer)),
      cert_cmdu_tx(m_cert_tx_buffer, sizeof(m_cert_tx_buffer)), database(database_),
      m_controller_ucc_listener(database_, cert_cmdu_tx, std::move(ucc_server)),
      m_broker_client_factory(std::move(broker_client_factory)),
      m_cmdu_server(std::move(cmdu_server)), m_timer_manager(timer_manager),
      m_event_loop(event_loop)
{
    LOG_IF(!m_broker_client_factory, FATAL) << "Broker client factory is a null pointer!";
    LOG_IF(!m_cmdu_server, FATAL) << "CMDU server is a null pointer!";
    LOG_IF(!m_timer_manager, FATAL) << "Timer manager is a null pointer!";
    LOG_IF(!m_event_loop, FATAL) << "Event loop is a null pointer!";

    database.set_controller_ctx(this);

#ifndef BEEROCKS_LINUX
    if (database.settings_diagnostics_measurements()) {
        LOG_IF(!tasks.add_task(std::make_shared<statistics_polling_task>(database, cmdu_tx, tasks)),
               FATAL)
            << "Failed adding statistics polling task!";
    }
#endif

    LOG_IF(!tasks.add_task(std::make_shared<bml_task>(database, cmdu_tx, tasks)), FATAL)
        << "Failed adding BML task!";

    LOG_IF(!tasks.add_task(std::make_shared<channel_selection_task>(database, cmdu_tx, tasks)),
           FATAL)
        << "Failed adding channel selection task!";

    LOG_IF(!tasks.add_task(
               std::make_shared<dynamic_channel_selection_r2_task>(database, cmdu_tx, tasks)),
           FATAL)
        << "Failed adding dynamic channel selection r2 task!";

    LOG_IF(!tasks.add_task(std::make_shared<topology_task>(database, cmdu_tx, tasks)), FATAL)
        << "Failed adding topology task!";

    LOG_IF(!tasks.add_task(std::make_shared<client_association_task>(database, cmdu_tx, tasks)),
           FATAL)
        << "Failed adding client association task!";

    LOG_IF(!tasks.add_task(std::make_shared<agent_monitoring_task>(database, cmdu_tx, tasks)),
           FATAL)
        << "Failed adding agent monitoring task!";

    if (database.settings_health_check()) {
        LOG_IF(!tasks.add_task(
                   std::make_shared<network_health_check_task>(database, cmdu_tx, tasks, 0)),
               FATAL)
            << "Failed adding network health check task!";
    } else {
        LOG(DEBUG) << "Health check is DISABLED!";
    }
    if (database.config.management_mode != BPL_MGMT_MODE_NOT_MULTIAP) {
        m_link_metrics_task =
            std::make_shared<LinkMetricsTask>(database, cmdu_tx, cert_cmdu_tx, tasks);
        LOG_IF(!tasks.add_task(m_link_metrics_task), FATAL) << "Failed adding link metrics task!";
    }

    LOG_IF(!tasks.add_task(std::make_shared<DhcpTask>(database, timer_manager)), FATAL)
        << "Failed adding dhcp task!";

    beerocks::CmduServer::EventHandlers handlers{
        .on_client_connected    = [&](int fd) { handle_connected(fd); },
        .on_client_disconnected = [&](int fd) { handle_disconnected(fd); },
        .on_cmdu_received =
            [&](int fd, uint32_t iface_index, const sMacAddr &dst_mac, const sMacAddr &src_mac,
                ieee1905_1::CmduMessageRx &cmdu_rx) {
                handle_cmdu(fd, iface_index, dst_mac, src_mac, cmdu_rx);
            },
    };
    m_cmdu_server->set_handlers(handlers);
}

Controller::~Controller()
{
    m_cmdu_server->clear_handlers();

    LOG(DEBUG) << "closing";
}

bool Controller::start()
{
    // In case of error in one of the steps of this method, we have to undo all the previous steps
    // (like when rolling back a database transaction, where either all steps get executed or none
    // of them gets executed)
    beerocks::Transaction transaction;

    LOG(DEBUG) << "persistent db enable=" << database.config.persistent_db;
    if (database.config.persistent_db) {
        LOG(DEBUG) << "loading clients from persistent db";
        if (!database.load_persistent_db_clients()) {
            LOG(WARNING) << "failed to load clients from persistent db";
        } else {
            LOG(DEBUG) << "load clients from persistent db finished successfully";
        }
        if (!database.restore_steer_history()) {
            LOG(WARNING) << "Failed to load steer history from persistent db or no entries found.";
        } else {
            LOG(DEBUG) << "Load steer history from persistent db finished successfully";
        }

        if (operations.is_operation_alive(database.get_persistent_db_aging_operation_id())) {
            LOG(DEBUG) << "persistent DB aging operation already running";
        } else {
            auto aging_interval_seconds =
                std::chrono::seconds(database.config.persistent_db_aging_interval);
            auto new_operation = std::make_shared<persistent_database_aging_operation>(
                aging_interval_seconds, database);
            operations.add_operation(new_operation);
        }

        if (operations.is_operation_alive(database.get_persistent_db_data_commit_operation_id())) {
            LOG(DEBUG) << "persistent DB data commit operation already running";
        } else {
            auto commit_interval_seconds =
                std::chrono::seconds(database.config.persistent_db_commit_changes_interval_seconds);
            auto commit_operation = std::make_shared<persistent_data_commit_operation>(
                database, commit_interval_seconds);
            operations.add_operation(commit_operation);
        }
    }

    // GW & GW Switch nodes are need to be added in case of Controller only mode
    // Normally node/database objects are added with SLAVE JOIN messages
    // In case of Controller only mode, prplMesh agent will not start JOIN process.
    if (database.config.management_mode == BPL_MGMT_MODE_MULTIAP_CONTROLLER) {
        LOG(INFO) << "Controller only Mode is selected. Add GW node to database";

        auto agent = database.m_agents.add(database.get_local_bridge_mac());
        database.set_prplmesh(database.get_local_bridge_mac());
        database.set_agent_manufacturer(*agent, "prplMesh");
        agent->is_gateway = true;

        auto eth_switch_mac = beerocks::net::network_utils::get_eth_sw_mac_from_bridge_mac(
            database.get_local_bridge_mac());
        auto eth_switch_mac_str = tlvf::mac_to_string(eth_switch_mac);
        database.add_node_wired_backhaul(eth_switch_mac, database.get_local_bridge_mac());
        database.set_node_state(eth_switch_mac_str, beerocks::STATE_CONNECTED);
        database.set_node_name(eth_switch_mac_str, "GW_CONTROLLER_ETH");
        database.set_node_manufacturer(eth_switch_mac_str, agent->manufacturer);
    }

    // Create a timer to run internal tasks periodically
    m_tasks_timer = m_timer_manager->add_timer(
        "Controller Tasks", tasks_timer_period, tasks_timer_period,
        [&](int fd, beerocks::EventLoop &loop) {
            // Allow tasks to execute up to 80% of the timer period
            tasks.run_tasks(int(double(tasks_timer_period.count()) * 0.8));
            return true;
        });
    if (m_tasks_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(ERROR) << "Failed to create the tasks timer";
        return false;
    }
    LOG(DEBUG) << "Tasks timer created with fd = " << m_tasks_timer;
    transaction.add_rollback_action([&]() { m_timer_manager->remove_timer(m_tasks_timer); });

    // Create a timer to execute periodic operations
    // TODO: as an enhancement, each periodic operation should have its own timer (PPM-717)
    m_operations_timer =
        m_timer_manager->add_timer("Periodic Operations", operations_timer_period,
                                   operations_timer_period, [&](int fd, beerocks::EventLoop &loop) {
                                       operations.run_operations();
                                       return true;
                                   });
    if (m_operations_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(ERROR) << "Failed to create the operations timer";
        return false;
    }
    LOG(DEBUG) << "Operations timer created with fd = " << m_operations_timer;
    transaction.add_rollback_action([&]() { m_timer_manager->remove_timer(m_operations_timer); });

    // Create an instance of a broker client connected to the broker server that is running in the
    // transport process
    m_broker_client = m_broker_client_factory->create_instance();
    if (!m_broker_client) {
        LOG(ERROR) << "Failed to create instance of broker client";
        return false;
    }
    transaction.add_rollback_action([&]() { m_broker_client.reset(); });

    beerocks::btl::BrokerClient::EventHandlers handlers;
    // Install a CMDU-received event handler for CMDU messages received from the transport process.
    // These messages are actually been sent by a remote process and the broker server running in
    // the transport process just forwards them to the broker client.
    handlers.on_cmdu_received = [&](uint32_t iface_index, const sMacAddr &dst_mac,
                                    const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx) {
        handle_cmdu_from_broker(iface_index, dst_mac, src_mac, cmdu_rx);
    };

    // Install a connection-closed event handler.
    // Currently there is no recovery mechanism if connection with broker server gets interrupted
    // (something that happens if the transport process dies). Just log a message and exit
    handlers.on_connection_closed = [&]() {
        LOG(ERROR) << "Broker client got disconnected!";
        return false;
    };

    m_broker_client->set_handlers(handlers);
    transaction.add_rollback_action([&]() { m_broker_client->clear_handlers(); });

    // Subscribe for the reception of CMDU messages that this process is interested in
    if (!m_broker_client->subscribe(std::set<ieee1905_1::eMessageType>{
            ieee1905_1::eMessageType::ACK_MESSAGE,
            ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_SEARCH_MESSAGE,
            ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_WSC_MESSAGE,
            ieee1905_1::eMessageType::AP_CAPABILITY_REPORT_MESSAGE,
            ieee1905_1::eMessageType::AP_METRICS_RESPONSE_MESSAGE,
            ieee1905_1::eMessageType::BEACON_METRICS_RESPONSE_MESSAGE,
            ieee1905_1::eMessageType::CHANNEL_PREFERENCE_REPORT_MESSAGE,
            ieee1905_1::eMessageType::CHANNEL_SELECTION_RESPONSE_MESSAGE,
            ieee1905_1::eMessageType::CHANNEL_SCAN_REPORT_MESSAGE,
            ieee1905_1::eMessageType::CLIENT_CAPABILITY_REPORT_MESSAGE,
            ieee1905_1::eMessageType::CLIENT_STEERING_BTM_REPORT_MESSAGE,
            ieee1905_1::eMessageType::HIGHER_LAYER_DATA_MESSAGE,
            ieee1905_1::eMessageType::LINK_METRIC_RESPONSE_MESSAGE,
            ieee1905_1::eMessageType::OPERATING_CHANNEL_REPORT_MESSAGE,
            ieee1905_1::eMessageType::STEERING_COMPLETED_MESSAGE,
            ieee1905_1::eMessageType::TOPOLOGY_NOTIFICATION_MESSAGE,
            ieee1905_1::eMessageType::TOPOLOGY_RESPONSE_MESSAGE,
            ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE,
            ieee1905_1::eMessageType::BACKHAUL_STEERING_RESPONSE_MESSAGE,
            ieee1905_1::eMessageType::TUNNELLED_MESSAGE,
            ieee1905_1::eMessageType::FAILED_CONNECTION_MESSAGE,
        })) {
        LOG(ERROR) << "Failed subscribing to the Bus";
        return false;
    }

    transaction.commit();

    LOG(DEBUG) << "started";

    return true;
}

bool Controller::stop()
{
    bool ok = true;

    if (m_broker_client) {
        m_broker_client->clear_handlers();
        m_broker_client.reset();
    }

    if (m_operations_timer != beerocks::net::FileDescriptor::invalid_descriptor) {
        if (!m_timer_manager->remove_timer(m_operations_timer)) {
            ok = false;
        }
    }

    if (m_tasks_timer != beerocks::net::FileDescriptor::invalid_descriptor) {
        if (!m_timer_manager->remove_timer(m_tasks_timer)) {
            ok = false;
        }
    }

    LOG(DEBUG) << "stopped";

    return ok;
}

bool Controller::send_cmdu(int fd, ieee1905_1::CmduMessageTx &cmdu_tx)
{
    return m_cmdu_server->send_cmdu(fd, cmdu_tx);
}

bool Controller::send_cmdu_to_broker(ieee1905_1::CmduMessageTx &cmdu_tx, const sMacAddr &dst_mac,
                                     const sMacAddr &src_mac, const std::string &iface_name)
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

void Controller::handle_connected(int fd) { LOG(INFO) << "UDS socket connected, fd = " << fd; }

void Controller::handle_disconnected(int fd)
{
    LOG(INFO) << "UDS socket disconnected, fd = " << fd;

    // Removing the socket only from the vector of socket in the database if exists,
    // not from socket thread.
    database.remove_cli_socket(fd);
    database.remove_bml_socket(fd);

#ifdef BEEROCKS_RDKB
    if (database.settings_rdkb_extensions()) {
        //TODO - use rdkb_wlan_hal_db instead of task event
        rdkb_wlan_task::listener_general_register_unregister_event new_event;
        new_event.sd = fd;
        tasks.push_event(database.get_rdkb_wlan_task_id(),
                         rdkb_wlan_task::events::STEERING_REMOVE_SOCKET, &new_event);
    }
#endif
}

bool Controller::handle_cmdu(int fd, uint32_t iface_index, const sMacAddr &dst_mac,
                             const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    bool vendor_specific = false;

    if (cmdu_rx.getMessageType() == ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE) {
        vendor_specific = true;
    }

    if (vendor_specific) {
        auto beerocks_header = beerocks::message_com::parse_intel_vs_message(cmdu_rx);
        if (!beerocks_header) {
            LOG(ERROR) << "Not a vendor specific message";
            return false;
        }
        switch (beerocks_header->action()) {
        case beerocks_message::ACTION_CLI: {
            son_management::handle_cli_message(fd, beerocks_header, cmdu_tx, database, tasks);
        } break;
        case beerocks_message::ACTION_BML: {
            son_management::handle_bml_message(fd, beerocks_header, cmdu_tx, database, tasks);
        } break;
        case beerocks_message::ACTION_CONTROL: {
            handle_cmdu_control_message(src_mac, beerocks_header);
        } break;
        default: {
            LOG(ERROR) << "Unknown message, action: " << int(beerocks_header->action());
        }
        }
    } else {
        LOG(DEBUG) << "received 1905.1 cmdu message";
        handle_cmdu_1905_1_message(src_mac, cmdu_rx);
        tasks.handle_ieee1905_1_msg(src_mac, cmdu_rx);

        database.update_last_contact_time(src_mac);
    }

    return true;
}

bool Controller::handle_cmdu_from_broker(uint32_t iface_index, const sMacAddr &dst_mac,
                                         const sMacAddr &src_mac,
                                         ieee1905_1::CmduMessageRx &cmdu_rx)
{
    if (src_mac == beerocks::net::network_utils::ZERO_MAC) {
        LOG(ERROR) << "src_mac is zero!";
        return false;
    }

    if (dst_mac == beerocks::net::network_utils::ZERO_MAC) {
        LOG(ERROR) << "dst_mac is zero!";
        return false;
    }

    // Filter out messages that are not addressed to the controller
    if (dst_mac != beerocks::net::network_utils::MULTICAST_1905_MAC_ADDR &&
        dst_mac != database.get_local_bridge_mac()) {
        return false;
    }

    // TODO: Add optimization of PID filtering for cases like the following:
    // If VS message was sent by Controllers local agent to the controller, it is looped back.

    // Handle CMDU as if it had been received from any other process.
    // The socket descriptor is not needed because this process never responds to the transport
    // process.
    return handle_cmdu(beerocks::net::FileDescriptor::invalid_descriptor, iface_index, dst_mac,
                       src_mac, cmdu_rx);
}

bool Controller::handle_cmdu_1905_1_message(const sMacAddr &src_mac,
                                            ieee1905_1::CmduMessageRx &cmdu_rx)
{
    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::ACK_MESSAGE:
        return handle_cmdu_1905_ack_message(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_SEARCH_MESSAGE:
        return handle_cmdu_1905_autoconfiguration_search(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_WSC_MESSAGE:
        return handle_cmdu_1905_autoconfiguration_WSC(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::AP_CAPABILITY_REPORT_MESSAGE:
        return handle_cmdu_1905_ap_capability_report(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::AP_METRICS_RESPONSE_MESSAGE:
        return handle_cmdu_1905_ap_metric_response(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::BEACON_METRICS_RESPONSE_MESSAGE:
        return handle_cmdu_1905_beacon_response(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::CHANNEL_PREFERENCE_REPORT_MESSAGE:
        return handle_cmdu_1905_channel_preference_report(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::CHANNEL_SELECTION_RESPONSE_MESSAGE:
        return handle_cmdu_1905_channel_selection_response(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::CHANNEL_SCAN_REPORT_MESSAGE:
        return handle_cmdu_1905_channel_scan_report(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::CLIENT_STEERING_BTM_REPORT_MESSAGE:
        return handle_cmdu_1905_client_steering_btm_report_message(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::HIGHER_LAYER_DATA_MESSAGE:
        return handle_cmdu_1905_higher_layer_data_message(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::OPERATING_CHANNEL_REPORT_MESSAGE:
        return handle_cmdu_1905_operating_channel_report(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::STEERING_COMPLETED_MESSAGE:
        return handle_cmdu_1905_steering_completed_message(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::BACKHAUL_STEERING_RESPONSE_MESSAGE:
        return handle_cmdu_1905_backhaul_sta_steering_response(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::TUNNELLED_MESSAGE:
        return handle_cmdu_1905_tunnelled_message(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::FAILED_CONNECTION_MESSAGE:
        return handle_cmdu_1905_failed_connection_message(src_mac, cmdu_rx);
    case ieee1905_1::eMessageType::ASSOCIATED_STA_LINK_METRICS_RESPONSE_MESSAGE:
        return handle_cmdu_1905_associated_sta_link_metrics_response_message(src_mac, cmdu_rx);

    // Empty cases are used to prevent error logs. Below message types are proccessed within tasks.
    case ieee1905_1::eMessageType::TOPOLOGY_RESPONSE_MESSAGE:
    case ieee1905_1::eMessageType::TOPOLOGY_NOTIFICATION_MESSAGE:
    case ieee1905_1::eMessageType::LINK_METRIC_RESPONSE_MESSAGE:
    case ieee1905_1::eMessageType::CLIENT_CAPABILITY_REPORT_MESSAGE:

        return true;
    default:
        break;
    }

    LOG(WARNING) << "Unknown 1905 message received message_type=" << std::hex
                 << int(cmdu_rx.getMessageType()) << " .Ignoring ";
    return true;
}

bool Controller::handle_cmdu_1905_autoconfiguration_search(const sMacAddr &src_mac,
                                                           ieee1905_1::CmduMessageRx &cmdu_rx)
{
    LOG(DEBUG) << "Received AP_AUTOCONFIGURATION_SEARCH_MESSAGE";

    auto tlvAlMacAddress = cmdu_rx.getClass<ieee1905_1::tlvAlMacAddress>();
    if (!tlvAlMacAddress) {
        LOG(ERROR) << "getClass<tlvAlMacAddress> failed";
        return false;
    }
    auto tlvSearchedRole = cmdu_rx.getClass<ieee1905_1::tlvSearchedRole>();
    if (!tlvSearchedRole) {
        LOG(ERROR) << "getClass<tlvSearchedRole> failed";
        return false;
    }
    auto tlvAutoconfigFreqBand = cmdu_rx.getClass<ieee1905_1::tlvAutoconfigFreqBand>();
    if (!tlvAutoconfigFreqBand) {
        LOG(ERROR) << "getClass<tlvAutoconfigFreqBand> failed";
        return false;
    }
    auto tlvSupportedServiceIn = cmdu_rx.getClass<wfa_map::tlvSupportedService>();
    if (!tlvSupportedServiceIn) {
        LOG(ERROR) << "getClass<tlvSupportedService> failed";
        return false;
    }
    auto tlvSearchedService = cmdu_rx.getClass<wfa_map::tlvSearchedService>();
    if (!tlvSearchedService) {
        LOG(ERROR) << "getClass<tlvSearchedService> failed";
        return false;
    }

    auto al_mac = tlvAlMacAddress->mac();
    LOG(DEBUG) << "mac=" << al_mac;

    LOG(DEBUG) << "searched_role=" << int(tlvSearchedRole->value());
    if (tlvSearchedRole->value() != ieee1905_1::tlvSearchedRole::REGISTRAR) {
        LOG(ERROR) << "invalid tlvSearchedRole value";
        return false;
    }

    auto &auto_config_freq_band = tlvAutoconfigFreqBand->value();
    LOG(DEBUG) << "band=" << int(auto_config_freq_band);

    for (int i = 0; i < tlvSupportedServiceIn->supported_service_list_length(); i++) {
        auto supportedServiceTuple = tlvSupportedServiceIn->supported_service_list(i);
        if (!std::get<0>(supportedServiceTuple)) {
            LOG(ERROR) << "Invalid tlvSupportedService";
            return false;
        }
        auto supportedService = std::get<1>(supportedServiceTuple);
        if (supportedService != wfa_map::tlvSupportedService::eSupportedService::MULTI_AP_AGENT) {
            LOG(WARNING) << "Invalid tlvSupportedService - supported service is not "
                            "MULTI_AP_AGENT. Received value: "
                         << std::hex << int(supportedService);
            return false;
        }
    }

    for (int i = 0; i < tlvSearchedService->searched_service_list_length(); i++) {
        auto searchedServiceTuple = tlvSearchedService->searched_service_list(i);
        if (!std::get<0>(searchedServiceTuple)) {
            LOG(ERROR) << "Invalid tlvSearchedService";
            return false;
        }
        if (std::get<1>(searchedServiceTuple) !=
            wfa_map::tlvSearchedService::eSearchedService::MULTI_AP_CONTROLLER) {
            LOG(WARNING)
                << "Invalid tlvSearchedService - searched service is not MULTI_AP_CONTROLLER";
            return false;
        }
    }

    auto cmdu_header = cmdu_tx.create(
        cmdu_rx.getMessageId(), ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_RESPONSE_MESSAGE);

    auto tlvSupportedRole = cmdu_tx.addClass<ieee1905_1::tlvSupportedRole>();
    if (!tlvSupportedRole) {
        LOG(ERROR) << "addClass ieee1905_1::tlvSupportedRole failed";
        return false;
    }
    tlvSupportedRole->value() = ieee1905_1::tlvSupportedRole::REGISTRAR;

    auto tlvSupportedFreqBand = cmdu_tx.addClass<ieee1905_1::tlvSupportedFreqBand>();
    if (!tlvSupportedFreqBand) {
        LOG(ERROR) << "addClass ieee1905_1::tlvSupportedFreqBand failed";
        return false;
    }

    switch (auto_config_freq_band) {
    case ieee1905_1::tlvAutoconfigFreqBand::IEEE_802_11_2_4_GHZ: {
        tlvSupportedFreqBand->value() = ieee1905_1::tlvSupportedFreqBand::BAND_2_4G;
        break;
    }
    case ieee1905_1::tlvAutoconfigFreqBand::IEEE_802_11_5_GHZ: {
        tlvSupportedFreqBand->value() = ieee1905_1::tlvSupportedFreqBand::BAND_5G;
        break;
    }
    case ieee1905_1::tlvAutoconfigFreqBand::IEEE_802_11_60_GHZ: {
        tlvSupportedFreqBand->value() = ieee1905_1::tlvSupportedFreqBand::BAND_60G;
        break;
    }
    default: {
        LOG(ERROR) << "unknown autoconfig freq band, value=" << int(auto_config_freq_band);
        return false;
    }
    }

    auto tlvSupportedServiceOut = cmdu_tx.addClass<wfa_map::tlvSupportedService>();
    if (!tlvSupportedServiceOut) {
        LOG(ERROR) << "addClass wfa_map::tlvSupportedService failed";
        return false;
    }
    if (!tlvSupportedServiceOut->alloc_supported_service_list()) {
        LOG(ERROR) << "alloc_supported_service_list failed";
        return false;
    }
    auto supportedServiceTuple = tlvSupportedServiceOut->supported_service_list(0);
    if (!std::get<0>(supportedServiceTuple)) {
        LOG(ERROR) << "Failed accessing supported_service_list";
        return false;
    }
    std::get<1>(supportedServiceTuple) =
        wfa_map::tlvSupportedService::eSupportedService::MULTI_AP_CONTROLLER;

    // Add MultiAp Profile TLV only if the agent added it to the seach message.
    // Although R2 is profile1 competible, we found out that some certified agent
    // fail to parse the response in case the TLV is present.
    auto tlvProfile2MultiApProfileAgent = cmdu_rx.getClass<wfa_map::tlvProfile2MultiApProfile>();
    if (tlvProfile2MultiApProfileAgent) {
        auto tlvProfile2MultiApProfileController =
            cmdu_tx.addClass<wfa_map::tlvProfile2MultiApProfile>();
        if (!tlvProfile2MultiApProfileController) {
            LOG(ERROR) << "addClass wfa_map::tlvProfile2MultiApProfile failed";
            return false;
        }
    }

    auto beerocks_header = beerocks::message_com::parse_intel_vs_message(cmdu_rx);
    if (beerocks_header) {
        if (beerocks_header->action_op() !=
            beerocks_message::ACTION_CONTROL_SLAVE_HANDSHAKE_REQUEST) {
            LOG(WARNING) << "Invalid action op";
            return false;
        }
        // mark slave as prplMesh
        LOG(DEBUG) << "prplMesh agent: received ACTION_CONTROL_SLAVE_HANDSHAKE_REQUEST from "
                   << src_mac;
        database.set_prplmesh(src_mac);
        // response with handshake response to mark the controller as prplmesh
        auto response = beerocks::message_com::add_vs_tlv<
            beerocks_message::cACTION_CONTROL_SLAVE_HANDSHAKE_RESPONSE>(cmdu_tx);
        if (!response) {
            LOG(ERROR) << "Failed adding cACTION_CONTROL_SLAVE_HANDSHAKE_RESPONSE";
            return false;
        }
        beerocks::message_com::get_beerocks_header(cmdu_tx)->actionhdr()->direction() =
            beerocks::BEEROCKS_DIRECTION_AGENT;

    } else {
        LOG(DEBUG) << "Not prplMesh agent " << src_mac;
    }
    LOG(DEBUG) << "sending autoconfig response message";

    if (tlvProfile2MultiApProfileAgent) {

        auto agent = database.add_node_ire(al_mac);
        if (!agent) {
            LOG(ERROR) << "Failed adding agent: " << al_mac;
            return false;
        }

        agent->profile = tlvProfile2MultiApProfileAgent->profile();
        LOG(DEBUG) << "Agent profile is updated with enum " << agent->profile;
    }

    return son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);
}

/**
 * @brief Encrypt the config data using AES and add to the WSC M2 TLV
 *        The encrypted data length is the config data length padded to 16 bytes boundary.
 *
 * @param[in] m2 WSC M2 TLV
 * @param[in] config_data config data in network byte order (swapped)
 * @param[in] authkey 32 bytes calculated authentication key
 * @param[in] keywrapkey 16 bytes calculated key wrap key
 * @return true on success
 * @return false on failure
 */
bool Controller::autoconfig_wsc_add_m2_encrypted_settings(WSC::m2::config &m2_cfg,
                                                          WSC::configData &config_data,
                                                          uint8_t authkey[32],
                                                          uint8_t keywrapkey[16])
{
    // Step 1 - key wrap authenticator calculation
    uint8_t *plaintext = config_data.getMessageBuff();
    int plaintextlen   = config_data.getMessageLength();

    uint8_t *kwa = config_data.key_wrap_authenticator();
    // The keywrap authenticator is part of the config_data (last member of the
    // config_data to be precise).
    // However, since we need to calculate it over the part of config_data without the keywrap
    // authenticator, substruct it's size from the computation length
    size_t config_data_len_for_kwa = plaintextlen - config_data.key_wrap_authenticator_size();
    // Add KWA which is the 1st 64 bits of HMAC of config_data using AuthKey
    if (!mapf::encryption::kwa_compute(authkey, plaintext, config_data_len_for_kwa, kwa)) {
        LOG(ERROR) << "KeyWrapAuth computation failed!";
        return false;
    }

    // The KWA is computed on the swapped config_data (network byte order).
    // So at this point the KWA class is already swapped. We don't need to swap the recently
    // calculated data since the data is a char array and there is no need to swap it.

    // Step 2 - AES encryption using temporary buffer. This is needed since we only
    // know the encrypted length after encryption.
    // Calculate initialization vector (IV), and encrypt the plaintext using aes128 cbc.
    // leave room for up to 16 bytes internal padding length - see aes_encrypt()
    // Create encrypted_settings
    int cipherlen = plaintextlen + 16;
    uint8_t ciphertext[cipherlen];
    if (!mapf::encryption::create_iv(m2_cfg.iv, WSC::WSC_ENCRYPTED_SETTINGS_IV_LENGTH)) {
        LOG(ERROR) << "create iv failure";
        return false;
    }
    if (!mapf::encryption::aes_encrypt(keywrapkey, m2_cfg.iv, plaintext, plaintextlen, ciphertext,
                                       cipherlen)) {
        LOG(ERROR) << "aes encrypt failure";
        return false;
    }
    m2_cfg.encrypted_settings = std::vector<uint8_t>(ciphertext, ciphertext + cipherlen);

    return true;
}

/**
 * @brief Calculate keys and update M2 attributes.
 *
 * @param[in] m1 WSC M1 attribute list received from the radio agent
 * @param[in] m2 WSC configuration struct used for creating WSC::m2
 * @param[in] dh diffie helman key exchange class containing the keypair
 * @param[out] authkey 32 bytes calculated authentication key
 * @param[out] keywrapkey 16 bytes calculated key wrap key
 * @return true on success
 * @return false on failure
 */
void Controller::autoconfig_wsc_calculate_keys(WSC::m1 &m1, WSC::m2::config &m2,
                                               const mapf::encryption::diffie_hellman &dh,
                                               uint8_t authkey[32], uint8_t keywrapkey[16])
{
    std::copy_n(m1.enrollee_nonce(), WSC::eWscLengths::WSC_NONCE_LENGTH, m2.enrollee_nonce);
    std::copy_n(dh.nonce(), dh.nonce_length(), m2.registrar_nonce);
    mapf::encryption::wps_calculate_keys(
        dh, m1.public_key(), WSC::eWscLengths::WSC_PUBLIC_KEY_LENGTH, m1.enrollee_nonce(),
        m1.mac_addr().oct, m2.registrar_nonce, authkey, keywrapkey);
    copy_pubkey(dh, m2.pub_key);
}

/**
 * @brief autoconfig global authenticator attribute calculation
 *
 * Calculate authentication on the Full M1 || M2* whereas M2* = M2 without the authenticator
 * attribute.
 *
 * @param m1 WSC M1 attribute list
 * @param m2 WSC M2 TLV
 * @param authkey authentication key
 * @return true on success
 * @return false on failure
 */
bool Controller::autoconfig_wsc_authentication(WSC::m1 &m1, WSC::m2 &m2, uint8_t authkey[32])
{
    // Authentication on Full M1 || M2* (without the authenticator attribute)
    // This is the content of M1 and M2, without the type and length.
    // Authentication is done on swapped data.
    // Since m1 is parsed, it is in host byte order, and needs to be swapped.
    // m2 is created, and already finalized so its in network byte order, so no
    // need to swap it.
    m1.swap();
    uint8_t buf[m1.getMessageLength() + m2.getMessageLength() -
                WSC::cWscAttrAuthenticator::get_initial_size()];
    auto next = std::copy_n(m1.getMessageBuff(), m1.getMessageLength(), buf);
    std::copy_n(m2.getMessageBuff(),
                m2.getMessageLength() - WSC::cWscAttrAuthenticator::get_initial_size(), next);
    // swap back
    m1.swap();
    uint8_t *kwa = reinterpret_cast<uint8_t *>(m2.authenticator());
    // Add KWA which is the 1st 64 bits of HMAC of config_data using AuthKey
    if (!mapf::encryption::kwa_compute(authkey, buf, sizeof(buf), kwa)) {
        LOG(ERROR) << "kwa_compute failure";
        return false;
    }
    return true;
}

/**
 * @brief add WSC M2 TLV to the current CMDU
 *
 *        the config_data contains the secret ssid, authentication and encryption types,
 *        the network key, bssid and the key_wrap_auth attribute.
 *        It does encryption using the keywrapkey and HMAC with the authkey generated
 *        in the WSC keys calculation from the M1 and M2 nonce values, the radio agent's
 *        mac, and a random initialization vector.
 *        The encrypted config_data blob is copied to the encrypted_data attribute
 *        in the M2 TLV, which marks the WSC M2 TLV ready to be sent to the agent.
 *
 * @param m1 WSC M1 attribute list received from the radio agent as part of the WSC autoconfiguration
 *        CMDU
 * @return true on success
 * @return false on failure
 */
bool Controller::autoconfig_wsc_add_m2(WSC::m1 &m1,
                                       const wireless_utils::sBssInfoConf *bss_info_conf)
{
    auto tlv = cmdu_tx.addClass<ieee1905_1::tlvWsc>();
    if (!tlv) {
        LOG(ERROR) << "Failed creating tlvWsc";
        return false;
    }
    // Allocate maximum allowed length for the payload, so it can accommodate variable length
    // data inside the internal TLV list.
    // On finalize(), the buffer is shrunk back to its real size.
    size_t payload_length =
        tlv->getBuffRemainingBytes() - ieee1905_1::tlvEndOfMessage::get_initial_size();
    tlv->alloc_payload(payload_length);

    WSC::m2::config m2_cfg;
    m2_cfg.msg_type = WSC::eWscMessageType::WSC_MSG_TYPE_M2;
    // enrolee_nonce and registrar_nonce are set in autoconfig_wsc_calculate_keys()
    // public_key is set in autoconfig_wsc_calculate_keys()
    // connection_type and configuration_methods have default values
    // TODO the following should be taken from the database
    m2_cfg.manufacturer        = "prplMesh";
    m2_cfg.model_name          = "Ubuntu";
    m2_cfg.model_number        = "18.04";
    m2_cfg.serial_number       = "prpl12345";
    m2_cfg.primary_dev_type_id = WSC::WSC_DEV_NETWORK_INFRA_GATEWAY;
    m2_cfg.device_name         = "prplmesh-controller";
    m2_cfg.encr_type_flags =
        uint16_t(WSC::eWscEncr::WSC_ENCR_NONE) | uint16_t(WSC::eWscEncr::WSC_ENCR_AES);
    m2_cfg.auth_type_flags =
        WSC::eWscAuth(WSC::eWscAuth::WSC_AUTH_OPEN | WSC::eWscAuth::WSC_AUTH_WPA2PSK |
                      WSC::eWscAuth::WSC_AUTH_SAE);
    // TODO Maybe the band should be taken from bss_info_conf.operating_class instead?
    m2_cfg.bands =
        (m1.rf_bands() & WSC::WSC_RF_BAND_5GHZ) ? WSC::WSC_RF_BAND_5GHZ : WSC::WSC_RF_BAND_2GHZ;

    // association_state, configuration_error, device_password_id, os_version and vendor_extension
    // have default values

    ///////////////////////////////
    // @brief encryption support //
    ///////////////////////////////
    mapf::encryption::diffie_hellman dh;
    uint8_t authkey[32];
    uint8_t keywrapkey[16];
    autoconfig_wsc_calculate_keys(m1, m2_cfg, dh, authkey, keywrapkey);

    // Encrypted settings
    // Encrypted settings are the ConfigData + IV. First create the ConfigData,
    // Then copy it to the encrypted data, add an IV and encrypt.
    // Finally, add HMAC

    // Create ConfigData
    uint8_t buf[1024];
    WSC::configData::config cfg;
    if (bss_info_conf) {
        cfg.ssid        = bss_info_conf->ssid;
        cfg.auth_type   = bss_info_conf->authentication_type;
        cfg.encr_type   = bss_info_conf->encryption_type;
        cfg.network_key = bss_info_conf->network_key;
        cfg.bss_type    = 0;
        if (bss_info_conf->fronthaul) {
            cfg.bss_type |= WSC::eWscVendorExtSubelementBssType::FRONTHAUL_BSS;
        }
        if (bss_info_conf->backhaul) {
            cfg.bss_type |= WSC::eWscVendorExtSubelementBssType::BACKHAUL_BSS;
        }

        LOG(DEBUG) << "WSC config_data:" << std::hex << std::endl
                   << "     ssid: " << cfg.ssid << std::endl
                   << "     authentication_type: " << int(cfg.auth_type) << std::endl
                   << "     encryption_type: " << int(cfg.encr_type) << std::dec << std::endl
                   << "     bss_type: " << std::hex << int(cfg.bss_type);
    } else {
        // Tear down. No need to set any parameter except the teardown bit and the MAC address.
        cfg.bss_type = WSC::eWscVendorExtSubelementBssType::TEARDOWN;
        LOG(DEBUG) << "WSC config_data: tear down";
    }

    // The MAC address in the config data is tricky... According to "Wi-Fi Simple Configuration
    // Technical Specification v2.0.6", section 7.2.2 "Validation of Configuration Data" the MAC
    // address should be validated to match the Enrollee's own MAC address. "IEEE Std 1905.1-2013"
    // section 10.1.2, Table 10-1 "IEEE 802.11 settings (ConfigData) in M2 frame" says that it
    // should be "AP’s MAC address (BSSID)". The Multi-AP doesn't say anything about the MAC
    // addresses in M2, but it does say that the Enrollee MAC address in the M1 message must be the
    // AL-MAC address.
    //
    // Clearly, we can't use the real BSSID for the MAC address, since it's the responsibility of
    // the agent to use one of its assigned unique addresses as BSSID, and we don't have that
    // information in the controller. So we could use the AL-MAC addresses or the Radio UID. It
    // seems the most logical to make sure it matches the MAC address in the M1, since that stays
    // the closest to the WSC-specified behaviour.
    //
    // Note that the BBF 1905.1 implementation (meshComms) simply ignores the MAC address in M2.
    cfg.bssid = m1.mac_addr();

    auto config_data = WSC::configData::create(cfg, buf, sizeof(buf));
    if (!config_data) {
        LOG(ERROR) << "Failed to create configData";
        return false;
    }
    config_data->finalize();

    if (!autoconfig_wsc_add_m2_encrypted_settings(m2_cfg, *config_data, authkey, keywrapkey))
        return false;

    auto m2 = WSC::m2::create(*tlv, m2_cfg);
    if (!m2)
        return false;

    // Finalize m2 since it needs to be in network byte order for global authentication
    m2->finalize();
    if (!autoconfig_wsc_authentication(m1, *m2, authkey))
        return false;

    return true;
}

/**
 * @brief Parse AP-Autoconfiguration WSC which should include one AP Radio Basic Capabilities
 *        TLV and one WSC TLV containing M1. If this is Intel agent, it will also have vendor specific tlv.
 *
 * @param sd socket descriptor
 * @param cmdu_rx received CMDU which contains M1
 * @return true on success
 * @return false on failure
 */
bool Controller::handle_cmdu_1905_autoconfiguration_WSC(const sMacAddr &src_mac,
                                                        ieee1905_1::CmduMessageRx &cmdu_rx)
{
    LOG(DEBUG) << "Received AP_AUTOCONFIGURATION_WSC_MESSAGE";
    auto tlvWsc = cmdu_rx.getClass<ieee1905_1::tlvWsc>();
    if (!tlvWsc) {
        LOG(ERROR) << "getClass<ieee1905_1::tlvWsc> failed";
        return false;
    }
    auto m1 = WSC::m1::parse(*tlvWsc);
    if (!m1) {
        LOG(INFO) << "Not a valid M1 - Ignoring WSC CMDU";
        return false;
    }

    auto time_since_m1 = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - cmdu_rx.received_time);
    if (time_since_m1 >
        std::chrono::seconds{beerocks::ieee1905_1_consts::AUTOCONFIG_M2_TIMEOUT_SECONDS}) {
        LOG(INFO) << "Time since M1 was received (" << time_since_m1.count()
                  << " seconds) is more than "
                  << beerocks::ieee1905_1_consts::AUTOCONFIG_M2_TIMEOUT_SECONDS
                  << " seconds, ignoring M1.";
        return false;
    }

    auto radio_basic_caps = cmdu_rx.getClass<wfa_map::tlvApRadioBasicCapabilities>();
    if (!radio_basic_caps) {
        LOG(ERROR) << "getClass<wfa_map::tlvApRadioBasicCapabilities> failed";
        return false;
    }
    auto al_mac = m1->mac_addr();
    auto ruid   = radio_basic_caps->radio_uid();
    LOG(INFO) << "AP_AUTOCONFIGURATION_WSC M1 al_mac=" << al_mac << " ruid=" << ruid;
    LOG(DEBUG) << "   device " << m1->manufacturer() << " " << m1->model_name() << " "
               << m1->device_name() << " " << m1->serial_number();

    // If the agent already exists, return the old one
    auto agent   = database.m_agents.add(al_mac);
    agent->state = beerocks::STATE_DISCONNECTED;

    database.set_agent_manufacturer(*agent, m1->manufacturer());

    // Profile-2 Multi AP profile is added for higher than Profile-1 agents.
    if (agent->profile > wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_1 &&
        !handle_tlv_profile2_ap_capability(agent, cmdu_rx)) {
        LOG(ERROR) << "Profile2 AP Capability is not supplied for Agent " << al_mac
                   << " with profile enum " << agent->profile;
    }

    //TODO autoconfig process the rest of the class
    //TODO autoconfig Keep intel agent support only as intel enhancements
    /**
     * @brief Reply with AP-Autoconfiguration WSC with a single AP Radio Identifier TLV
     * and one (TODO do we need more?) WSC TLV containing M2.
     */
    if (!cmdu_tx.create(0, ieee1905_1::eMessageType::AP_AUTOCONFIGURATION_WSC_MESSAGE)) {
        LOG(ERROR) << "Create AP_AUTOCONFIGURATION_WSC_MESSAGE response";
        return false;
    }
    // All attributes which are not explicitely set below are set to
    // default by the TLV factory, see WSC_Attributes.yml
    auto tlvRuid = cmdu_tx.addClass<wfa_map::tlvApRadioIdentifier>();
    if (!tlvRuid) {
        LOG(ERROR) << "error creating tlvApRadioIdentifier TLV";
        return false;
    }

    tlvRuid->radio_uid() = ruid;

    const auto &bss_info_confs = database.get_bss_info_configuration(m1->mac_addr());
    uint8_t num_bsss           = 0;

    for (const auto &bss_info_conf : bss_info_confs) {
        // Check if the radio supports it
        if (!son_actions::has_matching_operating_class(*radio_basic_caps, bss_info_conf)) {
            LOG(INFO) << "Skipping " << bss_info_conf.ssid << " due to operclass mismatch";
            continue;
        }
        if ((m1->auth_type_flags() & bss_info_conf.authentication_type) !=
            bss_info_conf.authentication_type) {
            LOG(INFO) << std::hex << "Auth mismatch for " << bss_info_conf.ssid << ": get 0x"
                      << m1->auth_type_flags() << " need at least 0x"
                      << uint16_t(bss_info_conf.authentication_type);
        }
        if (!(m1->encr_type_flags() & uint16_t(bss_info_conf.encryption_type))) {
            LOG(INFO) << std::hex << "Encr mismatch for " << bss_info_conf.ssid << ": get 0x"
                      << m1->encr_type_flags() << " need 0x"
                      << uint16_t(bss_info_conf.encryption_type);
        }
        if (num_bsss >= radio_basic_caps->maximum_number_of_bsss_supported()) {
            LOG(INFO) << "Configured #BSS exceeds maximum for " << al_mac << " radio " << ruid;
            break;
        }
        if (!autoconfig_wsc_add_m2(*m1, &bss_info_conf)) {
            LOG(ERROR) << "Failed setting M2 attributes";
            return false;
        }
        num_bsss++;
    }

    // If no BSS (either because none are configured, or because they don't match), tear down.
    if (num_bsss == 0) {
        if (!autoconfig_wsc_add_m2(*m1, nullptr)) {
            LOG(ERROR) << "Failed setting M2 attributes";
            return false;
        }
    } else {
        agent_monitoring_task::add_traffic_policy_tlv(database, cmdu_tx, m1);
        agent_monitoring_task::add_profile_2default_802q_settings_tlv(database, cmdu_tx, m1);
    }

    auto beerocks_header = beerocks::message_com::parse_intel_vs_message(cmdu_rx);
    if (beerocks_header) {
        LOG(INFO) << "Intel radio agent join (al_mac=" << al_mac << " ruid=" << ruid;
        if (!handle_intel_slave_join(src_mac, radio_basic_caps, *beerocks_header, cmdu_tx, agent)) {
            LOG(ERROR) << "Intel radio agent join failed (al_mac=" << al_mac << " ruid=" << ruid
                       << ")";
            return false;
        }
    } else {
        LOG(INFO) << "Non-Intel radio agent join (al_mac=" << al_mac << " ruid=" << ruid << ")";
        // Multi-AP Agent doesn't say anything about the bridge, so we have to rely on Intel Slave Join for that.
        // We'll use AL-MAC as the bridge
        // TODO convert source address into AL-MAC address
        if (!handle_non_intel_slave_join(src_mac, radio_basic_caps, *m1, agent, ruid, cmdu_tx)) {
            LOG(ERROR) << "Non-Intel radio agent join failed (al_mac=" << al_mac << " ruid=" << ruid
                       << ")";
            return false;
        }
    }

    database.dm_set_device_board_info(*agent,
                                      {m1->manufacturer(), m1->serial_number(), m1->model_name()});

    return true;
}

bool Controller::handle_cmdu_1905_channel_preference_report(const sMacAddr &src_mac,
                                                            ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(INFO) << "Received CHANNEL_PREFERENCE_REPORT_MESSAGE, mid=" << std::dec << int(mid);

    // Define ruid list in order to create only one reply of tlvChannelPreference per ruid
    std::list<sMacAddr> ruid_list;
    // parse all tlvs in cmdu
    // parse channel preference report message
    auto channel_preference_tlv_rx = cmdu_rx.getClass<wfa_map::tlvChannelPreference>();
    if (!channel_preference_tlv_rx) {
        LOG(ERROR) << "addClass wfa_map::tlvChannelPreference has failed";
        return false;
    }

    const auto &radio_uid = channel_preference_tlv_rx->radio_uid();
    auto radio            = database.get_radio_by_uid(radio_uid);
    if (!radio) {
        LOG(ERROR) << "Failed to get Radio Object with uid: " << radio_uid;
        return false;
    }

    auto agent = database.m_agents.get(src_mac);
    if (!agent) {
        LOG(ERROR) << "Agent with mac is not found in database mac=" << src_mac;
        return false;
    }

    if (agent->profile > wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_1 &&
        !handle_tlv_profile2_cac_status_report(radio, cmdu_rx)) {
        LOG(ERROR) << "Profile2 CAC Status Report is not supplied for Agent " << src_mac
                   << " with profile enum " << agent->profile;
    }

    // parse radio operation restriction tlv
    auto radio_operation_restriction_tlv_rx =
        cmdu_rx.getClass<wfa_map::tlvRadioOperationRestriction>();
    if (radio_operation_restriction_tlv_rx) {
        LOG(DEBUG) << "Found tlvRadioOperationRestriction";
        // TODO: continute to parse this tlv
        // This TLV contains radio restriction in channel selection that must be considered
        // in channel selection request message. Since this is a dummy message, this TLV is
        // ignored. Full implemtation will be as part of channel selection task.
    }

    // Build ACK message CMDU
    auto cmdu_tx_header = cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }
    LOG(DEBUG) << "sending ACK message back to agent";
    son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);

    // TODO In production mode (not certification), we need to store the channel preference (PPM-201)
    // report in the DB which will in turn be used by the channel selection task.
    // The plan is to create a real (and new) channel selection task that will properly handle all
    // the channel selection flows of the standard.
    // For now, "cert_cmdu_tx" is filled with dummy data based on the received preference report,
    // and not send it. When the UCC tells us to sends it, we have a ready cmdu and we send it.
    if (!database.setting_certification_mode())
        return true;

    // TODO: in actual channel selection task, it is important to validate that rx mid is identical
    // to the mid sent in channel preference request message

    // build channel request message
    if (!cert_cmdu_tx.create(0, ieee1905_1::eMessageType::CHANNEL_SELECTION_REQUEST_MESSAGE)) {
        LOG(ERROR) << "cmdu creation of type CHANNEL_SELECTION_REQUEST_MESSAGE, has failed";
        return false;
    }

    // read all operating class list
    auto operating_classes_list_length = channel_preference_tlv_rx->operating_classes_list_length();

    for (int oc_idx = 0; oc_idx < operating_classes_list_length; oc_idx++) {
        std::stringstream ss;
        auto operating_class_tuple = channel_preference_tlv_rx->operating_classes_list(oc_idx);
        if (!std::get<0>(operating_class_tuple)) {
            LOG(ERROR) << "getting operating class entry has failed!";
            return false;
        }
        auto &op_class_channels_rx = std::get<1>(operating_class_tuple);
        auto operating_class       = op_class_channels_rx.operating_class();
        ss << "operating class=" << int(operating_class);

        const auto &flags = op_class_channels_rx.flags();
        auto preference   = flags.preference;
        auto reason_code  = flags.reason_code;
        ss << ", preference=" << int(preference) << ", reason=" << int(reason_code);
        ss << ", channel_list={";

        auto channel_list_length = op_class_channels_rx.channel_list_length();
        for (int ch_idx = 0; ch_idx < channel_list_length; ch_idx++) {
            auto channel_rx = op_class_channels_rx.channel_list(ch_idx);
            if (!channel_rx) {
                LOG(ERROR) << "getting channel entry has failed!";
                return false;
            }

            ss << int(*channel_rx);

            // add comma if not last channel in the list, else close list by add curl brackets
            ss << (((ch_idx + 1) != channel_list_length) ? "," : "}");

            // If reply tlvChannelPreference was not created for this ruid, then:
            // mark first supported non-dfs channel as selected channel
            // TODO: need to check that selected channel does not violate radio restriction
            const auto &ruid = channel_preference_tlv_rx->radio_uid();

            if (std::find(ruid_list.begin(), ruid_list.end(), ruid) != ruid_list.end() ||
                preference == 0 || wireless_utils::is_dfs_channel(*channel_rx)) {
                continue;
            }

            LOG(INFO) << "ruid=" << ruid;
            LOG(INFO) << "selected_operating_class=" << std::dec << int(operating_class);
            LOG(INFO) << "selected_channel=" << int(*channel_rx);

            ruid_list.push_back(ruid);

            // send one channel preference report for each ruid
            auto channel_preference_tlv_tx = cert_cmdu_tx.addClass<wfa_map::tlvChannelPreference>();
            if (!channel_preference_tlv_tx) {
                LOG(ERROR) << "addClass ieee1905_1::tlvChannelPreference has failed";
                return false;
            }

            channel_preference_tlv_tx->radio_uid() = ruid;

            // Create operating class object
            auto op_class_channels_tx = channel_preference_tlv_tx->create_operating_classes_list();
            if (!op_class_channels_tx) {
                LOG(ERROR) << "create_operating_classes_list() has failed!";
                return false;
            }

            // TODO: check that the data is parsed properly after fixing the following bug:
            // Since sFlags is defined after dynamic list cPreferenceOperatingClasses it cause data override
            // on the first channel on the list and sFlags itself.
            // See: https://github.com/prplfoundation/prplMesh/issues/8

            // Fill operating class object
            op_class_channels_tx->operating_class() = operating_class;

            // allocate 1 channel
            if (!op_class_channels_tx->alloc_channel_list()) {
                LOG(ERROR) << "alloc_channel_list() has failed!";
                return false;
            }
            auto channel_idx = op_class_channels_tx->channel_list_length();
            auto channel_tx  = op_class_channels_tx->channel_list(channel_idx - 1);
            if (!channel_tx) {
                LOG(ERROR) << "getting channel entry has failed!";
                return false;
            }
            *channel_tx = *channel_rx;

            op_class_channels_tx->flags() = op_class_channels_rx.flags();

            // Push operating class object to the list of operating class objects
            if (!channel_preference_tlv_tx->add_operating_classes_list(op_class_channels_tx)) {
                LOG(ERROR) << "add_operating_classes_list() has failed!";
                return false;
            }
        }
        LOG(DEBUG) << ss.str();
    }

    cert_cmdu_tx.finalize();

    return true; // cert_cmdu_tx will be sent when triggered to by the UCC application
}

bool Controller::handle_cmdu_1905_ack_message(const sMacAddr &src_mac,
                                              ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();

    // extract error code, if any
    std::stringstream errorSS;
    auto error_tlv = cmdu_rx.getClass<wfa_map::tlvErrorCode>();
    if (error_tlv) {
        errorSS << "0x" << error_tlv->reason_code();
    } else {
        errorSS << "no error";
    }

    LOG(DEBUG) << "Received ACK_MESSAGE, mid=" << std::hex << int(mid)
               << " tlv error code: " << errorSS.str();

    // TODO: Send ACK message/event to dynamic_channel_selection_r2_task.

    //TODO: the ACK should be sent to the correct task and will be done as part of agent certification
    return true;
}

bool Controller::handle_cmdu_1905_steering_completed_message(const sMacAddr &src_mac,
                                                             ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(INFO) << "Received STEERING_COMPLETED_MESSAGE, mid=" << std::hex << int(mid);
    // build ACK message CMDU
    auto cmdu_tx_header = cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);

    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }
    LOG(DEBUG) << "sending ACK message back to agent, mid=" << std::hex << int(mid);
    return son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);
}

bool Controller::handle_cmdu_1905_client_steering_btm_report_message(
    const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(INFO) << "Received CLIENT_STEERING_BTM_REPORT_MESSAGE, mid=" << std::hex << int(mid);

    auto steering_btm_report = cmdu_rx.getClass<wfa_map::tlvSteeringBTMReport>();
    if (!steering_btm_report) {
        LOG(ERROR) << "addClass wfa_map::tlvSteeringBTMReportfailed";
        return false;
    }

    // build ACK message CMDU
    auto cmdu_tx_header = cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);

    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }
    LOG(DEBUG) << "sending ACK message back to agent";
    son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);

    std::string client_mac = tlvf::mac_to_string(steering_btm_report->sta_mac());
    uint8_t status_code    = steering_btm_report->btm_status_code();

    LOG(DEBUG) << "BTM_REPORT from source bssid " << steering_btm_report->bssid()
               << " for client_mac=" << client_mac << " status_code=" << (int)status_code;

    auto client = database.get_station(tlvf::mac_from_string(client_mac));
    if (!client) {
        LOG(ERROR) << "sta " << client_mac << " not found";
        return false;
    }

    int steering_task_id = client->steering_task_id;
    // Check if task is running before pushing the event
    if (tasks.is_task_running(steering_task_id)) {
        tasks.push_event(steering_task_id, client_steering_task::BTM_REPORT_RECEIVED,
                         (void *)&status_code);
    }
    database.update_node_11v_responsiveness(*client, true);

    if (status_code != 0) {
        LOG(DEBUG) << "sta " << client_mac << " rejected BSS steer request";
        LOG(DEBUG) << "killing roaming task";

        tasks.kill_task(client->roaming_task_id);

        tasks.push_event(steering_task_id, client_steering_task::BSS_TM_REQUEST_REJECTED);
    }

    return true;
}

bool Controller::handle_cmdu_1905_channel_selection_response(const sMacAddr &src_mac,
                                                             ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(INFO) << "Received CHANNEL_SELECTION_RESPONSE_MESSAGE, mid=" << std::dec << int(mid);

    for (auto channel_selection_response_tlv :
         cmdu_rx.getClassList<wfa_map::tlvChannelSelectionResponse>()) {
        auto &ruid         = channel_selection_response_tlv->radio_uid();
        auto response_code = channel_selection_response_tlv->response_code();

        LOG(DEBUG)
            << "channel selection response from ruid=" << ruid << ", response_code="
            << ([](const wfa_map::tlvChannelSelectionResponse::eResponseCode &response_code) {
                   std::string ret_str;
                   switch (response_code) {
                   case wfa_map::tlvChannelSelectionResponse::eResponseCode::ACCEPT:
                       ret_str.assign("ACCEPT");
                       break;
                   case wfa_map::tlvChannelSelectionResponse::eResponseCode::
                       DECLINE_VIOLATES_CURRENT_PREFERENCES:
                       ret_str.assign("DECLINE_VIOLATES_CURRENT_PREFERENCES");
                       break;
                   case wfa_map::tlvChannelSelectionResponse::eResponseCode::
                       DECLINE_VIOLATES_MOST_RECENTLY_REPORTED_PREFERENCES:
                       ret_str.assign("DECLINE_VIOLATES_MOST_RECENTLY_REPORTED_PREFERENCES");
                       break;
                   case wfa_map::tlvChannelSelectionResponse::eResponseCode::
                       DECLINE_PREVENT_OPERATION_OF_BACKHAUL_LINK:
                       ret_str.assign("DECLINE_PREVENT_OPERATION_OF_BACKHAUL_LINK");
                       break;
                   default:
                       ret_str.assign("ERROR:UNFAMILIAR_RESPONSE_CODE");
                       break;
                   }
                   return ret_str;
               })(response_code);
    }

    return true;
}

bool Controller::handle_cmdu_1905_channel_scan_report(const sMacAddr &src_mac,
                                                      ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto current_message_mid = cmdu_rx.getMessageId();
    LOG(INFO) << "Received CHANNEL_SCAN_REPORT_MESSAGE, agent src_mac=" << src_mac
              << ", mid=" << std::hex << current_message_mid;

    // get Timestamp TLV
    auto timestamp_tlv = cmdu_rx.getClass<wfa_map::tlvTimestamp>();
    if (!timestamp_tlv) {
        LOG(ERROR) << "getClass wfa_map::tlvTimestamp has failed";
        return false;
    }
    auto ISO_8601_timestamp = timestamp_tlv->timestamp_str();
    LOG(INFO) << "Report Timestamp: " << ISO_8601_timestamp;

    int result_count = 0;
    for (auto const result_tlv : cmdu_rx.getClassList<wfa_map::tlvProfile2ChannelScanResult>()) {

        //If scans status is not successful, it will not include other fields.
        if (result_tlv->success() != wfa_map::tlvProfile2ChannelScanResult::eScanStatus::SUCCESS) {
            LOG(WARNING) << "Channel Scan Status: " << result_tlv->success();
            continue;
        }
        LOG(INFO) << "Channel Scan Status: " << result_tlv->success();

        auto neighbors_list_length = result_tlv->neighbors_list_length();
        LOG(DEBUG) << "Received Result TLV for:" << std::endl
                   << "RUID: " << result_tlv->radio_uid() << ", "
                   << "Operating Class: " << result_tlv->operating_class() << ", "
                   << "Channel: " << result_tlv->channel() << ", "
                   << " containing " << neighbors_list_length << " neighbors";
        /**
         * To correctly store the results of the most current report, we need to know whether to
         * override any existing records.
         * In case of fragmentation in prplmesh the entire report could be split into several
         * report messages, thus to confirm if we should override the existing records we compare
         * the recorded timestamp against the received timestamp as fragmented reports share the
         * same timestamp.
         * 
         * Reports with the same timestamp are recorded in the report-record-index
         */
        bool should_override_existing_records = true;
        if (database.has_channel_report_record(result_tlv->radio_uid(), ISO_8601_timestamp,
                                               result_tlv->operating_class(),
                                               result_tlv->channel())) {
            LOG(DEBUG) << "Report record found for " << ISO_8601_timestamp << " ["
                       << result_tlv->operating_class() << "," << result_tlv->channel() << "] "
                       << " from radio: " << result_tlv->radio_uid()
                       << ", Not overriding existing records.";
            should_override_existing_records = false;
        } else {
            LOG(DEBUG) << "No previous report record were found for " << ISO_8601_timestamp << " ["
                       << result_tlv->operating_class() << "," << result_tlv->channel() << "] "
                       << " from radio:" << result_tlv->radio_uid() << ".";
        }

        std::vector<wfa_map::cNeighbors> neighbor_vec;
        for (int nbr_idx = 0; nbr_idx < neighbors_list_length; nbr_idx++) {
            auto neighbor_tuple = result_tlv->neighbors_list(nbr_idx);
            if (!std::get<0>(neighbor_tuple)) {
                LOG(ERROR) << "getting neighbor entry #" << nbr_idx << " has failed!";
                return false;
            }

            auto neighbor = std::get<1>(neighbor_tuple);
            neighbor_vec.push_back(neighbor);
        }
        if (!database.add_channel_report(result_tlv->radio_uid(), result_tlv->operating_class(),
                                         result_tlv->channel(), neighbor_vec, result_tlv->noise(),
                                         result_tlv->utilization(), ISO_8601_timestamp,
                                         should_override_existing_records)) {
            LOG(ERROR) << "Failed to add channel report entry #" << result_count << "!";
            return false;
        }
        if (!database.dm_add_scan_result(
                result_tlv->radio_uid(), result_tlv->operating_class(), result_tlv->channel(),
                result_tlv->noise(), result_tlv->utilization(), neighbor_vec, ISO_8601_timestamp)) {
            LOG(ERROR) << "Failed to add ScanResult entry #" << result_count << " !";
        }
        result_count++;
    }
    LOG(DEBUG) << "Done with Channel Scan Results TLVs";

    // Build and send ACK message CMDU to the originator.
    auto cmdu_tx_header =
        cmdu_tx.create(current_message_mid, ieee1905_1::eMessageType::ACK_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }

    // Zero Error Code TLVs in this ACK message
    LOG(DEBUG) << "Sending ACK message to the originator, mid=" << std::hex << current_message_mid;
    if (!send_cmdu_to_broker(cmdu_tx, src_mac, database.get_local_bridge_mac())) {
        LOG(ERROR) << "Failed to send ACK_MESSAGE back to agent";
        return false;
    }

    /**
     * To support scan reports that may exceed the maximum size supported by the transport layer
     * (currently not addressed in the EasyMesh specifications), for prplmesh agent, we might
     * de-fragment the report across several report messages.
     * For non-prplmesh agents, receiving the report message means the scan is complete.
     * For prplmesh agents need to check the report_done flag inside tlvChannelScanReportDone
     */
    bool report_done = true;
    if (database.is_prplmesh(src_mac)) {
        /**
         * For prplmesh agents, we wish to support de-fragmentation of channel scan results across
         * multiple report CMDUs when the size of the results exceeds the max TX buffer size.
         * This means that only the last report message should clear the report records
         * NOTE: Clearing the report record will not erase the existing scan results, but the next
         * incoming report will.
         */
        auto beerocks_header = beerocks::message_com::parse_intel_vs_message(cmdu_rx);
        if (!beerocks_header) {
            LOG(ERROR) << "expecting wfa_map::tlvChannelScanReportDone";
            return false;
        }
        auto vs_tlv = beerocks_header->addClass<beerocks_message::tlvVsChannelScanReportDone>();
        if (!vs_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvChannelScanReportDone failed";
            return false;
        }
        report_done = vs_tlv->report_done();
    }

    if (report_done) {
        LOG(DEBUG) << "Sending RECEIVED_CHANNEL_SCAN_REPORT event to DCS R2 task.";
        dynamic_channel_selection_r2_task::sScanReportEvent new_event = {};
        new_event.ISO_8601_timestamp                                  = ISO_8601_timestamp;
        new_event.agent_mac                                           = src_mac;
        tasks.push_event(database.get_dynamic_channel_selection_r2_task_id(),
                         dynamic_channel_selection_r2_task::eEvent::RECEIVED_CHANNEL_SCAN_REPORT,
                         &new_event);
    }
    LOG(DEBUG) << "Report handling is done";
    return true;
}

void Controller::set_esp(const std::string &param_name, const sMacAddr &reporting_agent_bssid,
                         uint8_t *est_service_info_field)
{
    union {
        uint8_t bytes[4];
        uint32_t value;
    } estimated_service_param = {0};

    estimated_service_param.bytes[0] = est_service_info_field[0];
    estimated_service_param.bytes[1] = est_service_info_field[1];
    estimated_service_param.bytes[2] = est_service_info_field[2];
    (estimated_service_param.bytes[2]);
    database.set_estimated_service_param(reporting_agent_bssid, param_name,
                                         estimated_service_param.value);
}

bool Controller::handle_cmdu_1905_ap_metric_response(const sMacAddr &src_mac,
                                                     ieee1905_1::CmduMessageRx &cmdu_rx)
{

    auto mid = cmdu_rx.getMessageId();
    LOG(INFO) << "Received AP_METRICS_RESPONSE_MESSAGE, mid=" << std::dec << int(mid);

    bool ret_val = true;

    //getting reference for ap metric data storage from db
    auto &ap_metric_data = database.get_ap_metric_data_map();

    for (auto ap_metric_tlv : cmdu_rx.getClassList<wfa_map::tlvApMetrics>()) {
        //parse tx_ap_metric_data
        sMacAddr reporting_agent_bssid = ap_metric_tlv->bssid();

        if (!database.set_radio_utilization(reporting_agent_bssid,
                                            ap_metric_tlv->channel_utilization())) {
            LOG(ERROR) << "Failed to set radio utilization dor bssid: " << reporting_agent_bssid;
            ret_val = false;
            continue;
        }

        LOG(DEBUG) << "received tlvApMetrics from BSSID =" << reporting_agent_bssid;

        //fill tx data from TLV
        if (!ap_metric_data[reporting_agent_bssid].add_ap_metric_data(ap_metric_tlv)) {
            LOG(ERROR) << "adding apMetricData from tlv has failed";
            ret_val = false;
            continue;
        }

        if (ap_metric_tlv->estimated_service_parameters().include_ac_be) {
            set_esp("EstServiceParametersBE", reporting_agent_bssid,
                    ap_metric_tlv->estimated_service_info_field());
        } else {
            LOG(WARNING) << "Include bit for the Estimated Service Parameters AC = BE should "
                            "always be 1";
        }
        if (ap_metric_tlv->estimated_service_parameters().include_ac_bk) {
            set_esp("EstServiceParametersBK", reporting_agent_bssid,
                    ap_metric_tlv->estimated_service_info_field(3));
        }
        if (ap_metric_tlv->estimated_service_parameters().include_ac_vo) {
            set_esp("EstServiceParametersVO", reporting_agent_bssid,
                    ap_metric_tlv->estimated_service_info_field(6));
        }
        if (ap_metric_tlv->estimated_service_parameters().include_ac_vi) {
            set_esp("EstServiceParametersVI", reporting_agent_bssid,
                    ap_metric_tlv->estimated_service_info_field(9));
        }
    }

    for (auto radio_tlv : cmdu_rx.getClassList<wfa_map::tlvProfile2RadioMetrics>()) {
        ret_val &= database.set_radio_metrics(radio_tlv->radio_uid(), radio_tlv->noise(),
                                              radio_tlv->transmit(), radio_tlv->receive_self(),
                                              radio_tlv->receive_other());
    }

    auto agent = database.m_agents.get(src_mac);
    if (!agent) {
        LOG(ERROR) << "Agent with mac is not found in database mac=" << src_mac;
        return false;
    }

    ret_val &= handle_tlv_ap_extended_metrics(agent, cmdu_rx);
    ret_val &= handle_tlv_associated_sta_link_metrics(src_mac, cmdu_rx);
    ret_val &= handle_tlv_associated_sta_extended_link_metrics(src_mac, cmdu_rx);
    ret_val &= handle_tlv_associated_sta_traffic_stats(src_mac, cmdu_rx);

    // For now, this is only used for certification so update the certification cmdu.
    if (database.setting_certification_mode() &&
        (database.config.management_mode != BPL_MGMT_MODE_NOT_MULTIAP)) {
        m_link_metrics_task->construct_combined_infra_metric();
    }

    return ret_val;
}

bool Controller::handle_tlv_ap_ht_capabilities(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    bool ret_val = true;

    for (const auto &ap_ht_caps_tlv : cmdu_rx.getClassList<wfa_map::tlvApHtCapabilities>()) {
        if (!database.set_ap_ht_capabilities(ap_ht_caps_tlv->radio_uid(),
                                             ap_ht_caps_tlv->flags())) {
            LOG(ERROR) << "Couldn't set values for AP HT Capabilities in Controller Data Model.";
            ret_val = false;
        }
    }
    return ret_val;
}

bool Controller::handle_tlv_ap_he_capabilities(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    bool ret_val = true;

    for (const auto &ap_he_caps_tlv : cmdu_rx.getClassList<wfa_map::tlvApHeCapabilities>()) {
        if (!database.set_ap_he_capabilities(*ap_he_caps_tlv)) {
            LOG(ERROR) << "Couldn't set values for ap HEcapabilities data model";
            ret_val = false;
        }
    }
    return ret_val;
}

bool Controller::handle_cmdu_1905_associated_sta_link_metrics_response_message(
    const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received ASSOCIATED_STA_LINK_METRICS_RESPONSE_MESSAGE, mid=" << std::hex << mid;
    handle_tlv_associated_sta_link_metrics(src_mac, cmdu_rx);
    handle_tlv_associated_sta_extended_link_metrics(src_mac, cmdu_rx);
    return true;
}

bool Controller::handle_tlv_ap_extended_metrics(std::shared_ptr<Agent> agent,
                                                ieee1905_1::CmduMessageRx &cmdu_rx)
{
    bool ret_val = true;

    for (auto ap_extended_metric_tlv : cmdu_rx.getClassList<wfa_map::tlvApExtendedMetrics>()) {

        // Recalculate counters according to Agent Byte Units.
        ret_val &= database.set_vap_stats_info(
            ap_extended_metric_tlv->bssid(),
            database.recalculate_attr_to_byte_units(agent->byte_counter_units,
                                                    ap_extended_metric_tlv->unicast_bytes_sent()),
            database.recalculate_attr_to_byte_units(
                agent->byte_counter_units, ap_extended_metric_tlv->unicast_bytes_received()),
            database.recalculate_attr_to_byte_units(agent->byte_counter_units,
                                                    ap_extended_metric_tlv->multicast_bytes_sent()),
            database.recalculate_attr_to_byte_units(
                agent->byte_counter_units, ap_extended_metric_tlv->multicast_bytes_received()),
            database.recalculate_attr_to_byte_units(agent->byte_counter_units,
                                                    ap_extended_metric_tlv->broadcast_bytes_sent()),
            database.recalculate_attr_to_byte_units(
                agent->byte_counter_units, ap_extended_metric_tlv->broadcast_bytes_received()));
    }

    return ret_val;
}

bool Controller::handle_tlv_associated_sta_link_metrics(const sMacAddr &src_mac,
                                                        ieee1905_1::CmduMessageRx &cmdu_rx)
{
    bool ret_val = true;

    for (auto &sta_link_metric : cmdu_rx.getClassList<wfa_map::tlvAssociatedStaLinkMetrics>()) {

        // STA Metrics can hold information from different BSS sources, mostly when steering
        // Metrics of last registered and active value needs to be considered.
        auto response_list = sta_link_metric->bssid_info_list(0);

        if (!std::get<0>(response_list)) {
            LOG(ERROR) << "Failed to get bssid info list.";
            continue;
        }

        auto bssid_info = std::get<1>(response_list);

        // Verify reported BSSID and data model registered STAs BSSID is same.
        if (database.get_node_parent(tlvf::mac_to_string(sta_link_metric->sta_mac())) !=
            tlvf::mac_to_string(bssid_info.bssid)) {
            LOG(INFO) << "Reported STA BSSID is not matching with datamodel. Reported bssid:"
                      << bssid_info.bssid;
            continue;
        }
        if (!database.dm_set_sta_link_metrics(sta_link_metric->sta_mac(),
                                              bssid_info.downlink_estimated_mac_data_rate_mbps,
                                              bssid_info.uplink_estimated_mac_data_rate_mbps,
                                              bssid_info.sta_measured_uplink_rcpi_dbm_enc)) {
            LOG(ERROR) << "Failed to set link metrics for STA:" << sta_link_metric->sta_mac();
            ret_val = false;
        }
    }
    return ret_val;
}

bool Controller::handle_tlv_associated_sta_extended_link_metrics(const sMacAddr &src_mac,
                                                                 ieee1905_1::CmduMessageRx &cmdu_rx)
{
    bool ret_val = true;

    for (auto &sta_extended_link_metric :
         cmdu_rx.getClassList<wfa_map::tlvAssociatedStaExtendedLinkMetrics>()) {

        if (sta_extended_link_metric->metrics_list_length() == 0) {
            LOG(INFO) << "No metrics provided for the station "
                      << sta_extended_link_metric->associated_sta();
            continue;
        }

        auto metrics_list = sta_extended_link_metric->metrics_list(0);
        if (!std::get<0>(metrics_list)) {
            LOG(ERROR) << "Failed to get metrics info list.";
            continue;
        }

        auto metrics = std::get<1>(metrics_list);

        // Verify reported BSSID and data model registered STAs BSSID is same.
        if (database.get_node_parent(
                tlvf::mac_to_string(sta_extended_link_metric->associated_sta())) !=
            tlvf::mac_to_string(metrics.bssid)) {
            LOG(INFO) << "Reported STA BSSID is not matching with datamodel. Reported bssid:"
                      << metrics.bssid;
            continue;
        }
        if (!database.dm_set_sta_extended_link_metrics(sta_extended_link_metric->associated_sta(),
                                                       metrics)) {
            LOG(ERROR) << "Failed to set extended link metrics for STA:"
                       << sta_extended_link_metric->associated_sta();
            ret_val = false;
        }
    }
    return ret_val;
}

bool Controller::handle_tlv_associated_sta_traffic_stats(const sMacAddr &src_mac,
                                                         ieee1905_1::CmduMessageRx &cmdu_rx)
{
    bool ret_val = true;

    auto agent = database.m_agents.get(src_mac);

    if (!agent) {
        LOG(ERROR) << "Agent with mac is not found in database mac=" << src_mac;
        return false;
    }

    for (auto &sta_traffic_stat : cmdu_rx.getClassList<wfa_map::tlvAssociatedStaTrafficStats>()) {

        db::sAssociatedStaTrafficStats stats;

        // Recalculate counters according to Agent Byte Units.
        stats.m_byte_received = database.recalculate_attr_to_byte_units(
            agent->byte_counter_units, sta_traffic_stat->byte_received());

        stats.m_byte_sent = database.recalculate_attr_to_byte_units(agent->byte_counter_units,
                                                                    sta_traffic_stat->byte_sent());

        stats.m_packets_received     = sta_traffic_stat->packets_received();
        stats.m_packets_sent         = sta_traffic_stat->packets_sent();
        stats.m_retransmission_count = sta_traffic_stat->retransmission_count();
        stats.m_rx_packets_error     = sta_traffic_stat->rx_packets_error();
        stats.m_tx_packets_error     = sta_traffic_stat->tx_packets_error();

        if (!database.dm_set_sta_traffic_stats(sta_traffic_stat->sta_mac(), stats)) {
            LOG(ERROR) << "Failed to set traffic stats for STA:" << sta_traffic_stat->sta_mac();
            ret_val = false;
        }
    }
    return ret_val;
}

bool Controller::handle_tlv_ap_vht_capabilities(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    bool ret_val = true;

    for (const auto &vht_caps_tlv : cmdu_rx.getClassList<wfa_map::tlvApVhtCapabilities>()) {
        if (!database.set_ap_vht_capabilities(*vht_caps_tlv)) {
            LOG(ERROR) << "Couldn't set values for ap VHTcapabilities data model";
            ret_val = false;
        }
    }
    return ret_val;
}

bool Controller::handle_cmdu_1905_ap_capability_report(const sMacAddr &src_mac,
                                                       ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(INFO) << "Received AP_CAPABILITY_REPORT_MESSAGE, mid=" << std::dec << int(mid);

    auto channel_scan_capabilities_tlv = cmdu_rx.getClass<wfa_map::tlvChannelScanCapabilities>();
    if (!channel_scan_capabilities_tlv) {
        LOG(ERROR) << "addClass wfa_map::channel_scan_capabilities_tlv failed";
        return false;
    }

    // read all operating class list
    auto radio_list_length = channel_scan_capabilities_tlv->radio_list_length();
    if (!radio_list_length) {
        LOG(WARNING) << "Received AP Capability Report without any radios";
        return true;
    }

    bool all_radio_capabilities_saved_successfully = true;
    for (int rc_idx = 0; rc_idx < radio_list_length; rc_idx++) {
        LOG(DEBUG) << "radio-index=" << rc_idx;
        auto radio_capabilities_tuple = channel_scan_capabilities_tlv->radio_list(rc_idx);
        if (!std::get<0>(radio_capabilities_tuple)) {
            LOG(ERROR) << "getting radio capabilities entry has failed!";
            return false;
        }
        auto &radio_capabilities_entry = std::get<1>(radio_capabilities_tuple);
        auto &ruid                     = radio_capabilities_entry.radio_uid();
        LOG(DEBUG) << "ruid=" << ruid;

        // Remove all previously set Capabilities of radio from data model
        database.clear_ap_capabilities(ruid);

        if (!database.fill_radio_channel_scan_capabilites(ruid, radio_capabilities_entry)) {

            // We want to save the channel-scan-capabilities for the radios we can
            LOG(ERROR) << "Failed to save radio channel-scan-capabilities for radio=" << ruid;
            all_radio_capabilities_saved_successfully = false;
        }
    }

    if (!handle_tlv_ap_ht_capabilities(cmdu_rx)) {
        LOG(ERROR) << "Couldn't handle TLV AP HT Capabilities";
        return false;
    }
    if (!handle_tlv_ap_he_capabilities(cmdu_rx)) {
        LOG(ERROR) << "Couldn't handle TLV AP HE Capabilities";
        return false;
    }
    if (!handle_tlv_ap_vht_capabilities(cmdu_rx)) {
        LOG(ERROR) << "Couldn't handle TLV AP VHTCapabilities";
        return false;
    }

    auto agent = database.m_agents.get(src_mac);
    if (!agent) {
        LOG(ERROR) << "Agent with mac is not found in database mac=" << src_mac;
        return false;
    }

    // Profile-2 Multi AP profile is added for higher than Profile-1 agents.
    if (agent->profile > wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_1 &&
        !handle_tlv_profile2_ap_capability(agent, cmdu_rx)) {
        LOG(ERROR) << "Profile2 AP Capability is not supplied for Agent " << src_mac
                   << " with profile enum " << agent->profile;
    }

    return all_radio_capabilities_saved_successfully;
}

bool Controller::handle_cmdu_1905_operating_channel_report(const sMacAddr &src_mac,
                                                           ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(INFO) << "Received OPERATING_CHANNEL_REPORT_MESSAGE, mid=" << std::dec << int(mid);

    for (auto operating_channel_report_tlv :
         cmdu_rx.getClassList<wfa_map::tlvOperatingChannelReport>()) {
        auto &ruid    = operating_channel_report_tlv->radio_uid();
        auto tx_power = operating_channel_report_tlv->current_transmit_power();

        /*
            Here need to remove the CurrentOperatingClass data from the Controler Data Model which was
            set in previous OPERATING_CHANNEL_REPORT_MESSAGE.
         */
        database.remove_current_op_classes(ruid);

        LOG(INFO) << "operating channel report, ruid=" << ruid << ", tx_power=" << std::dec
                  << int(tx_power);

        auto operating_classes_list_length =
            operating_channel_report_tlv->operating_classes_list_length();

        for (uint8_t oc = 0; oc < operating_classes_list_length; oc++) {
            auto operating_class_tuple = operating_channel_report_tlv->operating_classes_list(oc);
            if (!std::get<0>(operating_class_tuple)) {
                LOG(ERROR) << "getting operating class entry has failed!";
                return false;
            }

            auto &operating_class_struct = std::get<1>(operating_class_tuple);
            auto operating_class         = operating_class_struct.operating_class;
            auto channel                 = operating_class_struct.channel_number;
            LOG(INFO) << "operating_class=" << int(operating_class)
                      << ", operating_channel=" << int(channel);

            database.add_current_op_class(ruid, operating_class, channel, tx_power);
        }
    }

    // send ACK_MESSAGE back to the Agent
    if (!cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE)) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }

    return son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);
}

bool Controller::handle_cmdu_1905_higher_layer_data_message(const sMacAddr &src_mac,
                                                            ieee1905_1::CmduMessageRx &cmdu_rx)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received HIGHER_LAYER_DATA_MESSAGE , mid=" << std::hex << int(mid);

    auto tlvHigherLayerData = cmdu_rx.getClass<wfa_map::tlvHigherLayerData>();
    if (!tlvHigherLayerData) {
        LOG(ERROR) << "addClass wfa_map::tlvHigherLayerData failed";
        return false;
    }

    const auto protocol       = tlvHigherLayerData->protocol();
    const auto payload_length = tlvHigherLayerData->payload_length();
    LOG(DEBUG) << "Protocol: " << std::hex << protocol;
    LOG(DEBUG) << "Payload-Length: " << int(payload_length);

    // build ACK message CMDU
    auto cmdu_tx_header = cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }
    LOG(DEBUG) << "sending ACK message to the agent, mid=" << std::hex << int(mid);
    return son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);
}

bool Controller::handle_cmdu_1905_backhaul_sta_steering_response(const sMacAddr &src_mac,
                                                                 ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received BACKHAUL_STA_STEERING_MESSAGE from " << src_mac << ", mid=" << std::hex
               << mid;

    auto tlv_backhaul_sta_steering_resp = cmdu_rx.getClass<wfa_map::tlvBackhaulSteeringResponse>();
    if (!tlv_backhaul_sta_steering_resp) {
        LOG(ERROR) << "Failed getClass<wfa_map::tlvBackhaulSteeringResponse>";
        return false;
    }

    auto bh_steering_resp_code = tlv_backhaul_sta_steering_resp->result_code();
    LOG(DEBUG) << "BACKHAUL_STA_STEERING_MESSAGE result_code: " << int(bh_steering_resp_code);

    if (bh_steering_resp_code) {
        auto error_code_tlv = cmdu_rx.getClass<wfa_map::tlvErrorCode>();
        if (!error_code_tlv) {
            LOG(ERROR) << "Failed getClass<wfa_map::tlvErrorCode>";
            return false;
        }
        LOG(DEBUG) << "BACKHAUL_STA_STEERING_MESSAGE error_code: "
                   << int(error_code_tlv->reason_code());
    }

    // build ACK message CMDU
    auto cmdu_tx_header = cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }

    LOG(DEBUG) << "sending ACK message to the agent, mid=" << std::hex << mid;

    return son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);
}

bool Controller::handle_cmdu_1905_tunnelled_message(const sMacAddr &src_mac,
                                                    ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received Tunnelled Message from " << src_mac << ", mid=" << std::hex << mid;

    // Parse the Source Info TLV
    auto source_info_tlv = cmdu_rx.getClass<wfa_map::tlvTunnelledSourceInfo>();
    if (!source_info_tlv) {
        LOG(ERROR) << "Failed parsing tlvTunnelledSourceInfo!";
        return false;
    }

    // Parse the Type
    auto type_tlv = cmdu_rx.getClass<wfa_map::tlvTunnelledProtocolType>();
    if (!type_tlv) {
        LOG(ERROR) << "Failed parsing tlvTunnelledProtocolType!";
        return false;
    }

    // Parse the Data
    auto data_tlv = cmdu_rx.getClass<wfa_map::tlvTunnelledData>();
    if (!data_tlv) {
        LOG(ERROR) << "Failed parsing tlvTunnelledData!";
        return false;
    }

    LOG(DEBUG) << "Tunnelled Message STA MAC: " << source_info_tlv->mac() << ", Type: " << std::hex
               << int(type_tlv->protocol_type()) << ", Data Length: " << std::dec
               << data_tlv->data_length() << ", Data: " << std::endl
               << beerocks::utils::dump_buffer(data_tlv->data(0), data_tlv->data_length());

    if (type_tlv->protocol_type() ==
        wfa_map::tlvTunnelledProtocolType::eTunnelledProtocolType::ASSOCIATION_REQUEST) {
        auto assoc_frame = assoc_frame::AssocReqFrame::parse(
            data_tlv->data(), data_tlv->data_length(),
            assoc_frame::AssocReqFrame::eFrameType::ASSOCIATION_REQUEST);
        if (!assoc_frame) {
            LOG(ERROR) << "Failed to parse Association Request frame";
        }
    }
    if (type_tlv->protocol_type() ==
        wfa_map::tlvTunnelledProtocolType::eTunnelledProtocolType::REASSOCIATION_REQUEST) {
        auto reassoc_frame = assoc_frame::AssocReqFrame::parse(
            data_tlv->data(), data_tlv->data_length(),
            assoc_frame::AssocReqFrame::eFrameType::REASSOCIATION_REQUEST);
        if (!reassoc_frame) {
            LOG(ERROR) << "Failed to parse Reassociation Request frame";
        }
    }
    return true;
}

bool Controller::handle_cmdu_1905_failed_connection_message(const sMacAddr &src_mac,
                                                            ieee1905_1::CmduMessageRx &cmdu_rx)
{
    LOG(DEBUG) << "Received Failed Connection Message for STA";

    auto sta_mac_tlv = cmdu_rx.getClass<wfa_map::tlvStaMacAddressType>();

    if (!sta_mac_tlv) {
        LOG(ERROR) << "Failed to get tlvStaMacAddressType!";
        return false;
    }

    auto status_code              = 0x0001; // Set default to Unspecified failure.
    auto profile2_status_code_tlv = cmdu_rx.getClass<wfa_map::tlvProfile2StatusCode>();
    auto agent                    = database.m_agents.get(src_mac);

    if (!agent) {
        LOG(ERROR) << "Agent with mac is not found in database mac=" << src_mac;
        return false;
    }
    if (agent->profile > wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_1 &&
        !profile2_status_code_tlv) {
        LOG(ERROR) << "Profile2 Status Code tlv is not supplied for Agent " << src_mac
                   << " with profile enum " << agent->profile;
    }
    if (profile2_status_code_tlv) {
        status_code = profile2_status_code_tlv->status_code();
    }

    auto reason_code              = wfa_map::tlvProfile2ReasonCode::UNSPECIFIED_REASON;
    auto profile2_reason_code_tlv = cmdu_rx.getClass<wfa_map::tlvProfile2ReasonCode>();

    if (agent->profile > wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_1 &&
        !profile2_reason_code_tlv) {
        LOG(ERROR) << "Profile2 Reason Code tlv is not supplied for Agent " << src_mac
                   << " with profile enum " << agent->profile;
    }
    if (profile2_reason_code_tlv) {
        reason_code = profile2_reason_code_tlv->reason_code();
    }
    if (status_code != 0) {
        if (!database.dm_add_failed_connection_event(sta_mac_tlv->sta_mac(), status_code,
                                                     reason_code)) {
            LOG(ERROR) << "Failed to add FailedConnectionEvent.";
            return false;
        }
    }
    return true;
}

bool Controller::handle_cmdu_1905_beacon_response(const sMacAddr &src_mac,
                                                  ieee1905_1::CmduMessageRx &cmdu_rx)
{
    // here we need to extract and keep the data received from the STA
    // but currently we'll just print that we are here
    auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "got beacon response from STA. mid: 0x" << std::hex << mid;
    return true;
}

bool Controller::handle_intel_slave_join(
    const sMacAddr &src_mac, std::shared_ptr<wfa_map::tlvApRadioBasicCapabilities> radio_caps,
    beerocks::beerocks_header &beerocks_header, ieee1905_1::CmduMessageTx &cmdu_tx,
    const std::shared_ptr<Agent> &agent)
{
    // Prepare outcoming response vs tlv
    auto join_response =
        beerocks::message_com::add_vs_tlv<beerocks_message::cACTION_CONTROL_SLAVE_JOINED_RESPONSE>(
            cmdu_tx);
    if (!join_response) {
        LOG(ERROR) << "Failed adding intel vendor specific TLV";
        return false;
    }

    if (beerocks_header.action_op() != beerocks_message::ACTION_CONTROL_SLAVE_JOINED_NOTIFICATION) {
        LOG(ERROR) << "Unexpected Intel action op " << beerocks_header.action_op();
        return false;
    }

    auto notification =
        beerocks_header.addClass<beerocks_message::cACTION_CONTROL_SLAVE_JOINED_NOTIFICATION>();
    if (!notification) {
        LOG(ERROR) << "addClass cACTION_CONTROL_SLAVE_JOINED_NOTIFICATION failed";
        return false;
    }

    // mark slave as prplMesh
    // This is redundent for the normal initilization flow, but is needed for the renew flow
    LOG(DEBUG) << "prplMesh agent: received cACTION_CONTROL_SLAVE_JOINED_NOTIFICATION from "
               << src_mac;
    database.set_prplmesh(src_mac);

    std::string slave_version =
        std::string(notification->slave_version(beerocks::message::VERSION_LENGTH));
    sMacAddr radio_mac        = notification->hostap().iface_mac;
    sMacAddr parent_bssid_mac = notification->backhaul_params().backhaul_bssid;
    std::string backhaul_mac  = tlvf::mac_to_string(notification->backhaul_params().backhaul_mac);
    std::string backhaul_ipv4 =
        beerocks::net::network_utils::ipv4_to_string(notification->backhaul_params().backhaul_ipv4);
    beerocks::eIfaceType backhaul_iface_type =
        (beerocks::eIfaceType)notification->backhaul_params().backhaul_iface_type;
    bool is_gw_slave           = (backhaul_iface_type == beerocks::IFACE_TYPE_GW_BRIDGE);
    beerocks::eType ire_type   = is_gw_slave ? beerocks::TYPE_GW : beerocks::TYPE_IRE;
    int backhaul_channel       = notification->backhaul_params().backhaul_channel;
    sMacAddr bridge_mac        = agent->al_mac;
    std::string bridge_mac_str = tlvf::mac_to_string(bridge_mac);
    std::string bridge_ipv4 =
        beerocks::net::network_utils::ipv4_to_string(notification->backhaul_params().bridge_ipv4);
    bool backhaul_manager = (bool)notification->backhaul_params().is_backhaul_manager;
    bool acs_enabled      = (notification->wlan_settings().channel == 0);

    std::string slave_name;
    if (is_gw_slave) {
        // Get device name from etc/config/prplmesh
        std::string gw_device_name;
        constexpr char sf_option_path[] = "%s.%s.%s";
        char conf_path[32] = {0};
        // Generate a uci path to the option we wish to lookup
        if (snprintf(conf_path, 32, sf_option_path, "prplmesh", "config", "device_name") > 0) {
            if (0 != uci_get_option_value_controller(conf_path, gw_device_name))
                slave_name = gw_device_name;
                //"GW" + std::string(notification->platform_settings().local_master ? "_MASTER" : "_SLAVE_ONLY");
        }
    } else {
        if (!(std::string(notification->backhaul_params().backhaul_device_name)).empty())
            slave_name = std::string(notification->backhaul_params().backhaul_device_name);
    }
            //: ("IRE_" +
            //   (notification->platform_settings().local_master ? "MASTER_" : std::string()) +
            //   bridge_mac_str.substr(bridge_mac_str.size() - 5, bridge_mac_str.size() - 1));

    LOG(INFO) << "IRE Slave joined" << std::endl
              << "    slave_version=" << slave_version << std::endl
              << "    slave_name=" << slave_name << std::endl
              << "    parent_bssid_mac=" << parent_bssid_mac << std::endl
              << "    backhaul_mac=" << backhaul_mac << std::endl
              << "    backhaul_ipv4=" << backhaul_ipv4 << std::endl
              << "    bridge_mac=" << bridge_mac << std::endl
              << "    bridge_ipv4=" << bridge_ipv4 << std::endl
              << "    backhaul_manager=" << int(backhaul_manager) << std::endl
              << "    backhaul_type=" << beerocks::utils::get_iface_type_string(backhaul_iface_type)
              << std::endl
              << "    low_pass_filter_on = " << int(notification->low_pass_filter_on()) << std::endl
              << "    radio_mac = " << radio_mac << std::endl
              << "    channel = " << int(notification->wlan_settings().channel) << std::endl
              << "    is_gw_slave = " << int(is_gw_slave) << std::endl;

    if (!is_gw_slave) {

        // if not local GW but local master - then the parent bssid is remote AP (aka "far AP")
        // and is not yet in map
        if (!notification->platform_settings().local_master) {
            // rejecting join if gw haven't joined yet
            if ((parent_bssid_mac != beerocks::net::network_utils::ZERO_MAC) &&
                (!database.has_node(parent_bssid_mac) ||
                 (database.get_node_state(tlvf::mac_to_string(parent_bssid_mac)) !=
                  beerocks::STATE_CONNECTED))) {
                LOG(DEBUG) << "sending back join reject!";
                LOG(DEBUG) << "reject_debug: parent_bssid_has_node="
                           << (int)(database.has_node(parent_bssid_mac));

                join_response->err_code() = beerocks::JOIN_RESP_REJECT;
                return son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);
            }
        }

        // sending to BML listeners, client disconnect notification on ire backhaul before changing it type from TYPE_CLIENT to TYPE_IRE_BACKHAUL
        if (database.get_node_type(backhaul_mac) == beerocks::TYPE_CLIENT &&
            database.get_node_state(backhaul_mac) == beerocks::STATE_CONNECTED) {
            LOG(DEBUG) << "BML, sending IRE connect CONNECTION_CHANGE for mac " << backhaul_mac
                       << ", FORCING DISCONNECT NOTIFICATION!";
            bml_task::connection_change_event new_event;
            new_event.mac                     = backhaul_mac;
            new_event.force_client_disconnect = true;
            tasks.push_event(database.get_bml_task_id(), bml_task::CONNECTION_CHANGE, &new_event);
        }

        //TODO might need to handle bssids of VAP nodes as well in this case
        if (parent_bssid_mac != beerocks::net::network_utils::ZERO_MAC) {
            //add a placeholder
            LOG(DEBUG) << "add a placeholder backhaul_mac = " << backhaul_mac
                       << ", parent_bssid_mac = " << parent_bssid_mac;
            database.add_node_wireless_backhaul(tlvf::mac_from_string(backhaul_mac),
                                                parent_bssid_mac);
        } else if (database.get_node_state(backhaul_mac) != beerocks::STATE_CONNECTED) {
            /* if the backhaul node doesn't exist, or is not already marked as connected,
            * we assume it is connected to the GW's LAN switch
            */
            LOG(DEBUG) << "connected to the GW's LAN switch ";
            auto gw = database.get_gw();
            if (!gw) {
                LOG(ERROR) << "can't get GW node!";
                return false;
            }

            auto gw_mac          = tlvf::mac_to_string(gw->al_mac);
            auto gw_lan_switches = database.get_node_children(gw_mac, beerocks::TYPE_ETH_SWITCH);

            if (gw_lan_switches.empty()) {
                LOG(ERROR) << "GW has no LAN SWITCH node!";
                return false;
            }

            auto gw_lan_switch = *gw_lan_switches.begin();

            LOG(DEBUG) << "add a placeholder backhaul_mac = " << backhaul_mac
                       << " gw_lan_switch = " << gw_lan_switch
                       << " TYPE_IRE_BACKHAUL , STATE_CONNECTED";
            database.add_node_wireless_backhaul(tlvf::mac_from_string(backhaul_mac),
                                                tlvf::mac_from_string(gw_lan_switch));
            database.set_node_state(backhaul_mac, beerocks::STATE_CONNECTED);
        }
    } else {
        backhaul_mac.clear();
    }

    //if the IRE connects via a different backhaul, mark previous backhaul as disconnected
    std::string previous_backhaul = database.get_node_parent(bridge_mac_str);
    if (!previous_backhaul.empty() && previous_backhaul != backhaul_mac &&
        database.get_node_type(previous_backhaul) == beerocks::TYPE_IRE_BACKHAUL) {
        LOG(DEBUG) << "marking previous backhaul " << previous_backhaul << " for IRE " << bridge_mac
                   << " as disconnected";
        database.set_node_state(previous_backhaul, beerocks::STATE_DISCONNECTED);
    }

    // bridge_mac node may have been created from DHCP/ARP event, if so delete it
    // this may only occur once
    if (database.has_node(bridge_mac) && (database.get_node_type(bridge_mac_str) != ire_type)) {
        database.remove_node(bridge_mac);
    }
    // add new GW/IRE bridge_mac
    LOG(DEBUG) << "adding node " << bridge_mac << " under " << backhaul_mac << ", and mark as type "
               << ire_type;
    if (is_gw_slave) {
        database.add_node_gateway(bridge_mac);
        agent->is_gateway = true;
    } else {
        database.add_node_ire(bridge_mac, tlvf::mac_from_string(backhaul_mac));
    }

    database.set_node_state(bridge_mac_str, beerocks::STATE_CONNECTED);
    agent->state = beerocks::STATE_CONNECTED;

    /*
    * Set IRE backhaul manager slave
    * keep in mind that the socket's peer mac will be the hostap mac
    */
    if (backhaul_manager) {
        /*
        * handle the IRE node itself, representing the backhaul
        */
        database.set_node_backhaul_iface_type(backhaul_mac, backhaul_iface_type);
        database.set_node_backhaul_iface_type(bridge_mac_str, beerocks::IFACE_TYPE_BRIDGE);

        database.set_node_ipv4(backhaul_mac, bridge_ipv4);
        database.set_node_ipv4(bridge_mac_str, bridge_ipv4);

        database.set_node_manufacturer(backhaul_mac, agent->manufacturer);

        database.set_node_type(backhaul_mac, beerocks::TYPE_IRE_BACKHAUL);

        database.set_node_name(backhaul_mac, slave_name + "_BH");
        database.set_node_name(bridge_mac_str, slave_name);

        //TODO slave should include eth switch mac in the message
        //until then, generate eth address from bridge address
        auto eth_sw_mac_binary =
            beerocks::net::network_utils::get_eth_sw_mac_from_bridge_mac(bridge_mac);

        std::string eth_switch_mac = tlvf::mac_to_string(eth_sw_mac_binary);
        database.add_node_wired_backhaul(tlvf::mac_from_string(eth_switch_mac), bridge_mac);
        database.set_node_state(eth_switch_mac, beerocks::STATE_CONNECTED);
        database.set_node_name(eth_switch_mac, slave_name + "_ETH");
        database.set_node_ipv4(eth_switch_mac, bridge_ipv4);
        database.set_node_manufacturer(eth_switch_mac, agent->manufacturer);

        //run locating task on ire
        if (!database.is_node_wireless(backhaul_mac)) {
            LOG(DEBUG) << "run_client_locating_task client_mac = " << bridge_mac;
            auto new_task = std::make_shared<client_locating_task>(database, cmdu_tx, tasks,
                                                                   bridge_mac_str, true, 2000);
            tasks.add_task(new_task);
        }

        //Run the client locating tasks for the previously located wired IRE. If cascaded IREs are connected with wire
        //the slave_join notification for the 2nd level IRE can come before 1st level IRE, causing the 2nd
        //level IRE to be placed at the same level as the 1st IRE in the DB
        auto agents = database.get_all_connected_agents();
        for (const auto &a : agents) {
            if (a->al_mac == bridge_mac || a->is_gateway) {
                LOG(INFO) << "client_locating_task is not run again for this ire: " << a->al_mac;
                continue;
            }
            auto ire_backhaul_mac =
                database.get_node_parent_backhaul(tlvf::mac_to_string(a->al_mac));
            if (!database.is_node_wireless(ire_backhaul_mac)) {
                LOG(DEBUG) << "run_client_locating_task client_mac = " << a->al_mac;
                auto new_task = std::make_shared<client_locating_task>(
                    database, cmdu_tx, tasks, tlvf::mac_to_string(a->al_mac), true, 2000);
                tasks.add_task(new_task);
            }
        }
    }

    // Check Slave BeeRocks version //
    auto slave_version_s  = beerocks::version::version_from_string(slave_version);
    auto master_version_s = beerocks::version::version_from_string(BEEROCKS_VERSION);

    beerocks::string_utils::copy_string(join_response->master_version(), BEEROCKS_VERSION,
                                        beerocks::message::VERSION_LENGTH);

    // check if mismatch
    if (slave_version_s.major != master_version_s.major ||
        slave_version_s.minor != master_version_s.minor) {
        LOG(WARNING) << "IRE Slave joined, Mismatch version! slave_version="
                     << std::string(slave_version)
                     << " master_version=" << std::string(BEEROCKS_VERSION);
        LOG(WARNING) << " bridge_mac=" << bridge_mac << " bridge_ipv4=" << bridge_ipv4;
    }

    beerocks::eIfaceType hostap_iface_type =
        (beerocks::eIfaceType)notification->hostap().iface_type;

    LOG(INFO) << std::endl
              << "    hostap_iface_name=" << notification->hostap().iface_name << std::endl
              << "    hostap_iface_type="
              << beerocks::utils::get_iface_type_string(hostap_iface_type) << std::endl
              << "    ant_num=" << int(notification->hostap().ant_num)
              << " ant_gain=" << int(notification->hostap().ant_gain)
              << " channel=" << int(notification->cs_params().channel)
              << " conducted=" << int(notification->hostap().tx_power) << std::endl
              << "    radio_mac=" << radio_mac << std::endl;

    bool local_master = (bool)notification->platform_settings().local_master;
    if (local_master) {
#ifdef BEEROCKS_RDKB
        LOG(DEBUG) << "platform rdkb_extensions_enabled="
                   << int(notification->platform_settings().rdkb_extensions_enabled);

        database.settings_rdkb_extensions(
            notification->platform_settings().rdkb_extensions_enabled);
        if (database.settings_rdkb_extensions()) {
            int prev_task_id = database.get_rdkb_wlan_task_id();
            if (!tasks.is_task_running(prev_task_id)) {
                LOG(DEBUG) << "starting RDKB task";
                auto new_rdkb_wlan_task =
                    std::make_shared<rdkb_wlan_task>(database, cmdu_tx, tasks);
                tasks.add_task(new_rdkb_wlan_task);
            }
        } else {
            LOG(DEBUG) << "rdkb_extensions is not enabled";
        }
#endif
        database.settings_client_band_steering(
            notification->platform_settings().client_band_steering_enabled);
        database.settings_client_optimal_path_roaming(
            notification->platform_settings().client_optimal_path_roaming_enabled);
        database.settings_client_optimal_path_roaming_prefer_signal_strength(
            notification->platform_settings()
                .client_optimal_path_roaming_prefer_signal_strength_enabled);
        database.settings_client_11k_roaming(
            notification->platform_settings().client_11k_roaming_enabled);
        database.settings_load_balancing(notification->platform_settings().load_balancing_enabled);
        database.settings_service_fairness(
            notification->platform_settings().service_fairness_enabled);
        database.settings_dfs_reentry(false);
    }

    /*
    * handle the HOSTAP node
    */
    if (database.has_node(radio_mac)) {
        if (database.get_node_type(tlvf::mac_to_string(radio_mac)) != beerocks::TYPE_SLAVE) {
            database.set_node_type(tlvf::mac_to_string(radio_mac), beerocks::TYPE_SLAVE);
            LOG(ERROR) << "Existing mac node is not TYPE_SLAVE";
        }
    } else {
        database.add_node_radio(radio_mac, bridge_mac);
    }

    //reset/init radio stats when adding slave's radio node
    database.clear_hostap_stats_info(bridge_mac, radio_mac);

    auto radio = database.get_radio(bridge_mac, radio_mac);
    if (!radio) {
        LOG(ERROR) << "Radio not found";
        return false;
    }

    radio->is_acs_enabled = acs_enabled;
    // Make sure AP is marked as not active. It will be set as active after setting all the
    // radio parameters on the database.
    son_actions::set_hostap_active(database, tasks, tlvf::mac_to_string(radio_mac), false);
    if (backhaul_manager) {
        agent->backhaul.wireless_backhaul_radio = radio;
    }

    database.set_node_state(tlvf::mac_to_string(radio_mac), beerocks::STATE_CONNECTED);
    database.set_node_backhaul_iface_type(tlvf::mac_to_string(radio_mac),
                                          is_gw_slave ? beerocks::IFACE_TYPE_GW_BRIDGE
                                                      : beerocks::IFACE_TYPE_BRIDGE);
    database.set_hostap_iface_name(bridge_mac, radio_mac, notification->hostap().iface_name);
    database.set_hostap_iface_type(bridge_mac, radio_mac, hostap_iface_type);

    database.set_hostap_ant_num(radio_mac, (beerocks::eWiFiAntNum)notification->hostap().ant_num);
    database.set_hostap_ant_gain(bridge_mac, radio_mac, notification->hostap().ant_gain);
    database.set_hostap_tx_power(bridge_mac, radio_mac, notification->hostap().tx_power);

    database.set_node_ipv4(tlvf::mac_to_string(radio_mac), bridge_ipv4);

    if (database.get_node_5ghz_support(tlvf::mac_to_string(radio_mac))) {
        if (notification->low_pass_filter_on()) {
            database.set_hostap_band_capability(bridge_mac, radio_mac, beerocks::LOW_SUBBAND_ONLY);
        } else {
            database.set_hostap_band_capability(bridge_mac, radio_mac, beerocks::BOTH_SUBBAND);
        }
    } else {
        database.set_hostap_band_capability(bridge_mac, radio_mac,
                                            beerocks::SUBBAND_CAPABILITY_UNKNOWN);
    }
    autoconfig_wsc_parse_radio_caps(radio_mac, radio_caps);

    // send JOINED_RESPONSE with son config
    {

        beerocks::string_utils::copy_string(
            join_response->master_version(beerocks::message::VERSION_LENGTH), BEEROCKS_VERSION,
            beerocks::message::VERSION_LENGTH);
        join_response->config().monitor_total_ch_load_notification_hi_th_percent =
            database.config.monitor_total_ch_load_notification_hi_th_percent;
        join_response->config().monitor_total_ch_load_notification_lo_th_percent =
            database.config.monitor_total_ch_load_notification_lo_th_percent;
        join_response->config().monitor_total_ch_load_notification_delta_th_percent =
            database.config.monitor_total_ch_load_notification_delta_th_percent;
        join_response->config().monitor_min_active_clients =
            database.config.monitor_min_active_clients;
        join_response->config().monitor_active_client_th = database.config.monitor_active_client_th;
        join_response->config().monitor_client_load_notification_delta_th_percent =
            database.config.monitor_client_load_notification_delta_th_percent;
        join_response->config().monitor_rx_rssi_notification_threshold_dbm =
            database.config.monitor_rx_rssi_notification_threshold_dbm;
        join_response->config().monitor_rx_rssi_notification_delta_db =
            database.config.monitor_rx_rssi_notification_delta_db;
        join_response->config().monitor_ap_idle_threshold_B =
            database.config.monitor_ap_idle_threshold_B;
        join_response->config().monitor_ap_active_threshold_B =
            database.config.monitor_ap_active_threshold_B;
        join_response->config().monitor_ap_idle_stable_time_sec =
            database.config.monitor_ap_idle_stable_time_sec;
        join_response->config().monitor_disable_initiative_arp =
            database.config.monitor_disable_initiative_arp;
        join_response->config().ire_rssi_report_rate_sec = database.config.ire_rssi_report_rate_sec;

        LOG(DEBUG) << "send SLAVE_JOINED_RESPONSE";
        son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);
    }

    // calling this function to update arp monitor with new ip addr (bridge ip), which is diffrent from the ip received from, dhcp on the backhaul
    if (backhaul_manager && (!is_gw_slave) && database.is_node_wireless(backhaul_mac)) {
        son_actions::handle_completed_connection(database, cmdu_tx, tasks, backhaul_mac);
    }

    // update bml listeners
    bml_task::connection_change_event bml_new_event;
    bml_new_event.mac = bridge_mac_str;
    tasks.push_event(database.get_bml_task_id(), bml_task::CONNECTION_CHANGE, &bml_new_event);
    LOG(DEBUG) << "BML, sending IRE connect CONNECTION_CHANGE for mac " << bml_new_event.mac;

    // sending event to CS task
    LOG(DEBUG) << "CS_task,sending SLAVE_JOINED_EVENT for mac " << radio_mac;
    auto cs_new_event                  = new channel_selection_task::sSlaveJoined_event;
    cs_new_event->backhaul_is_wireless = beerocks::utils::is_node_wireless(backhaul_iface_type);
    cs_new_event->backhaul_channel     = backhaul_channel;
    cs_new_event->channel              = notification->cs_params().channel;
    cs_new_event->low_pass_filter_on   = notification->low_pass_filter_on();
    LOG(DEBUG) << "cs_new_event->low_pass_filter_on = " << int(cs_new_event->low_pass_filter_on)
               << " cs_new_event = " << intptr_t(cs_new_event);
    cs_new_event->hostap_mac = radio_mac;
    cs_new_event->cs_params  = notification->cs_params();

    tasks.push_event(database.get_channel_selection_task_id(),
                     (int)channel_selection_task::eEvent::SLAVE_JOINED_EVENT, (void *)cs_new_event);
#ifdef BEEROCKS_RDKB
    // sending event to rdkb_wlan_task
    if (database.settings_rdkb_extensions()) {
        LOG(DEBUG) << "rdkb_wlan_task,sending STEERING_SLAVE_JOIN for mac " << radio_mac;
        rdkb_wlan_task::steering_slave_join_event new_event{};
        new_event.radio_mac = tlvf::mac_to_string(radio_mac);
        tasks.push_event(database.get_rdkb_wlan_task_id(),
                         rdkb_wlan_task::events::STEERING_SLAVE_JOIN, &new_event);
    }
#endif
    // In the case where wireless-BH is lost and agents reconnect to the controller
    // it is required to re-activate the AP in the nodes-map since it is set as not-active
    // when the topology-response not containing it is received by the controller.
    // When it joins the controller we need to activate it if not activated.
    database.set_hostap_active(radio_mac, true);

    // Update all (Slaves) last seen timestamp
    if (database.get_node_type(tlvf::mac_to_string(radio_mac)) == beerocks::TYPE_SLAVE) {
        database.update_node_last_seen(tlvf::mac_to_string(radio_mac));
    }

    if (!database.dm_set_device_multi_ap_backhaul(
            *agent, parent_bssid_mac, tlvf::mac_from_string(backhaul_mac), backhaul_iface_type)) {
        LOG(ERROR) << "Device set MultiAPDevice.Backhaul object is failed.";
    }

    return true;
}

/**
 * @brief Parse the radio basic capabilities TLV and store the operating class
 * in the database as supported channels.
 *
 * @param radio_mac radio mac address (RUID in non-Intel agent case)
 * @param radio_caps radio basic capabilities TLV received from the remote agent
 * @return true on success
 * @return false on failure
 */
bool Controller::autoconfig_wsc_parse_radio_caps(
    const sMacAddr &radio_mac, std::shared_ptr<wfa_map::tlvApRadioBasicCapabilities> radio_caps)
{
    // read all operating class list
    auto operating_classes_list_length = radio_caps->operating_classes_info_list_length();
    if (operating_classes_list_length > beerocks::message::SUPPORTED_CHANNELS_LENGTH) {
        LOG(WARNING) << "operating class info list larger then maximum supported channels";
        operating_classes_list_length = beerocks::message::SUPPORTED_CHANNELS_LENGTH;
    }

    /*
    ** Here need to remove the OperatingClasses data element
    ** from the Controler Data Model because we are entering a new one
    */
    database.remove_hostap_supported_operating_classes(radio_mac);

    std::stringstream ss;
    for (int oc_idx = 0; oc_idx < operating_classes_list_length; oc_idx++) {
        auto operating_class_tuple = radio_caps->operating_classes_info_list(oc_idx);
        if (!std::get<0>(operating_class_tuple)) {
            LOG(ERROR) << "getting operating class entry has failed!";
            return false;
        }
        auto &op_class                  = std::get<1>(operating_class_tuple);
        auto operating_class            = op_class.operating_class();
        auto maximum_transmit_power_dbm = op_class.maximum_transmit_power_dbm();
        ss << "operating_class=" << int(operating_class) << std::endl;
        ss << "maximum_transmit_power_dbm=" << int(maximum_transmit_power_dbm) << std::endl;
        ss << "channel list={ ";
        auto channel_list = son::wireless_utils::operating_class_to_channel_set(operating_class);
        for (auto channel : channel_list) {
            ss << int(channel) << " ";
        }
        ss << "}" << std::endl;
        ss << "statically_non_operable_channel_list={ ";

        auto non_oper_channels_list_length =
            op_class.statically_non_operable_channels_list_length();
        std::vector<uint8_t> non_operable_channels;
        for (int ch_idx = 0; ch_idx < non_oper_channels_list_length; ch_idx++) {
            auto channel = op_class.statically_non_operable_channels_list(ch_idx);
            ss << int(*channel) << " ";
            non_operable_channels.push_back(*channel);
        }
        ss << " }" << std::endl;
        // store operating class in the DB for this hostap
        database.add_hostap_supported_operating_class(
            radio_mac, operating_class, maximum_transmit_power_dbm, non_operable_channels);
    }
    LOG(DEBUG) << "Radio basic capabilities:" << std::endl
               << ss.str() << std::endl
               << "Supported Channels:" << std::endl
               << database.get_hostap_supported_channels_string(radio_mac);

    return true;
}

bool Controller::handle_non_intel_slave_join(
    const sMacAddr &src_mac, std::shared_ptr<wfa_map::tlvApRadioBasicCapabilities> radio_caps,
    const WSC::m1 &m1, const std::shared_ptr<Agent> &agent, const sMacAddr &radio_mac,
    ieee1905_1::CmduMessageTx &cmdu_tx)
{

    // Multi-AP Agent doesn't say anything about the backhaul, so simulate ethernet backhaul to satisfy
    // network map. MAC address is the bridge MAC with the last octet incremented by 1.
    // The mac address for the backhaul is the same since it is ethernet backhaul.
    sMacAddr bridge_mac = agent->al_mac;
    sMacAddr mac        = bridge_mac;
    mac.oct[5]++;
    std::string backhaul_mac = tlvf::mac_to_string(mac);
    // generate eth address from bridge address
    auto eth_switch_mac_binary = beerocks::net::network_utils::get_eth_sw_mac_from_bridge_mac(mac);
    std::string eth_switch_mac = tlvf::mac_to_string(eth_switch_mac_binary);
    LOG(INFO) << "IRE generic Slave joined" << std::endl
              << "    manufacturer=" << agent->manufacturer << std::endl
              << "    al_mac=" << bridge_mac << std::endl
              << "    eth_switch_mac=" << eth_switch_mac << std::endl
              << "    backhaul_mac=" << backhaul_mac << std::endl
              << "    radio_identifier = " << radio_mac << std::endl;

    LOG(DEBUG) << "simulate backhaul connected to the GW's LAN switch ";
    auto gw = database.get_gw();
    if (!gw) {
        LOG(ERROR) << "can't get GW node!";
        return false;
    }

    auto gw_mac          = tlvf::mac_to_string(gw->al_mac);
    auto gw_lan_switches = database.get_node_children(gw_mac, beerocks::TYPE_ETH_SWITCH);

    if (gw_lan_switches.empty()) {
        LOG(ERROR) << "GW has no LAN SWITCH node!";
        return false;
    }

    auto gw_lan_switch = *gw_lan_switches.begin();

    LOG(DEBUG) << "add a placeholder backhaul_mac = " << backhaul_mac
               << " gw_lan_switch = " << gw_lan_switch << " TYPE_IRE_BACKHAUL , STATE_CONNECTED";
    database.add_node_wireless_backhaul(tlvf::mac_from_string(backhaul_mac),
                                        tlvf::mac_from_string(gw_lan_switch));
    database.set_node_state(backhaul_mac, beerocks::STATE_CONNECTED);

    // TODO bridge handling.
    // Assume repeater
    beerocks::eType ire_type = beerocks::TYPE_IRE;

    std::string bridge_mac_str = tlvf::mac_to_string(bridge_mac);

    // bridge_mac node may have been created from DHCP/ARP event, if so delete it
    // this may only occur once
    if (database.has_node(bridge_mac) && (database.get_node_type(bridge_mac_str) != ire_type)) {
        database.remove_node(bridge_mac);
    }
    // add new GW/IRE bridge_mac
    LOG(DEBUG) << "adding node " << bridge_mac << " under " << backhaul_mac << ", and mark as type "
               << ire_type;

    database.add_node_ire(bridge_mac, tlvf::mac_from_string(backhaul_mac));

    agent->state = beerocks::STATE_CONNECTED;
    database.set_node_state(bridge_mac_str, beerocks::STATE_CONNECTED);
    database.set_node_backhaul_iface_type(backhaul_mac, beerocks::eIfaceType::IFACE_TYPE_ETHERNET);
    database.set_node_backhaul_iface_type(bridge_mac_str, beerocks::IFACE_TYPE_BRIDGE);
    database.set_node_manufacturer(backhaul_mac, agent->manufacturer);
    database.set_node_type(backhaul_mac, beerocks::TYPE_IRE_BACKHAUL);
    database.set_node_name(backhaul_mac, agent->manufacturer + "_BH");
    database.set_node_name(bridge_mac_str, agent->manufacturer);
    database.add_node_wired_backhaul(tlvf::mac_from_string(eth_switch_mac), bridge_mac);
    database.set_node_state(eth_switch_mac, beerocks::STATE_CONNECTED);
    database.set_node_name(eth_switch_mac, agent->manufacturer + "_ETH");
    database.set_node_manufacturer(eth_switch_mac, agent->manufacturer);

    // Update existing node, or add a new one
    if (database.has_node(radio_mac)) {
        if (database.get_node_type(tlvf::mac_to_string(radio_mac)) != beerocks::TYPE_SLAVE) {
            database.set_node_type(tlvf::mac_to_string(radio_mac), beerocks::TYPE_SLAVE);
            LOG(ERROR) << "Existing mac node is not TYPE_SLAVE";
        }
    } else {
        database.add_node_radio(radio_mac, bridge_mac);
    }

    //reset/init radio stats when adding slave's radio node
    database.clear_hostap_stats_info(bridge_mac, radio_mac);

    // TODO Assume no backhaul manager

    database.set_node_state(tlvf::mac_to_string(radio_mac), beerocks::STATE_CONNECTED);
    database.set_node_backhaul_iface_type(tlvf::mac_to_string(radio_mac),
                                          beerocks::IFACE_TYPE_BRIDGE);
    database.set_hostap_iface_name(bridge_mac, radio_mac, "N/A");
    database.set_hostap_iface_type(bridge_mac, radio_mac, beerocks::IFACE_TYPE_WIFI_UNSPECIFIED);

    // TODO number of antennas comes from HT/VHT capabilities (implicit from NxM)
    // TODO ant_gain and tx_power will not be set
    database.set_hostap_ant_num(radio_mac, beerocks::eWiFiAntNum::ANT_NONE);
    database.set_hostap_ant_gain(bridge_mac, radio_mac, 0);
    database.set_hostap_tx_power(bridge_mac, radio_mac, 0);
    database.set_hostap_active(radio_mac, true);
    // TODO ipv4 will not be set

    autoconfig_wsc_parse_radio_caps(radio_mac, radio_caps);
    // TODO assume SSIDs are not hidden

    // TODO
    //        if (database.get_node_5ghz_support(radio_mac)) {
    //            if (notification->low_pass_filter_on()) {
    //                database.set_hostap_band_capability(radio_mac, beerocks::LOW_SUBBAND_ONLY);
    //            } else {
    //                database.set_hostap_band_capability(radio_mac, beerocks::BOTH_SUBBAND);
    //            }
    //        } else {
    database.set_hostap_band_capability(bridge_mac, radio_mac,
                                        beerocks::SUBBAND_CAPABILITY_UNKNOWN);
    //        }

    // TODO: Improve MultiAPDevice.Backhaul from EasyMesh messages not only VS (PPM-1659)
    if (!database.dm_set_device_multi_ap_backhaul(*agent, beerocks::net::network_utils::ZERO_MAC,
                                                  tlvf::mac_from_string(backhaul_mac),
                                                  beerocks::eIfaceType::IFACE_TYPE_ETHERNET)) {
        LOG(ERROR) << "Device set MultiAPDevice.Backhaul object is failed.";
    }

    // update bml listeners
    bml_task::connection_change_event bml_new_event;
    bml_new_event.mac = bridge_mac_str;
    tasks.push_event(database.get_bml_task_id(), bml_task::CONNECTION_CHANGE, &bml_new_event);
    LOG(DEBUG) << "BML, sending IRE connect CONNECTION_CHANGE for mac " << bml_new_event.mac;

    LOG(DEBUG) << "send AP_AUTOCONFIG_WSC M2";
    return son_actions::send_cmdu_to_agent(src_mac, cmdu_tx, database);
}

bool Controller::handle_cmdu_control_message(
    const sMacAddr &src_mac, std::shared_ptr<beerocks::beerocks_header> beerocks_header)
{
    sMacAddr radio_mac        = beerocks_header->actionhdr()->radio_mac();
    std::string radio_mac_str = tlvf::mac_to_string(radio_mac);

    // Sanity tests
    if (radio_mac == beerocks::net::network_utils::ZERO_MAC) {
        if (beerocks_header->action_op() !=
            beerocks_message::ACTION_CONTROL_CLIENT_DHCP_COMPLETE_NOTIFICATION) {
            /*
                dhcp_complete_notification is sent to the controller
                without a radio mac. This is the expected behavior.
                Currently, the message is ignored which leads to the
                IP of the station not showing in the bml-connection-map.
                Don't ignore this message even though radio_mac is zero.
            */
            LOG(ERROR) << "CMDU received with id=" << int(beerocks_header->id())
                       << " op=" << int(beerocks_header->action_op()) << " with empty mac!";
            return false;
        }
    }

    if (beerocks_header->actionhdr()->direction() == beerocks::BEEROCKS_DIRECTION_AGENT) {
        return true;
    }

    //Update all (Slaves) last seen timestamp
    if (database.get_node_type(radio_mac_str) == beerocks::TYPE_SLAVE) {
        database.update_node_last_seen(radio_mac_str);
    }

    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_CONTROL_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE: {
        LOG(DEBUG)
            << "received ACTION_CONTROL_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE from "
            << radio_mac;
        auto response = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE>();

        if (response == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        auto new_event        = new channel_selection_task::sRestrictedChannelResponse_event;
        new_event->hostap_mac = beerocks_header->actionhdr()->radio_mac();
        new_event->success    = response->success();
        tasks.push_event(database.get_channel_selection_task_id(),
                         (int)channel_selection_task::eEvent::RESTRICTED_CHANNEL_RESPONSE_EVENT,
                         (void *)new_event);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_AP_DISABLED_NOTIFICATION: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_HOSTAP_AP_DISABLED_NOTIFICATION>();

        if (notification == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        int vap_id = notification->vap_id();
        LOG(INFO) << "received ACTION_CONTROL_HOSTAP_AP_DISABLED_NOTIFICATION from " << radio_mac
                  << " vap_id=" << vap_id;

        const auto disabled_bssid = database.get_hostap_vap_mac(radio_mac, vap_id);
        if (disabled_bssid == beerocks::net::network_utils::ZERO_MAC) {
            LOG(INFO) << "AP Disabled on unknown vap, vap_id=" << vap_id;
            break;
        }
        auto client_list =
            database.get_node_children(tlvf::mac_to_string(disabled_bssid), beerocks::TYPE_CLIENT);

        for (auto &client : client_list) {
            son_actions::handle_dead_node(client, true, database, cmdu_tx, tasks);
        }

        // Update BSSes in the Agent
        auto radio = database.get_radio(src_mac, radio_mac);
        if (!radio) {
            LOG(ERROR) << "No radio found for radio_uid " << radio_mac << " on " << src_mac;
            break;
        }

        database.remove_vap(*radio, vap_id);

        if (radio->bsses.erase(disabled_bssid) != 1) {
            LOG(ERROR) << "No BSS " << disabled_bssid << " could be erased on " << radio_mac;
        }

        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_AP_ENABLED_NOTIFICATION: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_HOSTAP_AP_ENABLED_NOTIFICATION>();

        if (notification == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        int vap_id = notification->vap_id();
        LOG(INFO) << "received ACTION_CONTROL_HOSTAP_AP_ENABLED_NOTIFICATION from " << radio_mac
                  << " vap_id=" << vap_id;

        auto bssid = notification->vap_info().mac;
        auto ssid  = std::string((char *)notification->vap_info().ssid);

        database.add_vap(radio_mac_str, vap_id, tlvf::mac_to_string(bssid), ssid,
                         notification->vap_info().backhaul_vap);

        // Update BSSes in the Agent
        auto radio = database.get_radio(src_mac, radio_mac);
        if (!radio) {
            LOG(ERROR) << "No radio found for radio_uid " << radio_mac << " on " << src_mac;
            break;
        }

        auto bss = radio->bsses.add(bssid, *radio, vap_id);
        LOG_IF(bss->vap_id != vap_id, ERROR)
            << "BSS " << bssid << " changed vap_id " << bss->vap_id << " -> " << vap_id;
        bss->ssid     = ssid;
        bss->backhaul = notification->vap_info().backhaul_vap;

        // update bml listeners
        bml_task::connection_change_event new_event;
        new_event.mac = tlvf::mac_to_string(database.get_node_parent_ire(radio_mac_str));
        tasks.push_event(database.get_bml_task_id(), bml_task::CONNECTION_CHANGE, &new_event);
        LOG(DEBUG) << "BML, sending IRE connect CONNECTION_CHANGE for mac " << new_event.mac;

        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_CSA_ERROR_NOTIFICATION: {
        std::string backhaul_mac = database.get_node_parent(radio_mac_str);

        LOG(ERROR) << "Hostap CSA ERROR for IRE " << backhaul_mac << " hostap mac=" << radio_mac;

        // TODO handle CSA error
        son_actions::set_hostap_active(database, tasks, radio_mac_str, false);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_CSA_NOTIFICATION: {
        LOG(DEBUG) << "ACTION_CONTROL_HOSTAP_CSA_NOTIFICATION from " << radio_mac;

        auto notification =
            beerocks_header->addClass<beerocks_message::cACTION_CONTROL_HOSTAP_CSA_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_CSA_NOTIFICATION failed";
            return false;
        }

        LOG(DEBUG) << "CS_task,sending CSA_EVENT for mac " << radio_mac;
        auto new_event        = new channel_selection_task::sCsa_event;
        new_event->hostap_mac = beerocks_header->actionhdr()->radio_mac();
        new_event->cs_params  = notification->cs_params();
        tasks.push_event(database.get_channel_selection_task_id(),
                         (int)channel_selection_task::eEvent::CSA_EVENT, (void *)new_event);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_ACS_NOTIFICATION: {
        LOG(DEBUG) << "ACTION_CONTROL_HOSTAP_ACS_NOTIFICATION from " << radio_mac;

        auto notification =
            beerocks_header->addClass<beerocks_message::cACTION_CONTROL_HOSTAP_ACS_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_ACS_NOTIFICATION failed";
            return false;
        }
        LOG(DEBUG) << "CS_task,sending ACS_RESPONSE_EVENT for mac " << radio_mac;

        auto new_event        = new channel_selection_task::sAcsResponse_event;
        new_event->hostap_mac = radio_mac;
        new_event->cs_params  = notification->cs_params();
        tasks.push_event(database.get_channel_selection_task_id(),
                         (int)channel_selection_task::eEvent::ACS_RESPONSE_EVENT,
                         (void *)new_event);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION: {
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_ACS_NOTIFICATION failed";
            return false;
        }

        // Update BSSes in the Agent
        auto radio = database.get_radio(src_mac, radio_mac);
        if (!radio) {
            LOG(ERROR) << "No radio found for radio_uid " << radio_mac << " on " << src_mac;
            break;
        }
        radio->bsses.clear();

        std::unordered_map<int8_t, sVapElement> vaps_info;
        std::string vaps_list;
        for (int8_t vap_id = beerocks::IFACE_VAP_ID_MIN; vap_id <= beerocks::IFACE_VAP_ID_MAX;
             vap_id++) {
            auto vap_mac = tlvf::mac_to_string(notification->params().vaps[vap_id].mac);
            if (vap_mac != beerocks::net::network_utils::ZERO_MAC_STRING) {
                auto bss =
                    radio->bsses.add(notification->params().vaps[vap_id].mac, *radio, vap_id);
                bss->ssid     = std::string((char *)notification->params().vaps[vap_id].ssid);
                bss->backhaul = notification->params().vaps[vap_id].backhaul_vap;

                vaps_info[vap_id].mac = vap_mac;
                vaps_info[vap_id].ssid =
                    std::string((char *)notification->params().vaps[vap_id].ssid);
                vaps_info[vap_id].backhaul_vap = notification->params().vaps[vap_id].backhaul_vap;
                vaps_list += ("    vap_id=" + std::to_string(vap_id) +
                              ", vap_mac=" + (vaps_info[vap_id]).mac +
                              " , ssid=" + (vaps_info[vap_id]).ssid + std::string("\n"));
            }
        }

        LOG(INFO) << "sACTION_CONTROL_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION from slave " << radio_mac
                  << std::endl
                  << "vaps_list:" << std::endl
                  << vaps_list;

        for (auto vap : vaps_info) {
            if (!database.has_node(tlvf::mac_from_string(vap.second.mac))) {
                database.add_virtual_node(tlvf::mac_from_string(vap.second.mac), radio_mac);
            }
        }

        database.set_hostap_vap_list(radio_mac, vaps_info);

        // update bml listeners
        bml_task::connection_change_event new_event;
        new_event.mac = tlvf::mac_to_string(database.get_node_parent_ire(radio_mac_str));
        tasks.push_event(database.get_bml_task_id(), bml_task::CONNECTION_CHANGE, &new_event);
        LOG(DEBUG) << "BML, sending IRE connect CONNECTION_CHANGE for mac " << new_event.mac;

        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_ARP_MONITOR_NOTIFICATION: {

        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_ARP_MONITOR_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_ARP_MONITOR_NOTIFICATION failed";
            return false;
        }

        std::string client_mac = tlvf::mac_to_string(notification->params().mac);
        std::string client_ipv4 =
            beerocks::net::network_utils::ipv4_to_string(notification->params().ipv4);
        LOG(DEBUG) << "received arp monitor notification from slave mac " << radio_mac << ":"
                   << std::endl
                   << "   client_mac=" << client_mac << std::endl
                   << "   client_ipv4=" << client_ipv4 << std::endl
                   << "   state=" << int(notification->params().state)
                   << "   source=" << int(notification->params().source)
                   << "   type=" << int(notification->params().type);

        // IMPORTANT: Ignore RTM_DELNEIGH messages on the GRX350/IRE220 platforms.
        // Since the transport layer is accelerated, the OS may incorrectly decide
        // that a connected client has disconnected.
        //  if(notification->params.type == ARP_TYPE_DELNEIGH && !database.is_node_wireless(client_mac)) {
        //     LOG(INFO) << "ARP type RTM_DELNEIGH received!! handle dead client mac = " << client_mac;
        //     son_actions::handle_dead_node(client_mac, true, database, tasks);
        //     break;
        //  }

        if (client_ipv4 == beerocks::net::network_utils::ZERO_IP_STRING) {
            LOG(DEBUG) << "arp ipv4 is 0.0.0.0, ignoring";
            break;
        }

        bool new_node = !database.has_node(tlvf::mac_from_string(client_mac));

        beerocks::eType new_node_type = database.get_node_type(client_mac);

        if ((new_node == false) && (new_node_type != beerocks::TYPE_CLIENT) &&
            (new_node_type != beerocks::TYPE_UNDEFINED)) {
            LOG(DEBUG) << "node " << client_mac << " type: " << (int)new_node_type
                       << " is (not a client/backhaul node) and (not stale), ignoring";
            break;
        }
        bool run_locating_task = false;
        // Since wireless clients are added to the DB on association, an ARP on non-existing node
        // can only be received for Ethernet clients
        if (new_node || !database.is_node_wireless(client_mac)) {

            // Assume node is connected to the GW's LAN switch
            // client_locating_task will find the correct position
            if (new_node) {
                LOG(DEBUG) << "handle_control_message - calling add_node_to_gw_default_location "
                              "client_mac = "
                           << client_mac;
                if (!son_actions::add_node_to_default_location(database, client_mac)) {
                    LOG(ERROR) << "handle_control_message - add_node_to_default_location failed!";
                    break;
                }
                new_node_type = database.get_node_type(client_mac);
            }

            // New IP
            if (new_node || database.get_node_ipv4(client_mac) != client_ipv4) {
                LOG(DEBUG) << "Update node IP - mac: " << client_mac << " ipv4: " << client_ipv4;
                database.set_node_ipv4(client_mac, client_ipv4);
                son_actions::handle_completed_connection(database, cmdu_tx, tasks, client_mac);
            }

            // Run locating task only on CLIENTs or IREs
            if ((new_node_type == beerocks::TYPE_CLIENT) || (new_node_type == beerocks::TYPE_IRE)) {
                run_locating_task = true;
            }

            // Wireless Node
        } else {

            // Client NOT connected
            if (database.get_node_state(client_mac) == beerocks::STATE_DISCONNECTED) {
                LOG(DEBUG) << "node_state = DISCONNECTED client_mac = " << client_mac
                           << " client_ipv4 =" << client_ipv4;

                // The IP has changed
            } else if (database.get_node_ipv4(client_mac) != client_ipv4) {

                LOG(DEBUG) << "Update node IP - mac: " << client_mac << " ipv4: " << client_ipv4;
                database.set_node_ipv4(client_mac, client_ipv4);
                son_actions::handle_completed_connection(database, cmdu_tx, tasks, client_mac);
            }
        }

        // Update the last-seen timestamp
        // Handled at this point to make sure the client was added to the DB
        database.update_node_last_seen(client_mac);

        // Run client locating task for reachable or stale client/IRE nodes only if on ETH_FRONT port
        // or WIRELESS_FRONT (in case of eth devices connected to IREs and arp notf was send from GW)
        if (run_locating_task &&
            ((notification->params().source == beerocks::ARP_SRC_ETH_FRONT) ||
             (notification->params().source == beerocks::ARP_SRC_WIRELESS_FRONT))) {
            LOG(DEBUG) << "run_client_locating_task client_mac = " << client_mac;

            auto eth_switches =
                database.get_node_siblings(radio_mac_str, beerocks::TYPE_ETH_SWITCH);
            if (eth_switches.size() != 1) {
                LOG(ERROR) << "SLAVE " << radio_mac << " does not have an Ethernet switch sibling!";
                break;
            }

            std::string eth_switch = *(eth_switches.begin());

            auto client = database.get_station(tlvf::mac_from_string(client_mac));
            if (!client) {
                LOG(ERROR) << "client " << client_mac << " not found";
                break;
            }

            int prev_task_id = client->get_client_locating_task_id(true /*reachable*/);

            if (tasks.is_task_running(prev_task_id)) {
                LOG(DEBUG) << "client locating task already running for " << client_mac;
            } else {
                LOG(DEBUG) << "running client_locating_task on client = " << client_mac;
                auto new_task = std::make_shared<client_locating_task>(
                    database, cmdu_tx, tasks, client_mac, true /*reachable*/, 2000, eth_switch);
                tasks.add_task(new_task);
            }
        } else {
            LOG(INFO) << "Not running client_locating_task for client_mac " << client_mac
                      << " notification->params.source: " << (int)notification->params().source;
        }

        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_START_NOTIFICATION: {
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_START_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass "
                          "ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_START_NOTIFICATION failed";
            return false;
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE failed";
            return false;
        }

        std::string client_mac = tlvf::mac_to_string(notification->params().result.mac);
        std::string ap_mac     = radio_mac_str;

        auto client = database.get_station(tlvf::mac_from_string(client_mac));
        if (!client) {
            LOG(ERROR) << "client " << client_mac << " not found";
            return false;
        }

        bool is_parent = (tlvf::mac_from_string(database.get_node_parent(client_mac)) ==
                          database.get_hostap_vap_mac(tlvf::mac_from_string(ap_mac),
                                                      notification->params().vap_id));

        auto agent = database.m_agents.get(src_mac);
        if (!agent) {
            LOG(ERROR) << "agent " << src_mac << " not found";
            break;
        }

        auto radio = database.get_radio(src_mac, radio_mac);
        if (!radio) {
            LOG(ERROR) << "No radio found with uid " << radio_mac << " on " << src_mac;
            break;
        }

        auto bh                  = agent->backhaul.wireless_backhaul_radio;
        bool is_backhaul_manager = (bh && (bh->radio_uid == radio->radio_uid));

        LOG_CLI(DEBUG,
                "rssi measurement response: "
                    << client_mac << " (sta) <-> (ap) " << ap_mac
                    << " rx_packets=" << int(notification->params().rx_packets)
                    << " rx_rssi=" << int(notification->params().rx_rssi)
                    << " phy_rate_100kb (RX|TX)=" << int(notification->params().rx_phy_rate_100kb)
                    << " | " << int(notification->params().tx_phy_rate_100kb)
                    << " is_parent=" << (is_parent ? "1" : "0")
                    << " is_backhaul_manager=" << (is_backhaul_manager ? "1" : "0")
                    << " src_module=" << int(notification->params().src_module)
                    << " id=" << int(beerocks_header->id())
                    << " bssid=" << database.get_node_parent(client_mac)
                    << " vap_id=" << int(notification->params().vap_id));

        //response return from slave backhaul manager , updating the matching same band sibling.
        if (is_backhaul_manager &&
            database.is_node_wireless(database.get_node_parent_backhaul(ap_mac)) &&
            database.is_node_5ghz(client_mac)) {
            auto priv_ap_mac = ap_mac;
            ap_mac           = database.get_5ghz_sibling_hostap(ap_mac);
            LOG(DEBUG) << "update rssi measurement BH manager from ap_mac = " << priv_ap_mac
                       << " to = " << ap_mac;
        }
        if (ap_mac.empty()) {
            LOG(ERROR) << "update rssi measurement failed";
        }
        client->set_cross_rx_rssi(ap_mac, notification->params().rx_rssi,
                                  notification->params().rx_packets);
        if (is_parent) {
            client->cross_tx_phy_rate_100kb = notification->params().tx_phy_rate_100kb;
            client->cross_rx_phy_rate_100kb = notification->params().rx_phy_rate_100kb;
        }
#ifdef BEEROCKS_RDKB
        if (database.settings_rdkb_extensions() &&
            (beerocks_header->id() == database.get_rdkb_wlan_task_id())) {
            beerocks_message::sSteeringEvSnr new_event;
            new_event.snr        = notification->params().rx_snr;
            new_event.client_mac = notification->params().result.mac;
            new_event.bssid      = database.get_hostap_vap_mac(tlvf::mac_from_string(ap_mac),
                                                          notification->params().vap_id);
            tasks.push_event(database.get_rdkb_wlan_task_id(),
                             rdkb_wlan_task::events::STEERING_EVENT_SNR_AVAILABLE, &new_event);
        }
#endif
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_NOTIFICATION: {
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_NOTIFICATION failed";
            return false;
        }
        std::string client_mac = tlvf::mac_to_string(notification->params().result.mac);

        auto client = database.get_station(tlvf::mac_from_string(client_mac));
        if (!client) {
            LOG(ERROR) << "client " << client_mac << " not found";
            return false;
        }

        std::string client_parent_mac = database.get_node_parent(client_mac);
        sMacAddr bssid = database.get_hostap_vap_mac(radio_mac, notification->params().vap_id);
        bool is_parent = (tlvf::mac_from_string(client_parent_mac) == bssid);

        int rx_rssi = int(notification->params().rx_rssi);

        LOG_CLI(DEBUG,
                "measurement change notification: "
                    << client_mac << " (sta) <-> (ap) " << radio_mac << " rx_rssi=" << rx_rssi
                    << " phy_rate_100kb (RX|TX)=" << int(notification->params().rx_phy_rate_100kb)
                    << " | " << int(notification->params().tx_phy_rate_100kb));

        if ((database.get_node_type(client_mac) == beerocks::TYPE_CLIENT) &&
            (database.get_node_state(client_mac) == beerocks::STATE_CONNECTED) &&
            (!database.get_node_handoff_flag(*client)) && is_parent) {

            client->set_cross_rx_rssi(radio_mac_str, notification->params().rx_rssi,
                                      notification->params().rx_packets);
            client->cross_tx_phy_rate_100kb = notification->params().tx_phy_rate_100kb;
            client->cross_rx_phy_rate_100kb = notification->params().rx_phy_rate_100kb;

            /*
                * when a notification arrives, it means a large change in rx_rssi occurred (above the defined thershold)
                * therefore, we need to create an optimal path task to relocate the node if needed
                */
            if (tasks.is_task_running(client->roaming_task_id)) {
                LOG(DEBUG) << "roaming task already running for " << client_mac;
            } else {
                auto new_task = std::make_shared<optimal_path_task>(database, cmdu_tx, tasks,
                                                                    client_mac, 0, "");
                tasks.add_task(new_task);
            }
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_NO_RESPONSE_NOTIFICATION: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_NO_RESPONSE_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_NO_RESPONSE_NOTIFICATION failed";
            return false;
        }
        std::string client_mac = tlvf::mac_to_string(notification->mac());

        LOG(DEBUG) << "ACTION_CONTROL_CLIENT_NO_RESPONSE_NOTIFICATION, client_mac=" << client_mac
                   << " hostap mac=" << radio_mac;

        if (database.get_node_type(client_mac) == beerocks::TYPE_IRE_BACKHAUL) {
            LOG(INFO) << "IRE CLIENT_NO_RESPONSE_NOTIFICATION, client_mac=" << client_mac
                      << " hostap mac=" << radio_mac
                      << " closing socket and marking as disconnected";
            bool reported_by_parent = radio_mac_str == database.get_node_parent(client_mac);
            son_actions::handle_dead_node(client_mac, reported_by_parent, database, cmdu_tx, tasks);
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_DHCP_COMPLETE_NOTIFICATION: {
        auto notification_in =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_DHCP_COMPLETE_NOTIFICATION>();
        if (!notification_in) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_DHCP_COMPLETE_NOTIFICATION failed";
            return false;
        }

        std::string client_mac = tlvf::mac_to_string(notification_in->mac());
        std::string ipv4 = beerocks::net::network_utils::ipv4_to_string(notification_in->ipv4());
        LOG(DEBUG) << "dhcp complete for client " << client_mac << " new ip=" << ipv4
                   << " previous ip=" << database.get_node_ipv4(client_mac);

        if (!database.has_node(tlvf::mac_from_string(client_mac))) {
            LOG(DEBUG) << "client mac not in DB, add temp node " << client_mac;
            database.add_node_station(tlvf::mac_from_string(client_mac));
            database.update_node_last_seen(client_mac);
        }

        if (database.get_node_type(client_mac) != beerocks::TYPE_CLIENT) {
            LOG(INFO) << "Ignoring DHCP notification for mac " << client_mac
                      << ", as it's not a client";
            return true;
        }

        database.set_node_ipv4(client_mac, ipv4);
        database.set_node_name(
            client_mac, std::string(notification_in->name(beerocks::message::NODE_NAME_LENGTH)));

        if (database.is_node_wireless(client_mac)) {
            auto notification_out = beerocks::message_com::create_vs_message<
                beerocks_message::cACTION_CONTROL_CLIENT_NEW_IP_ADDRESS_NOTIFICATION>(cmdu_tx);

            if (!notification_out) {
                LOG(ERROR) << "Failed building message!";
                return false;
            }
            notification_out->mac()  = notification_in->mac();
            notification_out->ipv4() = notification_in->ipv4();

            auto client_bssid = database.get_node_parent(client_mac);
            if (client_bssid.empty()) {
                LOG(WARNING) << "Client does not have a valid parent hostap on the database";
                return true;
            }
            auto client_radio = database.get_node_parent_radio(client_bssid);
            LOG(WARNING) << "Client " << client_mac
                         << " is connected wirelessly, Sending IP addr notification to radio="
                         << client_radio;

            auto agent_mac = tlvf::mac_from_string(database.get_node_parent(client_radio));
            son_actions::send_cmdu_to_agent(agent_mac, cmdu_tx, database, client_radio);

        } else {
            LOG(DEBUG) << "run_client_locating_task client_mac = " << client_mac;
            auto client = database.get_station(tlvf::mac_from_string(client_mac));
            if (!client) {
                LOG(ERROR) << "client " << client_mac << " not found";
                break;
            }
            int prev_task_id = client->get_client_locating_task_id(true);
            if (tasks.is_task_running(prev_task_id)) {
                LOG(DEBUG) << "client locating task already running for " << client_mac;
            } else {
                auto new_task = std::make_shared<client_locating_task>(database, cmdu_tx, tasks,
                                                                       client_mac, true, 2000);
                tasks.add_task(new_task);
            }
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION: {
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION failed";
            return false;
        }
        LOG(DEBUG) << "received ACTION_CONTROL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION hostap_mac="
                   << radio_mac;

        auto new_event        = new channel_selection_task::sCacCompleted_event;
        new_event->hostap_mac = radio_mac;
        new_event->params     = notification->params();
        tasks.push_event(database.get_channel_selection_task_id(),
                         (int)channel_selection_task::eEvent::CAC_COMPLETED_EVENT,
                         (void *)new_event);

        auto channel_ext_above =
            (notification->params().frequency < notification->params().center_frequency1) ? true
                                                                                          : false;
        if (!database.set_node_channel_bw(
                radio_mac, notification->params().channel,
                beerocks::eWiFiBandwidth(notification->params().bandwidth), channel_ext_above,
                channel_ext_above, notification->params().center_frequency1)) {
            LOG(ERROR) << "set node channel bw failed, mac=" << radio_mac;
        }

        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION: {
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR)
                << "addClass cACTION_CONTROL_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION failed";
            return false;
        }
        LOG(DEBUG)
            << "received ACTION_CONTROL_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION hostap_mac="
            << radio_mac;

        auto new_event        = new channel_selection_task::sDfsChannelAvailable_event;
        new_event->hostap_mac = radio_mac;
        new_event->params     = notification->params();
        tasks.push_event(database.get_channel_selection_task_id(),
                         (int)channel_selection_task::eEvent::DFS_CHANNEL_AVAILABLE_EVENT,
                         (void *)new_event);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_STATS_MEASUREMENT_RESPONSE: {
        auto response =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_HOSTAP_STATS_MEASUREMENT_RESPONSE>();
        if (response == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_STATS_MEASUREMENT_RESPONSE failed";
            return false;
        }

        for (auto i = 0; i < response->sta_stats_size(); i++) {
            auto sta_stats_tuple = response->sta_stats(i);
            if (!std::get<0>(sta_stats_tuple)) {
                LOG(ERROR) << "Couldn't access sta in location " << i;
                continue;
            }
            auto &sta_stats = std::get<1>(sta_stats_tuple);
            auto client_mac = tlvf::mac_to_string(sta_stats.mac);

            if (!database.has_node(tlvf::mac_from_string(client_mac))) {
                LOG(ERROR) << "sta " << client_mac << " is not in DB!";
                continue;
            } else if (database.get_node_state(client_mac) != beerocks::STATE_CONNECTED) {
                LOG(DEBUG) << "sta " << client_mac << " is not connected to hostap " << radio_mac
                           << ", update is invalid!";
                continue;
            }

            //update station bandwidth from the current downlink bandwidth
            if ((sta_stats.dl_bandwidth != beerocks::BANDWIDTH_UNKNOWN) &&
                (sta_stats.dl_bandwidth < beerocks::BANDWIDTH_MAX)) {
                database.update_node_bw(
                    sta_stats.mac, static_cast<beerocks::eWiFiBandwidth>(sta_stats.dl_bandwidth));
            }
            if (database.get_node_type(client_mac) == beerocks::TYPE_IRE_BACKHAUL &&
                    database.get_node_state(client_mac) == beerocks::STATE_CONNECTED)
                database.set_node_link_rssi(client_mac, sta_stats.rx_rssi);

            // Note: The Database node stats and the Datamodels' stats are not the same.
            // Therefore, client information in data model and in node DB might differ.
            database.set_node_stats_info(sta_stats.mac, &sta_stats);
        }

        if (response->ap_stats_size() == 0) {
            break;
        }

        auto ap_stats_tuple = response->ap_stats(response->ap_stats_size() - 1);
        if (!std::get<0>(ap_stats_tuple)) {
            LOG(ERROR) << "Couldn't access ap element";
            return false;
        }
        auto &ap_stats = std::get<1>(ap_stats_tuple);
        database.set_hostap_stats_info(radio_mac, &ap_stats);
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_LOAD_MEASUREMENT_NOTIFICATION: {
        auto ire_mac      = tlvf::mac_to_string(database.get_node_parent_ire(radio_mac_str));
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_HOSTAP_LOAD_MEASUREMENT_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_LOAD_MEASUREMENT_NOTIFICATION failed";
            return false;
        }
        int active_client_count = notification->params().active_client_count;
        int client_load_percent = notification->params().client_tx_load_percent +
                                  notification->params().client_rx_load_percent;

        LOG(DEBUG) << "load notification from hostap " << radio_mac << " ire mac=" << ire_mac
                   << " active_client_count=" << active_client_count
                   << " client_load=" << client_load_percent;

        /*
            * start load balancing
            */
        if (active_client_count > database.config.monitor_min_active_clients &&
            client_load_percent >
                database.config.monitor_total_ch_load_notification_hi_th_percent &&
            database.settings_load_balancing() && database.is_hostap_active(radio_mac) &&
            database.get_node_state(ire_mac) == beerocks::STATE_CONNECTED) {
            /*
                * when a notification arrives, it means a large change in rx_rssi occurred (above the defined thershold)
                * therefore, we need to create a load balancing task to optimize the network
                */
            LOG(DEBUG) << "high load conditions, starting load balancer for ire " << ire_mac;
            int prev_task_id = database.get_load_balancer_task_id(ire_mac);
            if (tasks.is_task_running(prev_task_id)) {
                LOG(DEBUG) << "load balancer task already running for " << ire_mac;
            } else {
                auto new_task = std::make_shared<load_balancer_task>(
                    database, cmdu_tx, tasks, ire_mac, "load notif (high)- load_balancer");
                tasks.add_task(new_task);
            }
        } else if ((active_client_count < database.config.monitor_min_active_clients) &&
                   (client_load_percent <
                    database.config.monitor_total_ch_load_notification_lo_th_percent)) {
            LOG(DEBUG) << "low load conditions, removing confinements from STAs on ire " << ire_mac;
            /*
                * need to free and move previously confined sta
                * TODO
                * need to improve this logic and make it more robust
                */
            auto hostaps = database.get_node_children(ire_mac);
            for (auto &hostap : hostaps) {
                auto stations = database.get_node_children(hostap);
                for (auto sta : stations) {
                    auto station = database.get_station(tlvf::mac_from_string(sta));
                    if (!station) {
                        LOG(ERROR) << "station " << sta << " not found";
                        continue;
                    }

                    if (station->confined) {
                        LOG(DEBUG) << "removing confined flag from sta " << sta;
                        station->confined = false;
                        /*
                            * launch optimal path task
                            */
                        if (database.get_node_state(sta) == beerocks::STATE_CONNECTED) {
                            if (tasks.is_task_running(station->roaming_task_id)) {
                                LOG(DEBUG) << "roaming task already running for " << sta;
                            } else {
                                auto new_task = std::make_shared<optimal_path_task>(
                                    database, cmdu_tx, tasks, sta, 0,
                                    "load notif (low) - optimal_path");
                                tasks.add_task(new_task);
                            }
                        } else {
                            database.set_node_handoff_flag(*station, false);
                        }
                    }
                }
            }
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_BEACON_11K_RESPONSE: {
        auto response =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_BEACON_11K_RESPONSE>();
        if (response == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_BEACON_11K_RESPONSE failed";
            return false;
        }
        LOG_CLI(
            DEBUG,
            "beacon response , ID: "
                << beerocks_header->id() << std::endl
                << "sta_mac: " << response->params().sta_mac << std::endl
                << "measurement_rep_mode: " << (int)response->params().rep_mode << std::endl
                << "op_class: " << (int)response->params().op_class << std::endl
                << "channel: "
                << (int)response->params().channel
                //<< std::endl << "start_time: "           << (int)response->params.start_time
                << std::endl
                << "duration: "
                << (int)response->params().duration
                //<< std::endl << "phy_type: "             << (int)response->params.phy_type
                //<< std::endl << "frame_type: "           << (int)response->params.frame_type
                << std::endl
                << "rcpi: " << (int)response->params().rcpi << std::endl
                << "rsni: " << (int)response->params().rsni << std::endl
                << "bssid: " << response->params().bssid << std::endl
                << "dialog token: " << response->params().dialog_token
            //<< std::endl << "ant_id: "               << (int)response->params.ant_id
            //<< std::endl << "tsf: "                  << (int)response->params.parent_tsf
            //<< std::endl << "new_ch_width: "                         << (int)response->params.new_ch_width
            //<< std::endl << "new_ch_center_freq_seg_0: "             << (int)response->params.new_ch_center_freq_seg_0
            //<< std::endl << "new_ch_center_freq_seg_1: "             << (int)response->params.new_ch_center_freq_seg_1
        );
        if (son_actions::validate_beacon_measurement_report(
                response->params(), tlvf::mac_to_string(response->params().sta_mac),
                tlvf::mac_to_string(response->params().bssid))) {
            database.dm_add_sta_beacon_measurement(response->params());
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE: {
        auto response = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE>();
        if (response == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE failed";
            return false;
        }
        std::string client_mac = tlvf::mac_to_string(response->mac());
        int channel            = database.get_node_channel(client_mac);
        LOG(DEBUG) << "ACTION_CONTROL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE client_mac="
                   << client_mac << " received from hostap " << radio_mac
                   << " channel=" << int(channel) << " ïd = " << int(beerocks_header->id());
        //calculating response delay for associate client ap and cross ap's
        database.set_measurement_recv_delta(radio_mac_str);
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_NO_ACTIVITY_NOTIFICATION: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_NO_ACTIVITY_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass ACTION_CONTROL_CLIENT_NO_ACTIVITY_NOTIFICATION failed";
            return false;
        }
        std::string client_mac = tlvf::mac_to_string(notification->mac());
        LOG(INFO) << "CLIENT NO ACTIVITY MSG RX'ed for client" << client_mac;

        auto client = database.get_station(tlvf::mac_from_string(client_mac));
        if (!client) {
            LOG(ERROR) << "Client " << client_mac << " not found";
            return false;
        }

        if (tasks.is_task_running(client->roaming_task_id)) {
            LOG(DEBUG) << "roaming task already running for " << client_mac;
        } else {
            LOG(INFO) << "Starting optimal path for client" << client_mac;
            auto new_task =
                std::make_shared<optimal_path_task>(database, cmdu_tx, tasks, client_mac, 0, "");
            tasks.add_task(new_task);
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_HOSTAP_ACTIVITY_NOTIFICATION: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_HOSTAP_ACTIVITY_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_HOSTAP_ACTIVITY_NOTIFICATION failed";
            return false;
        }

        database.set_hostap_activity_mode(
            radio_mac, beerocks::eApActiveMode(notification->params().ap_activity_mode));
        if (notification->params().ap_activity_mode == beerocks::AP_IDLE_MODE) {
            LOG(DEBUG) << "CS_task,sending AP_ACTIVITY_IDLE_EVENT for mac " << radio_mac;
            auto new_event        = new channel_selection_task::sApActivityIdle_event;
            new_event->hostap_mac = radio_mac;
            tasks.push_event(database.get_channel_selection_task_id(),
                             (int)channel_selection_task::eEvent::AP_ACTIVITY_IDLE_EVENT,
                             (void *)new_event);
        }

        break;
    }
    case beerocks_message::ACTION_CONTROL_ARP_QUERY_RESPONSE: {
        LOG(DEBUG) << "ACTION_CONTROL_ARP_QUERY_RESPONSE from "
                   << " id=" << beerocks_header->id();
        auto response =
            beerocks_header->addClass<beerocks_message::cACTION_CONTROL_ARP_QUERY_RESPONSE>();
        if (response == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_ARP_QUERY_RESPONSE failed";
            return false;
        }
        break;
    }
#ifdef BEEROCKS_RDKB
    case beerocks_message::ACTION_CONTROL_STEERING_EVENT_CLIENT_ACTIVITY_NOTIFICATION: {
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_STEERING_EVENT_CLIENT_ACTIVITY_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass "
                          "cACTION_CONTROL_STEERING_EVENT_CLIENT_ACTIVITY_NOTIFICATION failed";
            return false;
        }
        //push event to rdkb_wlan_hal task
        if (database.settings_rdkb_extensions()) {
            beerocks_message::sSteeringEvActivity new_event;
            new_event = notification->params();
            tasks.push_event(database.get_rdkb_wlan_task_id(),
                             rdkb_wlan_task::events::STEERING_EVENT_CLIENT_ACTIVITY_AVAILABLE,
                             &new_event);
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_STEERING_EVENT_SNR_XING_NOTIFICATION: {
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_STEERING_EVENT_SNR_XING_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_STEERING_EVENT_SNR_XING_NOTIFICATION failed";
            return false;
        }
        //push event to rdkb_wlan_hal task
        if (database.settings_rdkb_extensions()) {
            beerocks_message::sSteeringEvSnrXing new_event;
            new_event = notification->params();
            tasks.push_event(database.get_rdkb_wlan_task_id(),
                             rdkb_wlan_task::events::STEERING_EVENT_SNR_XING_AVAILABLE, &new_event);
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_STEERING_EVENT_PROBE_REQ_NOTIFICATION: {
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_STEERING_EVENT_PROBE_REQ_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_STEERING_EVENT_PROBE_REQ_NOTIFICATION failed";
            return false;
        }
        //push event to rdkb_wlan_hal task
        if (database.settings_rdkb_extensions()) {
            beerocks_message::sSteeringEvProbeReq new_event;
            new_event = notification->params();
            tasks.push_event(database.get_rdkb_wlan_task_id(),
                             rdkb_wlan_task::events::STEERING_EVENT_PROBE_REQ_AVAILABLE,
                             &new_event);
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_STEERING_EVENT_AUTH_FAIL_NOTIFICATION: {
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_CONTROL_STEERING_EVENT_AUTH_FAIL_NOTIFICATION>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_STEERING_EVENT_AUTH_FAIL_NOTIFICATION failed";
            return false;
        }
        //push event to rdkb_wlan_hal task
        if (database.settings_rdkb_extensions()) {
            beerocks_message::sSteeringEvAuthFail new_event;
            new_event = notification->params();
            tasks.push_event(database.get_rdkb_wlan_task_id(),
                             rdkb_wlan_task::events::STEERING_EVENT_AUTH_FAIL_AVAILABLE,
                             &new_event);
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_STEERING_CLIENT_SET_GROUP_RESPONSE: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_STEERING_CLIENT_SET_GROUP_RESPONSE>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_STEERING_CLIENT_SET_GROUP_REQUEST failed";
            return false;
        }
        //push event to rdkb_wlan_hal task
        if (database.settings_rdkb_extensions()) {
            rdkb_wlan_task::steering_set_group_response_event new_event;
            new_event.ret_code = notification->params().error_code;
            tasks.push_event(database.get_rdkb_wlan_task_id(),
                             rdkb_wlan_task::events::STEERING_SET_GROUP_RESPONSE, &new_event);
        }
        break;
    }
    case beerocks_message::ACTION_CONTROL_STEERING_CLIENT_SET_RESPONSE: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_STEERING_CLIENT_SET_RESPONSE>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_STEERING_CLIENT_SET_RESPONSE failed";
            return false;
        }

        //push event to rdkb_wlan_hal task
        if (database.settings_rdkb_extensions()) {
            rdkb_wlan_task::steering_client_set_response_event new_event;
            new_event.ret_code = notification->params().error_code;
            tasks.push_event(database.get_rdkb_wlan_task_id(),
                             rdkb_wlan_task::events::STEERING_CLIENT_SET_RESPONSE, &new_event);
        }
        break;
    }
#endif // BEEROCKS_RDKB
    case beerocks_message::ACTION_CONTROL_CLIENT_DISCONNECT_RESPONSE: {
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_CONTROL_CLIENT_DISCONNECT_RESPONSE>();
        if (notification == nullptr) {
            LOG(ERROR) << "addClass cACTION_CONTROL_CLIENT_DISCONNECT_RESPONSE failed";
            return false;
        }
#ifdef BEEROCKS_RDKB
        //push event to rdkb_wlan_hal task
        if (database.settings_rdkb_extensions()) {
            rdkb_wlan_task::steering_client_disconnect_response_event new_event;
            new_event.ret_code = notification->params().error_code;
            tasks.push_event(database.get_rdkb_wlan_task_id(),
                             rdkb_wlan_task::events::STEERING_CLIENT_DISCONNECT_RESPONSE,
                             &new_event);
        }
#endif
        break;
    }
    case beerocks_message::ACTION_CONTROL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE: {
        break;
    }
    case beerocks_message::ACTION_CONTROL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION: {
    }
    case beerocks_message::ACTION_CONTROL_CHANNEL_SCAN_RESULTS_NOTIFICATION: {
        break;
    }
    case beerocks_message::ACTION_CONTROL_CHANNEL_SCAN_FINISHED_NOTIFICATION: {
        break;
    }
    case beerocks_message::ACTION_CONTROL_CHANNEL_SCAN_ABORT_NOTIFICATION: {
        break;
    }
    case beerocks_message::ACTION_CONTROL_CLIENT_START_MONITORING_RESPONSE: {
        // handled in association handling task
        break;
    }
    default: {
        LOG_CLI(ERROR, "Unsupported CONTROL action_op: " << int(beerocks_header->action_op()));
        return false;
    }
    }

    // If this is a response message to a task (header->id() == task id), send it to it directly - cmdu_rx is owned by the task
    // e.g. only the task may call addClass
    if (beerocks_header->id()) {
        tasks.response_received(radio_mac_str, beerocks_header);
        return true;
    }

    return true;
}

bool Controller::start_client_steering(const std::string &sta_mac, const std::string &target_bssid)
{
    std::string triggered_by{"NBAPI"};
    bool disassoc_imminent = false;

    LOG(DEBUG) << "NBAPI client steer request for " << sta_mac << " to hostap: " << target_bssid;
    son_actions::steer_sta(database, cmdu_tx, tasks, sta_mac, target_bssid, triggered_by,
                           std::string(), disassoc_imminent,
                           database.config.steering_disassoc_timer_msec.count());
    return true;
}

bool Controller::trigger_scan(
    const sMacAddr &radio_mac,
    std::array<uint8_t, beerocks::message::SUPPORTED_CHANNELS_LENGTH> channel_pool,
    uint8_t pool_size, int dwell_time)
{
    auto channel_pool_set = std::unordered_set<uint8_t>(&channel_pool[0], &channel_pool[pool_size]);

    if (pool_size == 0) {
        LOG(DEBUG) << "Scan for all channels of radio " << radio_mac;
        if (!database.get_pool_of_all_supported_channels(channel_pool_set, radio_mac)) {
            return false;
        }
    }
    if (!database.set_channel_scan_pool(radio_mac, channel_pool_set, true)) {
        LOG(ERROR) << "set_channel_scan_pool failed";
        return false;
    }
    if (!database.set_channel_scan_dwell_time_msec(radio_mac, dwell_time, true)) {
        LOG(ERROR) << "set_channel_scan_dwell_time_msec failed";
        return false;
    }

    LOG(DEBUG) << "NBAPI trigger scan for radio " << radio_mac;

    dynamic_channel_selection_r2_task::sSingleScanRequestEvent new_event = {};
    new_event.radio_mac                                                  = radio_mac;
    tasks.push_event(database.get_dynamic_channel_selection_r2_task_id(),
                     dynamic_channel_selection_r2_task::eEvent::TRIGGER_SINGLE_SCAN, &new_event);
    return true;
}

bool Controller::handle_tlv_profile2_ap_capability(std::shared_ptr<Agent> agent,
                                                   ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto profile2_ap_capability_tlv = cmdu_rx.getClass<wfa_map::tlvProfile2ApCapability>();
    if (!profile2_ap_capability_tlv) {
        LOG(DEBUG) << "getClass wfa_map::tlvProfile2ApCapability has failed";
        return false;
    }

    agent->byte_counter_units = static_cast<wfa_map::tlvProfile2ApCapability::eByteCounterUnits>(
        profile2_ap_capability_tlv->capabilities_bit_field().byte_counter_units);

    agent->max_total_number_of_vids = profile2_ap_capability_tlv->max_total_number_of_vids();

    LOG(DEBUG) << "Profile-2 AP Capability is received, agent bytecounters enum="
               << agent->byte_counter_units;

    return true;
}

bool Controller::handle_tlv_profile2_cac_status_report(std::shared_ptr<Agent::sRadio> radio,
                                                       ieee1905_1::CmduMessageRx &cmdu_rx)
{

    // TODO: Full implementation of CAC Status Report NBAPI (PPM-1496)
    // CAC Report is stored only in datamodels, not in database.
    auto cac_status_report_tlv = cmdu_rx.getClass<wfa_map::tlvProfile2CacStatusReport>();
    if (!cac_status_report_tlv) {
        LOG(DEBUG) << "getClass wfa_map::tlvProfile2CacStatusReport has failed";
        return false;
    }

    LOG(DEBUG) << "Profile-2 CAC Status Report is received";

    database.dm_clear_cac_status_report(radio);

    for (size_t i = 0; i < cac_status_report_tlv->number_of_available_channels(); i++) {

        if (!std::get<0>(cac_status_report_tlv->available_channels(i))) {
            LOG(ERROR) << "Invalid available channel in tlvProfile2CacStatusReport";
            continue;
        }

        const auto &available_channel = std::get<1>(cac_status_report_tlv->available_channels(i));
        database.dm_add_cac_status_available_channel(radio, available_channel.operating_class,
                                                     available_channel.channel);
    }
    return true;
}

} // namespace son
