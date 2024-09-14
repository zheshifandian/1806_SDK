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
#include "task_pool_interface.h"

#include <chrono>
#include <queue>
#include <unordered_map>

namespace beerocks {

/**
 * @brief The TaskPool is single instance tasks container which responsible to run each
 * task and allow passing messages and events from the Agent to the task.
 * 
 */
class TaskPool : public TaskPoolInterface {
public:
    void add_task(const std::shared_ptr<Task> new_task) override;

    /**
     * @brief Run all tasks on the pool, by calling each task work() function.
     * 
     * @param max_exec_duration_ms Maximal duration (in milliseconds) for tasks execution.
     * Tasks that won't be executed within this duration will be handled first on during
     * the next execution slot.
     */
    void run_tasks(int max_exec_duration_ms = 0);

    /**
     * @brief Send an 'event' defined on a specific task 'task_type'. 
     * 
     * @param task_type Task type, defined on Task base class.
     * @param event Event type, defined on the task itself.
     * @param event_obj Pointer to some chunk of memory used to pass data to the event handler.
     */
    void send_event(eTaskType task_type, uint8_t event, const void *event_obj = nullptr);

    void send_event(eTaskEvent event, std::shared_ptr<void> event_obj = nullptr) override;

    /**
     * @brief Iterate over all tasks on the pool and pass them the message on 'cmdu_rx'.
     * 
     * @param cmdu_rx CMDU object containing the received message to be handled.
     * @param iface_index Index of the network interface that the CMDU message was received on.
     * @param dst_mac Destination MAC address.
     * @param src_mac MAC address of the message sender.
     * @param fd File descriptor of the socket connection with the slave that sent the message.
     * @param beerocks_header Beerocks header (Only on VS message).
     * @return true if the message has been handled, otherwise false.
     */
    bool handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, uint32_t iface_index,
                     const sMacAddr &dst_mac, const sMacAddr &src_mac, int fd,
                     std::shared_ptr<beerocks_header> beerocks_header = nullptr);

private:
    /**
     * @brief Add new task to the task pool.
     * 
     * @param new_task Shared pointer to the task.
     * @return true on success, otherwise false.
     */
    bool add_task_no_events(const std::shared_ptr<Task> &new_task);

private:
    // Decalaring unordered_map with key which is an enum, does not compiles on older gcc version.
    // It was considered a defect in the standard, and was fixed in C++14, and also fixed in the
    // version of libstdc++ shipping with gcc as of 6.1.
    // To make unordered_map work with an enum as key, std::hash<int> function was added as third
    // template argument.
    std::unordered_map<eTaskType, std::shared_ptr<Task>, std::hash<int>> m_task_pool;

private:
    // map each event to the tasks that want to handle it
    std::unordered_multimap<eTaskEvent, std::shared_ptr<Task>, TaskEventHash> m_event_to_tasks_map;

    // queue of events to handle
    std::queue<std::pair<eTaskEvent, std::shared_ptr<void>>> m_event_queue;

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

} // namespace beerocks
#endif // _TASK_POOL_H_
