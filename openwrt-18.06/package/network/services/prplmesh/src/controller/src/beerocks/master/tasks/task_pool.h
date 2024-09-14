/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _TASK_POOL_H_
#define _TASK_POOL_H_

#include "task.h"

#include <beerocks/tlvf/beerocks_message_action.h>

namespace son {

class task_pool {

public:
    task_pool() {}
    ~task_pool() {}

    bool add_task(std::shared_ptr<task> new_task);
    bool is_task_running(int id);
    void kill_task(int id);
    void push_event(int task_id, int event_type, void *obj = nullptr);
    void response_received(std::string mac,
                           std::shared_ptr<beerocks::beerocks_header> beerocks_header);
    void pending_task_ended(int task_id);
    void run_tasks(int max_exec_duration_ms = 0);

    /**
     * @brief Handle ieee1905 message.
     *
     * @param src_mac MAC address of the message sender.
     * @param cmdu_rx CMDU object containing the received message to be handled.
     */
    void handle_ieee1905_1_msg(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx);

private:
    std::map<int, std::shared_ptr<task>> m_scheduled_tasks;

    /**
     * Used to mark the beginning of an execution iteration.
     * Each iteration can be span over multiple execution slots.
     */
    std::chrono::steady_clock::time_point m_exec_iteration_start_time =
        std::chrono::steady_clock::time_point::max();

    /**
     * Counts the number of slots it took to process all the tasks within
     * the current execution iteration.
     */
    int m_exec_iteration_slots = 0;
};

} // namespace son

#endif
