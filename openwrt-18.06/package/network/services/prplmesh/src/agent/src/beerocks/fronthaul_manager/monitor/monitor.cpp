/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "monitor.h"

#include <bcl/beerocks_cmdu_client_factory_factory.h>
#include <bcl/beerocks_timer_factory_impl.h>
#include <bcl/beerocks_timer_manager_impl.h>
#include <bcl/network/network_utils.h>
#include <bcl/network/sockets.h>
#include <bcl/transaction.h>

#define BEEROCKS_CUSTOM_LOGGER_ID BEEROCKS_MONITOR
#include <bcl/beerocks_logging_custom.h>
#include <bpl/bpl_cfg.h>

#include <beerocks/tlvf/beerocks_message.h>

#include <tlvf/tlvftypes.h>
#include <tlvf/wfa_map/tlvApMetricQuery.h>
#include <tlvf/wfa_map/tlvMetricReportingPolicy.h>

#include <cmath>
#include <vector>

using namespace beerocks;
using namespace son;

#define HAL_MAX_COMMAND_FAILURES 10
#define OPERATION_SUCCESS 0
#define OPERATION_FAIL -1
#define MAX_RADIO_DISBALED_TIMEOUT_SEC 4

// To allow the monitor FSM to not skip a polling iteration due to a small drift, need to use a safety margin buffer
static constexpr std::chrono::milliseconds polling_timer_buffer_msec = std::chrono::milliseconds(5);

/**
 * Implementation-specific measurement period of channel utilization.
 * Currently we use this constant value but a more elaborate solution should read it from
 * configuration.
 */
static constexpr uint8_t ap_metrics_channel_utilization_measurement_period_s = 10;

Monitor::Monitor(const std::string &monitor_iface_,
                 beerocks::config_file::sConfigSlave &beerocks_slave_conf_,
                 beerocks::logging &logger_)
    : EventLoopThread(), monitor_iface(monitor_iface_), beerocks_slave_conf(beerocks_slave_conf_),
      bridge_iface(beerocks_slave_conf.bridge_iface), cmdu_tx(m_tx_buffer, sizeof(m_tx_buffer)),
      logger(logger_), mon_rssi(cmdu_tx),
#ifdef BEEROCKS_RDKB
      mon_rdkb_hal(cmdu_tx),
#endif
      mon_stats(cmdu_tx)
{
    /**
     * Get the MAC address of the radio interface that this monitor instance operates on.
     * This MAC address will later on be used to, for example, extract the information in messages
     * received from controller that is addressed to this monitor instance.
     */
    std::string radio_mac = beerocks::net::network_utils::ZERO_MAC_STRING;
    if (!beerocks::net::network_utils::linux_iface_get_mac(monitor_iface, radio_mac)) {
        LOG(ERROR) << "Failed getting MAC address for interface: " << monitor_iface;
        m_radio_mac = beerocks::net::network_utils::ZERO_MAC;
    } else {
        m_radio_mac = tlvf::mac_from_string(radio_mac);
    }

    auto radio_node = mon_db.get_radio_node();
    radio_node->set_iface(monitor_iface);

    // set polling-rate(msec)
    auto polling_rate_msec = string_utils::stoi(beerocks_slave_conf_.monitor_polling_rate_msec);
    if (polling_rate_msec != 0) {
        mon_db.set_polling_rate_msec(polling_rate_msec);
        LOG(DEBUG) << "Set poll-rate(msec) to: " << polling_rate_msec;
    } else {
        LOG(DEBUG) << "poll-rate(msec) not configured, using default value: "
                   << mon_db.get_polling_rate_msec();
    }

    // set measurement-window(msec)
    auto measurement_window_poll_count =
        string_utils::stoi(beerocks_slave_conf_.monitor_measurement_window_poll_count);
    if ((measurement_window_poll_count != 0) || (polling_rate_msec != 0)) {
        auto measurement_window_msec =
            (measurement_window_poll_count != 0)
                ? (measurement_window_poll_count * polling_rate_msec)
                : (mon_db.MONITOR_DB_DEFAULT_MEASUREMENT_WINDOW_POLL_COUNT * polling_rate_msec);
        mon_db.set_measurement_window_msec(measurement_window_msec);
        LOG(DEBUG) << "Set measurement-window(msec) to: " << measurement_window_msec;
    } else {
        LOG(DEBUG) << "measurement-window-poll-count and poll-rate not configured, using default "
                   << "measurement-window(msec): " << mon_db.get_measurement_window_msec();
    }
}

bool Monitor::send_cmdu(ieee1905_1::CmduMessageTx &cmdu_tx)
{
    return m_slave_client->send_cmdu(cmdu_tx);
}

bool Monitor::thread_init()
{
    // Get Agent UDS file
    std::string agent_uds_path = beerocks_slave_conf.temp_path + std::string(BEEROCKS_AGENT_UDS);

    m_slave_cmdu_client_factory =
        std::move(beerocks::create_cmdu_client_factory(agent_uds_path, m_event_loop));
    LOG_IF(!m_slave_cmdu_client_factory, FATAL) << "Unable to create CMDU client factory!";

    // Create timer factory to create instances of timers.
    auto timer_factory = std::make_shared<beerocks::TimerFactoryImpl>();
    LOG_IF(!timer_factory, FATAL) << "Unable to create timer factory!";

    // Create timer manager to help using application timers.
    m_timer_manager = std::make_shared<beerocks::TimerManagerImpl>(timer_factory, m_event_loop);
    LOG_IF(!m_timer_manager, FATAL) << "Unable to create timer manager!";

    if (m_slave_client) {
        LOG(ERROR) << "Monitor is already started";
        return false;
    }

    // In case of error in one of the steps of this method, we have to undo all the previous steps
    // (like when rolling back a database transaction, where either all steps get executed or none
    // of them gets executed)
    beerocks::Transaction transaction;

    /**
     * Time between successive timer executions of the FSM timer
     */
    auto fsm_timer_period = std::chrono::milliseconds(mon_db.get_polling_rate_msec());
    // Create a timer to run the FSM periodically
    m_fsm_timer = m_timer_manager->add_timer(
        "Monitor FSM", fsm_timer_period, fsm_timer_period,
        [&](int fd, beerocks::EventLoop &loop) { return monitor_fsm(); });
    if (m_fsm_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(ERROR) << "Failed to create the FSM timer";
        return false;
    }
    LOG(DEBUG) << "FSM timer created with fd = " << m_fsm_timer;
    transaction.add_rollback_action([&]() { m_timer_manager->remove_timer(m_fsm_timer); });

    // Create an instance of a CMDU client connected to the CMDU server that is running in the slave
    m_slave_client = std::move(m_slave_cmdu_client_factory->create_instance());
    if (!m_slave_client) {
        LOG(ERROR) << "Failed to create instance of CMDU client";
        return false;
    }
    transaction.add_rollback_action([&]() { m_slave_client.reset(); });

    beerocks::CmduClient::EventHandlers handlers;
    // Install a CMDU-received event handler for CMDU messages received from the slave.
    handlers.on_cmdu_received = [&](uint32_t iface_index, const sMacAddr &dst_mac,
                                    const sMacAddr &src_mac,
                                    ieee1905_1::CmduMessageRx &cmdu_rx) { handle_cmdu(cmdu_rx); };

    // Install a connection-closed event handler.
    handlers.on_connection_closed = [&]() {
        LOG(ERROR) << "Slave socket disconnected!";
        m_slave_client.reset();
    };

    m_slave_client->set_handlers(handlers);
    transaction.add_rollback_action([&]() { m_slave_client->clear_handlers(); });

    // Create new Monitor HAL instance
    bwl::hal_conf_t hal_conf;
    if (!beerocks::bpl::bpl_cfg_get_hostapd_ctrl_path(monitor_iface, hal_conf.wpa_ctrl_path)) {
        LOG(ERROR) << "Couldn't get hostapd control path for interface " << monitor_iface;
        return false;
    }

    if (!beerocks::bpl::bpl_cfg_get_monitored_BSSs_by_radio_iface(monitor_iface,
                                                                  hal_conf.monitored_BSSs)) {
        LOG(DEBUG) << "Failed to get radio-monitored-BSSs for interface " << monitor_iface;
    }

    using namespace std::placeholders; // for `_1`
    mon_wlan_hal = bwl::mon_wlan_hal_create(
        monitor_iface, std::bind(&Monitor::hal_event_handler, this, _1), hal_conf);
    if (!mon_wlan_hal) {
        LOG(ERROR) << "Failed to create HAL instance!";
        return false;
    }

    mon_hal_attached = false;

    transaction.commit();

    bpl::eClientsMeasurementMode clients_measuremet_mode;
    if (!beerocks::bpl::cfg_get_clients_measurement_mode(clients_measuremet_mode)) {
        LOG(WARNING) << "Failed to read clients measurement mode - using defaule value: enable "
                        "measurements for all clients";
        clients_measuremet_mode = bpl::eClientsMeasurementMode::ENABLE_ALL;
    }

    mon_db.set_clients_measuremet_mode(
        (monitor_db::eClientsMeasurementMode)clients_measuremet_mode);

    bool radio_stats_enable;
    if (!beerocks::bpl::cfg_get_radio_stats_enable(radio_stats_enable)) {
        LOG(DEBUG) << "Failed to read radio_stats_enable - using default value: true ";
        radio_stats_enable = true;
    }

    mon_db.set_radio_stats_enable(radio_stats_enable);

    LOG(DEBUG) << "started";

    return true;
}

void Monitor::on_thread_stop()
{
    if (m_slave_client) {
        m_slave_client.reset();
    }

    if (!m_timer_manager->remove_timer(m_fsm_timer)) {
        LOG(ERROR) << "Failed to remove timer";
    }

    if (mon_wlan_hal) {
        if (m_arp_fd != beerocks::net::FileDescriptor::invalid_descriptor) {
            m_event_loop->remove_handlers(m_arp_fd);
        }

        for (auto &fd : m_mon_hal_ext_events) {
            if (fd > 0) {
                m_event_loop->remove_handlers(fd);
            }
        }

        if (m_mon_hal_int_events > 0) {
            m_event_loop->remove_handlers(m_mon_hal_int_events);
        }

        if (m_mon_hal_nl_events > 0) {
            m_event_loop->remove_handlers(m_mon_hal_nl_events);
        }

        mon_rssi.stop();
        mon_stats.stop();
#ifdef BEEROCKS_RDKB
        mon_rdkb_hal.stop();
#endif

        mon_wlan_hal->detach();
        mon_wlan_hal.reset();
    }

    LOG(DEBUG) << "stopped";

    return;
}

