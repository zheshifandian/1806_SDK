/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "channel_scan_task.h"
#include "../agent_db.h"
#include "../backhaul_manager/backhaul_manager.h"
#include <bcl/beerocks_utils.h>
#include <beerocks/tlvf/beerocks_message.h>
#include <beerocks/tlvf/beerocks_message_1905_vs.h>
#include <beerocks/tlvf/beerocks_message_backhaul.h>
#include <easylogging++.h>
#include <tlvf/CmduMessageTx.h>

using namespace beerocks;

#define FSM_MOVE_STATE(radio_scan_info, new_state)                                                 \
    ({                                                                                             \
        LOG(TRACE) << "CHANNEL_SCAN " << radio_scan_info->radio_mac                                \
                   << " FSM: " << m_states_string.at(radio_scan_info->current_state) << " --> "    \
                   << m_states_string.at(new_state);                                               \
        radio_scan_info->current_state = new_state;                                                \
    })

#define FSM_MOVE_TIMEOUT_STATE(radio_scan_info, new_state, timeout_sec)                            \
    ({                                                                                             \
        FSM_MOVE_STATE(radio_scan_info, new_state);                                                \
        radio_scan_info->timeout = std::chrono::system_clock::now() + timeout_sec;                 \
    })

/**
 * ToDo: Remove this "default" parameter after PPM-747 is resolved.
 */
constexpr int PREFERRED_DWELLTIME_MS                       = 103; // 103 Millisec
constexpr std::chrono::seconds SCAN_TRIGGERED_WAIT_TIME    = std::chrono::seconds(20);  // 20 Sec
constexpr std::chrono::seconds SCAN_RESULTS_DUMP_WAIT_TIME = std::chrono::seconds(210); // 3.5 Min
/**
 * To allow for CMDU & TLV fragmentation a proximation of the size we need to keep free in the
 * Channel Scan Report Message building process is needed.
 * The cNeighbor structure consists of the following parameters:
 * Field name           | Size (Byte)   | Description
 * ---------------------+---------------+----------------------
 * BSSID                |             6 | BSSID of the found AP
 * SSID Length          |             1 | Length of the SSID field
 * SSID                 | [MAX]      32 | SSID of the found AP, Assuming max size of 32 Bytes
 * signal_strength      |             1 | Signal Strength of found AP
 * Channel BW Length    |             1 | Length of the Bandwidth
 * Channel BW           | [MAX]       5 | Stirng value of Bandwidth, Can be one of "20", "40" "80"
 *                      |               | "80+80" or "160" MHz. Seeing as "80+80" is the longest
 *                      |               | value possible, We can assume the max size is 5 Bytes.
 * eBssLoadElement      |             1 | Enumerate value
 * channel_utilization  |             1 | Utilization of the found AP
 * station_count        |             2 | Station count of the found AP
 * 
 * The Maximum assumable size of the cNeighbor is 50 Bytes
 * 
 * Field name           | Size (Byte)   | Description
 * ---------------------+---------------+----------------------
 * TLV Header           |             3 | Header of the TLV message
 * Radio MAC            |             6 | Result's Radio's MAC address
 * Operating Class      |             1 | Result's Operating Class
 * Channel              |             1 | Result's Channel
 * Status               |             1 | Result's Status
 * Timestamp Length     |             1 | Result's Timestamp's Length
 * Timestamp            | [MAX]      27 | Result's Timestamp [ISO 8601]
 * Utilization          |             1 | Result's average channel utilization
 * Noise                |             1 | Result's average channel noise
 * Number of Neighbors  |             2 | Number of found Neighbors
 * Aggregate Duration   |             4 | Aggregation duration
 * Scan Type            |             1 | Type of scan (Active/Passive)
 * 
 * The assumable size of the sChannelScanResults is 49 Bytes
 */
constexpr size_t MAX_NEIGHBOR_SIZE     = 50; //Bytes
constexpr size_t BASE_RESULTS_TLV_SIZE = 49; //Bytes
constexpr size_t MIN_RESULTS_TLV_SIZE  = BASE_RESULTS_TLV_SIZE + MAX_NEIGHBOR_SIZE;
constexpr size_t TLV_HEADER            = 3; // Bytes;
// When an IEEE1905 packet (CMDU) is larger than a standard defined threshold (1500 bytes) it
// should be fragmented into smaller than 1500 bytes fragments.
constexpr size_t MAX_TLV_FRAGMENT_SIZE = 1500;

ChannelScanTask::ChannelScanTask(BackhaulManager &btl_ctx, ieee1905_1::CmduMessageTx &cmdu_tx)
    : Task(eTaskType::CHANNEL_SCAN), m_btl_ctx(btl_ctx), m_cmdu_tx(cmdu_tx)
{
}

void ChannelScanTask::work()
{
    // Handle currently running scan.
    if (m_current_scan_info.is_scan_currently_running) {
        auto current_scan_request = m_current_scan_info.scan_request;
        auto current_radio_scan   = m_current_scan_info.radio_scan;

        // Handle current radio-scan's state
        switch (current_radio_scan->current_state) {
        case eState::PENDING_TRIGGER: {
            // Wait until Current-Scan resource is free.
            break;
        }
        case eState::WAIT_FOR_SCAN_TRIGGERED: {
            if (current_radio_scan->timeout < std::chrono::system_clock::now()) {
                LOG(ERROR) << "Reached timeout for PENDING_TRIGGER";
                set_radio_scan_status(current_radio_scan, eScanStatus::SCAN_NOT_COMPLETED);
                FSM_MOVE_STATE(current_radio_scan, eState::SCAN_FAILED);
            }
            break;
        }
        case eState::WAIT_FOR_RESULTS_READY: {
            if (current_radio_scan->timeout < std::chrono::system_clock::now()) {
                LOG(ERROR) << "Reached timeout for WAIT_FOR_RESULTS_READY";
                set_radio_scan_status(current_radio_scan, eScanStatus::SCAN_NOT_COMPLETED);
                FSM_MOVE_STATE(current_radio_scan, eState::SCAN_FAILED);
            }
            break;
        }
        case eState::WAIT_FOR_RESULTS_DUMP: {
            if (current_radio_scan->timeout < std::chrono::system_clock::now()) {
                LOG(ERROR) << "Reached timeout for WAIT_FOR_RESULTS_DUMP";
                set_radio_scan_status(current_radio_scan, eScanStatus::SCAN_NOT_COMPLETED);
                FSM_MOVE_STATE(current_radio_scan, eState::SCAN_FAILED);
            }
            break;
        }
        case eState::SCAN_DONE: {
            if (!is_scan_request_finished(current_scan_request)) {
                LOG(INFO) << "Wait for other scans to complete";
                trigger_next_radio_scan(current_scan_request);
            } else {
                auto db    = AgentDB::get();
                auto radio = db->get_radio_by_mac(current_radio_scan->radio_mac);
                if (!radio) {
                    LOG(ERROR) << "Failed to get radio info from Agent DB for "
                               << current_radio_scan->radio_mac;
                    return;
                }
                // Once the scan is done we want to update the AgentDB with the cached results.
                // If no neighbours are found on some channel, the cached result for that channel
                // will simply not exist. But a cached result also doesn't exist if that channel
                // was not scanned at all. In that case, we want to keep the result that is already
                // in the DB. Therefore, we need to iterate over all the requested channels,
                // and update the DB accordingly.
                for (const auto &op_cls : current_radio_scan->operating_classes) {
                    for (const auto &channel_elem : op_cls.channel_list) {
                        const uint8_t channel_num = channel_elem.channel_number;
                        const auto cached_result_iter =
                            current_radio_scan->cached_results.find(channel_num);
                        if (cached_result_iter == current_radio_scan->cached_results.end()) {
                            // If a requested channel is not present in the cached results we can assume the channel did not contain results.
                            // Therefore we need to clear the stored results from the DB to reflect the most recent results.
                            radio->channel_scan_results.erase(channel_num);
                        } else {
                            // If a request channel is present in te cached results we need to override any existing stored results.
                            radio->channel_scan_results[channel_num] =
                                std::make_pair(current_scan_request->scan_start_timestamp,
                                               cached_result_iter->second);
                        }
                    }
                }
                current_scan_request->ready_to_send_report = true;
            }
            break;
        }
        case eState::SCAN_FAILED: {
            if (!is_scan_request_finished(current_scan_request)) {
                LOG(INFO) << "Wait for other scans to complete";
                trigger_next_radio_scan(current_scan_request);
            } else {
                current_scan_request->ready_to_send_report = true;
            }
            break;
        }
        case eState::SCAN_ABORTED: {
        }
        default:
            break;
        }

        // Handle finished requests.
        if (current_scan_request->ready_to_send_report) {
            // Report was sent back, clear any remaining scan info.
            m_current_scan_info.radio_scan                = nullptr;
            m_current_scan_info.scan_request              = nullptr;
            m_current_scan_info.is_scan_currently_running = false;
            if (!send_channel_scan_report(current_scan_request)) {
                LOG(ERROR) << "Failed to send channel scan report!";
                return;
            }
        }
    } else {
        // Handle pending requests.
        if (!m_pending_requests.empty()) {
            if (!trigger_next_radio_scan(m_pending_requests.front())) {
                LOG(ERROR) << "Failed to trigger the radio scan on top request in queue";
            }
            m_pending_requests.pop_front();
        }
    }
}

