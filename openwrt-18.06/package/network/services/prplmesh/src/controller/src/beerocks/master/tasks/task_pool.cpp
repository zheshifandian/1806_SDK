/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "task_pool.h"

#include <easylogging++.h>

using namespace beerocks;
using namespace son;

bool task_pool::add_task(std::shared_ptr<task> new_task)
{
    if (!new_task) {
        LOG(ERROR) << "task to add is null";
        return false;
    }

    LOG(TRACE) << "inserting new task, id=" << int(new_task->id)
               << " task_name=" << new_task->task_name;
    return (m_scheduled_tasks.insert(std::make_pair(new_task->id, new_task))).second;
}

bool task_pool::is_task_running(int id)
{
    auto it = m_scheduled_tasks.find(id);
    if (it != m_scheduled_tasks.end() && it->second != nullptr && !it->second->is_done()) {
        return true;
    } else {
        return false;
    }
}

void task_pool::kill_task(int id)
{
    auto it = m_scheduled_tasks.find(id);
    if (it != m_scheduled_tasks.end() && it->second != nullptr) {
        LOG(DEBUG) << "killing task " << it->second->task_name << ", id " << it->first;
        it->second->kill();
    }
}

void task_pool::push_event(int task_id, int event_type, void *obj)
{
    auto it = m_scheduled_tasks.find(task_id);
    if (it != m_scheduled_tasks.end()) {
        if (it->second != nullptr) {
            it->second->event_received(event_type, obj);
        } else {
            LOG(ERROR) << "invalid task " << task_id;
        }
    } else {
        LOG(ERROR) << "can't find task " << task_id;
    }
}

void task_pool::pending_task_ended(int task_id)
{
    //TODO find a more efficient way for this
    for (auto t : m_scheduled_tasks) {
        t.second->pending_task_ended(task_id);
    }
}

void task_pool::response_received(std::string mac,
                                  std::shared_ptr<beerocks::beerocks_header> beerocks_header)
{
    auto got = m_scheduled_tasks.find(beerocks_header->id());
    if (got != m_scheduled_tasks.end()) {
        got->second->response_received(mac, beerocks_header);
    }
}

void task_pool::run_tasks(int max_exec_duration_ms)
{
    // Check if a new pool iteration should be started
    if (m_exec_iteration_start_time > std::chrono::steady_clock::now()) {
        m_exec_iteration_start_time = std::chrono::steady_clock::now();
        m_exec_iteration_slots      = 0;
    }

    // Calculate the execution deadline time point
    auto exec_deadline_time =
        (max_exec_duration_ms)
            ? std::chrono::steady_clock::now() + std::chrono::milliseconds(max_exec_duration_ms)
            : std::chrono::steady_clock::time_point(std::chrono::milliseconds::max());

    for (auto it = m_scheduled_tasks.begin(); it != m_scheduled_tasks.end();) {
        // If the maximal execution time in a single execution slot is reached
        if (std::chrono::steady_clock::now() >= exec_deadline_time) {
            m_exec_iteration_slots++;
            return;
        }

        // Skip tasks that were already executed in this iteration
        if (it->second->get_last_exec_time() == m_exec_iteration_start_time) {
            ++it;
            continue;
        }

        // Execute the task and update the iteration execution time
        it->second->execute();
        it->second->set_last_exec_time(m_exec_iteration_start_time);

        if (it->second->is_done()) {
            pending_task_ended(it->first);
            LOG(DEBUG) << "Erasing task " << it->second->task_name << ", id " << it->first;
            it = m_scheduled_tasks.erase(it);
        } else {
            ++it;
        }
    }

    // If the iteration was completed in more than one slot, print a warning
    if (m_exec_iteration_slots) {
        auto total_iter_exec_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_exec_iteration_start_time);

        LOG(DEBUG) << "Task pool execution iteration completed in " << m_exec_iteration_slots + 1
                   << " slots of " << max_exec_duration_ms
                   << "ms each, with a total execution time of " << total_iter_exec_time.count()
                   << "ms. Number of tasks in pool: " << m_scheduled_tasks.size();
    }

    // Execution iteration completed, reset the state
    m_exec_iteration_start_time = std::chrono::steady_clock::time_point::max();
}

void task_pool::handle_ieee1905_1_msg(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx)
{
    if (cmdu_rx.getMessageType() == ieee1905_1::eMessageType::VENDOR_SPECIFIC_MESSAGE) {
        LOG(DEBUG) << "Message with mid: " << cmdu_rx.getMessageId()
                   << " is VENDOR_SPECIFIC message.";
        return;
    }
    for (auto &task_element : m_scheduled_tasks) {
        auto &task = task_element.second;
        if (task->handle_ieee1905_1_msg(src_mac, cmdu_rx)) {
            LOG(DEBUG) << "Handled message " << (uint16_t)cmdu_rx.getMessageType()
                       << " with mid: " << cmdu_rx.getMessageId() << " by " << task->task_name;
        }
    }
}