bool Monitor::monitor_fsm()
{
    if (!m_logger_configured) {
        logger.set_thread_name(thread_name);
        logger.attach_current_thread_to_logger_id();
        m_logger_configured = true;
    }

    if (!m_slave_client) {
        LOG(ERROR) << "Not connected to slave!";
        return false;
    }

    // Unexpected HAL detach or too many failed commands
    if ((mon_wlan_hal->get_state() != bwl::HALState::Operational && mon_hal_attached == true) ||
        (hal_command_failures_count > HAL_MAX_COMMAND_FAILURES)) {
        LOG(ERROR) << "Unexpected HAL detach detected - Failed commands: "
                   << hal_command_failures_count;
        return false;
    }

    // If the HAL is not yet attached
    if (m_mon_hal_int_events ==
        beerocks::net::FileDescriptor::invalid_descriptor) { // monitor not attached
        auto attach_state = mon_wlan_hal->attach();
        if (last_attach_state != attach_state) {
            LOG(DEBUG) << "attach_state = " << int(attach_state);
            last_attach_state = attach_state;
        }
        if (attach_state == bwl::HALState::Operational) {

            LOG(DEBUG) << "attach_state == bwl::HALState::Operational";

            mon_db.clear();

            // Initialize VAPs in the DB
            update_vaps_in_db();

            // External events
            int ext_event_fd_max = -1;
            m_mon_hal_ext_events = mon_wlan_hal->get_ext_events_fds();
            if (m_mon_hal_ext_events.empty()) {
                ext_event_fd_max = 0;
                LOG(DEBUG)
                    << "No external event FD is available, periodic polling will be done instead.";
            } else {
                beerocks::EventLoop::EventHandlers ext_events_handlers{
                    .name = "mon_hal_ext_events",
                    .on_read =
                        [&](int fd, EventLoop &loop) {
                            if (!mon_wlan_hal->process_ext_events(fd)) {
                                LOG(ERROR) << "process_ext_events(" << fd << " failed!";
                                return false;
                            }
                            return true;
                        },
                    .on_write = nullptr,
                    .on_disconnect =
                        [&](int fd, EventLoop &loop) {
                            LOG(ERROR) << "mon_hal_ext_events disconnected! on fd " << fd;
                            auto it = std::find(m_mon_hal_ext_events.begin(),
                                                m_mon_hal_ext_events.end(), fd);
                            if (it != m_mon_hal_ext_events.end()) {
                                *it = beerocks::net::FileDescriptor::invalid_descriptor;
                            }
                            return false;
                        },
                    .on_error =
                        [&](int fd, EventLoop &loop) {
                            LOG(ERROR) << "mon_hal_ext_events error! on fd " << fd;
                            auto it = std::find(m_mon_hal_ext_events.begin(),
                                                m_mon_hal_ext_events.end(), fd);
                            if (it != m_mon_hal_ext_events.end()) {
                                *it = beerocks::net::FileDescriptor::invalid_descriptor;
                            }
                            return false;
                        },
                };
                for (auto &ext_event_fd : m_mon_hal_ext_events) {
                    if (ext_event_fd > 0) {
                        if (!m_event_loop->register_handlers(ext_event_fd, ext_events_handlers)) {
                            LOG(ERROR) << "Unable to register handlers for external events fd "
                                       << ext_event_fd;
                            return false;
                        }
                        LOG(DEBUG) << "External events queue with fd = " << ext_event_fd;
                    } else if (ext_event_fd < 0) {
                        ext_event_fd = beerocks::net::FileDescriptor::invalid_descriptor;
                    }
                    ext_event_fd_max = std::max(ext_event_fd_max, ext_event_fd);
                }
            }
            if (ext_event_fd_max == 0) {
                LOG(DEBUG)
                    << "No external event FD is available, periodic polling will be done instead.";
            } else if (ext_event_fd_max < 0) {
                LOG(ERROR) << "Invalid external event file descriptors: " << ext_event_fd_max;
                return false;
            }

            // Internal events
            m_mon_hal_int_events = mon_wlan_hal->get_int_events_fd();
            if (m_mon_hal_int_events > 0) {
                beerocks::EventLoop::EventHandlers int_events_handlers{
                    .name = "mon_hal_int_events",
                    .on_read =
                        [&](int fd, EventLoop &loop) {
                            if (!mon_wlan_hal->process_int_events()) {
                                LOG(ERROR) << "process_int_events() failed!";
                                return false;
                            }
                            return true;
                        },
                    .on_write = nullptr,
                    .on_disconnect =
                        [&](int fd, EventLoop &loop) {
                            LOG(ERROR) << "mon_hal_int_events disconnected!";
                            m_mon_hal_int_events =
                                beerocks::net::FileDescriptor::invalid_descriptor;
                            return false;
                        },
                    .on_error =
                        [&](int fd, EventLoop &loop) {
                            LOG(ERROR) << "mon_hal_int_events error!";
                            m_mon_hal_int_events =
                                beerocks::net::FileDescriptor::invalid_descriptor;
                            return false;
                        },
                };
                if (!m_event_loop->register_handlers(m_mon_hal_int_events, int_events_handlers)) {
                    LOG(ERROR) << "Unable to register handlers for internal events queue!";
                    return false;
                }
                LOG(DEBUG) << "Internal events queue with fd = " << m_mon_hal_int_events;
            } else {
                LOG(ERROR) << "Invalid internal event file descriptor: " << m_mon_hal_int_events;
                m_mon_hal_int_events = beerocks::net::FileDescriptor::invalid_descriptor;
                return false;
            }

            m_mon_hal_nl_events = mon_wlan_hal->get_nl_events_fd();
            if (m_mon_hal_nl_events > 0) {
                beerocks::EventLoop::EventHandlers nl_events_handlers{
                    .name = "mon_hal_nl_events",
                    .on_read =
                        [&](int fd, EventLoop &loop) {
                            if (!mon_wlan_hal->process_nl_events()) {
                                LOG(ERROR) << "process_nl_events() failed!";
                                return false;
                            }
                            return true;
                        },
                    .on_write = nullptr,
                    .on_disconnect =
                        [&](int fd, EventLoop &loop) {
                            LOG(ERROR) << "mon_hal_nl_events disconnected!";
                            m_mon_hal_nl_events = beerocks::net::FileDescriptor::invalid_descriptor;
                            return false;
                        },
                    .on_error =
                        [&](int fd, EventLoop &loop) {
                            LOG(ERROR) << "mon_hal_nl_events error!";
                            m_mon_hal_nl_events = beerocks::net::FileDescriptor::invalid_descriptor;
                            return false;
                        },
                };
                if (!m_event_loop->register_handlers(m_mon_hal_nl_events, nl_events_handlers)) {
                    LOG(ERROR) << "Unable to register handlers for Netlink events queue!";
                    return false;
                }
                LOG(DEBUG) << "Netlink events queue with fd = " << m_mon_hal_nl_events;
            } else {
                LOG(ERROR) << "Couldn't get NL socket ";
                m_mon_hal_nl_events = beerocks::net::FileDescriptor::invalid_descriptor;
            }

            LOG(DEBUG) << "sending ACTION_MONITOR_JOINED_NOTIFICATION";
            auto request = message_com::create_vs_message<
                beerocks_message::cACTION_MONITOR_JOINED_NOTIFICATION>(cmdu_tx);
            if (request == nullptr) {
                LOG(ERROR) << "Failed building message!";
                return false;
            }
            request->set_iface_name(monitor_iface);
            send_cmdu(cmdu_tx);

            // On init - set the flag to generate pre-existing client STA_Connected to true
            m_generate_connected_clients_events = true;
            if (!mon_wlan_hal->pre_generate_connected_clients_events()) {
                LOG(WARNING) << "Failed to prepare for generate of clients connected events";
            }

            // start local monitors //
            LOG(TRACE) << "mon_stats.start()";
            if (!mon_stats.start(&mon_db, m_slave_client)) {
                LOG(ERROR) << "mon_stats.start() failed";
                return false;
            }

            LOG(TRACE) << "mon_rssi.start()";
            if (!mon_rssi.start(&mon_db, m_slave_client)) {
                // If monitor rssi failed to start, continue without it. It might failed due to
                // insufficient permissions. Detailed error message is printed inside.
                LOG(WARNING) << "mon_rssi.start() failed, ignore and continue without it";
                mon_rssi.stop();
            } else {
                m_arp_fd = mon_rssi.get_arp_socket()->getSocketFd();
                beerocks::EventLoop::EventHandlers arp_events_handlers{
                    .name = "mon_arp_events",
                    .on_read =
                        [&](int fd, EventLoop &loop) {
                            mon_rssi.arp_recv();
                            return true;
                        },
                    .on_write = nullptr,
                    .on_disconnect =
                        [&](int fd, EventLoop &loop) {
                            LOG(ERROR) << "ARP socket disconnected!";
                            m_arp_fd = beerocks::net::FileDescriptor::invalid_descriptor;
                            return false;
                        },
                    .on_error =
                        [&](int fd, EventLoop &loop) {
                            LOG(ERROR) << "ARP socket error!";
                            m_arp_fd = beerocks::net::FileDescriptor::invalid_descriptor;
                            return false;
                        },
                };
                if (!m_event_loop->register_handlers(m_arp_fd, arp_events_handlers)) {
                    LOG(WARNING) << "Unable to register handlers for arp monitor, ignore and "
                                    "continue without it";
                    mon_rssi.stop();
                }
            }

#ifdef BEEROCKS_RDKB
            LOG(TRACE) << "mon_rdkb_hal.start()";
            if (!mon_rdkb_hal.start(&mon_db, m_slave_client)) {
                LOG(ERROR) << "mon_rdkb_hal.start() failed";
                return false;
            }
#endif

            mon_rssi.is_5ghz = mon_wlan_hal->get_radio_info().is_5ghz;

            LOG(DEBUG) << "Monitor attach process finished successfully!";
            mon_hal_attached = true;

        } else if (attach_state == bwl::HALState::Failed) {
            LOG(ERROR) << "Failed attaching to WLAN HAL";
            return false;
        }

        // HAL is attached and operational
    } else {

        // Process external events
        auto itFdMax =
            std::max_element(std::begin(m_mon_hal_ext_events), std::end(m_mon_hal_ext_events));
        if (itFdMax == std::end(m_mon_hal_ext_events) || *itFdMax == 0) {
            // There is no socket for external events, so we simply try
            // to process any available periodically
            if (!mon_wlan_hal->process_ext_events()) {
                LOG(ERROR) << "process_ext_events() failed!";
                return false;
            }
        }

        if (pending_11k_events.size() > 0) {
            auto now_time = std::chrono::steady_clock::now();
            for (auto it = pending_11k_events.begin(); it != pending_11k_events.end();) {
                if (std::chrono::milliseconds(5000) <= (now_time - it->second.timestamp)) {
                    it = pending_11k_events.erase(it);
                } else {
                    ++it;
                }
            }
        }

        auto now = std::chrono::steady_clock::now();
        // Long running operations prevent the event loop from doing anything else (i.e.: the
        // event loop is not able to react to any incoming request in the meantime).
        // Therefore, limit the maximum amount of time that the following method can run,
        // instead of letting it run non-stop for what could be quite a long time if there
        // are many clients already connected.
        auto max_iteration_timeout =
            now + std::chrono::milliseconds(mon_db.get_polling_rate_msec() / 2);

        if (m_generate_connected_clients_events && (m_next_generate_connected_events_time < now)) {
            bool is_finished_all_clients = false;
            // If there is not enough time to generate all events, the method will be called in the
            // next FSM iteration, and so on until all connected clients are eventually reported.
            auto max_generate_timeout =
                now + std::chrono::milliseconds(GENERATE_CONNECTED_EVENTS_WORK_TIME_LIMIT_MSEC);
            max_generate_timeout = std::min(max_generate_timeout, max_iteration_timeout);
            // If there is not enough time to generate all events, the method will be called in the
            // next FSM iteration, and so on until all connected clients are eventually reported.
            if (!mon_wlan_hal->generate_connected_clients_events(is_finished_all_clients,
                                                                 max_generate_timeout)) {
                LOG(ERROR) << "Failed to generate connected clients events";
                return false;
            }
            m_next_generate_connected_events_time =
                now + std::chrono::milliseconds(GENERATE_CONNECTED_EVENTS_DELAY_MSEC);
            // Reset the flag if finished to generate all clients' events
            m_generate_connected_clients_events = !is_finished_all_clients;
        }

        // Update DB - Polling
        if (now >= (mon_db.get_poll_next_time() - polling_timer_buffer_msec)) {

            mon_db.set_poll_next_time(now +
                                      std::chrono::milliseconds(mon_db.get_polling_rate_msec()));

            // If clients measurement mode is disabled - no need to call update_sta_stats.
            // The differentiation between measure all clients and only specific clients is done
            // as internally in the update_sta_stats.

            update_sta_qos_ctrl_params();

            if (mon_db.get_clients_measuremet_mode() !=
                monitor_db::eClientsMeasurementMode::DISABLE_ALL) {
                // Update the statistics
                update_sta_stats(max_iteration_timeout);
            }

            // NOTE: Radio & VAP statistics are updated only on last poll cycle
            if (mon_db.get_radio_stats_enable() && mon_db.is_last_poll())
                update_ap_stats();

            send_heartbeat();

            mon_db.poll_done();
        }

        if (now >= (mon_db.get_ap_poll_next_time() - polling_timer_buffer_msec)) {
            mon_db.set_ap_poll_next_time(
                now + std::chrono::seconds(mon_db.MONITOR_DB_AP_POLLING_RATE_SEC));

            // Updated tx state in mon_man_hal
            if (!mon_wlan_hal->refresh_radio_info()) {
                LOG(WARNING) << "Failed refreshing the radio info";
                // Try to ping hostap
                if (!mon_wlan_hal->ping()) {
                    LOG(ERROR) << "Failed ping hostap, notify agent...";
                    auto notification = message_com::create_vs_message<
                        beerocks_message::cACTION_MONITOR_HOSTAP_AP_DISABLED_NOTIFICATION>(cmdu_tx);
                    if (notification == nullptr) {
                        LOG(ERROR) << "Failed building "
                                      "cACTION_MONITOR_HOSTAP_AP_DISABLED_NOTIFICATION message!";
                        return false;
                    }

                    notification->vap_id() = beerocks::IFACE_RADIO_ID;
                    send_cmdu(cmdu_tx);
                    return true;
                }
            }

            int8_t new_tx_state             = mon_wlan_hal->get_radio_info().tx_enabled;
            int8_t new_hostap_enabled_state = mon_wlan_hal->get_radio_info().wifi_ctrl_enabled;

            if (mon_db.get_ap_tx_enabled() != new_tx_state) { // tx was changed
                mon_db.set_ap_tx_enabled(new_tx_state);
            }

            if (mon_db.get_hostapd_enabled() !=
                new_hostap_enabled_state) { // ap_enabled was changed
                if (new_hostap_enabled_state == 2) {
                    LOG(DEBUG) << "wifi_ctrl_enabled=2 on already attached to Hostapd";
                }
                mon_db.set_hostapd_enabled(new_hostap_enabled_state);
            }
        }

        if (!mon_db.is_last_poll()) {
            return true;
        }

        // Break if measurement_window_msec didn't pass
        if (now < (mon_db.get_last_stats_update_time() - polling_timer_buffer_msec)) {
            return true;
        }
        mon_db.set_last_stats_update_time(
            now + std::chrono::milliseconds(mon_db.get_measurement_window_msec()));

        //Reach here only after measurement_window_msec has passed and is_last_poll

        mon_rssi.process();
        mon_stats.process();
#ifdef BEEROCKS_RDKB
        mon_rdkb_hal.process(max_iteration_timeout);
#endif

        /**
         * If a Multi-AP Agent receives a Metric Reporting Policy TLV with AP Metrics Channel
         * Utilization Reporting Threshold field set to a non-zero value for a given radio, it
         * shall measure the channel utilization on that radio in each consecutive implementation-
         * specific measurement period and, if the most recently measured channel utilization has
         * crossed the reporting threshold in either direction (with respect to the previous
         * measurement), it shall send an AP Metrics Response message to the Multi-AP Controller
         * containing one AP Metrics TLV for each of the BSSs on that radio.
         *
         * Note that data included in AP Metrics Response message is not fresh data but the data
         * that was obtained in the last polling cycle. If the device in turn implements a similar
         * polling mechanism, returned data can be quite stale. This behavior leads to a situation
         * in which after detecting that channel utilization has crossed the threshold value,
         * statistics have never been obtained yet (all counters are 0). This is actually the case
         * with a RAX40 and test 4.7.6 fails because bytes received is 0.
         * To avoid reporting invalid data, a `vap_stats_available` flag has been created that
         * changes to true when both transmitted and received bytes are greater than 0 for any VAP.
         * Then we do check the flag before checking if channel utilization has crossed the
         * threshold value (and thus, before reporting metrics).
         */
        auto radio_node = mon_db.get_radio_node();
        auto &info      = radio_node->ap_metrics_reporting_info();
        if (radio_node->get_stats().vap_stats_available) {
            int elapsed_time_s =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - info.ap_metrics_channel_utilization_last_reporting_time_point)
                    .count();

            if (elapsed_time_s >= ap_metrics_channel_utilization_measurement_period_s) {
                info.ap_metrics_channel_utilization_last_reporting_time_point = now;

                on_channel_utilization_measurement_period_elapsed();
            }
        }
    }

    return true;
}