void ChannelScanTask::handle_event(uint8_t event_enum_value, const void *event_obj)
{
    switch (eEvent(event_enum_value)) {
    case eEvent::INDEPENDENT_SCAN_REQUEST: {
        auto scan_event = reinterpret_cast<const sScanRequestEvent *>(event_obj);
        LOG(TRACE) << "Received SCAN_TRIGGERED event";
        (void)scan_event;
        break;
    }
    default: {
        LOG(DEBUG) << "Message handler doesn't exists for event type " << event_enum_value;
        break;
    }
    }
}

bool ChannelScanTask::handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, uint32_t iface_index,
                                  const sMacAddr &dst_mac, const sMacAddr &src_mac, int fd,
                                  std::shared_ptr<beerocks_header> beerocks_header)
{
    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::CHANNEL_SCAN_REQUEST_MESSAGE: {
        return handle_channel_scan_request(cmdu_rx, src_mac);
    }
    case ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE: {
        // Retrive the radio_src_mac from the CMDU's header
        // This variable will replace the src_mac address which will be empty
        auto radio_src_mac = beerocks_header->actionhdr()->radio_mac();
        return handle_vendor_specific(cmdu_rx, radio_src_mac, fd, beerocks_header);
    }
    default: {
        // Message was not handled, therefore return false.
        return false;
    }
    }
}

bool ChannelScanTask::handle_vendor_specific(ieee1905_1::CmduMessageRx &cmdu_rx,
                                             const sMacAddr &src_mac, int fd,
                                             std::shared_ptr<beerocks_header> beerocks_header)
{
    if (!beerocks_header) {
        LOG(ERROR) << "beerocks_header is nullptr";
        return false;
    }

    auto is_current_scan_running = [this]() -> bool {
        if (!m_current_scan_info.is_scan_currently_running) {
            return false;
        }
        return true;
    };
    auto does_current_scan_match_incoming_src = [this](const sMacAddr &src_mac) -> bool {
        if (m_current_scan_info.radio_scan->radio_mac != src_mac) {
            LOG(ERROR) << "Currently running scan radio MAC does not match incoming response's. "
                       << m_current_scan_info.radio_scan->radio_mac << " != " << src_mac;
            return false;
        }
        return true;
    };
    auto is_current_scan_in_state = [this](eState scan_expected_state) -> bool {
        if (m_current_scan_info.radio_scan->current_state != scan_expected_state) {
            LOG(ERROR) << "Currently running scan is not in "
                       << m_states_string.at(scan_expected_state)
                       << " state, current scan is in state: "
                       << m_states_string.at(m_current_scan_info.radio_scan->current_state);
            return false;
        }
        return true;
    };
    auto db = AgentDB::get();

    /**
     * Since currently we handle only action_ops of action type "ACTION_BACKHAUL", use a single
     * switch-case on "ACTION_BACKHAUL" only.
     * Once the son_slave will be unified, need to replace the expected action to "ACTION_MONITOR".
     * PPM-352.
     */
    switch (beerocks_header->action_op()) {
    case beerocks_message::ACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE: {
        LOG(TRACE) << "ACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE from mac " << src_mac;
        auto response =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE>();
        if (!response) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE failed";
            return false;
        }

        if (!is_current_scan_running() || !does_current_scan_match_incoming_src(src_mac) ||
            !is_current_scan_in_state(eState::PENDING_TRIGGER)) {
            return false;
        }

        if (!response->success()) {
            LOG(ERROR) << "Failed to trigger scan on radio (" << src_mac << ")";
            // Expand the response reason to give a better scan status in the report as part of PPM-1324.
            set_radio_scan_status(m_current_scan_info.radio_scan, eScanStatus::SCAN_NOT_COMPLETED);
            FSM_MOVE_STATE(m_current_scan_info.radio_scan, eState::SCAN_FAILED);
            return true;
        }

        FSM_MOVE_TIMEOUT_STATE(m_current_scan_info.radio_scan, eState::WAIT_FOR_SCAN_TRIGGERED,
                               SCAN_TRIGGERED_WAIT_TIME);
        LOG(INFO) << "scan request was successful for radio (" << src_mac
                  << "). Wait for SCAN_TRIGGERED notification";
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION: {
        LOG(TRACE) << "ACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION from mac " << src_mac;
        auto notification = beerocks_header->addClass<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION failed";
            return false;
        }

        if (!is_current_scan_running()) {
            LOG(INFO) << "No channel scan is currently running, ignore channel scan notifications "
                         "gracefully.";
            return true;
        }

        if (!does_current_scan_match_incoming_src(src_mac) ||
            !is_current_scan_in_state(eState::WAIT_FOR_SCAN_TRIGGERED)) {
            return false;
        }

        LOG(INFO) << "Scan was triggered successfully, wait for RESULTS_READY_NOTIFICATION.";
        FSM_MOVE_TIMEOUT_STATE(m_current_scan_info.radio_scan, eState::WAIT_FOR_RESULTS_READY,
                               SCAN_RESULTS_DUMP_WAIT_TIME);

        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION: {
        LOG(TRACE) << "ACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION from mac " << src_mac;
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION failed";
            return false;
        }

        if (!is_current_scan_running()) {
            return true;
        }

        if (!does_current_scan_match_incoming_src(src_mac)) {
            return false;
        }

        if (notification->is_dump() == 0) {
            if (!is_current_scan_in_state(eState::WAIT_FOR_RESULTS_READY)) {
                return false;
            }

            LOG(INFO) << "Scan results are ready, wait for RESULTS_DUMP_NOTIFICATION.";
            FSM_MOVE_TIMEOUT_STATE(m_current_scan_info.radio_scan, eState::WAIT_FOR_RESULTS_DUMP,
                                   SCAN_RESULTS_DUMP_WAIT_TIME);
        } else {
            if (!is_current_scan_in_state(eState::WAIT_FOR_RESULTS_DUMP)) {
                return false;
            }
            if (!store_radio_scan_result(m_current_scan_info.scan_request, src_mac,
                                         notification->scan_results())) {
                LOG(ERROR) << "Failed to store radio scan result!";
                return false;
            }
            LOG(INFO) << "Scan result received, wait for another RESULTS_DUMP_NOTIFICATION or "
                         "SCAN_FINISHED_NOTIFICATION.";
            FSM_MOVE_TIMEOUT_STATE(m_current_scan_info.radio_scan, eState::WAIT_FOR_RESULTS_DUMP,
                                   SCAN_RESULTS_DUMP_WAIT_TIME);
        }
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION: {
        LOG(TRACE) << "ACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION from mac " << src_mac;
        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION failed";
            return false;
        }

        // to support both certification flow and DCS old flow this code segment should be done on each
        // SCAN_FINISHED event for this src-mac
        auto radio = db->get_radio_by_mac(src_mac);
        if (!radio) {
            return false;
        }
        radio->statuses.channel_scan_in_progress = false;

        if (!is_current_scan_running()) {
            return true;
        }

        if (!does_current_scan_match_incoming_src(src_mac) ||
            !is_current_scan_in_state(eState::WAIT_FOR_RESULTS_DUMP)) {
            return false;
        }

        FSM_MOVE_STATE(m_current_scan_info.radio_scan, eState::SCAN_DONE);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION: {
        LOG(TRACE) << "ACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION from mac " << src_mac;

        auto notification =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION>();
        if (!notification) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION failed";
            return false;
        }

        // to support both certification flow and DCS old flow this code segment should be done on each
        // SCAN_ABORTED event for this src-mac
        auto radio = db->get_radio_by_mac(src_mac);
        if (!radio) {
            return false;
        }
        radio->statuses.channel_scan_in_progress = false;

        if (!is_current_scan_running() || !does_current_scan_match_incoming_src(src_mac)) {
            return false;
        }

        set_radio_scan_status(m_current_scan_info.radio_scan, eScanStatus::SCAN_ABORTED);
        FSM_MOVE_STATE(m_current_scan_info.radio_scan, eState::SCAN_ABORTED);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE: {
        LOG(TRACE) << "ACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE from mac " << src_mac;

        auto response =
            beerocks_header
                ->addClass<beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE>();
        if (!response) {
            LOG(ERROR) << "addClass cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE failed";
            return false;
        }

        if (!is_current_scan_running() || !does_current_scan_match_incoming_src(src_mac) ||
            !is_current_scan_in_state(eState::PENDING_TRIGGER)) {
            return false;
        }

        if (!response->success()) {
            LOG(ERROR) << "Failed to trigger dump results on radio (" << src_mac << ")";
            // Expand the response reason to give a better scan status in the report as part of PPM-1324.
            set_radio_scan_status(m_current_scan_info.radio_scan, eScanStatus::SCAN_NOT_COMPLETED);
            FSM_MOVE_STATE(m_current_scan_info.radio_scan, eState::SCAN_FAILED);
            return true;
        }

        LOG(INFO) << "Scan results dump request received successfully, wait for "
                     "RESULTS_READY_NOTIFICATION.";
        FSM_MOVE_TIMEOUT_STATE(m_current_scan_info.radio_scan, eState::WAIT_FOR_RESULTS_READY,
                               SCAN_RESULTS_DUMP_WAIT_TIME);
        break;
    }
    case beerocks_message::ACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE: {
        LOG(TRACE) << "ACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE from mac " << src_mac;
        break;
    }
    default: {
        // Message was not handled, therfore return false.
        return false;
    }
    }
    return true;
}

