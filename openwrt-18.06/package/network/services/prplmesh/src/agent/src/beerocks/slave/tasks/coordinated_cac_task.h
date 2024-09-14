/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _COORDINATED_CAC_TASK_H_
#define _COORDINATED_CAC_TASK_H_

#include "../agent_db.h"
#include "task.h"
#include "task_messages.h"
#include <bcl/beerocks_state_machine.h>
#include <tlvf/CmduMessageTx.h>
#include <tlvf/wfa_map/tlvProfile2CacRequest.h>
#include <tlvf/wfa_map/tlvProfile2CacTermination.h>

namespace beerocks {
class BackhaulManager;
class TaskPoolInerface;

namespace coordinated_cac {

/////////
// fsm //
/////////
enum class fsm_state {
    IDLE,
    WAIT_FOR_SWITCH_CHANNEL_REPORT,
    WAIT_FOR_SWITCH_BACK_TO_ORIGINAL_CHANNEL_REPORT,
    WAIT_FOR_CAC_TERMINATION,
    ERROR
};
enum class fsm_event {
    CAC_REQUEST,
    CAC_TERMINATION_REQUEST,
    CAC_TERMINATION_RESPONSE,
    SWITCH_CHANNEL_REPORT,
    SWITCH_CHANNEL_DURATION_TIME,
    PERIODIC,
    CAC_COMPLETED_NOTIFICATION,
    CAC_STARTED_NOTIFICATION,
};

class CacFsm : public beerocks::beerocks_fsm<fsm_state, fsm_event> {
public:
    CacFsm()               = delete;
    CacFsm(const CacFsm &) = delete;
    CacFsm(TaskPoolInterface &task_pool, BackhaulManager &backhaul_manager,
           ieee1905_1::CmduMessageTx &cmdu_tx);
    ~CacFsm() {}

private:
    void config_fsm();
    bool send_preference_report();

    /* @brief Send switch channel request to the interface managed
     * by this fsm according to the given parameters.
     *
     * @param channel The channel to switch to
     * @param bandwidth The bandwidth to switch to
     * @return A pointer to the switch-channel-request that was sent or
     * nullptr if an error occurred.
     */
    std::shared_ptr<sSwitchChannelRequest>
    send_switch_channel_request(uint8_t channel, beerocks::eWiFiBandwidth bandwidth);
    bool send_cac_request(beerocks::AgentDB::sRadio *db_radio);
    void reset();
    bool is_timeout_waiting_for_switch_channel_report();
    bool is_timeout_waiting_for_cac_termination();

private:
    // environment
    TaskPoolInterface &m_task_pool;
    BackhaulManager &m_backhaul_manager;
    ieee1905_1::CmduMessageTx &m_cmdu_tx;

    std::shared_ptr<sSwitchChannelRequest> m_first_switch_channel_request = nullptr;

    // extracted form the request
    std::string m_ifname;
    wfa_map::tlvProfile2CacRequest::sCacRequestRadio m_cac_request_radio;

    // original values
    uint8_t m_original_channel                  = 0;
    eWiFiBandwidth m_original_bandwidth         = eWiFiBandwidth::BANDWIDTH_UNKNOWN;
    uint16_t m_original_center_frequency        = 0;
    uint8_t m_original_secondary_channel_offset = 0;

    // max time to wait for switch channel
    static constexpr std::chrono::seconds DEFAULT_MAX_WAIT_FOR_SWITCH_CHANNEL{3};
    std::chrono::seconds m_max_wait_for_switch_channel = DEFAULT_MAX_WAIT_FOR_SWITCH_CHANNEL;

    // the point in time we we started waiting for switch channel
    std::chrono::time_point<std::chrono::steady_clock> m_switch_channel_start_time_point;

    // max time to wait for cac termination
    static constexpr std::chrono::seconds DEFAULT_MAX_WAIT_FOR_CAC_TERMINATION{10};
    std::chrono::seconds m_max_wait_for_cac_termination = DEFAULT_MAX_WAIT_FOR_CAC_TERMINATION;

    // the point in time we we started waiting for cac termination
    std::chrono::time_point<std::chrono::steady_clock> m_terminate_cac_start_time_point;
};

//////////
// task //
//////////
class CoordinatedCacTask : public Task {
public:
    CoordinatedCacTask(TaskPoolInterface &task_pool, BackhaulManager &backhaul_manager,
                       ieee1905_1::CmduMessageTx &cmdu_tx);
    std::vector<eTaskEvent> get_task_event_list() const override;
    void work();
    void handle_event(uint8_t event_enum_value, const void *event_obj) override;
    void handle_event(eTaskEvent event, std::shared_ptr<void> event_obj) override;
    bool handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, uint32_t iface_index,
                     const sMacAddr &dst_mac, const sMacAddr &src_mac, int fd,
                     std::shared_ptr<beerocks_header> beerocks_header) override;

    bool handle_vendor_specific(ieee1905_1::CmduMessageRx &cmdu_rx, const sMacAddr &src_mac, int sd,
                                std::shared_ptr<beerocks_header> beerocks_header);

private:
    BackhaulManager &m_backhaul_manager;
    ieee1905_1::CmduMessageTx &m_cmdu_tx;
    CacFsm m_fsm;
};

} // namespace coordinated_cac
} // namespace beerocks

#endif