void Monitor::on_channel_utilization_measurement_period_elapsed()
{
    /**
     * Measure current channel utilization on the radio.
     */
    uint8_t channel_utilization;
    if (!mon_wlan_hal->get_channel_utilization(channel_utilization)) {
        LOG(ERROR) << "Unable to get channel utilization";
        return;
    }

    /**
     * If previous channel utilization was lower than the threshold and now it is higher than the
     * threshold, report it.
     * Or if previous channel utilization was higher than the threshold and now it's lower than
     * the threshold, report it.
     */
    auto radio_node        = mon_db.get_radio_node();
    auto &info             = radio_node->ap_metrics_reporting_info();
    bool threshold_crossed = false;

    // Check if a threshold has been set
    if (0 != info.ap_channel_utilization_reporting_threshold) {
        if (channel_utilization > info.ap_channel_utilization_reporting_threshold) {
            if (info.ap_metrics_channel_utilization_reporting_value <=
                info.ap_channel_utilization_reporting_threshold) {
                threshold_crossed = true;
            }
        } else if (info.ap_metrics_channel_utilization_reporting_value >
                   info.ap_channel_utilization_reporting_threshold) {
            threshold_crossed = true;
        }
    }
    LOG(DEBUG) << "Channel utilization: previous_value="
               << std::to_string(info.ap_metrics_channel_utilization_reporting_value)
               << ", current_value=" << std::to_string(channel_utilization) << ", threshold_value="
               << std::to_string(info.ap_channel_utilization_reporting_threshold)
               << ", threshold_crossed=" << std::to_string(threshold_crossed);

    info.ap_metrics_channel_utilization_reporting_value = channel_utilization;

    if (threshold_crossed) {
        std::vector<sMacAddr> bssid_list;
        mon_db.get_bssid_list(bssid_list);

        if (!create_ap_metrics_response(0, bssid_list)) {
            LOG(ERROR) << "Unable to create AP Metrics Response message";
            return;
        }

        send_cmdu(cmdu_tx);
    }
}

bool Monitor::create_ap_metrics_response(uint16_t mid, const std::vector<sMacAddr> &bssid_list)
{
    auto cmdu_tx_header =
        cmdu_tx.create(mid, ieee1905_1::eMessageType::AP_METRICS_RESPONSE_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "Failed creating AP_METRICS_RESPONSE_MESSAGE";
        return false;
    }

    for (const auto &bssid : bssid_list) {
        auto vap_node = mon_db.get_vap_node(tlvf::mac_to_string(bssid));
        if (!vap_node) {
            LOG(WARNING) << "Unknown BSSID " << bssid << " - skipping";
            continue;
        }

        auto radio_node = mon_db.get_radio_node();
        if (!mon_stats.add_ap_metrics(cmdu_tx, *vap_node, *radio_node, mon_wlan_hal)) {
            return false;
        }

        auto reporting_info = radio_node->ap_metrics_reporting_info();

        auto include_sta_traffic_stats_tlv =
            reporting_info.include_associated_sta_traffic_stats_tlv_in_ap_metrics_response;
        auto include_sta_link_metrics_tlv =
            reporting_info.include_associated_sta_link_metrics_tlv_in_ap_metrics_response;
        auto include_wifi_6_sta_status_report_tlv =
            reporting_info.include_associated_wifi_6_sta_status_report_tlv_in_ap_metrics_response;

        if (include_sta_traffic_stats_tlv || include_sta_link_metrics_tlv ||
            include_wifi_6_sta_status_report_tlv) {
            for (auto it = mon_db.sta_begin(); it != mon_db.sta_end(); ++it) {
                const auto &sta_mac  = it->first;
                const auto &sta_node = it->second;

                if (sta_node == nullptr) {
                    LOG(WARNING) << "Invalid node pointer for STA = " << sta_mac;
                    continue;
                }
                if (sta_node->get_vap_id() != vap_node->get_vap_id()) {
                    continue;
                }

                if (include_sta_traffic_stats_tlv) {
                    LOG(TRACE) << "Include STA traffic stats for " << sta_node->get_mac();
                    if (!mon_stats.add_ap_assoc_sta_traffic_stat(cmdu_tx, *sta_node)) {
                        LOG(ERROR) << "Failed to add sta_traffic_stat tlv";
                    }
                }
                if (include_sta_link_metrics_tlv) {
                    LOG(TRACE) << "Include STA link metrics for " << sta_node->get_mac();
                    if (!mon_stats.add_ap_assoc_sta_link_metric(cmdu_tx, bssid, *sta_node)) {
                        LOG(ERROR) << "Failed to add sta_link_metric tlv";
                    }
                }
                if (include_wifi_6_sta_status_report_tlv) {
                    LOG(TRACE) << "Include Wifi 6 STA status report for " << sta_node->get_mac();
                    if (!mon_stats.add_ap_assoc_wifi_6_sta_status_report(cmdu_tx, *sta_node)) {
                        LOG(ERROR) << "Failed to add wifi_6_sta_status_report tlv";
                        return false;
                    }
                }
            }
        }
    }

    if (!mon_stats.add_radio_metrics(cmdu_tx, m_radio_mac, *mon_db.get_radio_node())) {
        LOG(ERROR) << "Failed to add radio metrics.";
        return false;
    }
    return true;
}

bool Monitor::update_sta_qos_ctrl_params()
{
    for (auto it = mon_db.sta_begin(); it != mon_db.sta_end(); ++it) {

        auto sta_mac  = it->first;
        auto sta_node = it->second;

        if (sta_node == nullptr) {
            LOG(ERROR) << "Invalid node pointer for STA = " << sta_mac;
            return false;
        }

        auto vap_node             = mon_db.vap_get_by_id(sta_node->get_vap_id());
        auto &sta_qos_ctrl_params = sta_node->get_qos_ctrl_params();

        // Update the stats
        if (!mon_wlan_hal->update_station_qos_control_params(vap_node->get_iface(), sta_mac,
                                                             sta_qos_ctrl_params)) {
            LOG(ERROR) << "Failed updating STA (" << sta_mac << ") QoS control params!";
            return false;
        }
    }
    return true;
}