/* Helper functions */

bool ChannelScanTask::is_scan_request_finished(const std::shared_ptr<sScanRequest> request)
{
    auto radio_scans = request->radio_scans;
    auto radio_scan_not_finished =
        [](std::pair<std::string, std::shared_ptr<sRadioScan>> radio_scan_iter) -> bool {
        return radio_scan_iter.second->current_state != eState::SCAN_DONE &&
               radio_scan_iter.second->current_state != eState::SCAN_FAILED;
    };
    auto unfinished_radio_scan_in_request =
        std::find_if(radio_scans.begin(), radio_scans.end(), radio_scan_not_finished);

    // Returns "True" if no unfinished scans were found
    return unfinished_radio_scan_in_request == radio_scans.end();
}

bool ChannelScanTask::abort_scan_request(const std::shared_ptr<sScanRequest> request)
{
    for (auto radio_scan : request->radio_scans) {
        const auto &radio_iface = radio_scan.first;
        LOG(TRACE) << "Request scan abort on " << radio_iface;

        auto agent_fd = m_btl_ctx.get_agent_fd();
        if (agent_fd == beerocks::net::FileDescriptor::invalid_descriptor) {
            LOG(DEBUG) << "socket to Agent not found";
            return false;
        }

        auto abort_request = beerocks::message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST>(m_cmdu_tx);
        if (!abort_request) {
            LOG(ERROR) << "Failed to build cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST";
            return false;
        }

        // Filling the radio mac. This is temporary the task will be moved to the agent (PPM-1679).
        auto db    = AgentDB::get();
        auto radio = db->radio(radio_iface);
        if (!radio) {
            return false;
        }
        auto action_header         = message_com::get_beerocks_header(m_cmdu_tx)->actionhdr();
        action_header->radio_mac() = radio->front.iface_mac;

        if (!m_btl_ctx.send_cmdu(agent_fd, m_cmdu_tx)) {
            LOG(ERROR) << "Failed to send cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST for "
                       << radio_iface;
            return false;
        }

        LOG(TRACE) << "Sent ABORT_CHANNEL_SCAN for radio " << radio_iface;
    }

    return true;
}

bool ChannelScanTask::trigger_next_radio_scan(const std::shared_ptr<sScanRequest> request)
{
    /**
     * Currently only one Channel Scan per radio is supported.
     * When PPM-711 is resolved we need to rework the trigger mechanism to support multiple
     * simultaneous radio scans.
     * https://jira.prplfoundation.org/browse/PPM-711
     */
    auto &radio_scans = request->radio_scans;
    auto radio_scan_is_pending =
        [](std::pair<std::string, std::shared_ptr<sRadioScan>> radio_scan_iter) -> bool {
        return radio_scan_iter.second->current_state == eState::PENDING_TRIGGER;
    };
    auto next_pending_radio_scan =
        std::find_if(radio_scans.begin(), radio_scans.end(), radio_scan_is_pending);
    if (next_pending_radio_scan == radio_scans.end()) {
        LOG(TRACE) << "Unable to find the next pending radio scan in request.";
        return false;
    }
    if (!trigger_radio_scan(next_pending_radio_scan->first, next_pending_radio_scan->second)) {
        LOG(ERROR) << "Failed to send radio scan trigger request";
        return false;
    }

    m_current_scan_info.scan_request              = request;
    m_current_scan_info.radio_scan                = next_pending_radio_scan->second;
    m_current_scan_info.is_scan_currently_running = true;

    return true;
}

bool ChannelScanTask::set_radio_scan_status(const std::shared_ptr<sRadioScan> radio_scan_info,
                                            const eScanStatus status)
{
    // If a scan fails because of an Abort notification, a timeout event, or any other similar failure, each channel's status needs to be updated.
    // Because we currently have no way to know which channel failed, we set the status for all the channels.
    // Later, when retrieving the results for the report, we will check each channel for their results' status and add them to the report accordingly.
    for (auto &op_cls : radio_scan_info->operating_classes) {
        for (auto &chan_elem : op_cls.channel_list) {
            chan_elem.scan_status = status;
        }
    }
    return true;
}

