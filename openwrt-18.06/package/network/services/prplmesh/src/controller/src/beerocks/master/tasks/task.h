/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _TASK_H_
#define _TASK_H_

#define TASK_LOG(LEVEL) (LOG(LEVEL) << "task " << task_name << " id " << id << ": ")
#define TASK_LOG_IF(condition, LEVEL)                                                              \
    LOG_IF(condition, LEVEL) << "task " << task_name << " id " << id << ": "

#include <beerocks/tlvf/beerocks_message.h>
#include <beerocks/tlvf/beerocks_message_control.h>

#include <chrono>
#include <memory>
#include <set>
#include <utility>

namespace son {

class task {

public:
    task(const std::string &task_name_ = std::string(),
         const std::string &node_mac   = std::string());
    virtual ~task() {}
    void execute();
    void response_received(std::string mac,
                           std::shared_ptr<beerocks::beerocks_header> beerocks_header);
    void event_received(int event_type, void *obj = nullptr);
    void pending_task_ended(int task_id);
    bool is_done();
    void kill();

    /**
     * @brief Handle ieee1905 message.
     *
     * @param src_mac MAC address of the message sender.
     * @param cmdu_rx CMDU object containing the received message to be handled.
     * @return True if the message has been handled, otherwise false.
     */
    virtual bool handle_ieee1905_1_msg(const sMacAddr &src_mac, ieee1905_1::CmduMessageRx &cmdu_rx)
    {
        return false;
    }

    void set_last_exec_time(std::chrono::steady_clock::time_point time) { last_exec_time = time; }
    std::chrono::steady_clock::time_point get_last_exec_time() { return last_exec_time; }

    std::string task_name;
    const std::string assigned_node;
    const int id;
    const std::chrono::steady_clock::time_point start_timestamp;

protected:
    void add_pending_macs(std::set<std::string> macs,
                          beerocks_message::eActionOp_CONTROL action_op);
    void add_pending_mac(const std::string &mac, beerocks_message::eActionOp_CONTROL action_op);
    void clear_pending_macs();
    void wait_for(int ms);
    void set_task_timeout(int ms);
    void set_responses_timeout(int ms);
    void set_events_timeout(int ms);
    void wait_for_event(int event);
    void wait_for_task_end(int id, int ms);
    void clear_pending_events();
    void finish();

    virtual void work() = 0;
    virtual void handle_events_timeout(std::multiset<int> pending_events) {}
    virtual void handle_pending_task_timeout(int task_id) {}
    virtual void handle_task_end() {}
    virtual void handle_event(int event_type, void *obj) {}
    virtual void handle_response(std::string slave_mac,
                                 std::shared_ptr<beerocks::beerocks_header> beerocks_header)
    {
    }
    virtual void handle_responses_timeout(
        std::unordered_multimap<std::string, beerocks_message::eActionOp_CONTROL> timed_out_macs)
    {
    }

private:
    bool done = false;
    std::unordered_multimap<std::string, beerocks_message::eActionOp_CONTROL> pending_macs;
    bool waiting               = false;
    bool responses_timeout_set = false;
    bool waiting_for_responses = false;
    std::chrono::steady_clock::time_point responses_timeout;
    bool task_timeout_set = false;

    std::chrono::steady_clock::time_point events_timeout;
    std::multiset<int> pending_events;
    bool events_timeout_set = false;
    bool waiting_for_events = false;

    std::chrono::steady_clock::time_point pending_task_timeout;
    int pending_task_id           = -1;
    bool waiting_for_pending_task = false;

    std::chrono::steady_clock::time_point task_timeout;
    std::chrono::steady_clock::time_point next_action_time;
    std::chrono::steady_clock::time_point last_exec_time;

    static int latest_id;
};

} // namespace son

#endif