bool Monitor::update_sta_stats(const std::chrono::steady_clock::time_point &timeout)
{
    auto poll_cnt  = mon_db.get_poll_cnt();
    auto poll_last = mon_db.is_last_poll();

    if (m_sta_stats_polling_completed) {
        m_sta_stats_polling_start_timestamp = std::chrono::steady_clock::now();
        m_sta_stats_polling_completed       = false;
    }

    for (auto it = mon_db.sta_begin(); it != mon_db.sta_end(); ++it) {

        auto sta_mac  = it->first;
        auto sta_node = it->second;

        if (sta_node == nullptr) {
            LOG(WARNING) << "Invalid node pointer for STA = " << sta_mac;
            continue;
        }

        // If clients-measurement-mode is disabled or if it is set to selected-clients-only,
        // the measure_sta_enable flag might be disabled for the clients.
        // If it is disabled - skip the client.
        if (!sta_node->get_measure_sta_enable()) {
            continue;
        }

        auto vap_node   = mon_db.vap_get_by_id(sta_node->get_vap_id());
        auto &sta_stats = sta_node->get_stats();

        // Skip stations that were already updated in the current cycle
        if (sta_stats.last_update_time > m_sta_stats_polling_start_timestamp) {
            continue;
        }

        if (std::chrono::steady_clock::now() > timeout) {
            // This is a potentially long running operation.
            // If we haven't finished iterating on all stations, stop here and continue on next
            // method call from this point on.
            return true;
        }

        // Reset STA poll data
        if (poll_cnt == 0) {
            sta_node->reset_poll_data();
        }
        sta_stats.poll_cnt++;

        // Update the stats
        if (!mon_wlan_hal->update_stations_stats(vap_node->get_iface(), sta_mac,
                                                 sta_stats.hal_stats)) {
            LOG(ERROR) << "Failed updating STA (" << sta_mac << ") statistics!";
            continue;
        }

        // Update TX Phy Rate min
        auto val = sta_stats.hal_stats.tx_phy_rate_100kb;
        if (poll_cnt == 0 || val < sta_stats.tx_phy_rate_100kb_min) {
            sta_stats.tx_phy_rate_100kb_min = val;
        }
        sta_stats.tx_phy_rate_100kb_acc += val;

        // Update RX Phy Rate min
        val = sta_stats.hal_stats.rx_phy_rate_100kb;
        if (poll_cnt == 0 || val < sta_stats.rx_phy_rate_100kb_min) {
            sta_stats.rx_phy_rate_100kb_min = val;
        }
        sta_stats.rx_phy_rate_100kb_acc += val;

        if (poll_last) {
            // Update TX Phy Rate avg
            sta_stats.tx_phy_rate_100kb_avg =
                float(sta_stats.tx_phy_rate_100kb_acc) / float(sta_stats.poll_cnt);

            // Update RX Phy Rate avg
            sta_stats.rx_phy_rate_100kb_avg =
                float(sta_stats.rx_phy_rate_100kb_acc) / float(sta_stats.poll_cnt);

            // Update RSSI
            if (sta_stats.hal_stats.rx_rssi_watt_samples_cnt > 0) {
                float rssi_watt = sta_stats.hal_stats.rx_rssi_watt /
                                  float(sta_stats.hal_stats.rx_rssi_watt_samples_cnt);
                float rssi_db = 10 * log10(rssi_watt);
                if (sta_stats.rx_rssi_curr != int8_t(rssi_db)) {
                    sta_node->set_last_change_time();
                }
                sta_stats.rx_rssi_curr = int8_t(rssi_db);
                //LOG(INFO)  << sta_mac << ", rx_rssi=" << int(rssi_db);
            }

            sta_node->set_rx_rssi_ready(true);

            // Update SNR
            if (sta_stats.hal_stats.rx_snr_watt_samples_cnt > 0) {
                float snr_watt = sta_stats.hal_stats.rx_snr_watt /
                                 float(sta_stats.hal_stats.rx_snr_watt_samples_cnt);
                float snr_db = 10 * log10(snr_watt);
                if (sta_stats.rx_snr_curr != int8_t(snr_db)) {
                    sta_node->set_last_change_time();
                }
                sta_stats.rx_snr_curr = int8_t(snr_db);
                //LOG(INFO)  << sta_mac << ", rx_snr=" << int(snr_db);
            }

            sta_node->set_rx_snr_ready(true);
        }

        // Update the measurement timestamp
        auto now = std::chrono::steady_clock::now();
        auto time_span =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - sta_stats.last_update_time);
        sta_stats.delta_ms         = float(time_span.count());
        sta_stats.last_update_time = now;
    }

    m_sta_stats_polling_completed = true;

    return true;
}

bool Monitor::update_ap_stats()
{
    // Radio Statistics
    auto &radio_stats = mon_db.get_radio_node()->get_stats();

    // Update radio statistics
    if (!mon_wlan_hal->update_radio_stats(radio_stats.hal_stats)) {
        LOG(ERROR) << "Failed updating Radio statistics!";
        return false;
    }

    // Update the measurement timestamp
    auto now = std::chrono::steady_clock::now();
    auto time_span_radio =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - radio_stats.last_update_time);
    radio_stats.delta_ms         = float(time_span_radio.count());
    radio_stats.last_update_time = now;

    // VAP Statistics
    radio_stats.total_retrans_count = 0;
    radio_stats.sta_count           = 0;
    radio_stats.vap_stats_available = false;
    // For every available VAP
    for (int vap_id = beerocks::IFACE_VAP_ID_MIN; vap_id <= beerocks::IFACE_VAP_ID_MAX; vap_id++) {

        auto vap_node = mon_db.vap_get_by_id(vap_id);

        // Break if there are no more available VAPs
        if (!vap_node) {
            continue;
        }

        auto &vap_stats = vap_node->get_stats();

        // Update the stats
        if (!mon_wlan_hal->update_vap_stats(vap_node->get_iface(), vap_stats.hal_stats)) {
            LOG(ERROR) << "Failed updating VAP statistics!";
            return false;
        }

        /**
         * If both transmitted and received bytes counters contain sensible data, then set the flag
         * signaling that VAP statistics are available
         */
        if ((vap_stats.hal_stats.rx_bytes_cnt > 0) && (vap_stats.hal_stats.tx_bytes_cnt > 0)) {
            radio_stats.vap_stats_available = true;
        }

        // Update radio counters
        radio_stats.total_retrans_count += vap_stats.hal_stats.retrans_count;
        radio_stats.sta_count += vap_node->sta_get_count();

        // Update the measurement timestamp
        auto time_span_vap =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - vap_stats.last_update_time);
        vap_stats.delta_ms         = float(time_span_vap.count());
        vap_stats.last_update_time = now;
    }

    return true;
}

void Monitor::handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    if (cmdu_rx.getMessageType() == ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE) {
        handle_cmdu_vs_message(cmdu_rx);
    } else {
        handle_cmdu_ieee1905_1_message(cmdu_rx);
    }
}