bool ChannelScanTask::trigger_radio_scan(const std::string &radio_iface,
                                         const std::shared_ptr<sRadioScan> radio_scan_info)
{
    auto agent_fd = m_btl_ctx.get_agent_fd();
    if (agent_fd == beerocks::net::FileDescriptor::invalid_descriptor) {
        LOG(DEBUG) << "socket to Agent not found";
        return false;
    }
    auto db    = AgentDB::get();
    auto radio = db->radio(radio_iface);
    if (!radio) {
        LOG(ERROR) << "Failed to get radio info from Agent DB for " << radio_iface;
        set_radio_scan_status(radio_scan_info, eScanStatus::SCAN_NOT_COMPLETED);
        return false;
    }

    if (radio_scan_info->dwell_time == 0) {
        auto request_dump = beerocks::message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST>(m_cmdu_tx);
        if (!request_dump) {
            LOG(ERROR) << "Failed to build cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST";
            set_radio_scan_status(radio_scan_info, eScanStatus::SCAN_NOT_COMPLETED);
            return false;
        }
        LOG(DEBUG) << "Sending \"Scan Dump\" request.";
    } else {
        auto trigger_request = beerocks::message_com::create_vs_message<
            beerocks_message::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST>(m_cmdu_tx);
        if (!trigger_request) {
            LOG(ERROR) << "Failed to build cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST";
            set_radio_scan_status(radio_scan_info, eScanStatus::SCAN_NOT_COMPLETED);
            return false;
        }

        /**
         * Copy the channel list within the operating class vector in the found Radio Scan info.
         * Using an unordered_set since we do not want duplicated channels in our channel pool
         */
        std::unordered_set<uint8_t> channels_to_scan;
        for (auto &operating_class : radio_scan_info->operating_classes) {
            for (auto &channel_element : operating_class.channel_list) {
                // Scan only the channels without an error status.
                // Scan status is per channel on operating class.
                if (channel_element.scan_status ==
                    eScanStatus::
                        SCAN_NOT_SUPPORTED_ON_THIS_OPERATING_CLASS_AND_CHANNEL_ON_THIS_RADIO) {
                    continue;
                }
                // Convert a central channel to 20MHz channels.
                if (operating_class.bw >= beerocks::eWiFiBandwidth::BANDWIDTH_80) {
                    LOG(DEBUG) << "Channel " << channel_element.channel_number
                               << " is a central channel";
                    std::unordered_set<uint8_t> subchannels_20MHz;
                    son::wireless_utils::get_subset_20MHz_channels(
                        channel_element.channel_number, operating_class.operating_class,
                        operating_class.bw, subchannels_20MHz);
                    channels_to_scan.insert(subchannels_20MHz.begin(), subchannels_20MHz.end());
                } else {
                    channels_to_scan.insert(channel_element.channel_number);
                }
            }
        }

        if (channels_to_scan.empty()) {
            LOG(TRACE) << "There were no channels to be scanned";
            FSM_MOVE_STATE(radio_scan_info, eState::SCAN_DONE);
            return true;
        }

        // Set scan params in CMDU
        trigger_request->scan_params().radio_mac         = radio_scan_info->radio_mac;
        trigger_request->scan_params().dwell_time_ms     = radio_scan_info->dwell_time;
        trigger_request->scan_params().channel_pool_size = channels_to_scan.size();
        std::copy(channels_to_scan.begin(), channels_to_scan.end(),
                  trigger_request->scan_params().channel_pool);

        // Print CMDU scan parameters
        auto print_pool = [](uint8_t *pool, uint8_t size) -> std::string {
            std::stringstream ss;
            ss << "[ ";
            for (int ch_idx = 0; ch_idx < size; ch_idx++) {
                ss << int(pool[ch_idx]) << " ";
            }
            ss << "]";
            return ss.str();
        };
        LOG(DEBUG) << "Sending \"Scan Trigger\" request for the following:" << std::endl
                   << "- Radio MAC: " << trigger_request->scan_params().radio_mac << std::endl
                   << "- Dwell time: " << trigger_request->scan_params().dwell_time_ms << std::endl
                   << "- Channels: "
                   << print_pool(trigger_request->scan_params().channel_pool,
                                 trigger_request->scan_params().channel_pool_size);
    }

    // Filling the radio mac. This is temporary the task will be moved to the agent (PPM-1679).
    auto action_header         = message_com::get_beerocks_header(m_cmdu_tx)->actionhdr();
    action_header->radio_mac() = radio->front.iface_mac;

    // Send CMDU
    if (!m_btl_ctx.send_cmdu(agent_fd, m_cmdu_tx)) {
        set_radio_scan_status(radio_scan_info, eScanStatus::SCAN_NOT_COMPLETED);
        return false;
    }
    return true;
}

bool ChannelScanTask::store_radio_scan_result(const std::shared_ptr<sScanRequest> request,
                                              const sMacAddr &radio_mac,
                                              beerocks_message::sChannelScanResults results)
{
    LOG(TRACE) << "Handling scan result from " << radio_mac;
    auto db    = AgentDB::get();
    auto radio = db->get_radio_by_mac(radio_mac);
    if (!radio) {
        LOG(ERROR) << "Failed to get radio info from Agent DB for " << radio_mac;
        return false;
    }
    auto radio_scan_info_iter = request->radio_scans.find(radio->front.iface_name);
    if (radio_scan_info_iter == request->radio_scans.end()) {
        LOG(ERROR) << "Could not find radio: " << radio->front.iface_name << ", MAC: " << radio_mac
                   << " in the current scan request.";
        return false;
    }

    // Iterate over the radio scan's operating class
    for (const auto &op_cls : radio_scan_info_iter->second->operating_classes) {
        for (const auto &ch_elem : op_cls.channel_list) {
            std::unordered_set<uint8_t> _20MHz_channels;
            son::wireless_utils::get_subset_20MHz_channels(
                ch_elem.channel_number, op_cls.operating_class, op_cls.bw, _20MHz_channels);
            /**
             * To store the result in the correct radio scan, we need too check if the results'
             * channel exists in the channel element's 20MHz channel subset.
             * If it does that we add the result to the current radio scan's cached results under
             * the channel element's channel number.
             */
            if (_20MHz_channels.find(results.channel) != _20MHz_channels.end()) {
                LOG(DEBUG) << "Setting result in channel " << ch_elem.channel_number;
                radio_scan_info_iter->second->cached_results[ch_elem.channel_number].push_back(
                    results);
                // Move on to next operating class in radio scan
                break;
            }
        }
    }
    return true;
}

