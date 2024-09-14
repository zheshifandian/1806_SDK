
/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _TOPOLOGY_TASK_H_
#define _TOPOLOGY_TASK_H_

#include "../db/db.h"
#include "task.h"
#include "task_pool.h"

#include <beerocks/tlvf/beerocks_message.h>

namespace son {

class topology_task : public task {

public:
    topology_task(db &database_, ieee1905_1::CmduMessageTx &cmdu_tx_, task_pool &tasks_);
    virtual ~topology_task() {}
    bool handle_ieee1905_1_msg(const sMacAddr &src_mac,
                               ieee1905_1::CmduMessageRx &cmdu_rx) override;

protected:
    virtual void work() override;

private:
    /**
    * @brief Handles 1905 Topology Response message.
    * 
    * @param[in] cmdu_rx Received CMDU.
    * @param[in] src_mac MAC address of the message sender.
    * @return True if message, was successfully processed, false otherwise.
    */
    bool handle_topology_response(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
    * @brief Handles 1905 Topology Notification message.
    * 
    * @param[in] cmdu_rx Received CMDU.
    * @param[in] src_mac MAC address of the message sender.
    * @return True if message, was successfully processed, false otherwise.
    */
    bool handle_topology_notification(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);

    /**
     * Remove not reported neighbors.
     * 
     * @param src_mac MAC address of the message sender.
     * @param al_mac Al mac from Device Information TLV.
     * @param reported_neighbor_al_macs Set of Al macs from reported neighbors.
     */
    void handle_dead_neighbors(const sMacAddr &src_mac, const sMacAddr &al_mac,
                               std::unordered_set<sMacAddr> reported_neighbor_al_macs);

    db &database;
    ieee1905_1::CmduMessageTx &cmdu_tx;
    task_pool &tasks;
    std::unordered_map<sMacAddr, std::chrono::steady_clock::time_point> recently_reported_neighbors;
};

} // namespace son

#endif
