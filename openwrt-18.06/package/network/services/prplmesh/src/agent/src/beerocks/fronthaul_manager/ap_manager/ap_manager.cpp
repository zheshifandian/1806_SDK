/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "ap_manager.h"

#include <bcl/beerocks_string_utils.h>
#include <bcl/beerocks_utils.h>
#include <bcl/network/network_utils.h>
#include <bcl/son/son_wireless_utils.h>
#include <bcl/transaction.h>
#include <bpl/bpl_cfg.h>
#include <easylogging++.h>

#include <beerocks/tlvf/beerocks_message.h>
#include <beerocks/tlvf/beerocks_message_apmanager.h>

#include <tlvf/wfa_map/tlvChannelPreference.h>
#include <tlvf/wfa_map/tlvProfile2ReasonCode.h>
#include <tlvf/wfa_map/tlvProfile2StatusCode.h>
#include <tlvf/wfa_map/tlvStaMacAddressType.h>
#include <tlvf/wfa_map/tlvTunnelledData.h>
#include <tlvf/wfa_map/tlvTunnelledProtocolType.h>
#include <tlvf/wfa_map/tlvTunnelledSourceInfo.h>

#include <numeric>

//////////////////////////////////////////////////////////////////////////////
////////////////////////// Local Module Definitions //////////////////////////
//////////////////////////////////////////////////////////////////////////////

/**
 * Time between successive timer executions of the FSM timer
 */
constexpr auto fsm_timer_period = std::chrono::milliseconds(1000);

#define SELECT_TIMEOUT_MSC 1000
#define ACS_READ_SLEEP_USC 1000
#define READ_ACS_ATTEMPT_MAX 5
#define DISABLE_BACKHAUL_VAP_TIMEOUT_SEC 30
#define OPERATION_SUCCESS 0
#define OPERATION_FAIL -1
#define WAIT_FOR_RADIO_ENABLE_TIMEOUT_SEC 100
#define MAX_RADIO_DISABLED_TIMEOUT_SEC 4
#define MAX_CANCEL_CAC_TIMEOUT_SEC 10

//////////////////////////////////////////////////////////////////////////////
/////////////////////////// Local Module Functions ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

static void copy_vaps_info(std::shared_ptr<bwl::ap_wlan_hal> &ap_wlan_hal,
                           beerocks_message::sVapInfo vaps[])
{
    if (!ap_wlan_hal->refresh_vaps_info()) {
        LOG(ERROR) << "Failed to refresh vaps info!";
        return;
    }

    const auto &radio_vaps = ap_wlan_hal->get_radio_info().available_vaps;

    // Copy the VAPs
    for (int vap_id = beerocks::IFACE_VAP_ID_MIN, i = 0; vap_id <= beerocks::IFACE_VAP_ID_MAX;
         vap_id++, i++) {

        // Clear the memory
        vaps[i] = {};

        // If the VAP ID exists
        if (radio_vaps.find(vap_id) == radio_vaps.end()) {
            continue;
        }
        const auto &curr_vap = radio_vaps.at(vap_id);

        LOG(DEBUG) << "vap_id=" << int(vap_id) << ", mac=" << curr_vap.mac
                   << ", ssid=" << curr_vap.ssid << ", fronthaul=" << curr_vap.fronthaul
                   << ", backhaul=" << curr_vap.backhaul;

        if (curr_vap.backhaul) {
            LOG(DEBUG) << "disallow_profile1="
                       << curr_vap.profile1_backhaul_sta_association_disallowed
                       << ", disallow_profile2="
                       << curr_vap.profile2_backhaul_sta_association_disallowed;
        }

        // Copy the VAP MAC and SSID
        vaps[i].mac = tlvf::mac_from_string(curr_vap.mac);
        beerocks::string_utils::copy_string(vaps[i].ssid, curr_vap.ssid.c_str(),
                                            beerocks::message::WIFI_SSID_MAX_LENGTH);

        vaps[i].fronthaul_vap = curr_vap.fronthaul;
        vaps[i].backhaul_vap  = curr_vap.backhaul;
        vaps[i].profile1_backhaul_sta_association_disallowed =
            curr_vap.profile1_backhaul_sta_association_disallowed;
        vaps[i].profile2_backhaul_sta_association_disallowed =
            curr_vap.profile2_backhaul_sta_association_disallowed;
    }
}

static void build_channels_list(ieee1905_1::CmduMessageTx &cmdu_tx,
                                const std::map<uint8_t, bwl::sChannelInfo> &channels_list,
                                std::shared_ptr<beerocks_message::cChannelList> &channel_list_class)
{
    // Rank container for multiap preference calculation.
    // Key - rank average, value - rank average elements
    std::map<int32_t, std::set<int32_t>> ranks;

    // Copy the Rank from the channels list to a helper container "ranks" that will be used to
    // convert the rank to multi-ap preference.
    for (const auto &channel_element : channels_list) {
        const auto &channel_info = channel_element.second;
        for (const auto &bw_info : channel_info.bw_info_list) {
            auto rank = bw_info.second;
            if (rank == -1) {
                continue;
            }

            // Copy rank to helper container that will help to convert the rank to multi-ap preference
            ranks[rank].insert(rank);
        }
    }

    // Multi-AP allows only 15 options of preference whereas the ranking from the ACS-Report has
    // 2^31 options.
    // To scale the rank to 1-14 (0 is not operable and represented by a rank of  -1), group
    // rankings with small delta until only 14 groups are left:
    // {group_rank_average, { rank1, rank2, rank3}}.
    // Note: Since "std::map" is ordered container, we don't have to sort it.
    LOG(DEBUG) << "Narrow ranks to groups, ranks.size()=" << ranks.size();
    constexpr uint8_t max_score = wfa_map::cPreferenceOperatingClasses::ePreference::PREFERRED14;
    while (ranks.size() > max_score) {
        auto min_delta = INT32_MAX;

        // Key = Min delta ranks group ID, Value: Min delta ranks
        std::unordered_map<uint16_t, std::set<int32_t>> min_delta_ranks_groups;
        uint16_t group_id = 0;
        for (auto it = std::next(ranks.begin()); it != ranks.end(); it++) {
            auto this_rank = it->first;
            auto prev_rank = std::prev(it)->first;
            auto delta     = this_rank - prev_rank;
            if (delta <= min_delta) {
                if (delta < min_delta) {
                    min_delta_ranks_groups.clear();
                    min_delta = delta;
                }
                min_delta_ranks_groups[group_id].insert(this_rank);
                min_delta_ranks_groups[group_id].insert(prev_rank);
                continue;
            }
            group_id++;
        }

        // Unify the original ranks which are under the same group, and add the unified group to
        // the ranks list. The two separated groups that that made the new group are removed.
        for (const auto min_delta_ranks_group : min_delta_ranks_groups) {
            std::set<int32_t> unified_rank_elements;
            auto &min_delta_ranks_on_group = min_delta_ranks_group.second;
            for (const auto &min_delta_rank : min_delta_ranks_on_group) {
                unified_rank_elements.insert(ranks[min_delta_rank].begin(),
                                             ranks[min_delta_rank].end());
                ranks.erase(min_delta_rank);
            }
            auto average_rank =
                std::accumulate(unified_rank_elements.begin(), unified_rank_elements.end(), 0) /
                unified_rank_elements.size();

            ranks.emplace(average_rank, unified_rank_elements);
        }
    }

    // Fill the channels list on the CMDU using the helper container.
    LOG(DEBUG) << "Channels list: ";
    for (const auto &channel_info_pair : channels_list) {
        auto channel_info_tlv = channel_list_class->create_channels_list();
        if (!channel_info_tlv) {
            LOG(ERROR) << "Failed to allocate cChannel!";
            return;
        }
        const auto &channel_info           = channel_info_pair.second;
        channel_info_tlv->beacon_channel() = channel_info_pair.first;
        channel_info_tlv->tx_power_dbm()   = channel_info.tx_power_dbm;
        channel_info_tlv->dfs_state()      = [](const beerocks::eDfsState bwl_dfs_state) {
            switch (bwl_dfs_state) {
            case beerocks::eDfsState::USABLE: {
                return beerocks_message::eDfsState::USABLE;
            }
            case beerocks::eDfsState::UNAVAILABLE: {
                return beerocks_message::eDfsState::UNAVAILABLE;
            }
            case beerocks::eDfsState::AVAILABLE: {
                return beerocks_message::eDfsState::AVAILABLE;
            }
            default: {
                break;
            }
            }
            return beerocks_message::eDfsState::NOT_DFS;
        }(channel_info.dfs_state);

        if (!channel_info_tlv->alloc_supported_bandwidths(
                channel_info_pair.second.bw_info_list.size())) {
            LOG(ERROR) << "Failed to allocate sSupportedBandwidth list!";
            return;
        }

        for (uint8_t i = 0; i < channel_info_tlv->supported_bandwidths_length(); i++) {
            auto &supported_bw_info_tlv = std::get<1>(channel_info_tlv->supported_bandwidths(i));
            auto bw_it                  = std::next(channel_info.bw_info_list.begin(), i);
            supported_bw_info_tlv.bandwidth = bw_it->first;
            supported_bw_info_tlv.rank      = bw_it->second;

            auto print_channel_info = [&]() {
                auto dfs_state_to_string = [](beerocks_message::eDfsState dfs_state) {
                    if (dfs_state == beerocks_message::eDfsState::NOT_DFS) {
                        return "NOT_DFS";
                    } else if (dfs_state == beerocks_message::eDfsState::AVAILABLE) {
                        return "AVAILABLE";
                    } else if (dfs_state == beerocks_message::eDfsState::USABLE) {
                        return "USABLE";
                    } else if (dfs_state == beerocks_message::eDfsState::UNAVAILABLE) {
                        return "UNAVAILABLE";
                    }
                    return "Unknown_State";
                };

                LOG(DEBUG) << "channel=" << int(channel_info_pair.first) << ", bw="
                           << beerocks::utils::convert_bandwidth_to_int(
                                  beerocks::eWiFiBandwidth(bw_it->first))
                           << ", rank=" << supported_bw_info_tlv.rank << ", multiap_preference="
                           << int(supported_bw_info_tlv.multiap_preference)
                           << ", dfs_state=" << dfs_state_to_string(channel_info_tlv->dfs_state());
            };

            // If channel & bw has undefined rank (-1), set the channel preference to
            // "Not Usable" (0).
            if (supported_bw_info_tlv.rank == -1) {
                supported_bw_info_tlv.multiap_preference = 0;
                print_channel_info();
                continue;
            }

            if (channel_info_tlv->dfs_state() == beerocks_message::eDfsState::UNAVAILABLE) {
                supported_bw_info_tlv.multiap_preference = 0;
                supported_bw_info_tlv.rank               = -1;
                continue;
            }

            // The ranks are sorted since they are on an ordered container. Therefore, use the
            // the index of each element to calculate the Multi-AP preference by subtracting
            // the rank element index from 15 (Best rank).
            for (uint8_t rank_idx = 0; rank_idx < ranks.size(); rank_idx++) {
                auto &rank_group_set = std::next(ranks.begin(), rank_idx)->second;
                if (rank_group_set.find(supported_bw_info_tlv.rank) != rank_group_set.end()) {
                    supported_bw_info_tlv.multiap_preference = max_score - rank_idx;
                    break;
                }
            }
            print_channel_info();
        }

        channel_list_class->add_channels_list(channel_info_tlv);
    }
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Implementation ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////

using namespace beerocks;
using namespace son;

ApManager::ApManager(const std::string &iface, beerocks::logging &logger,
                     std::shared_ptr<beerocks::CmduClientFactory> slave_cmdu_client_factory,
                     std::shared_ptr<beerocks::TimerManager> timer_manager,
                     std::shared_ptr<beerocks::EventLoop> event_loop)
    : cmdu_tx(m_tx_buffer, sizeof(m_tx_buffer)), m_logger(logger),
      m_slave_cmdu_client_factory(slave_cmdu_client_factory), m_timer_manager(timer_manager),
      m_event_loop(event_loop)
{
    LOG_IF(!m_slave_cmdu_client_factory, FATAL) << "CMDU client factory is a null pointer!";
    LOG_IF(!m_timer_manager, FATAL) << "Timer manager is a null pointer!";
    LOG_IF(!m_event_loop, FATAL) << "Event loop is a null pointer!";

    m_iface = iface;
}

bool ApManager::create_ap_wlan_hal()
{
    using namespace std::placeholders; // for `_1`

    bwl::hal_conf_t hal_conf;
    hal_conf.ap_acs_enabled = acs_enabled;

    if (!beerocks::bpl::bpl_cfg_get_hostapd_ctrl_path(m_iface, hal_conf.wpa_ctrl_path)) {
        LOG(ERROR) << "Couldn't get hostapd control path for interface " << m_iface;
        return false;
    }

    if (!beerocks::bpl::bpl_cfg_get_monitored_BSSs_by_radio_iface(m_iface,
                                                                  hal_conf.monitored_BSSs)) {
        LOG(DEBUG) << "Failed to get radio-monitored-BSSs for interface " << m_iface;
    }

    // Create a new AP HAL instance
    ap_wlan_hal = bwl::ap_wlan_hal_create(m_iface, hal_conf,
                                          std::bind(&ApManager::hal_event_handler, this, _1));

    LOG_IF(!ap_wlan_hal, FATAL) << "Failed creating HAL instance!";

    return true;
}

bool ApManager::start()
{
    if (m_slave_client) {
        LOG(ERROR) << "AP manager is already started";
        return false;
    }

    // In case of error in one of the steps of this method, we have to undo all the previous steps
    // (like when rolling back a database transaction, where either all steps get executed or none
    // of them gets executed)
    beerocks::Transaction transaction;

    // Create a timer to run the FSM periodically
    m_fsm_timer = m_timer_manager->add_timer("AP Manager FSM", fsm_timer_period, fsm_timer_period,
                                             [&](int fd, beerocks::EventLoop &loop) {
                                                 bool continue_processing = true;
                                                 while (continue_processing) {
                                                     if (!ap_manager_fsm(continue_processing)) {
                                                         return false;
                                                     }
                                                 }

                                                 return true;
                                             });
    if (m_fsm_timer == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(ERROR) << "Failed to create the FSM timer";
        return false;
    }
    LOG(DEBUG) << "FSM timer created with fd = " << m_fsm_timer;
    transaction.add_rollback_action([&]() { m_timer_manager->remove_timer(m_fsm_timer); });

    // Create an instance of a CMDU client connected to the CMDU server that is running in the slave
    m_slave_client = m_slave_cmdu_client_factory->create_instance();
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

        m_state = eApManagerState::TERMINATED;
    };

    m_slave_client->set_handlers(handlers);
    transaction.add_rollback_action([&]() { m_slave_client->clear_handlers(); });

    m_state = eApManagerState::INIT;

    transaction.commit();

    LOG(DEBUG) << "started";

    return true;
}