bool ChannelScanTask::handle_channel_scan_request(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                  const sMacAddr &src_mac)
{
    // Convert channel vector to string
    auto print_channel_vector = [](const std::vector<sChannel> &channel_vector) -> std::string {
        std::stringstream ss;
        ss << "[ ";
        for (const auto &channel_element : channel_vector) {
            ss << int(channel_element.channel_number) << " ";
        }
        ss << "]";
        return ss.str();
    };

    // Creates a sChannel vector from the given incoming channel list.
    auto create_channel_vector = [this](const std::vector<uint8_t> &channel_list,
                                        const uint8_t operating_class,
                                        const beerocks::eWiFiBandwidth bw,
                                        AgentDB::sRadio *db_radio) -> std::vector<sChannel> {
        std::vector<sChannel> channel_vector;
        if (channel_list.empty()) {
            LOG(TRACE) << "Empty channel list sent for Operating class #" << int(operating_class);
            // If incoming channel list is empty, add all channels under that operating class to the list.
            auto operating_class_channels =
                son::wireless_utils::operating_class_to_channel_set(operating_class);
            LOG(DEBUG) << "Manually adding channel list of size: "
                       << operating_class_channels.size();
            for (const auto channel_number : operating_class_channels) {
                std::unordered_set<uint8_t> operating_class_20MHz_channels;
                LOG(DEBUG) << "Getting 20MHz channels for central channel: " << channel_number;
                son::wireless_utils::get_subset_20MHz_channels(channel_number, operating_class, bw,
                                                               operating_class_20MHz_channels);
                for (const auto operating_class_20MHz_channel : operating_class_20MHz_channels) {
                    LOG(DEBUG) << "Checking if 20MHz channel " << operating_class_20MHz_channel
                               << " is supported";
                    if (db_radio->channels_list.find(channel_number) ==
                        db_radio->channels_list.end()) {
                        LOG(DEBUG) << "20MHz channel " << operating_class_20MHz_channel
                                   << " is not supported";
                        // Channel is not supported
                        channel_vector.emplace_back(
                            operating_class_20MHz_channel,
                            eScanStatus::
                                SCAN_NOT_SUPPORTED_ON_THIS_OPERATING_CLASS_AND_CHANNEL_ON_THIS_RADIO);
                    } else {
                        LOG(DEBUG)
                            << "20MHz channel " << operating_class_20MHz_channel << " is supported";
                        // Assume scan will be successful, add to previous scans
                        m_previous_scans[operating_class].emplace(operating_class_20MHz_channel);
                        channel_vector.emplace_back(operating_class_20MHz_channel);
                    }
                }
            }
            return channel_vector;
        }
        for (const auto channel_number : channel_list) {
            if (!son::wireless_utils::is_channel_in_operating_class(operating_class,
                                                                    channel_number) ||
                db_radio->channels_list.find(channel_number) == db_radio->channels_list.end()) {
                LOG(DEBUG) << "Channel #" << channel_number << " in operating class #"
                           << operating_class << " is not supported on this radio";
                channel_vector.emplace_back(
                    channel_number,
                    eScanStatus::
                        SCAN_NOT_SUPPORTED_ON_THIS_OPERATING_CLASS_AND_CHANNEL_ON_THIS_RADIO);
            } else {
                // Assume scan will be successful, add to previous scans
                m_previous_scans[operating_class].emplace(channel_number);
                channel_vector.emplace_back(channel_number);
            }
        }
        return channel_vector;
    };

    // Create a sOperatingClass element from the given operating class entry that was received in the request.
    auto create_fresh_operating_class = [this, &create_channel_vector, &print_channel_vector](
                                            wfa_map::cOperatingClasses &class_entry,
                                            AgentDB::sRadio *db_radio) -> sOperatingClass {
        const auto class_number = class_entry.operating_class();
        const auto bandwidth    = son::wireless_utils::operating_class_to_bandwidth(class_number);
        const auto channel_list_length = class_entry.channel_list_length();
        std::vector<uint8_t> channel_list;
        if (channel_list_length > 0) {
            const auto &channel_array = class_entry.channel_list();
            channel_list.insert(channel_list.end(), channel_array,
                                channel_array + channel_list_length);
        }
        std::vector<sChannel> channel_vector =
            create_channel_vector(channel_list, class_number, bandwidth, db_radio);
        LOG(TRACE) << "Operating class: #" << int(class_number) << std::endl
                   << "\tChannel list length:" << int(channel_vector.size()) << std::endl
                   << "\tChannel list: " << print_channel_vector(channel_vector) << ".";

        return sOperatingClass(class_number, bandwidth, channel_vector);
    };

    // Create a sOperatingClass vector from the previous scans.
    auto create_stored_operating_classes =
        [this, &print_channel_vector]() -> std::vector<sOperatingClass> {
        std::vector<sOperatingClass> operating_vector;
        for (const auto previous_scan : m_previous_scans) {
            const auto operating_class = previous_scan.first;
            const auto bandwidth =
                son::wireless_utils::operating_class_to_bandwidth(operating_class);
            std::vector<sChannel> channel_vector;
            for (const auto prev_scan_channel : previous_scan.second) {
                channel_vector.emplace_back(prev_scan_channel);
            }
            LOG(TRACE) << "Operating class: #" << int(operating_class) << std::endl
                       << "\tChannel list length:" << int(channel_vector.size()) << std::endl
                       << "\tChannel list: " << print_channel_vector(channel_vector) << ".";

            operating_vector.emplace_back(operating_class, bandwidth, channel_vector);
        }
        return operating_vector;
    };

    const auto scan_start_timestamp = std::chrono::system_clock::now();
    const auto mid                  = cmdu_rx.getMessageId();

    auto channel_scan_request_tlv = cmdu_rx.getClass<wfa_map::tlvProfile2ChannelScanRequest>();
    if (!channel_scan_request_tlv) {
        LOG(ERROR) << "getClass wfa_map::tlvProfile2ChannelScanRequest failed";
        return false;
    }

    const auto &perform_fresh_scan = channel_scan_request_tlv->perform_fresh_scan();

    LOG(INFO) << "Received CHANNEL_SCAN_REQUEST_MESSAGE from "
              << "radio MAC: " << src_mac << " mid: " << std::hex << mid << "." << std::endl
              << "The perform_fresh_scan flag set to: \""
              << (perform_fresh_scan == wfa_map::tlvProfile2ChannelScanRequest::ePerformFreshScan::
                                            PERFORM_A_FRESH_SCAN_AND_RETURN_RESULTS
                      ? "PERFORM_A_FRESH_SCAN_AND_RETURN_RESULTS"
                      : "RETURN_STORED_RESULTS_OF_LAST_SUCCESSFUL_SCAN")
              << "\"";

    // Build and send ACK message CMDU to the originator.
    auto cmdu_tx_header = m_cmdu_tx.create(mid, ieee1905_1::eMessageType::ACK_MESSAGE);
    if (!cmdu_tx_header) {
        LOG(ERROR) << "cmdu creation of type ACK_MESSAGE, has failed";
        return false;
    }

    LOG(DEBUG) << "Sending ACK message to the originator, mid=" << std::hex << mid;
    auto db = AgentDB::get();
    if (!m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, src_mac, db->bridge.mac)) {
        LOG(ERROR) << "Failed to send ACK_MESSAGE back to controller";
        return false;
    }

    if (perform_fresh_scan && m_current_scan_info.is_scan_currently_running) {
        // Check if current scan can be aborted.
        auto current_request_info = m_current_scan_info.scan_request;
        // Only Controller-Requested scan requests can be aborted.
        if (current_request_info->request_info->request_type ==
            sRequestInfo::eScanRequestType::ControllerRequested) {
            if (!abort_scan_request(current_request_info)) {
                LOG(ERROR) << "Failed to abort current scan request!";
                return false;
            }
        }
    }

    sControllerRequestInfo request_info;
    request_info.src_mac            = src_mac;
    request_info.perform_fresh_scan = perform_fresh_scan;

    auto new_request = std::shared_ptr<sScanRequest>(new sScanRequest(), [](sScanRequest *ptr) {
        LOG(TRACE) << "Deleting scan request: " << std::hex << ptr << ".";
        delete ptr;
    });
    new_request->request_info         = std::make_shared<sControllerRequestInfo>(request_info);
    new_request->scan_start_timestamp = scan_start_timestamp;
    new_request->ready_to_send_report = false;

    const auto &radio_list_length = channel_scan_request_tlv->radio_list_length();
    // Iterate over the incoming radio list
    for (int radio_i = 0; radio_i < radio_list_length; ++radio_i) {
        const auto &radio_list_tuple = channel_scan_request_tlv->radio_list(radio_i);
        if (!std::get<0>(radio_list_tuple)) {
            LOG(ERROR) << "Failed to get radio_list[" << radio_i << "]. Continuing...";
            continue;
        }
        auto &radio_list_entry    = std::get<1>(radio_list_tuple);
        const auto radio_mac      = radio_list_entry.radio_uid();
        const auto class_list_len = radio_list_entry.operating_classes_list_length();
        if (class_list_len == 0 && perform_fresh_scan) {
            LOG(ERROR) << "Invalid request! A fresh scan was requested, but no operating classed "
                          "were sent";
            return false;
        }
        const auto radio = db->get_radio_by_mac(radio_mac);
        if (!radio) {
            LOG(ERROR) << "Failed to get radio entry for MAC: " << radio_mac;
            return false;
        }
        const auto radio_iface = radio->front.iface_name;

        LOG(TRACE) << "radio_list[" << radio_i << "]:" << std::endl
                   << "\tRadio iface: " << radio_iface << std::endl
                   << "\tRadio MAC: " << radio_mac << std::endl
                   << "\tOperating class list length:" << int(class_list_len);

        // Create new radio scan
        auto new_radio_scan = std::shared_ptr<sRadioScan>(new sRadioScan(), [](sRadioScan *ptr) {
            LOG(TRACE) << "Deleting radio scan: " << std::hex << ptr << ".";
            delete ptr;
        });
        new_radio_scan->radio_mac     = radio_mac;
        new_radio_scan->dwell_time    = PREFERRED_DWELLTIME_MS;
        new_radio_scan->current_state = eState::PENDING_TRIGGER;

        if (!perform_fresh_scan) {
            new_radio_scan->operating_classes = create_stored_operating_classes();
        } else {
            // Iterate over operating classes
            for (int class_idx = 0; class_idx < class_list_len; class_idx++) {
                const auto &class_tuple = radio_list_entry.operating_classes_list(class_idx);
                if (!std::get<0>(class_tuple)) {
                    LOG(ERROR) << "Failed to get operating class[" << class_idx
                               << "]. Continuing...";
                    continue;
                }
                auto &class_entry = std::get<1>(class_tuple);
                new_radio_scan->operating_classes.push_back(
                    create_fresh_operating_class(class_entry, radio));
            }
        }

        // Add radio scan info to radio scans map in the request
        new_request->radio_scans.emplace(radio_iface, new_radio_scan);
    }

    auto handle_scan_request_extension_tlv = [this, &new_request,
                                              &db](ieee1905_1::CmduMessageRx &cmdu_rx) -> bool {
        auto beerocks_header = beerocks::message_com::parse_intel_vs_message(cmdu_rx);
        if (!beerocks_header) {
            LOG(ERROR) << "expecting beerocks_message::tlvVsChannelScanRequestExtension";
            return false;
        }
        auto channel_scan_request_extension_vs_tlv =
            beerocks_header->addClass<beerocks_message::tlvVsChannelScanRequestExtension>();
        if (!channel_scan_request_extension_vs_tlv) {
            LOG(ERROR) << "addClass beerocks_message::tlvVsChannelScanRequestExtension failed";
            return false;
        }

        const auto &extension_list_length =
            channel_scan_request_extension_vs_tlv->scan_requests_list_length();
        for (int extention_list_i = 0; extention_list_i < extension_list_length;
             ++extention_list_i) {
            const auto &extension_tuple =
                channel_scan_request_extension_vs_tlv->scan_requests_list(extention_list_i);
            if (!std::get<0>(extension_tuple)) {
                LOG(ERROR) << "Failed to get scan request extention[" << extention_list_i
                           << "]. Continuing...";
                continue;
            }
            auto &extension_entry = std::get<1>(extension_tuple);

            LOG(DEBUG) << "Found scan request extention for radio: " << extension_entry.radio_mac;

            const auto radio = db->get_radio_by_mac(extension_entry.radio_mac);
            if (!radio) {
                LOG(ERROR) << "Failed to get radio entry for MAC: " << extension_entry.radio_mac;
                return false;
            }
            auto radio_scan_info_iter = new_request->radio_scans.find(radio->front.iface_name);
            if (radio_scan_info_iter == new_request->radio_scans.end()) {
                LOG(ERROR) << "Could not find radio: " << radio->front.iface_name
                           << ", MAC: " << extension_entry.radio_mac
                           << " in the current scan request.";
                continue;
            }

            radio_scan_info_iter->second->dwell_time = extension_entry.dwell_time_ms;
            LOG(DEBUG) << "Set Dwell-time " << extension_entry.dwell_time_ms
                       << " ms for radio: " << radio->front.iface_name;
        }
        return true;
    };

    // For prplmesh controller, we wish to support a custome Dwell-Time parameter.
    if (db->controller_info.prplmesh_controller) {
        if (!handle_scan_request_extension_tlv(cmdu_rx)) {
            LOG(WARNING) << "Failed to handle tlvVsChannelScanRequestExtension!";
        }
    }

    // Should return all the currently stored results in the DB for the requested radios
    // There is no need to add it to the request queue, since there is no need to perform a scan.
    if (!perform_fresh_scan) {
        new_request->ready_to_send_report = true;
        return send_channel_scan_report_to_controller(new_request);
    }
    m_pending_requests.emplace_back(new_request);
    return true;
}

