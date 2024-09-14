/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _TASK_MESSAGES_H_
#define _TASK_MESSAGES_H_

#include <beerocks/tlvf/beerocks_message_backhaul.h>
#include <tlvf/CmduMessageTx.h>

namespace beerocks {

/*
 * @brief A message indicating how long the switch-channel is expected to take,
 * used for example when cac-started is handled and the cac-time is taken from it.
 * When another task is asking for switch-channel (with switch-channel-request)
 * it doesn't know how long to wait for the switch channel to happen.
 * This message reports about the time it should take. receiving this message
 * enables the caller to update its timeout before taking failure actions.
 * With this message, the caller is careless about the actions taken to perform
 * a switch-channel nor is concerned about cac or anything. it needs just to wait
 * the time it is set in the duration_sec parameter before deciding on timeout.
 *
 * @param ifname The interface name this message relates to.
 * @param duration_sec The time in seconds there is to wait for the switch channel.

 */
struct sSwitchChannelDurationTime {
    std::string ifname;
    std::chrono::seconds duration_sec;
};
std::ostream &operator<<(std::ostream &os,
                         const sSwitchChannelDurationTime &switch_channel_duration_time);

/*
 * @brief A message a indicating a switch channel happned
 * with indication if the switch really happened.  *
 * (currently originated from csa-notification and csa-error-notification)
 *
 * @param ifname The interface name this message relates to.
 * @param ap_channel_switch Values related to the channel.
 * @param switched Boolean indication if really switched or error occurred.
 */
struct sSwitchChannelNotification {
    std::string ifname;
    beerocks_message::sApChannelSwitch ap_channel_switch;
    bool switched;
};

/*
 * @brief Data related to cac-started notification.
 *
 * @param ifname The interface name this message relates to.
 * @param cac_started_params The parameters related to cac-started.
 */
struct sCacStartedNotification {
    std::string ifname;
    beerocks_message::sCacStartedNotificationParams cac_started_params;
};

/*
 * @brief Data related to cac-completed notification.
 *
 * @param ifname The interface name this message relates to.
 * @param cac_completed_param The parameters related to cac-started.
 */
struct sCacCompletedNotification {
    std::string ifname;
    beerocks_message::sDfsCacCompleted cac_completed_params;
};

/*
 * @brief A request to switch a channel.
 *
 * @param ifname The interface name this message relates to.
 * @param channel The channel to switch to.
 * @param bandwidth The bandwidth to use.
 */
struct sSwitchChannelRequest {
    sSwitchChannelRequest();
    sSwitchChannelRequest(const sSwitchChannelRequest &rhs);

    std::string ifname;
    uint8_t channel          = 0;
    eWiFiBandwidth bandwidth = eWiFiBandwidth::BANDWIDTH_UNKNOWN;
};
std::ostream &operator<<(std::ostream &os, const sSwitchChannelRequest &switch_channel_request);

/*
 * @brief Possible end status of switch channel request.
 */
enum class eSwitchChannelReportStatus {
    SUCCESS,
    GENERAL_FAIL,
    NO_SWITCH_CHANNEL_NOTIFICATION,
    ANOTHER_SWITCH_IN_PROGRESS,
    CAC_FAILED,
    CSA_FAILED,
    NOT_COMPUTED
};

/*
 * @brief Switch channel report.
 *
 * @param cac_happened Indicates if cac happened during switch channel.
 * @param original_request The original request that triggered the switch channel.
 * @param switch_channel_notification The data related to the switch channel coming from the driver
 * relevant if cac didn't happen.
 * @param cac_started_notification The cac started data coming from the driver when cac started.
 * @param cac_completed_notification The cac completed data coming from the driver when cac completed.
 * @param status The status of the report. If not set may be computed by the struct itself
 * based on the values of other members.
 */
struct sSwitchChannelReport {
    sSwitchChannelReport();
    sSwitchChannelReport(
        bool cac_happened_, std::shared_ptr<const sSwitchChannelRequest> original_request_,
        std::shared_ptr<sSwitchChannelNotification> switch_channel_notification_,
        std::shared_ptr<sCacStartedNotification> cac_started_notification_,
        std::shared_ptr<sCacCompletedNotification> cac_completed_notification_,
        eSwitchChannelReportStatus status = eSwitchChannelReportStatus::NOT_COMPUTED);

    /*
     * @brief Based on the various values of the report
     * calculates its status. 
     *
     * @return The computed eSwitchChannelReportStatus.
     */
    eSwitchChannelReportStatus compute_switch_channel_report_status();

    bool cac_happened;
    std::shared_ptr<const sSwitchChannelRequest> original_request;
    std::shared_ptr<sSwitchChannelNotification> switch_channel_notification;
    std::shared_ptr<sCacStartedNotification> cac_started_notification;
    std::shared_ptr<sCacCompletedNotification> cac_completed_notification;
    eSwitchChannelReportStatus status;
};
std::ostream &operator<<(std::ostream &os, const sSwitchChannelReport &switch_channel_report);

} // namespace beerocks

#endif
