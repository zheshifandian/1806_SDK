/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _BEEROCKS_TIMER_MANAGER_IMPL_H_
#define _BEEROCKS_TIMER_MANAGER_IMPL_H_

#include <bcl/beerocks_timer_factory.h>
#include <bcl/beerocks_timer_manager.h>

#include <bcl/network/timer.h>

#include <chrono>
#include <memory>
#include <unordered_map>

namespace beerocks {

/**
 * @brief The timer manager is a helper component to facilitate the use of timers in an application.
 *
 * @see TimerManager
 *
 * When a new timer is requested, the timer manager creates a new instance using the timer factory
 * provided in constructor. A factory is used instead of directly creating the timers to avoid
 * dependencies on OS resources and thus facilitate unit testing.
 *
 * When a new timer is created, the timer manager register handlers for it in the event loop
 * provided in constructor. This way an application can have a single point of blocking on I/O
 * events, including waiting for scheduled timers to elapse.
 *
 * This implementation holds a list of timers currently created and removes them automatically on
 * destructor.
 */
class TimerManagerImpl : public TimerManager {
public:
    /**
     * @brief Class constructor
     *
     * @param timer_factory Timer factory used by the timer manager to create instances of timers.
     * @param event_loop Application event loop used by the application to wait for I/O events.
     */
    TimerManagerImpl(std::shared_ptr<TimerFactory> timer_factory,
                     std::shared_ptr<EventLoop> event_loop);

    /**
     * Default destructor.
     */
    ~TimerManagerImpl() override;

    /**
     * @brief Adds a new timer with given schedule.
     *
     * @see TimerManager::add_timer
     *
     * In this implementation, adding a new timer is a 4 step process:
     * - Create a new timer instance using the timer factory provided in constructor.
     * - Register given handler to be executed when the timer elapses in the application event loop.
     * - Schedule the timer with given frequency (delay and period).
     * - Add the timer to the list of timers (so it can be automatically removed in destructor).
     */
    int add_timer(const std::string &timer_name, std::chrono::milliseconds delay,
                  std::chrono::milliseconds period,
                  const EventLoop::EventHandler &handler) override;

    /**
     * @brief Removes previously created timer.
     *
     * In this implementation, removing an existing timer is a 4 step process:
     * - Find the new timer in the list of currently running timers.
     * - Cancel the timer.
     * - Remove handlers registered in the event loop for the timer.
     * - Remove the timer from the list of timers.
     *
     * @see TimerManager::remove_timer
     */
    bool remove_timer(int &fd) override;

private:
    /**
     * @brief Handles the read event in an elapsed timer.
     *
     * @param fd File descriptor of the timer.
     * @return true on success and false otherwise.
     */
    bool handle_read(int fd);

    /**
     * Timer factory used by the timer manager to create instances of timers.
     */
    std::shared_ptr<TimerFactory> m_timer_factory;

    /**
     * Application event loop used by the application to wait for I/O events.
     */
    std::shared_ptr<EventLoop> m_event_loop;

    /**
     * List of timers currently being managed.
     */
    std::unordered_map<int, std::unique_ptr<beerocks::net::Timer<>>> m_timers;
};

} // namespace beerocks

#endif // _BEEROCKS_TIMER_MANAGER_IMPL_H_