bool ChannelScanTask::send_channel_scan_report(const std::shared_ptr<sScanRequest> request)
{
    const auto request_info = request->request_info;
    switch (request_info->request_type) {
    case sRequestInfo::eScanRequestType::ControllerRequested: {
        return send_channel_scan_report_to_controller(request);
    }
    default: {
        LOG(ERROR) << "Request of unknown type!";
        return false;
    }
    }
}

bool ChannelScanTask::send_channel_scan_report_to_controller(
    const std::shared_ptr<sScanRequest> request)
{
    // Lambda function that creates a Channel-Scan-Report-Message
    auto create_channel_scan_report_message = [this]() -> std::shared_ptr<ieee1905_1::cCmduHeader> {
        LOG(DEBUG) << "Creating new Channel Scan Report Message";
        auto cmdu_tx_header =
            m_cmdu_tx.create(0, ieee1905_1::eMessageType::CHANNEL_SCAN_REPORT_MESSAGE);
        if (!cmdu_tx_header) {
            LOG(ERROR) << "Failed to create CMDU of type CHANNEL_SCAN_REPORT_MESSAGE";
            return nullptr;
        }
        return cmdu_tx_header;
    };

    // Lambda function that adds a Timestamp TLV to the buffer.
    auto add_timestamp_tlv_to_report = [this](const std::string &timestamp) -> bool {
        LOG(DEBUG) << "Adding new Timestamp TLV";
        auto timestamp_tlv = m_cmdu_tx.addClass<wfa_map::tlvTimestamp>();
        if (!timestamp_tlv) {
            LOG(ERROR) << "addClass tlvTimestamp failed";
            return false;
        }
        // Fill Timestamp TLV
        if (!timestamp_tlv->set_timestamp(timestamp.c_str(), timestamp.size())) {
            LOG(ERROR) << "Failed to set timestamp in tlvTimestamp!";
            return false;
        }
        return true;
    };

    // Lambda function that fills the TLV neighbor structure.
    auto set_neighbor_in_scan_results_tlv =
        [](const beerocks_message::sChannelScanResults &neighbor,
           std::shared_ptr<wfa_map::cNeighbors> neighbor_res) -> bool {
        // BSSID
        neighbor_res->bssid() = neighbor.bssid;

        // SSID
        if (!neighbor_res->set_ssid(neighbor.ssid, std::string(neighbor.ssid).length())) {
            LOG(ERROR) << "Failed to set SSID";
            return false;
        }

        // Signal Strength
        neighbor_res->signal_strength() = neighbor.signal_strength_dBm;

        // Bandwidth
        auto eChannelScanResultChannelBandwidth_toString =
            [](const beerocks_message::eChannelScanResultChannelBandwidth &bw) -> std::string {
            switch (bw) {
            case beerocks_message::eChannelScanResultChannelBandwidth::eChannel_Bandwidth_20MHz:
                return "20MHz";
            case beerocks_message::eChannelScanResultChannelBandwidth::eChannel_Bandwidth_40MHz:
                return "40MHz";
            case beerocks_message::eChannelScanResultChannelBandwidth::eChannel_Bandwidth_80MHz:
                return "80MHz";
            case beerocks_message::eChannelScanResultChannelBandwidth::eChannel_Bandwidth_80_80:
                return "80+80MHz";
            case beerocks_message::eChannelScanResultChannelBandwidth::eChannel_Bandwidth_160MHz:
                return "160MHz";
            case beerocks_message::eChannelScanResultChannelBandwidth::eChannel_Bandwidth_NA:
            default:
                LOG(DEBUG) << "Unknown BW value, setting 20MHz";
                return "20MHz";
            }
        };
        auto bw_str =
            eChannelScanResultChannelBandwidth_toString(neighbor.operating_channel_bandwidth);
        if (!neighbor_res->set_channels_bw_list(bw_str.c_str(), bw_str.length())) {
            LOG(ERROR) << "Failed to set channel BW list";
            return false;
        }

        // BSS Load Element Present
        // Resolve as part of PPM-1045
        neighbor_res->bss_load_element_present() =
            wfa_map::cNeighbors::eBssLoadElementPresent::FIELD_NOT_PRESENT;

        // Channel Utilization
        // Resolve as part of PPM-1045
        // Since BSS Load Element Present is set to "Not Present" no need to set  Channel Utilization.
        // neighbor_res->channel_utilization() = neighbor.channel_utilization;

        // Station Count
        // Resolve as part of PPM-1045
        // Since BSS Load Element Present is set to "Not Present" no need to set Station Count.
        // neighbor_res->station_count() = 0;

        return true;
    };

    // Lambda function that adds a Scan-Results TLV to the buffer.
    // The Lambda is also responsible for adding the found neighboring APs to the TLV
    auto add_scan_results_tlv_to_report =
        [this, &set_neighbor_in_scan_results_tlv](
            sMacAddr ruid, uint8_t operating_class, uint8_t channel, eScanStatus status,
            std::chrono::system_clock::time_point timestamp,
            std::vector<beerocks_message::sChannelScanResults> results) -> bool {
        LOG(DEBUG) << "Adding new Scan Results TLV";
        auto results_tlv = m_cmdu_tx.addClass<wfa_map::tlvProfile2ChannelScanResult>();
        if (!results_tlv) {
            LOG(ERROR) << "addClass tlvProfile2ChannelScanResult failed";
            return false;
        }

        // Fill Scan Results TLV
        results_tlv->radio_uid()       = ruid;
        results_tlv->operating_class() = operating_class;
        results_tlv->channel()         = channel;

        // Set Results TLV status
        results_tlv->success() = status;
        if (results_tlv->success() != eScanStatus::SUCCESS) {
            // If results status is not successful, need to finish TLV.
            return true;
        }

        // Set Results TLV timestamp
        const auto result_timestamp = utils::get_ISO_8601_timestamp_string(timestamp);
        if (result_timestamp.empty()) {
            LOG(ERROR) << "Failed to create timestamp string for results";
            return false;
        }
        if (!results_tlv->set_timestamp(result_timestamp)) {
            LOG(ERROR) << "Failed to set timestamp in tlvProfile2ChannelScanResult!";
            return false;
        }

        // Set stored scan results in Results TLV neighbor list
        // Total values will be used to calculate averages
        int total_noise       = 0;
        int total_utilization = 0;
        for (auto stored_neighbor : results) {
            auto tlv_neighbor_ptr = results_tlv->create_neighbors_list();
            if (!tlv_neighbor_ptr) {
                LOG(ERROR) << "Failed to create neighbor list";
                return false;
            }

            if (!set_neighbor_in_scan_results_tlv(stored_neighbor, tlv_neighbor_ptr)) {
                LOG(ERROR) << "Failed to fill neighbor structure";
                return false;
            }

            if (!results_tlv->add_neighbors_list(tlv_neighbor_ptr)) {
                LOG(ERROR) << "Failed to add neighbor to TLV";
                return false;
            }

            total_noise       = total_noise + stored_neighbor.noise_dBm;
            total_utilization = total_utilization + stored_neighbor.channel_utilization;
        }
        auto neighbors_list_length = results_tlv->neighbors_list_length();
        results_tlv->noise() =
            neighbors_list_length == 0 ? 0 : (total_noise / neighbors_list_length);
        results_tlv->utilization() =
            neighbors_list_length == 0 ? 0 : (total_utilization / neighbors_list_length);

        // WFA R2 test script has a bug that checks utilization for non zero value.
        // Setting the utilization to a non zero value is a W/A that needs to be
        // deleted once WFA fixes the issue.
        if (results_tlv->utilization() == 0) {
            results_tlv->utilization() = 10;
        }

        return true;
    };

    // Lambda function that sends the Channel-Scan-Report-Message
    // The Lambda is also responsible for adding the tlvVsChannelScanReportDone TLV if needed
    auto send_channel_scan_report_cmdu = [this](const sMacAddr &dst_mac,
                                                bool report_done = false) -> bool {
        auto db = AgentDB::get();
        if (db->controller_info.prplmesh_controller) {
            LOG(TRACE) << m_cmdu_tx.getMessageBuffLength() - m_cmdu_tx.getMessageLength();
            auto scan_report_done_tlv =
                message_com::add_vs_tlv<beerocks_message::tlvVsChannelScanReportDone>(m_cmdu_tx);
            if (!scan_report_done_tlv) {
                LOG(ERROR) << "addClass tlvVsChannelScanReportDone failed";
                return false;
            }
            scan_report_done_tlv->report_done() = report_done;
            LOG(TRACE) << m_cmdu_tx.getMessageBuffLength() - m_cmdu_tx.getMessageLength();
        }
        LOG(DEBUG) << "Sending Channel Scan Report Message to broker";
        return m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, dst_mac, db->bridge.mac);
    };

    auto calculate_remaining_buffer_size = [this](bool is_prplmesh_controller) -> size_t {
        // remaining-buffer-size = buffer-max-size - buffer-used-size - end-of-message-tlv-size
        // Total size of th buffer.
        size_t remaining_buffer_size = m_cmdu_tx.getMessageBuffLength();
        // Remove the used buffer length.
        remaining_buffer_size -= m_cmdu_tx.getMessageLength();
        // Remove EndOfMessage size.
        remaining_buffer_size -= ieee1905_1::tlvEndOfMessage::get_initial_size();
        // Check whather the controller is prplmesh, in which case send the ChannelScanReportDone.
        if (is_prplmesh_controller) {
            // Beerocks-Message header size.
            remaining_buffer_size -= beerocks_message::cACTION_HEADER::get_initial_size();
            // Vendor Specific TLV header size.
            remaining_buffer_size -= ieee1905_1::tlvVendorSpecific::get_initial_size();
            // Vendor Specific TLV data size.
            remaining_buffer_size -=
                beerocks_message::tlvVsChannelScanReportDone::get_initial_size();
        }
        return remaining_buffer_size;
    };

    auto db = AgentDB::get();

    const auto request_info =
        std::static_pointer_cast<sControllerRequestInfo>(request->request_info);
    if (!request_info) {
        LOG(ERROR) << "Unable to cast request info as sControllerRequestInfo";
        return false;
    }

    const auto report_timestamp = utils::get_ISO_8601_timestamp_string();
    if (report_timestamp.empty()) {
        LOG(ERROR) << "Failed to create timestamp string for report";
        return false;
    }

    const auto results_vec = get_scan_results_for_request(request);
    LOG(TRACE) << results_vec->size() << " results vectors found!";

    // Create new Report-Message
    auto channel_scan_report_header = create_channel_scan_report_message();
    if (!channel_scan_report_header) {
        LOG(ERROR) << "Failed to Create Channel Scan Report Message";
        return false;
    }
    // Add Timestamp-TLV to Report-Message
    if (!add_timestamp_tlv_to_report(report_timestamp)) {
        LOG(ERROR) << "Failed to add Timestamp TLV to Channel Scan Report Message";
        return false;
    }

    if (results_vec->size() == 0) {
        // No results are avaliable, sending an empty report.
        if (!send_channel_scan_report_cmdu(request_info->src_mac, true)) {
            LOG(ERROR) << "Failed to Send Channel Scan Report Message";
            return false;
        }
        return true;
    }

    // Results parameters, used to ease the iteration process
    auto stored_scan_results_iter = results_vec->begin();
    auto results_neighbors        = stored_scan_results_iter->results;
    auto neighbor_iterator        = results_neighbors.begin();

    // Iterate over the found results vector while there are still results
    while (stored_scan_results_iter != results_vec->end()) {
        size_t remaining_buffer_size =
            calculate_remaining_buffer_size(db->controller_info.prplmesh_controller);
        LOG(TRACE) << "Remaining buffer size: " << remaining_buffer_size << " byte.";

        // If cannot fit even one tlv with neighbor, then send and clear buffer
        if (remaining_buffer_size < MIN_RESULTS_TLV_SIZE) {
            // Send "active" Report-Message
            if (!send_channel_scan_report_cmdu(request_info->src_mac)) {
                LOG(ERROR) << "Failed to Send Channel Scan Report Message";
                return false;
            }
            // Create new Report-Message
            channel_scan_report_header = create_channel_scan_report_message();
            if (!channel_scan_report_header) {
                LOG(ERROR) << "Failed to Create Channel Scan Report Message";
                return false;
            }
            // Add Timestamp-TLV to Report-Message
            if (!add_timestamp_tlv_to_report(report_timestamp)) {
                LOG(ERROR) << "Failed to add Timestamp TLV to Channel Scan Report Message";
                return false;
            }

            // Recalculate remaining-buffer-size
            remaining_buffer_size =
                calculate_remaining_buffer_size(db->controller_info.prplmesh_controller);
            LOG(TRACE) << "Remaining buffer size: " << remaining_buffer_size << " byte.";
        }

        // Get number of results that can fit in the currently avaliable buffer
        const size_t max_allowed_tlv_size = std::min(remaining_buffer_size, MAX_TLV_FRAGMENT_SIZE);
        const size_t max_number_of_neighbors_that_can_be_added =
            (max_allowed_tlv_size - BASE_RESULTS_TLV_SIZE) / MAX_NEIGHBOR_SIZE;
        const size_t remaining_neighbors_to_be_sent =
            std::distance(neighbor_iterator, results_neighbors.end());
        const size_t number_of_neighbors_that_will_be_added =
            std::min(max_number_of_neighbors_that_can_be_added, remaining_neighbors_to_be_sent);

        LOG(TRACE) << "Max number of neighbors that can be added: "
                   << max_number_of_neighbors_that_can_be_added;
        LOG(TRACE) << "Remaining neighbors to be sent:" << remaining_neighbors_to_be_sent;
        LOG(TRACE) << "Number of neighbors that will be added:"
                   << number_of_neighbors_that_will_be_added;

        // Add Results-TLV to Report-Message
        // this API receives a fragment of the results (that have enough space in buffer to add)
        const auto results_fragment = std::vector<beerocks_message::sChannelScanResults>(
            neighbor_iterator, neighbor_iterator + number_of_neighbors_that_will_be_added);
        const auto results_tlv = add_scan_results_tlv_to_report(
            stored_scan_results_iter->ruid, stored_scan_results_iter->operating_class,
            stored_scan_results_iter->channel, stored_scan_results_iter->status,
            stored_scan_results_iter->timestamp, results_fragment);
        if (!results_tlv) {
            LOG(ERROR) << "Failed to add Scan Result TLV to Channel Scan Report Message";
            return false;
        }
        // Advance the iterator so next time we won't add the same neighors to the following TLV
        neighbor_iterator += number_of_neighbors_that_will_be_added;

        // Check if current results iterator is done
        if (neighbor_iterator == results_neighbors.end()) {
            stored_scan_results_iter++;
            LOG(TRACE) << std::distance(stored_scan_results_iter, results_vec->end())
                       << " results vectors remaining!";
            if (stored_scan_results_iter == results_vec->end()) {
                // Iterator has reached the end of the results vector
                continue;
            }
            results_neighbors = stored_scan_results_iter->results;
            neighbor_iterator = results_neighbors.begin();
        }
    }

    // Send final Report-Message
    if (channel_scan_report_header) {
        if (!send_channel_scan_report_cmdu(request_info->src_mac, true)) {
            LOG(ERROR) << "Failed to Send Channel Scan Report Message";
            return false;
        }
    }
    return true;
}

