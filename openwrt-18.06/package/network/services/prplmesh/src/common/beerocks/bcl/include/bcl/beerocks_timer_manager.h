/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_TIMER_MANAGER_H_
#define _BEEROCKS_TIMER_MANAGER_H_

#include <bcl/beerocks_event_loop.h>

#include <chrono>

namespace beerocks {

/**
 * @brief The timer manager is a helper component to facilitate the use of timers in an application.
 */
class TimerManager {
public:
    /**
     * Default destructor.
     */
    virtual ~TimerManager() = default;

    /**
     * @brief Adds a new timer with given schedule.
     *
     * The file descriptor value returned by this method is required to remove the timer when no
     * longer used.
     *
     * @param timer_name Time name.
     * @param delay Delay before timer elapses for the first time.
     * @param period Time between successive timer executions. Set to 0 for a one-shot timer.
     * @param handler Handler function to be called back when timer elapses.
     * @return On success, file descriptor of the timer object created and -1 on error.
     */
    virtual int add_timer(const std::string &timer_name, std::chrono::milliseconds delay,
                          std::chrono::milliseconds period,
                          const EventLoop::EventHandler &handler) = 0;

    /**
     * @brief Removes previously created timer.
     *
     * On success, sets given file descriptor to `invalid_descriptor`.
     * 
     * @param[in,out] fd File descriptor of the timer (obtained when the timer was added).
     * @return true on success and false otherwise.
     */
    virtual bool remove_timer(int &fd) = 0;
};

} // namespace beerocks

#endif // _BEEROCKS_TIMER_MANAGER_H_
