/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_TIMER_H_
#define BCL_NETWORK_TIMER_H_

#include "sockets.h"

#include <bcl/beerocks_backport.h>

namespace beerocks {
namespace net {

/**
 * Timer interface to deal with periodic tasks and timeouts.
 *
 * This interface extends the FileDescriptor interface. The file descriptor will be used to install
 * event handlers in an EventLoop instance, pretty much like we do with sockets. This way, a single
 * thread can block waiting for I/O events in a set of sockets at the same time that it waits for
 * timer events to occur.
 *
 * Code is programmed to interfaces so it does not care about which implementation is used.
 * Unit tests can use a mock and set different expectations per test. And what is more important in
 * this case, unit tests can emulate that a timer has elapsed without actually waiting at all,
 * allowing for quick test execution, no matter the delay, the period or the number of repetitions.
 */
template <class TimeUnits = std::chrono::milliseconds> class Timer : public FileDescriptor {
    // Fail the build if TimeUnits is not derived from std::chrono::duration
    static_assert(is_chrono_duration<TimeUnits>::value,
                  "T must be derived from std::chrono::duration");

public:
    /**
     * @brief Schedules the timer for repeated fixed-delay execution, beginning after the
     * specified initial delay.
     *
     * @param delay Delay before timer elapses for the first time.
     * @param period Time between successive timer executions. Set to 0 for a one-shot timer.
     * @return true on success and false otherwise.
     */
    virtual bool schedule(TimeUnits delay, TimeUnits period) = 0;

    /**
     * @brief Terminates this timer.
     *
     * This method may be called repeatedly; the second and subsequent calls have no effect.
     *
     * @return true on success and false otherwise.
     */
    virtual bool cancel() = 0;

    /**
     * @brief Reads the number of expirations that have occurred since the timer was scheduled or
     * last read (whatever happened last).
     *
     * @param number_of_expirations The number of expirations occurred.
     * @return true on success and false otherwise.
     */
    virtual bool read(uint64_t &number_of_expirations) = 0;
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_TIMER_IMPL_H_ */
