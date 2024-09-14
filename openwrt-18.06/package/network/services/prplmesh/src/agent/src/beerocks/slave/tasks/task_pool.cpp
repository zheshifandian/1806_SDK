/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2021 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "task_pool.h"

#include <algorithm>
#include <chrono>

using namespace beerocks;

bool TaskPool::add_task_no_events(const std::shared_ptr<Task> &new_task)
{
    if (!new_task) {
        LOG(ERROR) << "new task pointer is nullptr";
        return false;
    }
    m_task_pool[new_task->get_task_type()] = new_task;
    return true;
}

void TaskPool::add_task(const std::shared_ptr<Task> new_task)
{
    // escape
    LOG_IF(!new_task, FATAL);

    // get the events the new task wants to handle
    const auto &events = new_task->get_task_event_list();

    // insert into event-to-task map
    for (const auto &event : events) {
        m_event_to_tasks_map.emplace(event, new_task);
    }

    // insert into regular list
    add_task_no_events(new_task);
}

void TaskPool::run_tasks(int max_exec_duration_ms)
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

    // First, empty the queue of messages
    // by sending each one of them to the task that is registered
    // Note: tasks may _add_ more events to the queue in their handle_event().
    // calling handle_event() that itslef send_event() may potentially end
    // with never emptied queue: empty and filling it forever.
    // However, we don't expect such behavior of the system.
    // Possible solution if we encounter this:
    // * work with two queues: one to push to and the
    // second to pop from. Each cycle switch between them.

    // first, empty the queue
    while (!m_event_queue.empty()) {
        auto &event = m_event_queue.front();
        auto tasks  = m_event_to_tasks_map.equal_range(event.first);

        std::for_each(tasks.first, tasks.second,
                      [&](std::pair<const eTaskEvent, std::shared_ptr<Task>> event_task) {
                          if (event_task.second) {
                              event_task.second->handle_event(event.first, event.second);

                              // Increment the number of processed events and check if any limit has been reached
                              if (std::chrono::steady_clock::now() >= exec_deadline_time) {
                                  m_exec_iteration_slots++;
                                  return;
                              }
                          }
                      });
        m_event_queue.pop();
    }

    // second, do the work for all tasks
    for (auto &task_element : m_task_pool) {
        // If the maximal execution time in a single execution slot is reached
        if (std::chrono::steady_clock::now() >= exec_deadline_time) {
            m_exec_iteration_slots++;
            return;
        }

        auto &task = task_element.second;

        // Execute the task and update the iteration execution time
        task->work();
        task->set_last_exec_time(m_exec_iteration_start_time);
    }

    // If the iteration was completed in more than one slot, print a warning
    if (m_exec_iteration_slots) {
        auto total_iter_exec_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_exec_iteration_start_time);

        LOG(DEBUG) << "Task pool execution iteration completed in " << m_exec_iteration_slots + 1
                   << " slots of " << max_exec_duration_ms
                   << "ms each, with a total execution time of " << total_iter_exec_time.count()
                   << "ms. Number of tasks in pool: " << m_task_pool.size();
    }

    // Execution iteration completed, reset the state
    m_exec_iteration_start_time = std::chrono::steady_clock::time_point::max();
}

void TaskPool::send_event(eTaskType task_type, uint8_t event, const void *event_obj)
{
    auto task_it = m_task_pool.find(task_type);
    if (task_it == m_task_pool.end()) {
        LOG(ERROR) << "task of type " << int(task_type) << " does not exist in the task_pool";
        return;
    }

    auto &task = task_it->second;
    task->handle_event(event, event_obj);
}

void TaskPool::send_event(eTaskEvent event, std::shared_ptr<void> event_obj)
{
    m_event_queue.push(std::make_pair(event, event_obj));
}

bool TaskPool::handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, uint32_t iface_index,
                           const sMacAddr &dst_mac, const sMacAddr &src_mac, int fd,
                           std::shared_ptr<beerocks_header> beerocks_header)
{
    for (auto &task_element : m_task_pool) {
        auto &task = task_element.second;
        if (task->handle_cmdu(cmdu_rx, iface_index, dst_mac, src_mac, fd, beerocks_header)) {
            return true;
        }
    }
    return false;
}