void Monitor::handle_cmdu_vs_message(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto beerocks_header = message_com::parse_intel_vs_message(cmdu_rx);
    if (beerocks_header == nullptr) {
        LOG(ERROR) << "Not a vendor specific message";
        return;
    }

    if (beerocks_header->action() != beerocks_message::ACTION_MONITOR) {
        LOG(ERROR) << "Unsupported action: " << int(beerocks_header->action());
        return;
    }

    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_MONITOR_SON_CONFIG_UPDATE: {
        LOG(TRACE) << "received ACTION_MONITOR_SON_CONFIG_UPDATE";
        auto update =
            beerocks_header->addClass<beerocks_message::cACTION_MONITOR_SON_CONFIG_UPDATE>();
        if (update == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_SON_CONFIG_UPDATE failed";
            return;
        }
        mon_stats.conf_total_ch_load_notification_hi_th_percent =
            update->config().monitor_total_ch_load_notification_hi_th_percent;
        mon_stats.conf_total_ch_load_notification_lo_th_percent =
            update->config().monitor_total_ch_load_notification_lo_th_percent;
        mon_stats.conf_total_ch_load_notification_delta_th_percent =
            update->config().monitor_total_ch_load_notification_delta_th_percent;
        mon_stats.conf_min_active_client_count = update->config().monitor_min_active_clients;
        mon_stats.conf_active_client_th        = update->config().monitor_active_client_th;
        mon_stats.conf_client_load_notification_delta_th_percent =
            update->config().monitor_client_load_notification_delta_th_percent;
        mon_stats.conf_ap_idle_threshold_B     = update->config().monitor_ap_idle_threshold_B;
        mon_stats.conf_ap_active_threshold_B   = update->config().monitor_ap_active_threshold_B;
        mon_stats.conf_ap_idle_stable_time_sec = update->config().monitor_ap_idle_stable_time_sec;
        mon_rssi.conf_rx_rssi_notification_delta_db =
            update->config().monitor_rx_rssi_notification_delta_db;
        mon_rssi.conf_rx_rssi_notification_threshold_dbm =
            update->config().monitor_rx_rssi_notification_threshold_dbm;
        mon_rssi.conf_disable_initiative_arp = update->config().monitor_disable_initiative_arp;

        // Mark the enable flag as "false" to force update in hostapd status.
        // The status is polled every "MONITOR_DB_AP_POLLING_RATE_SEC" and update the value.
        mon_db.set_hostapd_enabled(false);
        break;
    }
    case beerocks_message::ACTION_MONITOR_HOSTAP_STATS_MEASUREMENT_REQUEST: {
        // LOG(TRACE) << "received ACTION_MONITOR_HOSTAP_STATS_MEASUREMENT_REQUEST"; // floods the log
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_HOSTAP_STATS_MEASUREMENT_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass ACTION_MONITOR_HOSTAP_STATS_MEASUREMENT_REQUEST failed";
            return;
        }
        mon_stats.add_request(beerocks_header->id(), request->sync());
        if (request->sync()) {
            mon_db.set_poll_next_time(std::chrono::steady_clock::now(), true);
        }
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_ASSOCIATED_STA_LINK_METRIC_REQUEST: {
        LOG(TRACE) << "received ACTION_MONITOR_CLIENT_ASSOCIATED_STA_LINK_METRIC_REQUEST";
        auto request = beerocks_header->addClass<
            beerocks_message::cACTION_MONITOR_CLIENT_ASSOCIATED_STA_LINK_METRIC_REQUEST>();
        if (!request) {
            LOG(ERROR)
                << "addClass ACTION_MONITOR_CLIENT_ASSOCIATED_STA_LINK_METRIC_REQUEST failed";
            return;
        }
        mon_stats.add_request(beerocks_header->id(), request->sync(), request->sta_mac());
        if (request->sync()) {
            mon_db.set_poll_next_time(std::chrono::steady_clock::now(), true);
        }
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANGE_MODULE_LOGGING_LEVEL: {
        LOG(TRACE) << "received ACTION_MONITOR_CHANGE_MODULE_LOGGING_LEVEL";
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANGE_MODULE_LOGGING_LEVEL>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANGE_MODULE_LOGGING_LEVEL failed";
            return;
        }
        logger.set_log_level_state((eLogLevel)request->params().log_level,
                                   request->params().enable);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_START_MONITORING_REQUEST: {
        LOG(TRACE) << "received ACTION_MONITOR_CLIENT_START_MONITORING_REQUEST";
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CLIENT_START_MONITORING_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CLIENT_START_MONITORING_REQUEST failed";
            return;
        }
        std::string sta_mac  = tlvf::mac_to_string(request->params().mac);
        std::string sta_ipv4 = beerocks::net::network_utils::ipv4_to_string(request->params().ipv4);
        std::string set_bridge_4addr_mac = tlvf::mac_to_string(request->params().bridge_4addr_mac);
        LOG(INFO) << "ACTION_MONITOR_CLIENT_START_MONITORING_REQUEST=" << sta_mac
                  << " ip=" << sta_ipv4 << " set_bridge_4addr_mac=" << set_bridge_4addr_mac;

        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CLIENT_START_MONITORING_RESPONSE>(
            cmdu_tx, beerocks_header->id());

        if (!response) {
            LOG(ERROR)
                << "Failed building ACTION_MONITOR_CLIENT_START_MONITORING_RESPONSE message!";
            return;
        }

        auto sta_node = mon_db.sta_find(sta_mac);
        if (!sta_node) {
            LOG(ERROR) << "Could not find sta_node " << sta_mac;
            response->success() = false;
            send_cmdu(cmdu_tx);
            return;
        }

        sta_node->set_ipv4(sta_ipv4);
        sta_node->set_bridge_4addr_mac(set_bridge_4addr_mac);

        response->success() = true;
        send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_NEW_IP_ADDRESS_NOTIFICATION: {
        LOG(TRACE) << "received ACTION_MONITOR_CLIENT_NEW_IP_ADDRESS_NOTIFICATION";
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CLIENT_NEW_IP_ADDRESS_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CLIENT_NEW_IP_ADDRESS_NOTIFICATION failed";
            return;
        }
        std::string sta_mac  = tlvf::mac_to_string(notification->mac());
        std::string sta_ipv4 = beerocks::net::network_utils::ipv4_to_string(notification->ipv4());

        auto sta_node = mon_db.sta_find(sta_mac);
        if (!sta_node) {
            LOG(ERROR) << "sta " << sta_mac << " hasn't been found on mon_db";
            return;
        }
        sta_node->set_ipv4(sta_ipv4);
        break;
    }
#ifdef BEEROCKS_RDKB
    case beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_GROUP_REQUEST: {

        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_STEERING_CLIENT_SET_GROUP_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_STEERING_CLIENT_SET_GROUP_REQUEST failed";
            send_steering_return_status(
                beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE,
                OPERATION_FAIL);
            return;
        }

        const auto bssid = tlvf::mac_to_string(request->params().cfg.bssid);
        int vap_id       = mon_db.get_vap_id(bssid);

        LOG(TRACE) << "ACTION_MONITOR_STEERING_CLIENT_SET_GROUP_REQUEST" << std::endl
                   << "remove " << int(request->params().remove) << std::endl
                   << "bssid " << bssid << std::endl
                   << "inactCheckIntervalSec " << request->params().cfg.inactCheckIntervalSec
                   << std::endl
                   << "inactCheckThresholdSec " << request->params().cfg.inactCheckThresholdSec
                   << std::endl
                   << "vap_id " << int(vap_id);

        if (vap_id == IFACE_ID_INVALID) {
            LOG(ERROR) << "wrong vap_id: " << int(vap_id);
            send_steering_return_status(
                beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE,
                OPERATION_FAIL);
            return;
        }

        if (request->params().remove) {
            if (mon_rdkb_hal.conf_erase_ap(vap_id) == false) {
                LOG(ERROR) << "failed removing vap_id:" << int(vap_id) << " configuration";
                send_steering_return_status(
                    beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE,
                    OPERATION_FAIL);
                return;
            }
            LOG(INFO) << "vap_id: " << int(vap_id) << " configuration was removed";
            send_steering_return_status(
                beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE,
                OPERATION_SUCCESS);
            break;
        }
        auto ap = mon_rdkb_hal.conf_add_ap(vap_id);
        if (ap == nullptr) {
            LOG(ERROR) << "add rdkb_hall ap configuration fail";
            send_steering_return_status(
                beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE,
                OPERATION_FAIL);
            return;
        }

        ap->setInactCheckIntervalSec(request->params().cfg.inactCheckIntervalSec);
        ap->setInactCheckThresholdSec(request->params().cfg.inactCheckThresholdSec);
        //TODO: set ThresholdPackets when cmdu has this param, meanwhile set the threshold as packet per sec
        ap->setInactCheckThresholdPackets(ap->getInactCheckThresholdSec());

        //send success status
        send_steering_return_status(
            beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE, OPERATION_SUCCESS);

        break;
    }
    case beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_REQUEST: {

        LOG(TRACE) << "received ACTION_MONITOR_STEERING_CLIENT_SET_REQUEST";
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_STEERING_CLIENT_SET_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_STEERING_CLIENT_SET_REQUEST failed";
            send_steering_return_status(
                beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE, OPERATION_FAIL);
            return;
        }

        std::string sta_mac = tlvf::mac_to_string(request->params().client_mac);
        if (request->params().remove) {
            if (mon_rdkb_hal.conf_erase_client(sta_mac) == false) {
                LOG(ERROR) << "failed removing client:" << sta_mac << " configuration";
                send_steering_return_status(
                    beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE, OPERATION_FAIL);
                return;
            }
            LOG(DEBUG) << "client: " << sta_mac << " configuration was removed";

            // For ONLY_CLIENTS_SELECTED_FOR_STEERING mode, need to updated the client's measure-sta-enable flag
            // if it  already connected. For not connected clients the flag will be determined as
            // part of the STA_Connected event handling.
            if (mon_db.get_clients_measuremet_mode() ==
                monitor_db::eClientsMeasurementMode::ONLY_CLIENTS_SELECTED_FOR_STEERING) {
                auto sta_node = mon_db.sta_find(sta_mac);
                if (sta_node) {
                    sta_node->set_measure_sta_enable(false);
                    LOG(DEBUG) << "Set sta measurements mode to false for sta_mac=" << sta_mac;
                }
            }

            send_steering_return_status(
                beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE, OPERATION_SUCCESS);
            break;
        }

        const auto bssid = tlvf::mac_to_string(request->params().bssid);
        int vap_id       = mon_db.get_vap_id(bssid);

        LOG(DEBUG) << "snrInactXing " << request->params().config.snrInactXing << std::endl
                   << "snrHighXing " << request->params().config.snrHighXing << std::endl
                   << "snrLowXing " << request->params().config.snrLowXing << std::endl
                   << "vapId " << vap_id;

        if (vap_id == IFACE_ID_INVALID) {
            LOG(ERROR) << "wrong vap_id:" << int(vap_id);
            send_steering_return_status(
                beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE, OPERATION_FAIL);
            return;
        }

        auto client = mon_rdkb_hal.conf_add_client(sta_mac);
        if (client == nullptr) {
            LOG(ERROR) << "add rdkb_hall client configuration fail";
            send_steering_return_status(
                beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE, OPERATION_FAIL);
            return;
        }

        client->setSnrHighXing(request->params().config.snrHighXing);
        client->setSnrLowXing(request->params().config.snrLowXing);
        client->setSnrInactXing(request->params().config.snrInactXing);
        client->setVapIndex(vap_id);

        // For ONLY_CLIENTS_SELECTED_FOR_STEERING mode, need to updated the client's measure-sta-enable flag
        // if it  already connected. For not connected clients the flag will be determined as
        // part of the STA_Connected event handling.
        if (mon_db.get_clients_measuremet_mode() ==
            monitor_db::eClientsMeasurementMode::ONLY_CLIENTS_SELECTED_FOR_STEERING) {
            auto sta_node = mon_db.sta_find(sta_mac);
            if (sta_node) {
                sta_node->set_measure_sta_enable(true);
                LOG(DEBUG) << "Set sta measurements mode to true for sta_mac=" << sta_mac;
            }
        }

        send_steering_return_status(beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE,
                                    OPERATION_SUCCESS);
        break;
    }