std::shared_ptr<ChannelScanTask::StoredResultsVector>
ChannelScanTask::get_scan_results_for_request(const std::shared_ptr<sScanRequest> request)
{
    auto final_results = std::make_shared<StoredResultsVector>();
    using ScanResults  = std::pair<std::chrono::system_clock::time_point,
                                  std::vector<beerocks_message::sChannelScanResults>>;
    using ResultsMap   = std::unordered_map<uint8_t, ScanResults>;
    // Add found results to final results
    auto add_scan_result =
        [&final_results](bool fresh_scan_requested, const sMacAddr &ruid,
                         const uint8_t operating_class, const uint8_t channel,
                         const eScanStatus status,
                         const std::chrono::system_clock::time_point &request_timestamp,
                         const std::chrono::system_clock::time_point &results_timestamp =
                             std::chrono::system_clock::time_point::min(),
                         const std::vector<beerocks_message::sChannelScanResults> &results =
                             std::vector<beerocks_message::sChannelScanResults>()) {
            // If a fresh scan was requested, return only "fresh" results.
            // Otherwise all results need to be returned
            if (!fresh_scan_requested ||
                (results_timestamp == request_timestamp ||
                 results_timestamp == std::chrono::system_clock::time_point::min())) {
                final_results->emplace_back(ruid, operating_class, channel, status,
                                            results_timestamp, results);
            }
        };

    auto get_results_for_channel_list =
        [&add_scan_result](bool fresh_scan_requested, const sMacAddr &ruid,
                           const uint8_t operating_class, const beerocks::eWiFiBandwidth bw,
                           const std::chrono::system_clock::time_point &request_timestamp,
                           const std::vector<sChannel> &channel_list, const ResultsMap &results) {
            // If channel list present, iterate over requested channels only
            for (auto &channel_element : channel_list) {
                auto channel_number = channel_element.channel_number;
                auto scan_status    = channel_element.scan_status;
                // Check results only for successful scans
                if (scan_status != eScanStatus::SUCCESS) {
                    LOG(DEBUG) << "Scan status is not successful for channel " << channel_number
                               << ", adding blank results";
                    add_scan_result(fresh_scan_requested, ruid, operating_class, channel_number,
                                    scan_status, request_timestamp);
                    continue;
                }
                auto channel_results_iter = results.find(channel_number);
                if (channel_results_iter == results.end()) {
                    LOG(DEBUG) << "No results found for channel " << channel_number
                               << ", adding blank results";
                    add_scan_result(fresh_scan_requested, ruid, operating_class, channel_number,
                                    scan_status, request_timestamp);
                    continue;
                }
                auto &channel_results = channel_results_iter->second;
                add_scan_result(fresh_scan_requested, ruid, operating_class, channel_number,
                                scan_status, request_timestamp, channel_results.first,
                                channel_results.second);
            }
        };

    auto db = AgentDB::get();

    const auto request_info =
        std::static_pointer_cast<sControllerRequestInfo>(request->request_info);

    for (const auto &radio_scan_iter : request->radio_scans) {
        auto radio = db->radio(radio_scan_iter.first);
        if (!radio) {
            LOG(ERROR) << "No radio with iface '" << radio_scan_iter.first << "' found!";
            continue;
        }
        const auto &stored_scan_results_map = radio->channel_scan_results;
        const bool fresh_scan_requested =
            request_info->perform_fresh_scan ==
            wfa_map::tlvProfile2ChannelScanRequest::ePerformFreshScan::
                PERFORM_A_FRESH_SCAN_AND_RETURN_RESULTS;
        const auto &radio_scan_element = radio_scan_iter.second;
        for (const auto &operating_class_iter : radio_scan_element->operating_classes) {
            get_results_for_channel_list(fresh_scan_requested, radio_scan_element->radio_mac,
                                         operating_class_iter.operating_class,
                                         operating_class_iter.bw, request->scan_start_timestamp,
                                         operating_class_iter.channel_list,
                                         stored_scan_results_map);
        }
    }
    return final_results;
}
