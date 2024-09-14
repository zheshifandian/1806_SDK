/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_THREAD_BASE_H_
#define _BEEROCKS_THREAD_BASE_H_

#define THREAD_LOG(a) (LOG(a) << get_name() << ": ")

#include <atomic>
#include <string>
#include <thread>

namespace beerocks {
class thread_base {
public:
    thread_base() : should_stop(false), worker_is_running(false) {}
    virtual ~thread_base();
    bool start(std::string name = "");
    void join();
    void stop(bool block = true);
    bool is_running() { return worker_is_running; }
    std::string get_name() { return thread_name; }

protected:
    std::string thread_name;
    bool should_stop;

    /**
     * @brief Perform initialization of the Class which will be done @b outside the thread context.
     *
     * @return true on success, otherwise false.
     */
    virtual bool init() { return true; }

    /**
     * @brief Perform initialization of the Class which will be done @b inside the thread context.
     *
     * @return true on success, otherwise false.
     */
    virtual bool thread_init() = 0;

    virtual bool work() = 0;
    virtual void before_stop() {}
    virtual void on_thread_stop() {}

private:
    void run();
    std::thread worker;
    bool worker_is_running;
};
} // namespace beerocks

#endif // _BEEROCKS_THREAD_BASE_H_
