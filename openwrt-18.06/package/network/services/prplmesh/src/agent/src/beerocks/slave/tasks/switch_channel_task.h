/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _SWITCH_CHANNEL_TASK_H_
#define _SWITCH_CHANNEL_TASK_H_

#include "task.h"
#include "task_messages.h"
#include "task_pool_interface.h"
#include <beerocks/tlvf/beerocks_message_backhaul.h>

namespace beerocks {

class BackhaulManager;

namespace switch_channel {

class SwitchChannelFsm;

/////////////////////////
// switch channel task //
/////////////////////////
class SwitchChannelTask : public Task {
public:
    SwitchChannelTask(TaskPoolInterface &task_pool, BackhaulManager &backhaul_manager,
                      ieee1905_1::CmduMessageTx &cmdu_tx);
    ~SwitchChannelTask();

    std::vector<eTaskEvent> get_task_event_list() const override;

    void work() override;

    void handle_event(eTaskEvent event, std::shared_ptr<void> event_obj) override;

    bool handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, uint32_t iface_index,
                     const sMacAddr &dst_mac, const sMacAddr &src_mac, int fd,
                     std::shared_ptr<beerocks_header> beerocks_header) override
    {
        return false;
    }

private:
    bool request_switch_channel();
    /*
     * @brief Returns a pointer to the fsm that manages this interface.

     * @param ifname The name of the interface to manage (e.g. wlan2).
     *
     * @return A pointer to the fsm that manages the given input.
     */
    SwitchChannelFsm &get_fsm_by_ifname(const std::string &ifname);

private:
    TaskPoolInterface &m_task_pool;
    BackhaulManager &m_backhaul_manager;
    ieee1905_1::CmduMessageTx &m_cmdu_tx;

    // map interface name to the fsm that manages it
    std::map<std::string, std::unique_ptr<SwitchChannelFsm>> m_fsms;
};

} // namespace switch_channel
} // namespace beerocks

#endif //_SWITCH_CHANNEL_TASK_H_