bool ApManager::stop()
{
    bool ok = true;

    if (m_slave_client) {
        m_slave_client.reset();
    }

    if (!m_timer_manager->remove_timer(m_fsm_timer)) {
        ok = false;
    }

    if (ap_wlan_hal) {
        for (auto &fd : m_ap_hal_ext_events) {
            if (fd > 0) {
                m_event_loop->remove_handlers(fd);
            }
        }

        if (m_ap_hal_int_events > 0) {
            m_event_loop->remove_handlers(m_ap_hal_int_events);
        }

        ap_wlan_hal->detach();
        ap_wlan_hal.reset();
    }

    m_state = eApManagerState::TERMINATED;

    LOG(DEBUG) << "stopped";

    return ok;
}

bool ApManager::send_cmdu(ieee1905_1::CmduMessageTx &cmdu_tx)
{
    return m_slave_client->send_cmdu(cmdu_tx);
}

bool ApManager::ap_manager_fsm(bool &continue_processing)
{
    // Continue processing events if the state machine is in a transient state.
    // Transient states are INIT and ATTACHED. All other states are steady states.
    continue_processing = false;

    switch (m_state) {
    case eApManagerState::INIT: {
        auto request =
            message_com::create_vs_message<beerocks_message::cACTION_APMANAGER_UP_NOTIFICATION>(
                cmdu_tx);

        if (!request) {
            LOG(ERROR) << "Failed building message!";
            break;
        }
        request->set_iface_name(m_iface);

        send_cmdu(cmdu_tx);

        m_state = eApManagerState::WAIT_FOR_CONFIGURATION;

        continue_processing = true;

        // Set the timout to next select cycle
        m_state_timeout =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(SELECT_TIMEOUT_MSC);
        break;
    }
    case eApManagerState::WAIT_FOR_CONFIGURATION: {
        // On ACTION_APMANAGER_CONFIGURE handler, the ap_hal will be created, and the state will
        // change to ATTACHING state.
        if (std::chrono::steady_clock::now() > m_state_timeout) {
            LOG(ERROR) << "Agent did not send configuration message";
            return false;
        }
        break;
    }
    case eApManagerState::ATTACHING: {
        auto attach_state = ap_wlan_hal->attach();

        if (attach_state == bwl::HALState::Operational) {
            LOG(DEBUG) << "Move to ATTACHED state";
            m_state = eApManagerState::ATTACHED;
            break;
        }

        if (attach_state == bwl::HALState::Failed) {
            LOG(ERROR) << "Failed attaching to WLAN HAL";
            return false;
        }

        LOG(INFO) << "waiting to attach to " << ap_wlan_hal->get_radio_info().iface_name;
        break;
    }
    case eApManagerState::ATTACHED: {
        // External events
        int ext_event_fd_max = -1;
        m_ap_hal_ext_events  = ap_wlan_hal->get_ext_events_fds();
        if (m_ap_hal_ext_events.empty()) {
            ext_event_fd_max = 0;
        } else {
            beerocks::EventLoop::EventHandlers ext_events_handlers{
                .name = "ap_hal_ext_events",
                .on_read =
                    [&](int fd, EventLoop &loop) {
                        if (!ap_wlan_hal->process_ext_events(fd)) {
                            LOG(ERROR) << "process_ext_events(" << fd << ") failed!";
                            return false;
                        }
                        return true;
                    },
                .on_write = nullptr,
                .on_disconnect =
                    [&](int fd, EventLoop &loop) {
                        LOG(ERROR) << "ap_hal_ext_event disconnected! on fd " << fd;
                        auto it =
                            std::find(m_ap_hal_ext_events.begin(), m_ap_hal_ext_events.end(), fd);
                        if (it != m_ap_hal_ext_events.end()) {
                            *it = beerocks::net::FileDescriptor::invalid_descriptor;
                        }
                        return false;
                    },
                .on_error =
                    [&](int fd, EventLoop &loop) {
                        LOG(ERROR) << "ap_hal_ext_events error! on fd " << fd;
                        auto it =
                            std::find(m_ap_hal_ext_events.begin(), m_ap_hal_ext_events.end(), fd);
                        if (it != m_ap_hal_ext_events.end()) {
                            *it = beerocks::net::FileDescriptor::invalid_descriptor;
                        }
                        return false;
                    },
            };
            for (auto &ext_event_fd : m_ap_hal_ext_events) {
                if (ext_event_fd > 0) {
                    if (!m_event_loop->register_handlers(ext_event_fd, ext_events_handlers)) {
                        LOG(ERROR)
                            << "Unable to register handlers for external event fd " << ext_event_fd;
                        return false;
                    } else if (ext_event_fd < 0) {
                        ext_event_fd = beerocks::net::FileDescriptor::invalid_descriptor;
                    }
                    LOG(DEBUG) << "External events queue with fd = " << ext_event_fd;
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
        m_ap_hal_int_events = ap_wlan_hal->get_int_events_fd();
        if (m_ap_hal_int_events > 0) {
            beerocks::EventLoop::EventHandlers int_events_handlers{
                .name = "ap_hal_int_events",
                .on_read =
                    [&](int fd, EventLoop &loop) {
                        if (!ap_wlan_hal->process_int_events()) {
                            LOG(ERROR) << "process_int_events() failed!";
                            return false;
                        }
                        return true;
                    },
                .on_write = nullptr,
                .on_disconnect =
                    [&](int fd, EventLoop &loop) {
                        LOG(ERROR) << "ap_hal_int_events disconnected!";
                        m_ap_hal_int_events = beerocks::net::FileDescriptor::invalid_descriptor;
                        return false;
                    },
                .on_error =
                    [&](int fd, EventLoop &loop) {
                        LOG(ERROR) << "ap_hal_int_events error!";
                        m_ap_hal_int_events = beerocks::net::FileDescriptor::invalid_descriptor;
                        return false;
                    },
            };
            if (!m_event_loop->register_handlers(m_ap_hal_int_events, int_events_handlers)) {
                LOG(ERROR) << "Unable to register handlers for internal events queue!";
                return false;
            }
            LOG(DEBUG) << "Internal events queue with fd = " << m_ap_hal_int_events;
        } else {
            LOG(ERROR) << "Invalid internal event file descriptor: " << m_ap_hal_int_events;
            return false;
        }

        // Set the time for the next heartbeat notification
        next_heartbeat_notification_timestamp =
            std::chrono::steady_clock::now() +
            std::chrono::seconds(HEARTBEAT_NOTIFICATION_DELAY_SEC);

        m_ap_support_zwdfs = ap_wlan_hal->is_zwdfs_supported();

        LOG(DEBUG) << "Move to OPERATIONAL state";
        m_state = eApManagerState::OPERATIONAL;

        continue_processing = true;

        break;
    }
    case eApManagerState::OPERATIONAL: {
        // Process external events
        auto itFdMax =
            std::max_element(std::begin(m_ap_hal_ext_events), std::end(m_ap_hal_ext_events));
        if (itFdMax == std::end(m_ap_hal_ext_events) || *itFdMax == 0) {
            // There is no socket for external events, so we simply try
            // to process any available periodically
            if (!ap_wlan_hal->process_ext_events()) {
                LOG(ERROR) << "process_ext_events() failed!";
                return false;
            }
        }

        // Send Heartbeat notification if needed
        auto now = std::chrono::steady_clock::now();
        if (now > next_heartbeat_notification_timestamp) {
            send_heartbeat();
            next_heartbeat_notification_timestamp =
                now + std::chrono::seconds(HEARTBEAT_NOTIFICATION_DELAY_SEC);
        }

        // Long running operations prevent the event loop from doing anything else (i.e.: the
        // event loop is not able to react to any incoming request in the meantime).
        // Therefore, limit the maximum amount of time that the following method can run,
        // instead of letting it run non-stop for what could be quite a long time if there
        // are many clients already connected.
        auto max_iteration_timeout = std::chrono::steady_clock::now() + fsm_timer_period / 2;

        if (m_generate_connected_clients_events &&
            (m_next_generate_connected_events_time < std::chrono::steady_clock::now())) {
            bool is_finished_all_clients = false;
            // If there is not enough time to generate all events, the method will be called in the
            // next FSM iteration, and so on until all connected clients are eventually reported.
            auto max_generate_timeout =
                (std::chrono::steady_clock::now() +
                 std::chrono::milliseconds(GENERATE_CONNECTED_EVENTS_WORK_TIME_LIMIT_MSEC));
            max_generate_timeout = std::min(max_generate_timeout, max_iteration_timeout);
            // If there is not enough time to generate all events, the method will be called in the
            // next FSM iteration, and so on until all connected clients are eventually reported.
            if (!ap_wlan_hal->generate_connected_clients_events(is_finished_all_clients,
                                                                max_generate_timeout)) {
                LOG(ERROR) << "Failed to generate connected clients events";
                return false;
            }
            m_next_generate_connected_events_time =
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(GENERATE_CONNECTED_EVENTS_DELAY_MSEC);
            // Reset the flag if finished to generate all clients' events
            m_generate_connected_clients_events = !is_finished_all_clients;
        }

        // Allow clients with expired blocking period timer
        allow_expired_clients();
        break;
    }
    case eApManagerState::TERMINATED: {
        return false;
    }
    default:
        break;
    }

    return true;
}

void ApManager::handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx)
{
    auto beerocks_header = message_com::parse_intel_vs_message(cmdu_rx);
    if (beerocks_header == nullptr) {
        LOG(ERROR) << "Not a vendor specific message";
        return;
    }

    if (beerocks_header->action() != beerocks_message::ACTION_APMANAGER) {
        LOG(ERROR) << "Unsupported action: " << int(beerocks_header->action())
                   << " op=" << int(beerocks_header->action_op());
        return;
    }

    // Ignore configuration message if already configured.
    if (ap_wlan_hal &&
        (beerocks_header->action_op() == beerocks_message::ACTION_APMANAGER_CONFIGURE)) {
        LOG(ERROR) << "Already configured. Ignoring configuration message";
        return;
    }

    // Ignore messages other than the configuration message if not yet configured.
    if (!ap_wlan_hal &&
        (beerocks_header->action_op() != beerocks_message::ACTION_APMANAGER_CONFIGURE)) {
        LOG(ERROR) << "Not configured. Ignoring message with op="
                   << int(beerocks_header->action_op());
        return;
    }

    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_APMANAGER_CONFIGURE: {
        LOG(TRACE) << "ACTION_APMANAGER_CONFIGURE";

        auto config = beerocks_header->addClass<beerocks_message::cACTION_APMANAGER_CONFIGURE>();
        if (!config) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_CONFIGURE failed";
            return;
        }

        acs_enabled = config->channel() == 0;

        if (create_ap_wlan_hal()) {
            LOG(DEBUG) << "Move to ATTACHING state";
            m_state = eApManagerState::ATTACHING;
        } else {
            m_state = eApManagerState::TERMINATED;
        }

        break;
    }
    case beerocks_message::ACTION_APMANAGER_ENABLE_APS_REQUEST: {
        LOG(TRACE) << "ACTION_APMANAGER_ENABLE_APS_REQUEST";

        auto notification =
            beerocks_header->addClass<beerocks_message::cACTION_APMANAGER_ENABLE_APS_REQUEST>();
        if (!notification) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_ENABLE_APS_REQUEST failed";
            return;
        }

        auto response =
            message_com::create_vs_message<beerocks_message::cACTION_APMANAGER_ENABLE_APS_RESPONSE>(
                cmdu_tx);

        if (!response) {
            LOG(ERROR) << "Failed building message!";
            return;
        }

        response->success() = true;

        // Disable the radio interface to make hostapd to consider the new configuration.
        if (!ap_wlan_hal->disable()) {
            LOG(DEBUG) << "ap disable() failed!, the interface might be already disabled or down";
        }

        // If it is not the radio of the BH, then channel, bandwidth and center channel paramenters
        // will be all set to 0.
        LOG(DEBUG) << "Setting AP channel: "
                   << ", channel=" << int(notification->channel())
                   << ", bandwidth=" << notification->bandwidth()
                   << ", center_channel=" << int(notification->center_channel());

        // Set original channel or BH channel
        if (!ap_wlan_hal->set_channel(notification->channel(), notification->bandwidth(),
                                      notification->center_channel())) {
            LOG(ERROR) << "Failed setting set_channel";
            response->success() = false;
            send_cmdu(cmdu_tx);
            break;
        }

        // Enable the radio interface to apply the new configuration.
        if (!ap_wlan_hal->enable()) {
            LOG(ERROR) << "Failed enable";
            response->success() = false;
        }

        LOG(INFO) << "send ACTION_APMANAGER_ENABLE_APS_RESPONSE, success="
                  << int(response->success());
        send_cmdu(cmdu_tx);

        // In case hostapd was re-enabled on different channel there may not
        // be a CSA (channel switch notification) notification event from hostapd.
        // Generate CSA notification anyway and send to upper layers to make
        // sure agent DB is up to date.
        if (response->success()) {
            auto csa_notification = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION>(cmdu_tx);
            if (!csa_notification) {
                LOG(ERROR) << "Failed building message!";
                return;
            }
            ap_wlan_hal->refresh_radio_info();
            fill_cs_params(csa_notification->cs_params());
            send_cmdu(cmdu_tx);
        }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST: {

        auto request = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass "
                          "cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_REQUEST failed";
            return;
        }

        bool success = true;

        uint32_t failsafe_channel = request->params().failsafe_channel;

        if (request->params().restricted_channels[0] == 0) {
            LOG(INFO) << "Clearing restricted channels...";
        } else {
            LOG(INFO) << "Setting radio restricted channels";
        }

        if (!ap_wlan_hal->restricted_channels_set((char *)request->params().restricted_channels)) {
            LOG(ERROR) << "Failed setting restricted channels!";
            success = false;
        }

        if (failsafe_channel != 0) {
            // Send channel_switch_request to bwl
            LOG(INFO) << " Calling failsafe_channel_set - "
                      << "failsafe_channel: " << failsafe_channel << ", channel_bandwidth: "
                      << beerocks::utils::convert_bandwidth_to_int(
                             (beerocks::eWiFiBandwidth)request->params().failsafe_channel_bandwidth)
                      << ", vht_center_frequency: " << request->params().vht_center_frequency;

            if (!ap_wlan_hal->failsafe_channel_set(failsafe_channel,
                                                   request->params().failsafe_channel_bandwidth,
                                                   request->params().vht_center_frequency)) {

                LOG(ERROR) << "Failed setting failsafe channel!";
                success = false;
            }

        } else {
            LOG(INFO) << "failsafe channel is zero";
        }

        LOG(INFO)
            << "send ACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE success = "
            << int(success);
        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_SET_RESTRICTED_FAILSAFE_CHANNEL_RESPONSE>(
            cmdu_tx);
        if (response == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return;
        }
        response->success() = success;

        send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START failed";
            return;
        }
        LOG(DEBUG) << "ACTION_APMANAGER_HOSTAP_CHANNEL_SWITCH_ACS_START: requested channel="
                   << int(request->cs_params().channel) << " bandwidth="
                   << beerocks::utils::convert_bandwidth_to_int(
                          (beerocks::eWiFiBandwidth)request->cs_params().bandwidth);

        LOG_IF(request->cs_params().channel == 0, DEBUG) << "Start ACS";

        // Set transmit power
        if (request->tx_limit_valid()) {
            ap_wlan_hal->set_tx_power_limit(request->tx_limit());
            LOG(INFO) << "Current channel: " << ap_wlan_hal->get_radio_info().channel
                      << " Current BW: " << ap_wlan_hal->get_radio_info().bandwidth;
            if (ap_wlan_hal->get_radio_info().channel == request->cs_params().channel &&
                utils::convert_bandwidth_to_enum(ap_wlan_hal->get_radio_info().bandwidth) ==
                    request->cs_params().bandwidth) {
                LOG(DEBUG) << "Setting tx power without channel switch, send CSA notification";
                auto notification = message_com::create_vs_message<
                    beerocks_message::cACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION>(cmdu_tx);
                if (!notification) {
                    LOG(ERROR) << "Failed building message!";
                    return;
                }
                ap_wlan_hal->refresh_radio_info();
                fill_cs_params(notification->cs_params());
                send_cmdu(cmdu_tx);
            }
        }

        // Set AP channel
        if (ap_wlan_hal->get_radio_info().channel != request->cs_params().channel &&
            !ap_wlan_hal->switch_channel(request->cs_params().channel,
                                         request->cs_params().bandwidth,
                                         request->cs_params().vht_center_frequency)) { //error
            std::string error("Failed to set AP channel!");
            LOG(ERROR) << error;

            ap_wlan_hal->refresh_radio_info();

            // Send the error reponse
            auto notification = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_HOSTAP_CSA_ERROR_NOTIFICATION>(
                cmdu_tx, beerocks_header->id());
            if (notification == nullptr) {
                LOG(ERROR) << "Failed building message!";
                return;
            }
            fill_cs_params(notification->cs_params());
            send_cmdu(cmdu_tx);
            return;
        }
        break;
    }

    case beerocks_message::ACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST failed";
            return;
        }
        bool cancel_cac_success = false;

        if (!ap_wlan_hal->cancel_cac(
                request->cs_params().channel,
                utils::convert_bandwidth_to_enum(request->cs_params().bandwidth),
                request->cs_params().vht_center_frequency,
                request->cs_params().channel_ext_above_primary)) {
            LOG(ERROR) << "Cancel cac failed!";
        }

        // In order to make sure cac was actually canceled and radio is operational again
        // we need to poll until radio state is ENABLED or DFS (in case radio is
        // enabled back on DFS channel) or timeout.
        // NOTE: bwl implementations default radio_state == UNKNOWN so consider them
        // also as ENABLED.
        auto timeout =
            std::chrono::steady_clock::now() + std::chrono::seconds(MAX_CANCEL_CAC_TIMEOUT_SEC);
        while (cancel_cac_success && std::chrono::steady_clock::now() < timeout) {
            if (!ap_wlan_hal->refresh_radio_info()) {
                LOG(WARNING) << "Radio could be temporary disabled, wait grace time "
                             << std::chrono::duration_cast<std::chrono::seconds>(
                                    timeout - std::chrono::steady_clock::now())
                                    .count()
                             << " sec.";
                UTILS_SLEEP_MSEC(500);
                continue;
            }

            if (ap_wlan_hal->get_radio_info().radio_state == bwl::eRadioState::ENABLED ||
                (ap_wlan_hal->get_radio_info().radio_state == bwl::eRadioState::DFS) ||
                (ap_wlan_hal->get_radio_info().radio_state == bwl::eRadioState::UNKNOWN)) {
                cancel_cac_success = true;
            } else {
                LOG(WARNING) << "radio state is still not enabled, waiting for more "
                             << std::chrono::duration_cast<std::chrono::seconds>(
                                    timeout - std::chrono::steady_clock::now())
                                    .count()
                             << " seconds.";
                UTILS_SLEEP_MSEC(500);
            }
        }

        // send the result
        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE>(
            cmdu_tx, beerocks_header->id());
        if (!response) {
            LOG(ERROR) << "Failed building cACTION_APMANAGER_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE!";
            return;
        }
        response->success() = cancel_cac_success;
        send_cmdu(cmdu_tx);

        // As part of cancel CAC flow hostapd is re-enabled on diffrent (previous) channel
        // and hostapd may not send CSA (channel switch notification) event.
        // Generate CSA notification anyway and send to upper layers to make
        // sure agent DB is up to date.
        if (response->success()) {
            auto notification = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION>(cmdu_tx);
            if (!notification) {
                LOG(ERROR) << "Failed building message!";
                return;
            }
            ap_wlan_hal->refresh_radio_info();
            fill_cs_params(notification->cs_params());
            send_cmdu(cmdu_tx);
        }

        break;
    }

    case beerocks_message::ACTION_APMANAGER_HOSTAP_SET_NEIGHBOR_11K_REQUEST: {
        LOG(WARNING) << "UNIMPLEMENTED - ACTION_APMANAGER_HOSTAP_SET_NEIGHBOR_11K_REQUEST";
        // auto request = (message::sACTION_APMANAGER_HOSTAP_SET_NEIGHBOR_11K_REQUEST*)rx_buffer;
        // if (!ap_man_hal.neighbor_set(request->params)){
        //     LOG(ERROR) << "Failed to set neighbor!";
        // }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST: {
        LOG(WARNING) << "UNIMPLEMENTED - ACTION_APMANAGER_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST";
        // auto request = (message::sACTION_APMANAGER_HOSTAP_REMOVE_NEIGHBOR_11K_REQUEST*)rx_buffer;
        // if (!ap_man_hal.neighbor_remove(request->params)){
        //     LOG(ERROR) << "Failed to set neighbor!";
        // }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_DISCONNECT_REQUEST: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_CLIENT_DISCONNECT_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_CLIENT_DISCONNECT_REQUEST failed";
            send_steering_return_status(
                beerocks_message::ACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE, OPERATION_FAIL);
            return;
        }
        std::string sta_mac = tlvf::mac_to_string(request->mac());
        auto vap_id         = request->vap_id();
        auto type           = request->type();
        auto reason         = request->reason();

        LOG(DEBUG) << "CLIENT_DISCONNECT, type "
                   << ((type == beerocks_message::eDisconnect_Type_Deauth) ? "DEAUTH" : "DISASSOC")
                   << " vap_id = " << int(vap_id) << " mac = " << sta_mac
                   << " reason = " << std::to_string(reason);
        bool res;
        if (type == beerocks_message::eDisconnect_Type_Deauth) {
            res = ap_wlan_hal->sta_deauth(vap_id, sta_mac, reason);
        } else {
            res = ap_wlan_hal->sta_disassoc(vap_id, sta_mac, reason);
        }
        send_steering_return_status(beerocks_message::ACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE,
                                    res ? OPERATION_SUCCESS : OPERATION_FAIL);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_DISALLOW_REQUEST: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_CLIENT_DISALLOW_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_CLIENT_DISALLOW_REQUEST failed";
            return;
        }

        std::string sta_mac = tlvf::mac_to_string(request->mac());
        std::string bssid   = tlvf::mac_to_string(request->bssid());

        const auto &vap_unordered_map = ap_wlan_hal->get_radio_info().available_vaps;
        auto it = std::find_if(vap_unordered_map.begin(), vap_unordered_map.end(),
                               [&](const std::pair<int, bwl::VAPElement> &element) {
                                   return element.second.mac == bssid;
                               });

        if (it == vap_unordered_map.end()) {
            //AP does not have the requested vap, probably will be handled on the other AP
            return;
        }

        LOG(DEBUG) << "CLIENT_DISALLOW: mac = " << sta_mac << ", bssid = " << bssid;

        ap_wlan_hal->sta_deny(sta_mac, bssid);

        // Check if validity period is set then add it to the "disallowed client timeouts" list
        // This list will be polled in ap_manager_fsm() while in operational state through method
        // `allow_expired_clients()`
        // When validity period is timed out sta_allow will be called.
        if (request->validity_period_sec()) {

            disallowed_client_t disallowed_client;

            // calculate new disallow timeout from client validity period parameter [sec]
            disallowed_client.timeout = std::chrono::steady_clock::now() +
                                        std::chrono::seconds(request->validity_period_sec());
            disallowed_client.mac   = request->mac();
            disallowed_client.bssid = request->bssid();

            // Remove old disallow period timeout from the list before inserting new
            remove_client_from_disallowed_list(request->mac(), request->bssid());

            // insert new disallow timeout to the list
            m_disallowed_clients.push_back(disallowed_client);

            LOG(DEBUG) << "client " << disallowed_client.mac
                       << " will be allowed to accosiate with bssid " << disallowed_client.bssid
                       << " in "
                       << std::chrono::duration_cast<std::chrono::seconds>(
                              disallowed_client.timeout - std::chrono::steady_clock::now())
                              .count()
                       << "sec";
        } else {
            LOG(WARNING) << "CLIENT_DISALLOW validity period set to 0, STA mac " << request->mac()
                         << " will remain blocked from bssid " << request->bssid();
        }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_ALLOW_REQUEST: {
        auto request =
            beerocks_header->addClass<beerocks_message::cACTION_APMANAGER_CLIENT_ALLOW_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_CLIENT_ALLOW_REQUEST failed";
            return;
        }

        std::string sta_mac = tlvf::mac_to_string(request->mac());
        std::string bssid   = tlvf::mac_to_string(request->bssid());

        const auto &vap_unordered_map = ap_wlan_hal->get_radio_info().available_vaps;
        auto it = std::find_if(vap_unordered_map.begin(), vap_unordered_map.end(),
                               [&](const std::pair<int, bwl::VAPElement> &element) {
                                   return element.second.mac == bssid;
                               });

        if (it == vap_unordered_map.end()) {
            //AP does not have the requested vap, probably will be handled on the other AP
            return;
        }

        remove_client_from_disallowed_list(request->mac(), request->bssid());

        LOG(DEBUG) << "CLIENT_ALLOW: mac = " << sta_mac << ", bssid = " << bssid;
        ap_wlan_hal->sta_allow(sta_mac, bssid);

        break;
    }
    case beerocks_message::ACTION_APMANAGER_CHANNELS_LIST_REQUEST: {

        // Read supported_channels (From Netlink HW Features)
        // Refreshing the radio info updates the DFS State of each channel on the channels list.
        if (!ap_wlan_hal->refresh_radio_info()) {
            LOG(ERROR) << "Failed to refresh_radio_info";
            return;
        }

        // Update channels ranking (From ACS Report)
        if (!ap_wlan_hal->read_acs_report()) {
            LOG(ERROR) << "Failed to read acs report";
            return;
        }

        auto response = beerocks::message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CHANNELS_LIST_RESPONSE>(cmdu_tx,
                                                                        beerocks_header->id());
        if (!response) {
            LOG(ERROR) << "Failed building message!";
            return;
        }

        auto channel_list_class = response->create_channel_list();

        build_channels_list(cmdu_tx, ap_wlan_hal->get_radio_info().channels_list,
                            channel_list_class);

        response->add_channel_list(channel_list_class);

        send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST: {
        auto request = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST failed";
            return;
        }
        std::string sta_mac = tlvf::mac_to_string(request->params().mac);
        LOG(DEBUG) << "APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_REQUEST cross, curr id="
                   << sta_unassociated_rssi_measurement_header_id
                   << " request id=" << int(beerocks_header->id());
        bool ap_busy   = false;
        bool bwl_error = false;
        if (sta_unassociated_rssi_measurement_header_id == -1) {
            auto response = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE>(
                cmdu_tx, beerocks_header->id());
            if (response == nullptr) {
                LOG(ERROR) << "Failed building message!";
                return;
            }

            response->mac() = tlvf::mac_from_string(sta_mac);
            send_cmdu(cmdu_tx);
            LOG(DEBUG)
                << "send sACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE, sta_mac = "
                << sta_mac;

            int bandwidth = beerocks::utils::convert_bandwidth_to_int(
                (beerocks::eWiFiBandwidth)request->params().bandwidth);
            if (ap_wlan_hal->sta_unassoc_rssi_measurement(
                    sta_mac, request->params().channel, bandwidth,
                    request->params().vht_center_frequency, request->params().measurement_delay,
                    request->params().mon_ping_burst_pkt_num)) {
            } else {
                bwl_error = true;
                LOG(ERROR) << "sta_unassociated_rssi_measurement failed!";
            }

            sta_unassociated_rssi_measurement_header_id = beerocks_header->id();
            LOG(DEBUG) << "CLIENT_RX_RSSI_MEASUREMENT_REQUEST, mac = " << sta_mac
                       << " channel = " << int(request->params().channel)
                       << " bandwidth = " << bandwidth
                       << " vht_center_frequency = " << int(request->params().vht_center_frequency)
                       << " id = " << sta_unassociated_rssi_measurement_header_id;
        } else {
            ap_busy = true;
            LOG(WARNING)
                << "busy!, send response to retry CLIENT_RX_RSSI_MEASUREMENT_REQUEST, mac = "
                << sta_mac;
        }

        if (ap_busy || bwl_error) {
            auto response = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>(
                cmdu_tx, beerocks_header->id());
            if (response == nullptr) {
                LOG(ERROR) << "Failed building message!";
                return;
            }
            response->params().result.mac = request->params().mac;
            response->params().rx_rssi    = beerocks::RSSI_INVALID;
            response->params().rx_snr     = beerocks::SNR_INVALID;
            response->params().rx_packets = -1;

            send_cmdu(cmdu_tx);
        }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_CLIENT_BSS_STEER_REQUEST: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_CLIENT_BSS_STEER_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_CLIENT_BSS_STEER_REQUEST failed";
            return;
        }

        auto bssid                    = tlvf::mac_to_string(request->params().cur_bssid);
        const auto &vap_unordered_map = ap_wlan_hal->get_radio_info().available_vaps;
        auto it = std::find_if(vap_unordered_map.begin(), vap_unordered_map.end(),
                               [&](const std::pair<int, bwl::VAPElement> &element) {
                                   return element.second.mac == bssid;
                               });

        if (it == vap_unordered_map.end()) {
            //AP does not have the requested vap, probably will be handled on the other AP
            return;
        }
        //TODO Check for STA errors, if error ACK with ErrorCodeTLV
        auto response = message_com::create_vs_message<beerocks_message::cACTION_APMANAGER_ACK>(
            cmdu_tx, beerocks_header->id());

        if (!response) {
            LOG(ERROR) << "Failed building message!";
            return;
        }

        send_cmdu(cmdu_tx);

        std::string sta_mac       = tlvf::mac_to_string(request->params().mac);
        std::string target_bssid  = tlvf::mac_to_string(request->params().target.bssid);
        uint8_t disassoc_imminent = request->params().disassoc_imminent;

        LOG(DEBUG) << "CLIENT_BSS_STEER (802.11v) for sta_mac = " << sta_mac
                   << " to bssid = " << target_bssid
                   << " channel = " << int(request->params().target.channel);
        ap_wlan_hal->sta_bss_steer(
            it->first, sta_mac, target_bssid, request->params().target.operating_class,
            request->params().target.channel,
            (disassoc_imminent) ? (request->params().disassoc_timer_ms / BEACON_TRANSMIT_TIME_MS)
                                : 0,
            (disassoc_imminent) ? bss_steer_imminent_valid_int : bss_steer_valid_int,
            request->params().target.reason);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_WIFI_CREDENTIALS_UPDATE_REQUEST: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_WIFI_CREDENTIALS_UPDATE_REQUEST>();
        if (!request) {
            LOG(ERROR) << "addClass ACTION_APMANAGER_WIFI_CREDENTIALS_UPDATE_REQUEST failed";
            return;
        }

        std::list<son::wireless_utils::sBssInfoConf> bss_info_conf_list;
        auto wifi_credentials_size = request->wifi_credentials_size();

        std::string backhaul_wps_ssid, backhaul_wps_passphrase;
        for (auto i = 0; i < wifi_credentials_size; i++) {
            son::wireless_utils::sBssInfoConf bss_info_conf;
            auto config_data_tuple = request->wifi_credentials(i);
            if (!std::get<0>(config_data_tuple)) {
                LOG(ERROR) << "getting config data entry has failed!";
                return;
            }
            auto &config_data = std::get<1>(config_data_tuple);

            // If a Multi-AP Agent receives an AP-Autoconfiguration WSC message containing an M2
            // with a Multi-AP Extension subelement with bit 4 (Tear Down bit) of the subelement
            // value set to one (see Table 4), it shall tear down all currently operating BSS(s)
            // on the radio indicated by the Radio Unique Identifier, and shall ignore the other
            // attributes in the M2.
            auto bss_type =
                static_cast<WSC::eWscVendorExtSubelementBssType>(config_data.bss_type());
            if ((bss_type & WSC::eWscVendorExtSubelementBssType::TEARDOWN) != 0) {
                LOG(DEBUG) << "received teardown";
                bss_info_conf_list.clear();
                break;
            }
            if ((bss_type & WSC::eWscVendorExtSubelementBssType::FRONTHAUL_BSS) != 0) {
                bss_info_conf.fronthaul = true;
            }
            if ((bss_type & WSC::eWscVendorExtSubelementBssType::BACKHAUL_BSS) != 0) {
                bss_info_conf.backhaul  = true;
                backhaul_wps_ssid       = config_data.ssid_str();
                backhaul_wps_passphrase = config_data.network_key_str();
            }
            bss_info_conf.profile1_backhaul_sta_association_disallowed =
                bss_type &
                WSC::eWscVendorExtSubelementBssType::PROFILE1_BACKHAUL_STA_ASSOCIATION_DISALLOWED;
            bss_info_conf.profile2_backhaul_sta_association_disallowed =
                bss_type &
                WSC::eWscVendorExtSubelementBssType::PROFILE2_BACKHAUL_STA_ASSOCIATION_DISALLOWED;

            bss_info_conf.ssid                = config_data.ssid_str();
            bss_info_conf.authentication_type = config_data.authentication_type_attr().data;
            bss_info_conf.encryption_type     = config_data.encryption_type_attr().data;
            bss_info_conf.network_key         = config_data.network_key_str();

            bss_info_conf_list.push_back(bss_info_conf);
        }

        // Before updating vap credentials we need to make sure hostapd is enabled.
        // Entering blocking state until radio is enabled again.
        auto timeout = std::chrono::steady_clock::now() +
                       std::chrono::seconds(WAIT_FOR_RADIO_ENABLE_TIMEOUT_SEC);
        auto perform_update = false;
        while (std::chrono::steady_clock::now() < timeout) {
            if (!ap_wlan_hal->refresh_radio_info()) {
                break;
            }

            if (ap_wlan_hal->get_radio_info().radio_state == bwl::eRadioState::ENABLED) {
                perform_update = true;
                LOG(DEBUG) << "Radio is in enabled state, performing vap credentials update";
                break;
            }
            UTILS_SLEEP_MSEC(500);
        }

        if (perform_update) {
            ap_wlan_hal->update_vap_credentials(bss_info_conf_list, backhaul_wps_ssid,
                                                backhaul_wps_passphrase);

            // hostapd is enabled and started radio beaconing after autoconfiguration
            // then radio tx power is updated, so to keep agent DB updated send CSA notification
            // like it is handled in cACTION_APMANAGER_ENABLE_APS_REQUEST.
            auto csa_notification = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION>(cmdu_tx);
            if (!csa_notification) {
                LOG(ERROR) << "Failed building message!";
                return;
            }
            ap_wlan_hal->refresh_radio_info();
            fill_cs_params(csa_notification->cs_params());
            send_cmdu(cmdu_tx);
        }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_START_WPS_PBC_REQUEST: {
        LOG(DEBUG) << "Got ACTION_APMANAGER_START_WPS_PBC_REQUEST";
        if (!ap_wlan_hal->start_wps_pbc()) {
            LOG(ERROR) << "Failed to start WPS PBC";
            return;
        }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST: {
        LOG(DEBUG) << "Got ACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST";

        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST>();
        if (!request) {
            LOG(ERROR) << "addClass ACTION_APMANAGER_SET_ASSOC_DISALLOW_REQUEST failed";
            return;
        }

        auto bssid = tlvf::mac_to_string(request->bssid());

        if (bssid == beerocks::net::network_utils::ZERO_MAC_STRING) {
            if (!ap_wlan_hal->set_radio_mbo_assoc_disallow(request->enable())) {
                LOG(ERROR) << "Failed to set MBO AssocDisallow";
                return;
            }
        } else {
            if (!ap_wlan_hal->set_mbo_assoc_disallow(bssid, request->enable())) {
                LOG(ERROR) << "Failed to set MBO AssocDisallow";
                return;
            }
        }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_RADIO_DISABLE_REQUEST: {
        LOG(DEBUG) << "Got ACTION_APMANAGER_RADIO_DISABLE_REQUEST";
        // Disable the radio interface
        if (!ap_wlan_hal->disable()) {
            LOG(ERROR) << "Failed disabling radio on iface: " << ap_wlan_hal->get_iface_name();
            return;
        }
        break;
    }

    case beerocks_message::ACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST: {
        auto request =
            beerocks_header
                ->addClass<beerocks_message::cACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST>();
        if (request == nullptr) {
            LOG(ERROR) << "addClass cACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST failed";
            send_steering_return_status(
                beerocks_message::ACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE, OPERATION_FAIL);
            return;
        }

        auto bssid                    = tlvf::mac_to_string(request->params().bssid);
        const auto &vap_unordered_map = ap_wlan_hal->get_radio_info().available_vaps;
        auto it = std::find_if(vap_unordered_map.begin(), vap_unordered_map.end(),
                               [&](const std::pair<int, bwl::VAPElement> &element) {
                                   return element.second.mac == bssid;
                               });

        if (vap_unordered_map.end() == it) {
            LOG(ERROR) << "BSSID " << bssid << " not found";
            return;
        }

        auto vap_name = it->second.bss;
        LOG(DEBUG) << "Got ACTION_APMANAGER_STEERING_CLIENT_SET_REQUEST for BSSID=" << bssid
                   << " VAP=" << vap_name;
        if (!request->params().remove && (request->params().config.snrProbeHWM > 0)) {
            if (!ap_wlan_hal->sta_softblock_add(
                    vap_name, tlvf::mac_to_string(request->params().client_mac),
                    request->params().config.authRejectReason, request->params().config.snrProbeHWM,
                    request->params().config.snrProbeLWM, request->params().config.snrAuthHWM,
                    request->params().config.snrAuthLWM)) {
                LOG(ERROR) << "sta_softblock_add failed!";
                send_steering_return_status(
                    beerocks_message::ACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE,
                    OPERATION_FAIL);
            }
        } else {
            if (!ap_wlan_hal->sta_softblock_remove(
                    vap_name, tlvf::mac_to_string(request->params().client_mac))) {
                LOG(ERROR) << "sta_softblock_remove failed!";
                send_steering_return_status(
                    beerocks_message::ACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE,
                    OPERATION_FAIL);
            }
        }
        send_steering_return_status(beerocks_message::ACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE,
                                    OPERATION_SUCCESS);

        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_REQUEST: {
        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_VAPS_LIST_UPDATE_NOTIFICATION>(cmdu_tx);
        if (notification == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return;
        }

        copy_vaps_info(ap_wlan_hal, notification->params().vaps);
        LOG(DEBUG) << "Sending Vap List update to controller";
        if (!send_cmdu(cmdu_tx)) {
            LOG(ERROR) << "Failed sending cmdu!";
            return;
        }

        break;
    }
    case beerocks_message::
        ACTION_APMANAGER_HOSTAP_GENERATE_CLIENT_ASSOCIATION_NOTIFICATIONS_REQUEST: {
        m_generate_connected_clients_events = true;
        if (!ap_wlan_hal->pre_generate_connected_clients_events()) {
            LOG(WARNING) << "Failed to prepare for generate of clients connected events";
        }
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST: {
        LOG(TRACE) << "cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST";

        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST>();
        if (!notification) {
            LOG(ERROR)
                << "addClass cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST failed";
            return;
        }

        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE>(cmdu_tx);

        if (!response) {
            LOG(ERROR) << "Failed building message!";
            return;
        }

        response->success() = true;

        if (notification->ant_switch_on()) {
            // Enable zwdfs radio antenna
            if (ap_wlan_hal->is_zwdfs_antenna_enabled()) {
                LOG(WARNING) << "trying to switch on zwdfs antenna but its already on!";
            } else if (!ap_wlan_hal->set_zwdfs_antenna(true)) {
                LOG(ERROR) << "set_zwdfs_antenna on failed!";
                response->success() = false;
            }
            // switch channel on zwdfs interface to start off channel CAC
            LOG(DEBUG) << "Switching channel channel=" << notification->channel()
                       << ", bw=" << utils::convert_bandwidth_to_int(notification->bandwidth())
                       << ", center_freq=" << notification->center_frequency();

            if (!ap_wlan_hal->switch_channel(notification->channel(), notification->bandwidth(),
                                             notification->center_frequency())) {
                LOG(ERROR) << "switch_channel failed!";
                response->success() = false;
            }
        } else {
            // Disable zwdfs radio antenna
            if (!ap_wlan_hal->is_zwdfs_antenna_enabled()) {
                LOG(WARNING) << "trying to switch off zwdfs antenna but its already off!";
            } else if (!ap_wlan_hal->set_zwdfs_antenna(false)) {
                LOG(ERROR) << "set_zwdfs_antenna off failed!";
                response->success() = false;
            }
        }

        LOG(INFO) << "send cACTION_APMANAGER_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE, success="
                  << int(response->success());
        send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_HOSTAP_SET_PRIMARY_VLAN_ID_REQUEST: {
        auto request = beerocks_header->addClass<
            beerocks_message::cACTION_APMANAGER_HOSTAP_SET_PRIMARY_VLAN_ID_REQUEST>();
        if (!request) {
            LOG(ERROR) << "addClass has failed";
            return;
        }
        ap_wlan_hal->set_primary_vlan_id(request->primary_vlan_id());
        break;
    }
    default: {
        LOG(ERROR) << "Unsupported header action_op: " << int(beerocks_header->action_op());
        break;
    }
    }
}

void ApManager::fill_cs_params(beerocks_message::sApChannelSwitch &params)
{
    params.tx_power  = static_cast<int8_t>(ap_wlan_hal->get_radio_info().tx_power);
    params.channel   = ap_wlan_hal->get_radio_info().channel;
    params.bandwidth = uint8_t(
        beerocks::utils::convert_bandwidth_to_enum(ap_wlan_hal->get_radio_info().bandwidth));
    params.channel_ext_above_primary = ap_wlan_hal->get_radio_info().channel_ext_above;
    params.vht_center_frequency      = ap_wlan_hal->get_radio_info().vht_center_freq;
    params.switch_reason             = uint8_t(ap_wlan_hal->get_radio_info().last_csa_sw_reason);
    params.is_dfs_channel            = ap_wlan_hal->get_radio_info().is_dfs_channel;
}

bool ApManager::hal_event_handler(bwl::base_wlan_hal::hal_event_ptr_t event_ptr)
{
    if (!event_ptr) {
        LOG(ERROR) << "Invalid event!";
        return false;
    }

    if (!m_slave_client) {
        LOG(ERROR) << "Not connected to slave!";
        return false;
    }

    // AP Event & Data
    typedef bwl::ap_wlan_hal::Event Event;
    auto event = (Event)(event_ptr->first);
    auto data  = event_ptr->second.get();

    switch (event) {

    case Event::AP_Attached: {
        handle_hostapd_attached();
    } break;

    case Event::AP_Enabled: {
        if (!data) {
            LOG(ERROR) << "AP_Enabled without data!";
            return false;
        }

        auto msg = static_cast<bwl::sHOSTAP_ENABLED_NOTIFICATION *>(data);
        handle_ap_enabled(msg->vap_id);

    } break;

    // ACS/CSA Completed
    case Event::ACS_Completed:
    case Event::CSA_Finished: {

        ap_wlan_hal->read_acs_report();
        ap_wlan_hal->refresh_radio_info();

        LOG(INFO) << ((event == Event::ACS_Completed) ? "ACS_Completed" : "CSA_Finished:")
                  << " channel = " << int(ap_wlan_hal->get_radio_info().channel)
                  << " bandwidth = " << ap_wlan_hal->get_radio_info().bandwidth
                  << " channel_ext_above_primary = "
                  << int(ap_wlan_hal->get_radio_info().channel_ext_above)
                  << " vht_center_frequency = "
                  << int(ap_wlan_hal->get_radio_info().vht_center_freq)
                  << " last_csa_switch_reason enum = "
                  << int(ap_wlan_hal->get_radio_info().last_csa_sw_reason);

        if (event == Event::ACS_Completed) {
            auto notification = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_HOSTAP_ACS_NOTIFICATION>(cmdu_tx);
            if (notification == nullptr) {
                LOG(ERROR) << "Failed building message!";
                return false;
            }
            fill_cs_params(notification->cs_params());
            acs_completed_vap_update = true;
        } else {
            auto notification = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_HOSTAP_CSA_NOTIFICATION>(cmdu_tx);
            if (notification == nullptr) {
                LOG(ERROR) << "Failed building message!";
                return false;
            }
            fill_cs_params(notification->cs_params());
        }

        send_cmdu(cmdu_tx);

    } break;

    // STA Connected
    case Event::STA_Connected: {

        if (!data) {
            LOG(ERROR) << "STA_Connected without data!";
            return false;
        }

        auto msg = static_cast<bwl::sACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION *>(data);
        std::string client_mac = tlvf::mac_to_string(msg->params.mac);

        LOG(INFO) << "STA_Connected mac = " << client_mac;

        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CLIENT_ASSOCIATED_NOTIFICATION>(cmdu_tx);
        if (notification == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        auto vap_node = ap_wlan_hal->get_radio_info().available_vaps.find(msg->params.vap_id);
        if (vap_node == ap_wlan_hal->get_radio_info().available_vaps.end()) {
            LOG(ERROR) << "Can't find vap with id " << int(msg->params.vap_id);
            return false;
        }

        notification->mac()          = msg->params.mac;
        notification->vap_id()       = msg->params.vap_id;
        notification->bssid()        = tlvf::mac_from_string(vap_node->second.mac);
        notification->capabilities() = msg->params.capabilities;
        if (msg->params.association_frame_length == 0) {
            LOG(DEBUG) << "no association frame";
        } else {
            notification->set_association_frame(msg->params.association_frame,
                                                msg->params.association_frame_length);
        }

        notification->multi_ap_profile() = msg->params.multi_ap_profile;

        send_cmdu(cmdu_tx);
    } break;

    // STA Disconnected
    case Event::STA_Disconnected: {

        if (!data) {
            LOG(ERROR) << "STA_Disconnected without data!";
            return false;
        }

        auto msg = static_cast<bwl::sACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION *>(data);
        std::string mac = tlvf::mac_to_string(msg->params.mac);
        LOG(INFO) << "STA_Disconnected client " << mac;

        auto it = std::find_if(
            pending_disable_vaps.begin(), pending_disable_vaps.end(),
            [&](pending_disable_vap_t vap) { return (vap.vap_id == msg->params.vap_id); });

        if (it == pending_disable_vaps.end()) {
            pending_disable_vaps.push_back(std::move((pending_disable_vap_t){
                .vap_id  = msg->params.vap_id,
                .timeout = std::chrono::steady_clock::now() +
                           std::chrono::seconds(DISABLE_BACKHAUL_VAP_TIMEOUT_SEC),

            }));
        }

        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CLIENT_DISCONNECTED_NOTIFICATION>(cmdu_tx);

        if (notification == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }
        auto vap_node = ap_wlan_hal->get_radio_info().available_vaps.find(msg->params.vap_id);
        if (vap_node == ap_wlan_hal->get_radio_info().available_vaps.end()) {
            LOG(ERROR) << "Can't find vap with id " << int(msg->params.vap_id);
            return false;
        }

        notification->params().mac    = msg->params.mac;
        notification->params().bssid  = tlvf::mac_from_string(vap_node->second.mac);
        notification->params().vap_id = msg->params.vap_id;
        notification->params().reason = msg->params.reason;
        notification->params().source = msg->params.source;
        notification->params().type   = msg->params.type;

        send_cmdu(cmdu_tx);
    } break;

    // BSS Transition (802.11v)
    case Event::BSS_TM_Response: {

        if (!data) {
            LOG(ERROR) << "BSS_TM_Response without data!";
            return false;
        }

        //TODO EasyMesh SteeringBTMReport should contain source BSSID and target BSSID
        auto msg = static_cast<bwl::sACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE *>(data);
        LOG(INFO) << "BSS_STEER_RESPONSE client " << msg->params.mac
                  << " status_code=" << int(msg->params.status_code);

        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CLIENT_BSS_STEER_RESPONSE>(cmdu_tx);

        if (response == nullptr) {
            LOG(ERROR) << "Failed building message!";
            return false;
        }

        response->params().mac          = msg->params.mac;
        response->params().status_code  = msg->params.status_code;
        response->params().source_bssid = msg->params.source_bssid;
        // TODO: add the optional target BSSID

        send_cmdu(cmdu_tx);

    } break;

    // Unassociated STA Measurement
    case Event::STA_Unassoc_RSSI: {

        if (!data) {
            LOG(ERROR) << "STA_Unassoc_RSSI without data!";
            return false;
        }

        auto msg = static_cast<bwl::sACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE *>(data);

        LOG(INFO) << "CLIENT_RX_RSSI_MEASUREMENT_RESPONSE for mac " << msg->params.result.mac
                  << " id=" << sta_unassociated_rssi_measurement_header_id;

        if (sta_unassociated_rssi_measurement_header_id > -1) {
            // Build the response message
            auto response = message_com::create_vs_message<
                beerocks_message::cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE>(
                cmdu_tx, sta_unassociated_rssi_measurement_header_id);
            if (response == nullptr) {
                LOG(ERROR) << "Failed building "
                              "cACTION_APMANAGER_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE message!";
                break;
            }

            response->params().result.mac        = msg->params.result.mac;
            response->params().rx_phy_rate_100kb = msg->params.rx_phy_rate_100kb;
            response->params().tx_phy_rate_100kb = msg->params.tx_phy_rate_100kb;
            response->params().rx_rssi           = msg->params.rx_rssi;
            response->params().rx_packets        = msg->params.rx_packets;
            response->params().src_module        = msg->params.src_module;

            send_cmdu(cmdu_tx);
        } else {
            LOG(ERROR) << "sta_unassociated_rssi_measurement_header_id == -1";
            return false;
        }

        // Clear the ID
        sta_unassociated_rssi_measurement_header_id = -1;

    } break;

    // STA Softblock Message Dropped
    case Event::STA_Steering_Probe_Req: {

        if (!data) {
            LOG(ERROR) << "STA_Steering_Probe_Req without data!";
            return false;
        }

        auto msg =
            static_cast<bwl::sACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION *>(data);

        LOG(INFO) << "CLIENT_SOFTBLOCK_NOTIFICATION for client mac " << msg->params.client_mac;

        // Build the response message
        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION>(cmdu_tx);
        if (notification == nullptr) {
            LOG(ERROR) << "Failed building cACTION_APMANAGER_STEERING_EVENT_PROBE_REQ_NOTIFICATION "
                          "message!";
            break;
        }

        notification->params().client_mac = msg->params.client_mac;
        notification->params().bssid      = msg->params.bssid;
        notification->params().rx_snr     = msg->params.rx_snr;
        notification->params().blocked    = msg->params.blocked;
        notification->params().broadcast  = msg->params.broadcast;

        send_cmdu(cmdu_tx);

    } break;

    case Event::STA_Steering_Auth_Fail: {

        if (!data) {
            LOG(ERROR) << "STA_Steering_Auth_Fail without data!";
            return false;
        }

        auto msg =
            static_cast<bwl::sACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION *>(data);

        LOG(INFO) << "CLIENT_SOFTBLOCK_NOTIFICATION for client mac " << msg->params.client_mac;

        // Build the response message
        auto notification = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION>(cmdu_tx);
        if (notification == nullptr) {
            LOG(ERROR) << "Failed building cACTION_APMANAGER_STEERING_EVENT_AUTH_FAIL_NOTIFICATION "
                          "message!";
            break;
        }

        notification->params().client_mac = msg->params.client_mac;
        notification->params().bssid      = msg->params.bssid;
        notification->params().rx_snr     = msg->params.rx_snr;
        notification->params().blocked    = msg->params.blocked;
        notification->params().reject     = msg->params.reject;
        notification->params().reason     = msg->params.reason;

        send_cmdu(cmdu_tx);

    } break;

    case Event::DFS_CAC_Started: {

        if (!data) {
            LOG(ERROR) << "DFS_CAC_Started without data!";
            return false;
        }

        auto msg = static_cast<bwl::sACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION *>(data);
        LOG(INFO) << "DFS_EVENT_CAC_STARTED channel: " << msg->params.channel
                  << " secondary_channel: " << msg->params.secondary_channel
                  << " bandwidth: " << msg->params.bandwidth
                  << " cac_duration_sec: " << msg->params.cac_duration_sec;

        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION>(cmdu_tx);
        if (!response) {
            LOG(ERROR) << "Failed building cACTION_APMANAGER_HOSTAP_DFS_CAC_STARTED_NOTIFICATION "
                          "message!";
            break;
        }

        response->params().channel           = msg->params.channel;
        response->params().secondary_channel = msg->params.secondary_channel;
        response->params().bandwidth         = msg->params.bandwidth;
        response->params().cac_duration_sec  = msg->params.cac_duration_sec;

        send_cmdu(cmdu_tx);

    } break;

    // DFS CAC Completed
    case Event::DFS_CAC_Completed: {

        if (!data) {
            LOG(ERROR) << "DFS_CAC_Completed without data!";
            return false;
        }

        auto msg =
            static_cast<bwl::sACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION *>(data);
        LOG(INFO) << "DFS_EVENT_CAC_COMPLETED succsess = " << int(msg->params.success);

        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION>(cmdu_tx);
        if (response == nullptr) {
            LOG(ERROR) << "Failed building ACTION_APMANAGER_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION "
                          "message!";
            break;
        }

        response->params().timeout           = msg->params.timeout;
        response->params().frequency         = msg->params.frequency;
        response->params().center_frequency1 = msg->params.center_frequency1;
        response->params().center_frequency2 = msg->params.center_frequency2;
        response->params().success           = msg->params.success;
        response->params().channel           = msg->params.channel;
        response->params().bandwidth         = msg->params.bandwidth;

        send_cmdu(cmdu_tx);

    } break;

    // DFS NOP Finished
    case Event::DFS_NOP_Finished: {

        if (!data) {
            LOG(ERROR) << "DFS_CAC_Completed without data!";
            return false;
        }

        auto msg =
            static_cast<bwl::sACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION *>(data);
        LOG(INFO) << "DFS_EVENT_NOP_FINISHED "
                  << " channel = " << int(msg->params.channel) << " bw = "
                  << beerocks::utils::convert_bandwidth_to_int(
                         (beerocks::eWiFiBandwidth)msg->params.bandwidth)
                  << " vht_center_frequency = " << int(msg->params.vht_center_frequency);

        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION>(cmdu_tx);
        if (response == nullptr) {
            LOG(ERROR) << "Failed building "
                          "ACTION_APMANAGER_HOSTAP_DFS_CHANNEL_AVAILABLE_NOTIFICATION message!";
            break;
        }

        response->params().frequency            = msg->params.frequency;
        response->params().channel              = msg->params.channel;
        response->params().bandwidth            = msg->params.bandwidth;
        response->params().vht_center_frequency = msg->params.vht_center_frequency;

        send_cmdu(cmdu_tx);

    } break;

    // AP/Interface Disabled
    case Event::AP_Disabled: {
        if (!data) {
            LOG(ERROR) << "AP_Disabled without data!";
            return false;
        }

        auto msg = static_cast<bwl::sHOSTAP_DISABLED_NOTIFICATION *>(data);
        LOG(INFO) << "AP_Disabled on vap_id = " << int(msg->vap_id);

        if (msg->vap_id == beerocks::IFACE_RADIO_ID) {
            auto timeout = std::chrono::steady_clock::now() +
                           std::chrono::seconds(MAX_RADIO_DISABLED_TIMEOUT_SEC);
            auto notify_disabled = true;

            while (std::chrono::steady_clock::now() < timeout) {
                if (!ap_wlan_hal->refresh_radio_info()) {
                    LOG(WARNING) << "Radio could be temporary disabled, wait grace time "
                                 << std::chrono::duration_cast<std::chrono::seconds>(
                                        timeout - std::chrono::steady_clock::now())
                                        .count()
                                 << " sec.";
                    UTILS_SLEEP_MSEC(500);
                    continue;
                }

                auto state = ap_wlan_hal->get_radio_info().radio_state;
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
                beerocks_message::cACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION>(cmdu_tx);
            if (response == nullptr) {
                LOG(ERROR)
                    << "Failed building cACTION_APMANAGER_HOSTAP_AP_DISABLED_NOTIFICATION message!";
                break;
            }

            response->vap_id() = msg->vap_id;

            send_cmdu(cmdu_tx);
        }
    } break;
    case Event::Interface_Disabled: {

        LOG(ERROR) << "Interface_Disabled event!";
        m_state = eApManagerState::TERMINATED;

    } break;

    case Event::ACS_Failed: {
        LOG(INFO) << "ACS_Failed event!";

    } break;
    case Event::MGMT_Frame: {
        if (!data) {
            LOG(ERROR) << "MGMT_Frame without data!";
            // That's indeed an error, but no reason to terminate the AP Manager in this case.
            // Return "true" to ignore the event and continue operating.
            return true;
        }

        auto mgmt_frame = static_cast<bwl::sMGMT_FRAME_NOTIFICATION *>(data);

        // Convert the BWL type to a tunnelled message type
        wfa_map::tlvTunnelledProtocolType::eTunnelledProtocolType tunnelled_proto_type;
        switch (mgmt_frame->type) {
        case bwl::eManagementFrameType::ASSOCIATION_REQUEST: {
            tunnelled_proto_type =
                wfa_map::tlvTunnelledProtocolType::eTunnelledProtocolType::ASSOCIATION_REQUEST;
        } break;
        case bwl::eManagementFrameType::REASSOCIATION_REQUEST: {
            tunnelled_proto_type =
                wfa_map::tlvTunnelledProtocolType::eTunnelledProtocolType::REASSOCIATION_REQUEST;
        } break;
        case bwl::eManagementFrameType::BTM_QUERY: {
            tunnelled_proto_type =
                wfa_map::tlvTunnelledProtocolType::eTunnelledProtocolType::BTM_QUERY;
        } break;
        case bwl::eManagementFrameType::WNM_REQUEST: {
            tunnelled_proto_type =
                wfa_map::tlvTunnelledProtocolType::eTunnelledProtocolType::WNM_REQUEST;
        } break;
        case bwl::eManagementFrameType::ANQP_REQUEST: {
            tunnelled_proto_type =
                wfa_map::tlvTunnelledProtocolType::eTunnelledProtocolType::ANQP_REQUEST;
        } break;
        default: {
            LOG(DEBUG) << "Unsupported 802.11 management frame: " << std::hex
                       << int(mgmt_frame->type);

            // Not supporting a specific frame is not really an error, so just stop processing
            return true;
        }
        }

        LOG(DEBUG) << "Processing management frame from " << mgmt_frame->mac
                   << ", of type: " << std::hex << int(mgmt_frame->type)
                   << " (tunnelled: " << int(tunnelled_proto_type) << ")"
                   << ", data length: " << std::dec << mgmt_frame->data.size();

        // Create a tunnelled message
        auto cmdu_tx_header = cmdu_tx.create(0, ieee1905_1::eMessageType::TUNNELLED_MESSAGE);
        if (!cmdu_tx_header) {
            LOG(ERROR) << "cmdu creation of type TUNNELLED_MESSAGE failed!";
            return false;
        }

        // Add the Source Info TLV
        auto source_info_tlv = cmdu_tx.addClass<wfa_map::tlvTunnelledSourceInfo>();
        if (!source_info_tlv) {
            LOG(ERROR) << "addClass ieee1905_1::tlvTunnelledSourceInfo failed!";
            return false;
        }

        // Store the MAC address of the transmitting station
        source_info_tlv->mac() = mgmt_frame->mac;

        // Add the Type TLV
        auto type_tlv = cmdu_tx.addClass<wfa_map::tlvTunnelledProtocolType>();
        if (!type_tlv) {
            LOG(ERROR) << "addClass ieee1905_1::tlvTunnelledProtocolType failed!";
            return false;
        }

        // Store the tunnelled message type and length
        type_tlv->protocol_type() = tunnelled_proto_type;

        // Add the Data TLV
        auto data_tlv = cmdu_tx.addClass<wfa_map::tlvTunnelledData>();
        if (!data_tlv) {
            LOG(ERROR) << "addClass ieee1905_1::tlvTunnelledData failed!";
            return false;
        }

        // Copy the frame body
        if (!data_tlv->set_data(mgmt_frame->data.data(), mgmt_frame->data.size())) {
            LOG(ERROR) << "Failed copying " << mgmt_frame->data.size()
                       << " bytes into the tunnelled message data tlv!";

            return false;
        }

        // Send the tunnelled message
        send_cmdu(cmdu_tx);

    } break;
    case Event::AP_Sta_Possible_Psk_Mismatch: {
        LOG(DEBUG) << "ap manager: Ap STA Possible PSK Mismatch";
        auto mismatch_psk = static_cast<bwl::sSTA_MISMATCH_PSK *>(data);
        // Create a Failed Connection Message
        auto cmdu_tx_header =
            cmdu_tx.create(0, ieee1905_1::eMessageType::FAILED_CONNECTION_MESSAGE);
        if (!cmdu_tx_header) {
            LOG(ERROR) << "cmdu creation of type FAILED_CONNECTION_MESSAGE failed!";
            return false;
        }
        // add STA
        auto sta_mac_tlv = cmdu_tx.addClass<wfa_map::tlvStaMacAddressType>();
        if (!sta_mac_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvStaMacAddressType!";
            return false;
        }
        sta_mac_tlv->sta_mac() = mismatch_psk->sta_mac;
        // add status code
        auto profile2_status_code_tlv = cmdu_tx.addClass<wfa_map::tlvProfile2StatusCode>();
        if (!profile2_status_code_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvProfile2StatusCode!";
            return false;
        }
        // note: at the moment just setting the code to non-zero
        profile2_status_code_tlv->status_code() = 0x0001;
        // add reason code
        // note: no value is set at the moment
        auto profile2_reason_code_tlv = cmdu_tx.addClass<wfa_map::tlvProfile2ReasonCode>();
        if (!profile2_reason_code_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvProfile2ReasonCode!";
            return false;
        }
        // Send the mismatched message
        send_cmdu(cmdu_tx);
    } break;
    // Unhandled events
    default:
        LOG(ERROR) << "Unhandled event: " << int(event);
        break;
    }

    return true;
}

void ApManager::handle_hostapd_attached()
{
    LOG(DEBUG) << "handling enabled hostapd";

    if (acs_enabled) {
        LOG(DEBUG) << "retrieving ACS report";
        int read_acs_attempt = 0;
        while (!ap_wlan_hal->read_acs_report()) {
            read_acs_attempt++;
            if (read_acs_attempt >= READ_ACS_ATTEMPT_MAX) {
                LOG(ERROR) << "retrieving ACS report fails " << int(READ_ACS_ATTEMPT_MAX)
                           << " times - stop ApManager";
                m_state = eApManagerState::TERMINATED;
                break;
            }

            usleep(ACS_READ_SLEEP_USC);
        }
    }

    auto notification =
        message_com::create_vs_message<beerocks_message::cACTION_APMANAGER_JOINED_NOTIFICATION>(
            cmdu_tx);

    if (notification == nullptr) {
        LOG(ERROR) << "Failed building message!";
        return;
    }

    string_utils::copy_string(notification->params().iface_name,
                              ap_wlan_hal->get_iface_name().c_str(), message::IFACE_NAME_LENGTH);

    notification->params().iface_type    = uint8_t(ap_wlan_hal->get_iface_type());
    notification->params().iface_mac     = tlvf::mac_from_string(ap_wlan_hal->get_radio_mac());
    notification->params().iface_is_5ghz = ap_wlan_hal->get_radio_info().is_5ghz;
    notification->params().ant_num       = ap_wlan_hal->get_radio_info().ant_num;
    notification->params().tx_power      = ap_wlan_hal->get_radio_info().tx_power;
    notification->cs_params().channel    = ap_wlan_hal->get_radio_info().channel;
    notification->cs_params().channel_ext_above_primary =
        ap_wlan_hal->get_radio_info().channel_ext_above;
    notification->cs_params().vht_center_frequency = ap_wlan_hal->get_radio_info().vht_center_freq;
    notification->cs_params().bandwidth            = uint8_t(
        beerocks::utils::convert_bandwidth_to_enum(ap_wlan_hal->get_radio_info().bandwidth));

    notification->params().frequency_band = ap_wlan_hal->get_radio_info().frequency_band;
    notification->params().max_bandwidth  = ap_wlan_hal->get_radio_info().max_bandwidth;
    notification->params().ht_supported   = ap_wlan_hal->get_radio_info().ht_supported;
    notification->params().ht_capability  = ap_wlan_hal->get_radio_info().ht_capability;
    std::copy_n(ap_wlan_hal->get_radio_info().ht_mcs_set.data(), beerocks::message::HT_MCS_SET_SIZE,
                notification->params().ht_mcs_set);
    notification->params().vht_supported  = ap_wlan_hal->get_radio_info().vht_supported;
    notification->params().vht_capability = ap_wlan_hal->get_radio_info().vht_capability;
    std::copy_n(ap_wlan_hal->get_radio_info().vht_mcs_set.data(),
                beerocks::message::VHT_MCS_SET_SIZE, notification->params().vht_mcs_set);
    notification->params().he_supported  = ap_wlan_hal->get_radio_info().he_supported;
    notification->params().he_capability = ap_wlan_hal->get_radio_info().he_capability;
    std::copy_n(ap_wlan_hal->get_radio_info().he_mcs_set.data(), beerocks::message::HE_MCS_SET_SIZE,
                notification->params().he_mcs_set);

    notification->params().zwdfs = m_ap_support_zwdfs;

    notification->params().hybrid_mode_supported = ap_wlan_hal->hybrid_mode_supported();

    auto channel_list_class = notification->create_channel_list();
    build_channels_list(cmdu_tx, ap_wlan_hal->get_radio_info().channels_list, channel_list_class);
    notification->add_channel_list(channel_list_class);

    LOG(INFO) << "send ACTION_APMANAGER_JOINED_NOTIFICATION";
    LOG(INFO) << " iface = " << ap_wlan_hal->get_iface_name();
    LOG(INFO) << " mac = " << ap_wlan_hal->get_radio_mac();
    LOG(INFO) << " ant_num = " << ap_wlan_hal->get_radio_info().ant_num;
    LOG(INFO) << " tx_power = " << ap_wlan_hal->get_radio_info().tx_power;
    LOG(INFO) << " current channel = " << ap_wlan_hal->get_radio_info().channel;
    LOG(INFO) << " vht_center_frequency = " << ap_wlan_hal->get_radio_info().vht_center_freq;
    LOG(INFO) << " current bw = " << ap_wlan_hal->get_radio_info().bandwidth;
    LOG(INFO) << " frequency_band = " << ap_wlan_hal->get_radio_info().frequency_band;
    LOG(INFO) << " max_bandwidth = " << ap_wlan_hal->get_radio_info().max_bandwidth;
    LOG(INFO) << " ht_supported = " << ap_wlan_hal->get_radio_info().ht_supported;
    LOG(INFO) << " ht_capability = " << std::hex << ap_wlan_hal->get_radio_info().ht_capability;
    LOG(INFO) << " vht_supported = " << ap_wlan_hal->get_radio_info().vht_supported;
    LOG(INFO) << " vht_capability = " << std::hex << ap_wlan_hal->get_radio_info().vht_capability;
    LOG(INFO) << " he_supported = " << ap_wlan_hal->get_radio_info().he_supported;
    LOG(INFO) << " he_capability = " << std::hex << ap_wlan_hal->get_radio_info().he_capability;
    LOG(INFO) << " zwdfs = " << m_ap_support_zwdfs;

    copy_vaps_info(ap_wlan_hal, notification->vap_list().vaps);

    // Send CMDU
    send_cmdu(cmdu_tx);
}

void ApManager::send_heartbeat()
{
    //LOG(DEBUG) << "sending HEARTBEAT notification";
    auto request =
        message_com::create_vs_message<beerocks_message::cACTION_APMANAGER_HEARTBEAT_NOTIFICATION>(
            cmdu_tx);

    if (request == nullptr) {
        LOG(ERROR) << "Failed building cACTION_APMANAGER_HEARTBEAT_NOTIFICATION message!";
        return;
    }

    send_cmdu(cmdu_tx);
}

bool ApManager::handle_ap_enabled(int vap_id)
{
    LOG(INFO) << "AP_Enabled on vap_id = " << int(vap_id);

    if (!ap_wlan_hal->refresh_vaps_info(vap_id)) {
        LOG(ERROR) << "Failed updating vap info!!!";
    }

    auto vap_iter = ap_wlan_hal->get_radio_info().available_vaps.find(vap_id);
    if (vap_iter == ap_wlan_hal->get_radio_info().available_vaps.end()) {
        LOG(ERROR) << "Received AP-ENABLED but can't get vap info";
        return false;
    }

    const auto vap_info = vap_iter->second;

    LOG(INFO) << "vap_id = " << int(vap_id) << ", bssid = " << vap_info.mac
              << ", ssid = " << vap_info.ssid << ", fronthaul = " << vap_info.fronthaul
              << ", backhaul = " << vap_info.backhaul;

    if (vap_info.backhaul) {
        LOG(DEBUG) << "disallow_profile1=" << vap_info.profile1_backhaul_sta_association_disallowed
                   << ", disallow_profile2="
                   << vap_info.profile2_backhaul_sta_association_disallowed;
    }

    auto notification = message_com::create_vs_message<
        beerocks_message::cACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION>(cmdu_tx);
    if (!notification) {
        LOG(ERROR) << "Failed building cACTION_APMANAGER_HOSTAP_AP_ENABLED_NOTIFICATION message!";
        return false;
    }

    notification->vap_id() = vap_id;

    // Copy the VAP MAC and SSID
    notification->vap_info().mac = tlvf::mac_from_string(vap_info.mac);
    string_utils::copy_string(notification->vap_info().ssid, vap_info.ssid.c_str(),
                              beerocks::message::WIFI_SSID_MAX_LENGTH);
    notification->vap_info().fronthaul_vap = vap_info.fronthaul;
    notification->vap_info().backhaul_vap  = vap_info.backhaul;

    notification->vap_info().profile1_backhaul_sta_association_disallowed =
        vap_info.profile1_backhaul_sta_association_disallowed;
    notification->vap_info().profile2_backhaul_sta_association_disallowed =
        vap_info.profile2_backhaul_sta_association_disallowed;

    send_cmdu(cmdu_tx);

    return true;
}

void ApManager::send_steering_return_status(beerocks_message::eActionOp_APMANAGER ActionOp,
                                            int32_t status)
{
    switch (ActionOp) {
    case beerocks_message::ACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE: {
        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_CLIENT_DISCONNECT_RESPONSE>(cmdu_tx);
        if (response == nullptr) {
            LOG(ERROR) << "Failed building message!";
            break;
        }
        response->params().error_code = status;
        send_cmdu(cmdu_tx);
        break;
    }
    case beerocks_message::ACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE: {
        auto response = message_com::create_vs_message<
            beerocks_message::cACTION_APMANAGER_STEERING_CLIENT_SET_RESPONSE>(cmdu_tx);
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

void ApManager::remove_client_from_disallowed_list(const sMacAddr &mac, const sMacAddr &bssid)
{
    auto it = std::find_if(m_disallowed_clients.begin(), m_disallowed_clients.end(),
                           [&](const son::ApManager::disallowed_client_t &element) {
                               return ((element.mac == mac) && (element.bssid == bssid));
                           });

    if (it != m_disallowed_clients.end()) {
        // remove client from the disallow list
        it = m_disallowed_clients.erase(it);
    }
}

void ApManager::allow_expired_clients()
{
    // check if any client disallow period has expired and allow it.
    for (auto it = m_disallowed_clients.begin(); it != m_disallowed_clients.end();) {
        if (std::chrono::steady_clock::now() > it->timeout) {
            LOG(DEBUG) << "CLIENT_ALLOW: mac = " << it->mac << ", bssid = " << it->bssid;
            ap_wlan_hal->sta_allow(tlvf::mac_to_string(it->mac), tlvf::mac_to_string(it->bssid));
            it = m_disallowed_clients.erase(it);
        } else {
            it++;
        }
    }
}

bool ApManager::zwdfs_ap() const
{
    if (m_state != eApManagerState::OPERATIONAL) {
        LOG(WARNING) << "Requested ZWDFS support status, but AP is not attached to BWL";
        return true;
    }

    return m_ap_support_zwdfs;
}