#endif //BEEROCKS_RDKB
    case beerocks_message::ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_REQUEST: {
        LOG(TRACE) << "received ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_REQUEST";

        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_REQUEST failed";
            return;
        }
        std::string sta_mac = tlvf::mac_to_string(request->params().mac);
        auto sta_node       = mon_db.sta_find(sta_mac);
        if (sta_node == nullptr) {
            LOG(ERROR) << "RX_RSSI_MEASUREMENT REQUEST sta_mac=" << sta_mac
                       << " sta not assoc, id=" << beerocks_header->id();
            break;
        }

        sta_node->push_rx_rssi_request_id(beerocks_header->id());

        if (request->params().cross) {
            sta_node->set_arp_burst(true);
            if (sta_node->get_arp_state() != monitor_sta_node::IDLE) {
                sta_node->arp_recv_count_clear();
            }
            mon_db.set_arp_burst_pkt_num(request->params().mon_ping_burst_pkt_num);
            mon_db.set_arp_burst_delay(request->params().measurement_delay);
            auto response = message_com::create_vs_message<
                beerocks_message::cACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE>(
                cmdu_tx, beerocks_header->id());
            if (response == nullptr) {
                LOG(ERROR) << "Failed building message!";
                break;
            }
            response->mac() = tlvf::mac_from_string(sta_mac);
            send_cmdu(cmdu_tx);
            LOG(DEBUG) << "send ACTION_MONITOR_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE, sta_mac = "
                       << sta_mac << " id=" << beerocks_header->id();
            sta_node->set_arp_state(monitor_sta_node::SEND_ARP);
            LOG(INFO) << "RX_RSSI_MEASUREMENT REQUEST cross, resetting state to SEND_ARP,"
                      << " sta_mac=" << sta_mac << " id=" << beerocks_header->id();

        } else if (sta_node->get_arp_state() == monitor_sta_node::IDLE) {
            sta_node->set_arp_burst(false);
            sta_node->set_arp_state(monitor_sta_node::SEND_ARP);
            LOG(INFO) << "RX_RSSI_MEASUREMENT REQUEST: state IDLE -> SEND_ARP,"
                      << " cross=0 sta_mac=" << sta_mac << " id=" << beerocks_header->id();

        } else {
            LOG(INFO) << "RX_RSSI_MEASUREMENT REQUEST state=" << int(sta_node->get_arp_state())
                      << " sta_mac=" << sta_mac << " id=" << beerocks_header->id();
        }

        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_BEACON_11K_REQUEST: {
        LOG(TRACE) << "received ACTION_MONITOR_CLIENT_BEACON_11K_REQUEST";
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CLIENT_BEACON_11K_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CLIENT_BEACON_11K_REQUEST failed";
            return;
        }

        std::string sta_mac = tlvf::mac_to_string(request->params().sta_mac);
        auto sta_node       = mon_db.sta_find(sta_mac);
        if (sta_node == nullptr) {
            LOG(ERROR) << "CLIENT_BEACON_11K_REQUEST sta_mac=" << sta_mac
                       << " sta not assoc, id=" << beerocks_header->id();
            return;
        }
        auto vap_node = mon_db.vap_get_by_id(sta_node->get_vap_id());
        if (!vap_node) {
            LOG(ERROR) << "station " << sta_mac << " has no vap";
            return;
        }

        int dialog_token;

        // TODO: TEMPORARY CONVERSION!
        bwl::SBeaconRequest11k bwl_request;

        bwl_request.measurement_mode = request->params().measurement_mode;
        bwl_request.channel          = request->params().channel;
        bwl_request.op_class         = request->params().op_class;
        bwl_request.repeats          = request->params().repeats;
        bwl_request.rand_ival        = request->params().rand_ival;
        bwl_request.duration         = request->params().duration;
        tlvf::mac_to_array(request->params().sta_mac, bwl_request.sta_mac.oct);
        tlvf::mac_to_array(request->params().bssid, bwl_request.bssid.oct);
        bwl_request.parallel               = request->params().parallel;
        bwl_request.enable                 = request->params().enable;
        bwl_request.request                = request->params().request;
        bwl_request.report                 = request->params().report;
        bwl_request.mandatory_duration     = request->params().mandatory_duration;
        bwl_request.expected_reports_count = request->params().expected_reports_count;
        bwl_request.use_optional_ssid      = request->params().use_optional_ssid;
        std::copy_n(request->params().ssid, sizeof(bwl_request.ssid), bwl_request.ssid);
        bwl_request.use_optional_ap_ch_report = request->params().use_optional_ap_ch_report;
        std::copy_n(request->params().ap_ch_report, sizeof(bwl_request.ap_ch_report),
                    bwl_request.ap_ch_report);
        bwl_request.use_optional_req_elements = request->params().use_optional_req_elements;
        std::copy_n(request->params().req_elements, sizeof(bwl_request.req_elements),
                    bwl_request.req_elements);
        bwl_request.use_optional_wide_band_ch_switch =
            request->params().use_optional_wide_band_ch_switch;
        bwl_request.new_ch_width             = request->params().new_ch_width;
        bwl_request.new_ch_center_freq_seg_0 = request->params().new_ch_center_freq_seg_0;
        bwl_request.new_ch_center_freq_seg_1 = request->params().new_ch_center_freq_seg_1;
        bwl_request.reporting_detail         = request->params().reporting_detail;

        mon_wlan_hal->sta_beacon_11k_request(vap_node->get_iface(), bwl_request, dialog_token);

        sEvent11k event_11k = {tlvf::mac_to_string(request->params().sta_mac), dialog_token,
                               std::chrono::steady_clock::now(), beerocks_header->id()};

        // USED IN TESTS
        LOG(DEBUG) << "inserting " << +request->params().expected_reports_count
                   << " RRM_EVENT_BEACON_REP_RXED event(s) to the pending list";
        for (int i = 0; i < request->params().expected_reports_count; i++) {
            pending_11k_events.insert(std::make_pair("RRM_EVENT_BEACON_REP_RXED", event_11k));
        }
        break;
    }
    case beerocks_message::ACTION_MONITOR_CLIENT_CHANNEL_LOAD_11K_REQUEST: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CLIENT_CHANNEL_LOAD_11K_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CLIENT_CHANNEL_LOAD_11K_REQUEST failed";
            return;
        }

        // debug_channel_load_11k_request(request);
        std::string sta_mac = tlvf::mac_to_string(request->params().sta_mac);
        auto sta_node       = mon_db.sta_find(sta_mac);
        if (sta_node == nullptr) {
            LOG(ERROR) << "CLIENT_CHANNEL_LOAD_11K_REQUEST sta_mac=" << sta_mac
                       << " sta not assoc, id=" << beerocks_header->id();
            return;
        }
        auto vap_node = mon_db.vap_get_by_id(sta_node->get_vap_id());
        if (!vap_node) {
            LOG(ERROR) << "station " << sta_mac << " has no vap";
            return;
        }

        // TODO: TEMPORARY CONVERSION!
        bwl::SStaChannelLoadRequest11k bwl_request;

        bwl_request.channel                  = request->params().channel;
        bwl_request.op_class                 = request->params().op_class;
        bwl_request.repeats                  = request->params().repeats;
        bwl_request.rand_ival                = request->params().rand_ival;
        bwl_request.duration                 = request->params().duration;
        bwl_request.parallel                 = request->params().parallel;
        bwl_request.enable                   = request->params().enable;
        bwl_request.request                  = request->params().request;
        bwl_request.report                   = request->params().report;
        bwl_request.mandatory_duration       = request->params().mandatory_duration;
        bwl_request.use_optional_ch_load_rep = request->params().use_optional_ch_load_rep;
        bwl_request.ch_load_rep_first        = request->params().ch_load_rep_first;
        bwl_request.ch_load_rep_second       = request->params().ch_load_rep_second;
        bwl_request.use_optional_wide_band_ch_switch =
            request->params().use_optional_wide_band_ch_switch;
        bwl_request.new_ch_width             = request->params().new_ch_width;
        bwl_request.new_ch_center_freq_seg_0 = request->params().new_ch_center_freq_seg_0;
        bwl_request.new_ch_center_freq_seg_1 = request->params().new_ch_center_freq_seg_1;
        tlvf::mac_to_array(request->params().sta_mac, bwl_request.sta_mac.oct);

        mon_wlan_hal->sta_channel_load_11k_request(vap_node->get_iface(), bwl_request);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST>();
        if (!request) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST failed";
            return;
        }

        auto radio_mac         = request->scan_params().radio_mac;
        auto dwell_time_ms     = request->scan_params().dwell_time_ms;
        auto channel_pool      = request->scan_params().channel_pool;
        auto channel_pool_size = request->scan_params().channel_pool_size;
        auto channel_pool_vector =
            std::vector<unsigned int>(channel_pool, channel_pool + channel_pool_size);
        std::string channels;

        //loop for priting the channal pool
        for (int index = 0; index < int(channel_pool_size); index++) {
            channels += ((index != 0) ? "," : "") + std::to_string(channel_pool[index]);
        }

        //debug print incoming information:
        LOG(DEBUG) << std::endl
                   << "scan_params:" << std::endl
                   << "radio_mac=" << radio_mac << std::endl
                   << "dwell_time_ms=" << dwell_time_ms << std::endl
                   << "channel_pool_size=" << int(channel_pool_size) << std::endl
                   << "channel_pool=" << channels;

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE>(
            cmdu_tx, beerocks_header->id());
        if (!response_out) {
            LOG(ERROR) << "Failed building cACTION_MONITOR_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE "
                          "message!";
            return;
        }

        response_out->success() =
            mon_wlan_hal->channel_scan_trigger(int(dwell_time_ms), channel_pool_vector);
        LOG_IF(!response_out->success(), ERROR) << "channel_scan_trigger Failed";

        send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_REQUEST: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_REQUEST>();
        if (!request) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_REQUEST failed";
            return;
        }

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE>(
            cmdu_tx, beerocks_header->id());
        if (!response_out) {
            LOG(ERROR) << "Failed building cACTION_MONITOR_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE "
                          "message!";
            return;
        }

        bool result = mon_wlan_hal->channel_scan_dump_cached_results();
        LOG_IF(!result, ERROR) << "channel_scan_dump_cached_results Failed";

        response_out->success() = (result) ? 1 : 0;
        send_cmdu(cmdu_tx);

        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_RESULTS_NOTIFICATION>(cmdu_tx);
        if (!notification) {
            LOG(ERROR) << "Failed building cACTION_MONITOR_CHANNEL_SCAN_RESULTS_NOTIFICATION msg";
            return;
        }

        //Sending the cACTION_MONITOR_CHANNEL_SCAN_RESULTS_NOTIFICATION without
        //modifications will cause the DUMP_READY event to trigger in the DCS task
        send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_CHANNEL_SCAN_ABORT_REQUEST: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_ABORT_REQUEST>();
        if (!request) {
            LOG(ERROR) << "addClass cACTION_MONITOR_CHANNEL_SCAN_ABORT_REQUEST failed";
            return;
        }

        auto response_out = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_ABORT_RESPONSE>(cmdu_tx,
                                                                           beerocks_header->id());
        if (!response_out) {
            LOG(ERROR) << "Failed building cACTION_MONITOR_CHANNEL_SCAN_ABORT_RESPONSE message!";
            return;
        }

        response_out->success() = mon_wlan_hal->channel_scan_abort();
        LOG_IF(!response_out->success(), ERROR) << "channel_scan_abort failed";

        send_cmdu(cmdu_tx);
        break;
    }
    default: {
        LOG(ERROR) << "Unsupported MONITOR action_op: " << int(beerocks_header->action_op());
        break;
    }
    }
}

void Monitor::handle_cmdu_ieee1905_1_message(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto cmdu_message_type = cmdu_rx.getMessageType();

    switch (cmdu_message_type) {
    case ieee1905_1::eMessageType::AP_METRICS_QUERY_MESSAGE:
        handle_ap_metrics_query(cmdu_rx);
        break;
    case ieee1905_1::eMessageType::MULTI_AP_POLICY_CONFIG_REQUEST_MESSAGE:
        handle_multi_ap_policy_config_request(cmdu_rx);
        break;
    default:
        LOG(ERROR) << "Unknown CMDU message type: " << std::hex << int(cmdu_message_type);
    }
}

void Monitor::handle_multi_ap_policy_config_request(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    /**
     * The Multi-AP Policy Config Request message is sent by the controller, received and
     * acknowledged by the backhaul manager, forwarded "as is" to the slave thread and forwarded
     * back again to the monitor thread, where it is finally processed.
     */
    auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received MULTI_AP_POLICY_CONFIG_REQUEST_MESSAGE, mid=" << std::hex << int(mid);

    auto metric_reporting_policy_tlv = cmdu_rx.getClass<wfa_map::tlvMetricReportingPolicy>();
    if (metric_reporting_policy_tlv) {
        /**
         * Metric Reporting Policy TLV contains configuration for several radios
         */
        for (size_t i = 0; i < metric_reporting_policy_tlv->metrics_reporting_conf_list_length();
             i++) {
            auto tuple = metric_reporting_policy_tlv->metrics_reporting_conf_list(i);
            if (!std::get<0>(tuple)) {
                LOG(ERROR) << "Failed to get metrics_reporting_conf[" << i
                           << "] from TLV_METRIC_REPORTING_POLICY";
                return;
            }
            auto metrics_reporting_conf = std::get<1>(tuple);

            /**
             * Skip configurations not addressed to this radio
             */
            if (metrics_reporting_conf.radio_uid != m_radio_mac) {
                continue;
            }

            /**
             * Extract and store configuration for this radio
             */
            auto &info = mon_db.get_radio_node()->ap_metrics_reporting_info();

            info.sta_metrics_reporting_rcpi_threshold =
                metrics_reporting_conf.sta_metrics_reporting_rcpi_threshold;
            info.sta_metrics_reporting_rcpi_hysteresis_margin_override =
                metrics_reporting_conf.sta_metrics_reporting_rcpi_hysteresis_margin_override;
            info.ap_channel_utilization_reporting_threshold =
                metrics_reporting_conf.ap_channel_utilization_reporting_threshold;
            info.include_associated_sta_link_metrics_tlv_in_ap_metrics_response =
                metrics_reporting_conf.policy
                    .include_associated_sta_link_metrics_tlv_in_ap_metrics_response;
            info.include_associated_sta_traffic_stats_tlv_in_ap_metrics_response =
                metrics_reporting_conf.policy
                    .include_associated_sta_traffic_stats_tlv_in_ap_metrics_response;
            info.include_associated_wifi_6_sta_status_report_tlv_in_ap_metrics_response =
                metrics_reporting_conf.policy
                    .include_associated_wifi_6_sta_status_report_tlv_in_ap_metrics_response;

            break;
        }
    }
}

void Monitor::handle_ap_metrics_query(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    const auto mid           = cmdu_rx.getMessageId();
    auto ap_metric_query_tlv = cmdu_rx.getClass<wfa_map::tlvApMetricQuery>();
    if (!ap_metric_query_tlv) {
        LOG(ERROR) << "AP Metrics Query CMDU mid=" << mid << " does not have AP Metric Query TLV";
        return;
    }

    std::vector<sMacAddr> bssid_list;

    for (size_t bssid_idx = 0; bssid_idx < ap_metric_query_tlv->bssid_list_length(); bssid_idx++) {
        auto bssid_tuple = ap_metric_query_tlv->bssid_list(bssid_idx);
        if (!std::get<0>(bssid_tuple)) {
            LOG(ERROR) << "Failed to get bssid " << bssid_idx << " from AP_METRICS_QUERY";
            return;
        }
        const auto &bssid = std::get<1>(bssid_tuple);
        LOG(DEBUG) << "Received AP_METRICS_QUERY_MESSAGE, mid=" << std::hex << int(mid)
                   << "  bssid " << bssid;

        bssid_list.emplace_back(bssid);
    }

    if (!create_ap_metrics_response(mid, bssid_list)) {
        LOG(ERROR) << "Unable to create AP Metrics Response message";
        return;
    }

    LOG(DEBUG) << "Sending AP_METRICS_RESPONSE_MESSAGE to slave_socket, mid=" << std::hex
               << int(mid);
    send_cmdu(cmdu_tx);
}

bool Monitor::hal_event_handler(bwl::base_wlan_hal::hal_event_ptr_t event_ptr)
{
    if (!event_ptr) {
        LOG(ERROR) << "Invalid event!";
        return false;
    }

    if (!m_slave_client) {
        LOG(ERROR) << "Not connected to slave!";
        return false;
    }

    // Monitor Event & Data
    typedef bwl::mon_wlan_hal::Event Event;
    auto event = (Event)(event_ptr->first);
    auto data  = event_ptr->second.get();

    switch (event) {

    case Event::RRM_Channel_Load_Response: {

        auto hal_data = static_cast<bwl::SStaChannelLoadResponse11k *>(data);

        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CLIENT_CHANNEL_LOAD_11K_RESPONSE>(cmdu_tx);
        if (response == nullptr) {
            LOG(ERROR)
                << "Failed building cACTION_MONITOR_CLIENT_CHANNEL_LOAD_11K_RESPONSE message!";
            return false;
        }

        // TODO: TEMPORARY CONVERSION!
        response->params().channel           = hal_data->channel;
        response->params().channel_load      = hal_data->channel_load;
        response->params().op_class          = hal_data->op_class;
        response->params().rep_mode          = hal_data->rep_mode;
        response->params().dialog_token      = hal_data->dialog_token;
        response->params().measurement_token = hal_data->measurement_token;
        response->params().duration          = hal_data->duration;
        response->params().start_time        = hal_data->start_time;
        response->params().use_optional_wide_band_ch_switch =
            hal_data->use_optional_wide_band_ch_switch;
        response->params().new_ch_width             = hal_data->new_ch_width;
        response->params().new_ch_center_freq_seg_0 = hal_data->new_ch_center_freq_seg_0;
        response->params().new_ch_center_freq_seg_1 = hal_data->new_ch_center_freq_seg_1;
        tlvf::mac_from_array(hal_data->sta_mac.oct, response->params().sta_mac);

        // debug_channel_load_11k_response(msg);

        send_cmdu(cmdu_tx);

    } break;

    case Event::RRM_Beacon_Request_Status: {

        auto hal_data = static_cast<bwl::SBeaconRequestStatus11k *>(data);
        auto sta_mac  = tlvf::mac_to_string((sMacAddr &)hal_data->sta_mac);

        LOG(INFO) << "Received beacon measurement request status for STA: " << sta_mac
                  << ", dialog_token: " << int(hal_data->dialog_token)
                  << ", ack: " << int(hal_data->ack);

        // TODO: If ack == 0, remove the request?

        // Update the dialog token for the STA
        bool found     = false;
        auto event_map = pending_11k_events.equal_range("RRM_EVENT_BEACON_REP_RXED");
        for (auto it = event_map.first; it != event_map.second; it++) {
            auto &req = it->second;
            if (req.sta_mac == sta_mac) {
                req.dialog_token = hal_data->dialog_token;
                LOG(INFO) << "Updated dialog_token for STA: " << sta_mac;
                found = true;
                break;
            }
        }

        if (!found) {
            LOG(WARNING) << "Received 11k request status for STA: " << sta_mac
                         << ", but no request was sent...";
        }

    } break;

    case Event::RRM_Beacon_Response: {

        auto hal_data = static_cast<bwl::SBeaconResponse11k *>(data);
        LOG(INFO) << "Received beacon measurement response on BSSID: "
                  << (sMacAddr &)hal_data->bssid
                  << ", dialog_token: " << int(hal_data->dialog_token);

        // TODO: Can be changed to iterator loop?
        auto event_map = pending_11k_events.equal_range("RRM_EVENT_BEACON_REP_RXED");
        for (auto it = event_map.first; it != event_map.second;) {
            if ((it->second.dialog_token == hal_data->dialog_token) ||
                (hal_data->dialog_token == 0)) {

                auto id = it->second.id;

                auto response = message_com::create_vs_message<
                    beerocks_message::cACTION_MONITOR_CLIENT_BEACON_11K_RESPONSE>(cmdu_tx, id);
                if (response == nullptr) {
                    LOG(ERROR) << "Failed building cACTION_MONITOR_CLIENT_BEACON_11K_RESPONSE "
                                  "message!";
                    break;
                }

                // TODO: TEMPORARY CONVERSION!
                response->params().channel                  = hal_data->channel;
                response->params().op_class                 = hal_data->op_class;
                response->params().dialog_token             = hal_data->dialog_token;
                response->params().measurement_token        = hal_data->measurement_token;
                response->params().rep_mode                 = hal_data->rep_mode;
                response->params().phy_type                 = hal_data->phy_type;
                response->params().frame_type               = hal_data->frame_type;
                response->params().rcpi                     = hal_data->rcpi;
                response->params().rsni                     = hal_data->rsni;
                response->params().ant_id                   = hal_data->ant_id;
                response->params().duration                 = hal_data->duration;
                response->params().parent_tsf               = hal_data->parent_tsf;
                response->params().start_time               = hal_data->start_time;
                response->params().new_ch_width             = hal_data->new_ch_width;
                response->params().new_ch_center_freq_seg_0 = hal_data->new_ch_center_freq_seg_0;
                response->params().new_ch_center_freq_seg_1 = hal_data->new_ch_center_freq_seg_1;
                response->params().use_optional_wide_band_ch_switch =
                    hal_data->use_optional_wide_band_ch_switch;
                tlvf::mac_from_array(hal_data->sta_mac.oct, response->params().sta_mac);
                tlvf::mac_from_array(hal_data->bssid.oct, response->params().bssid);

                it = pending_11k_events.erase(it);
                LOG(INFO) << "Sending beacon measurement reponse on BSSID: "
                          << response->params().bssid << " to task_id: " << id;

                send_cmdu(cmdu_tx);
                break;
            } else {
                ++it;
            }
        }

    } break;

    case Event::AP_Enabled: {
        if (!data) {
            LOG(ERROR) << "AP_Enabled without data";
            return false;
        }
        auto msg = static_cast<bwl::sHOSTAP_ENABLED_NOTIFICATION *>(data);
        LOG(INFO) << "AP_Enabled on vap_id = " << int(msg->vap_id);

        update_vaps_in_db();
    } break;

    case Event::AP_Disabled: {
        if (!data) {
            LOG(ERROR) << "AP_Disabled without data!";
            return false;
        }

        auto msg = static_cast<bwl::sHOSTAP_DISABLED_NOTIFICATION *>(data);
        LOG(INFO) << "AP_Disabled on vap_id = " << int(msg->vap_id);

        if (msg->vap_id == beerocks::IFACE_RADIO_ID) {
            auto timeout = std::chrono::steady_clock::now() +
                           std::chrono::seconds(MAX_RADIO_DISBALED_TIMEOUT_SEC);
            auto notify_disabled = true;

            while (std::chrono::steady_clock::now() < timeout) {
                if (!mon_wlan_hal->refresh_radio_info()) {
                    LOG(WARNING) << "Radio could be temporary disabled, wait grace time "
                                 << std::chrono::duration_cast<std::chrono::seconds>(
                                        timeout - std::chrono::steady_clock::now())
                                        .count()
                                 << " sec.";
                    UTILS_SLEEP_MSEC(500);
                    continue;
                }

                auto state = mon_wlan_hal->get_radio_info().radio_state;
                if ((state != bwl::eRadioState::DISABLED) &&
                    (state != bwl::eRadioState::UNINITIALIZED)) {
                    LOG(DEBUG) << "Radio is not disabled (state=" << state
                               << "), not forwarding disabled notification.";
                    notify_disabled = false;
                    break;
                }
                UTILS_SLEEP_MSEC(500);
            }

            if (!notify_disabled) {
                break;
            }

            auto response = message_com::create_vs_message<
                beerocks_message::cACTION_MONITOR_HOSTAP_AP_DISABLED_NOTIFICATION>(cmdu_tx);
            if (response == nullptr) {
                LOG(ERROR) << "Failed building cACTION_MONITOR_HOSTAP_AP_DISABLED_NOTIFICATION "
                              "message!";
                break;
            }

            response->vap_id() = msg->vap_id;
            send_cmdu(cmdu_tx);
        }

        update_vaps_in_db();

    } break;
    case Event::Channel_Scan_Triggered: {
        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_TRIGGERED_NOTIFICATION>(cmdu_tx);
        if (!notification) {
            LOG(ERROR) << "Failed building cACTION_MONITOR_CHANNEL_SCAN_TRIGGERED_NOTIFICATION msg";
            return false;
        }

        send_cmdu(cmdu_tx);
    } break;
    case Event::Channel_Scan_New_Results_Ready:
    case Event::Channel_Scan_Dump_Result: {
        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_RESULTS_NOTIFICATION>(cmdu_tx);
        if (!notification) {
            LOG(ERROR) << "Failed building cACTION_MONITOR_CHANNEL_SCAN_RESULTS_NOTIFICATION msg";
            return false;
        }

        // If event == Channel_Scan_New_Results_Ready do nothing since is_dump's default is 0
        if (event == Event::Channel_Scan_Dump_Result) {
            auto msg = static_cast<bwl::sCHANNEL_SCAN_RESULTS_NOTIFICATION *>(data);

            auto &in_result  = msg->channel_scan_results;
            auto &out_result = notification->scan_results();

            // Arrays
            string_utils::copy_string(out_result.ssid, in_result.ssid,
                                      beerocks::message::WIFI_SSID_MAX_LENGTH);
            out_result.bssid = in_result.bssid;
            std::copy(in_result.basic_data_transfer_rates_kbps.begin(),
                      in_result.basic_data_transfer_rates_kbps.end(),
                      out_result.basic_data_transfer_rates_kbps);
            std::copy(in_result.supported_data_transfer_rates_kbps.begin(),
                      in_result.supported_data_transfer_rates_kbps.end(),
                      out_result.supported_data_transfer_rates_kbps);

            // Primery values
            out_result.channel             = in_result.channel;
            out_result.signal_strength_dBm = in_result.signal_strength_dBm;
            out_result.beacon_period_ms    = in_result.beacon_period_ms;
            out_result.noise_dBm           = in_result.noise_dBm;
            out_result.dtim_period         = in_result.dtim_period;
            out_result.channel_utilization = in_result.channel_utilization;

            // Enums
            out_result.mode = beerocks_message::eChannelScanResultMode(uint8_t(in_result.mode));
            out_result.operating_frequency_band =
                beerocks_message::eChannelScanResultOperatingFrequencyBand(
                    uint8_t(in_result.operating_frequency_band));
            out_result.operating_standards = beerocks_message::eChannelScanResultStandards(
                uint8_t(in_result.operating_standards));
            out_result.operating_channel_bandwidth =
                beerocks_message::eChannelScanResultChannelBandwidth(
                    uint8_t(in_result.operating_channel_bandwidth));

            // Enum list
            int i = 0;
            std::for_each(in_result.security_mode_enabled.begin(),
                          in_result.security_mode_enabled.end(),
                          [&i, &out_result](bwl::eChannelScanResultSecurityMode e) {
                              out_result.security_mode_enabled[i++] =
                                  beerocks_message::eChannelScanResultSecurityMode(uint8_t(e));
                          });
            i = 0;
            std::for_each(in_result.encryption_mode.begin(), in_result.encryption_mode.end(),
                          [&i, &out_result](bwl::eChannelScanResultEncryptionMode e) {
                              out_result.encryption_mode[i++] =
                                  beerocks_message::eChannelScanResultEncryptionMode(uint8_t(e));
                          });
            i = 0;
            std::for_each(in_result.supported_standards.begin(),
                          in_result.supported_standards.end(),
                          [&i, &out_result](bwl::eChannelScanResultStandards e) {
                              out_result.supported_standards[i++] =
                                  beerocks_message::eChannelScanResultStandards(uint8_t(e));
                          });

            notification->is_dump() = 1;
        }

        send_cmdu(cmdu_tx);
    } break;
    case Event::Channel_Scan_Finished: {
        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_FINISHED_NOTIFICATION>(cmdu_tx);
        if (!notification) {
            LOG(ERROR) << "Failed building cACTION_MONITOR_CHANNEL_SCAN_FINISHED_NOTIFICATION msg";
            return false;
        }

        send_cmdu(cmdu_tx);
    } break;
    case Event::Channel_Scan_Aborted: {
        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_CHANNEL_SCAN_ABORTED_NOTIFICATION>(cmdu_tx);
        if (!notification) {
            LOG(ERROR) << "Failed building cACTION_MONITOR_CHANNEL_SCAN_ABORTED_NOTIFICATION msg";
            return false;
        }

        send_cmdu(cmdu_tx);
    } break;
    case Event::STA_Connected: {
        LOG(TRACE) << "Received STA_Connected event";
        auto msg = static_cast<bwl::sACTION_MONITOR_CLIENT_ASSOCIATED_NOTIFICATION *>(data);

        auto sta_mac = tlvf::mac_to_string(msg->mac);
        auto vap_id  = msg->vap_id;

        LOG(INFO) << "STA_Connected: mac=" << sta_mac << " vap_id=" << int(vap_id);

        std::string sta_ipv4             = beerocks::net::network_utils::ZERO_IP_STRING;
        std::string set_bridge_4addr_mac = beerocks::net::network_utils::ZERO_MAC_STRING;

        auto old_node = mon_db.sta_find(sta_mac);
        if (old_node) {
            sta_ipv4             = old_node->get_ipv4();
            set_bridge_4addr_mac = old_node->get_bridge_4addr_mac();
        }

        mon_db.sta_erase(sta_mac);

        auto vap_node = mon_db.vap_get_by_id(vap_id);
        if (!vap_node) {
            LOG(ERROR) << "vap_id " << int(vap_id) << " does not exist";
            return false;
        }

        auto sta_node = mon_db.sta_add(sta_mac, vap_id);
        sta_node->set_ipv4(sta_ipv4);
        sta_node->set_bridge_4addr_mac(set_bridge_4addr_mac);

        sta_node->set_measure_sta_enable((mon_db.get_clients_measuremet_mode() ==
                                          monitor_db::eClientsMeasurementMode::ENABLE_ALL));

#ifdef BEEROCKS_RDKB
        //clean rdkb monitor data if already in database.
        auto client = mon_rdkb_hal.conf_get_client(sta_mac);
        if (client) {
            // override sta_node measurements configuration
            sta_node->set_measure_sta_enable((mon_db.get_clients_measuremet_mode() !=
                                              monitor_db::eClientsMeasurementMode::DISABLE_ALL));
            client->setStartTime(std::chrono::steady_clock::now());
            client->setLastSampleTime(std::chrono::steady_clock::now());
            client->setAccumulatedPackets(0);
            client->clearData();
        }
#endif
        break;
    }
    case Event::STA_Disconnected: {
        LOG(TRACE) << "Received STA_Disconnected event";
        auto msg = static_cast<bwl::sACTION_MONITOR_CLIENT_DISCONNECTED_NOTIFICATION *>(data);
        auto mac = tlvf::mac_to_string(msg->mac);

        LOG(INFO) << "STA_Disconnected event: mac = " << mac;

        mon_db.sta_erase(mac);
        break;
    }

    // Unhandled events
    default: {
        LOG(ERROR) << "Unhandled event: " << int(event);
        return false;
    }
    }

    return true;
}

// void Monitor::debug_channel_load_11k_request(message::sACTION_MONITOR_CLIENT_CHANNEL_LOAD_11K_REQUEST* request)
// {
//     LOG(DEBUG) << "ACTION_MONITOR_CLIENT_CLIENT_CHANNEL_LOAD_11K_REQUEST:"
//     << std::endl << "channel: "              << (int)request->params.channel
//     << std::endl << "op_class: "             << (int)request->params.op_class
//     << std::endl << "repeats: "              << (int)request->params.repeats
//     << std::endl << "rand_ival: "            << (int)request->params.rand_ival
//     << std::endl << "duration: "             << (int)request->params.duration
//     << std::endl << "sta_mac: "              << request->params.sta_mac
//     << std::endl << "parallel: "             << (int)request->params.parallel
//     << std::endl << "enable: "               << (int)request->params.enable
//     << std::endl << "request: "              << (int)request->params.request
//     << std::endl << "report: "               << (int)request->params.report
//     << std::endl << "mandatory_duration: "   << (int)request->params.mandatory_duration;
//     //Optional:
//     // << std::endl << "use_optional_ch_load_rep: "             << (int)request->params.use_optional_ch_load_rep
//     // << std::endl << "ch_load_rep_first: "                    << (int)request->params.ch_load_rep_first
//     // << std::endl << "ch_load_rep_second: "                   << (int)request->params.ch_load_rep_second
//     // << std::endl << "use_optional_wide_band_ch_switch: "     << (int)request->params.use_optional_wide_band_ch_switch
//     // << std::endl << "new_ch_width: "                         << (int)request->params.new_ch_width
//     // << std::endl << "new_ch_center_freq_seg_0: "             << (int)request->params.new_ch_center_freq_seg_0
//     // << std::endl << "new_ch_center_freq_seg_1: "             << (int)request->params.new_ch_center_freq_seg_1;
// }

// void Monitor::debug_beacon_11k_request(message::sACTION_MONITOR_CLIENT_BEACON_11K_REQUEST *request)
// {
//     LOG(DEBUG) << "ACTION_MONITOR_CLIENT_BEACON_REQUEST:" << std::endl
//     << std::endl << "measurement_mode: "                 << (int)request->params.measurement_mode
//     << std::endl << "channel: "                          << (int)request->params.channel
//     << std::endl << "op_class: "                         << (int)request->params.op_class
//     << std::endl << "repeats: "                          << (int)request->params.repeats
//     << std::endl << "rand_ival: "                        << (int)request->params.rand_ival
//     << std::endl << "duration: "                         << (int)request->params.duration
//     << std::endl << "sta_mac: "                          << request->params.sta_mac
//     << std::endl << "bssid: "                            << request->params.bssid
//     << std::endl << "parallel: "                         << (int)request->params.parallel
//     << std::endl << "enable: "                           << (int)request->params.enable
//     << std::endl << "request: "                          << (int)request->params.request
//     << std::endl << "report: "                           << (int)request->params.report
//     << std::endl << "mandatory_duration: "               << (int)request->params.mandatory_duration
//     //Optional:
//     << std::endl << "use_optional_ssid: "                << (int)request->params.use_optional_ssid
//     << std::endl << "ssid: "                             << (const char*)request->params.ssid
//     // << std::endl << "use_optional_ap_ch_report: "        << (int)request->params.use_optional_ap_ch_report
//     // << std::endl << "ap_ch_report: "                     << (int)request->params.ap_ch_report[0]
//     // << std::endl << "use_optional_req_elements: "        << (int)request->params.use_optional_req_elements
//     // << std::endl << "req_elements: "                     << (int)request->params.req_elements[0]
//     // << std::endl << "use_optional_wide_band_ch_switch: " << (int)request->params.use_optional_wide_band_ch_switch
//     // << std::endl << "new_ch_width: "                     << (int)request->params.new_ch_width
//     // << std::endl << "new_ch_center_freq_seg_0: "         << (int)request->params.new_ch_center_freq_seg_0
//     // << std::endl << "new_ch_center_freq_seg_1: "         << (int)request->params.new_ch_center_freq_seg_1
//     ;
// }

// void Monitor::debug_channel_load_11k_response(message::sACTION_MONITOR_CLIENT_CHANNEL_LOAD_11K_RESPONSE* event)
// {
//     LOG(DEBUG) << "DATA TEST:"
//     << std::endl << "sta_mac: "              << event->params.sta_mac
//     << std::endl << "measurement_rep_mode: " << (int)event->params.rep_mode
//     << std::endl << "op_class: "             << (int)event->params.op_class
//     << std::endl << "channel: "              << (int)event->params.channel
//     << std::endl << "start_time: "           << (int)event->params.start_time
//     << std::endl << "duration: "             << (int)event->params.duration
//     << std::endl << "channel_load: "         << (int)event->params.channel_load

//     << std::endl << "new_ch_width: "                         << (int)event->params.new_ch_width
//     << std::endl << "new_ch_center_freq_seg_0: "             << (int)event->params.new_ch_center_freq_seg_0
//     << std::endl << "new_ch_center_freq_seg_1: "             << (int)event->params.new_ch_center_freq_seg_1
//     ;
// }

// void Monitor::debug_beacon_11k_response(message::sACTION_MONITOR_CLIENT_BEACON_11K_RESPONSE* event)
// {
//     LOG(DEBUG) << "DATA TEST:"
//     << std::endl << "sta_mac: "              << event->params.sta_mac
//     << std::endl << "measurement_rep_mode: " << (int)event->params.rep_mode
//     << std::endl << "op_class: "             << (int)event->params.op_class
//     << std::endl << "channel: "              << (int)event->params.channel
//     << std::endl << "start_time: "           << (int)event->params.start_time
//     << std::endl << "duration: "             << (int)event->params.duration
//     << std::endl << "phy_type: "             << (int)event->params.phy_type
//     << std::endl << "frame_type: "           << (int)event->params.frame_type
//     << std::endl << "rcpi: "                 << (int)event->params.rcpi
//     << std::endl << "rsni: "                 << (int)event->params.rsni
//     << std::endl << "bssid: "                << event->params.bssid
//     << std::endl << "ant_id: "               << (int)event->params.ant_id
//     << std::endl << "tsf: "                  << (int)event->params.parent_tsf

//     << std::endl << "new_ch_width: "                         << (int)event->params.new_ch_width
//     << std::endl << "new_ch_center_freq_seg_0: "             << (int)event->params.new_ch_center_freq_seg_0
//     << std::endl << "new_ch_center_freq_seg_1: "             << (int)event->params.new_ch_center_freq_seg_1
//     ;
// }

void Monitor::send_heartbeat()
{
    if (!m_slave_client) {
        LOG(ERROR) << "Not connected to slave!";
        return;
    }

    //LOG(DEBUG) << "sending HEARTBEAT notification";
    auto request =
        message_com::create_vs_message<beerocks_message::cACTION_MONITOR_HEARTBEAT_NOTIFICATION>(
            cmdu_tx);
    if (request == nullptr) {
        LOG(ERROR) << "Failed building message!";
        return;
    }

    send_cmdu(cmdu_tx);
}

void Monitor::update_vaps_in_db()
{
    if (!mon_wlan_hal->refresh_vaps_info()) {
        LOG(ERROR) << "Failed to refresh vaps info!";
        return;
    }
    const auto &radio_vaps = mon_wlan_hal->get_radio_info().available_vaps;

    std::string bridge_iface_mac;
    std::string bridge_iface_ip;
    beerocks::net::network_utils::linux_iface_get_mac(bridge_iface, bridge_iface_mac);
    beerocks::net::network_utils::linux_iface_get_ip(bridge_iface, bridge_iface_ip);

    for (int vap_id = beerocks::IFACE_VAP_ID_MIN; vap_id <= beerocks::IFACE_VAP_ID_MAX; vap_id++) {

        // if vap exist in HAL, update it in the local db.
        if (radio_vaps.find(vap_id) != radio_vaps.end()) {
            auto iface_name = radio_vaps.at(vap_id).bss;

            auto curr_vap = radio_vaps.at(vap_id);

            auto vap_node = mon_db.vap_add(iface_name, vap_id);
            vap_node->set_mac(curr_vap.mac);

            vap_node->set_bridge_iface(bridge_iface);
            vap_node->set_bridge_mac(bridge_iface_mac);
            vap_node->set_bridge_ipv4(bridge_iface_ip);

        } else if (mon_db.vap_get_by_id(vap_id)) { // vap does not exist in HAL but is in local DB
            mon_db.vap_remove(vap_id);
        }
    }
}
#ifdef BEEROCKS_RDKB
void Monitor::send_steering_return_status(beerocks_message::eActionOp_MONITOR ActionOp,
                                          int32_t status)
{
    switch (ActionOp) {
    case beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE: {
        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_STEERING_CLIENT_SET_GROUP_RESPONSE>(cmdu_tx);
        if (response == nullptr) {
            LOG(ERROR) << "Failed building message!";
            break;
        }
        response->params().error_code = status;
        send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE: {
        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_MONITOR_STEERING_CLIENT_SET_RESPONSE>(cmdu_tx);
        if (response == nullptr) {
            LOG(ERROR) << "Failed building message!";
            break;
        }
        response->params().error_code = status;
        send_cmdu(cmdu_tx);
        break;
    }
    default: {
        LOG(ERROR) << "UNKNOWN ActionOp was received, ActionOp = " << int(ActionOp);
        break;
    }
    }
    return;
}
#endif //BEEROCKS_RDKB
