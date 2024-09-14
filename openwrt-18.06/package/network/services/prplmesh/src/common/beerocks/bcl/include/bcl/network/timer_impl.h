/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef BCL_NETWORK_TIMER_IMPL_H_
#define BCL_NETWORK_TIMER_IMPL_H_

#include <bcl/network/timer.h>

#include <bcl/network/file_descriptor_impl.h>

#include <sys/timerfd.h>

namespace beerocks {
namespace net {

/**
 * This class is an implementation of the Timer interface and a C++ wrapper for the
 * `timerfd_create(2)` and `timerfd_settime(2)` system calls. See the Linux manual page for
 * extended information.
 */
template <class TimeUnits = std::chrono::milliseconds> class TimerImpl : public Timer<TimeUnits> {
public:
    /**
     * @brief Class constructor.
     *
     * This implementation uses `timerfd_create()` to create a timer object that delivers timer
     * expiration notifications via a file descriptor.
     */
    TimerImpl() : m_descriptor(timerfd_create(CLOCK_MONOTONIC, 0)) {}

    /**
     * @brief Gets file descriptor.
     *
     * The timer delivers timer expiration notifications via a file descriptor.
     * The file descriptor may be monitored by select(2), poll(2), and epoll(7) system calls.
     *
     * @return File descriptor value.
     */
    int fd() override { return m_descriptor.fd(); }

    /**
     * @brief Schedules the timer for repeated fixed-delay execution, beginning after the
     * specified initial delay.
     *
     * @see Timer::schedule
     */
    bool schedule(TimeUnits delay, TimeUnits period) override
    {
        return set_time(
            {.it_interval = duration_to_timespec(period), .it_value = duration_to_timespec(delay)});
    }

    /**
     * @brief Terminates this timer.
     *
     * @see Timer::cancel
     */
    bool cancel() override { return set_time({}); }

    /**
     * @brief Reads the number of expirations that have occurred since the timer was scheduled or
     * last read (whatever happened last).
     *
     * @see Timer::read
     */
    bool read(uint64_t &number_of_expirations) override
    {
        ssize_t bytes_read = ::read(fd(), &number_of_expirations, sizeof(number_of_expirations));
        if (bytes_read != sizeof(number_of_expirations)) {
            LOG(ERROR) << "Unable to read timer: " << strerror(errno);
            return false;
        }

        return true;
    }

private:
    /**
     * File descriptor implementation (i.e.: wrapper to `int fd` that closes descriptor on
     * destructor).
     * To "favor composition over inheritance", the timer implementation does not extend the
     * FileDescriptor interface but aggregates this wrapper and delegates the calls to method fd()
     * to it.
     */
    FileDescriptorImpl m_descriptor;

    /**
     * @brief Converts given duration to a `timespec` structure.
     *
     * Both the interval and value fields used to configure the timer are set with `timespec`
     * structures.
     *
     * @param duration Time duration value in the units given by the class template argument.
     * @return timespec structure with a value equivalent to given duration.
     */
    timespec duration_to_timespec(TimeUnits duration)
    {
        auto seconds     = std::chrono::duration_cast<std::chrono::seconds>(duration);
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds);

        return timespec({.tv_sec  = static_cast<time_t>(seconds.count()),
                         .tv_nsec = static_cast<long>(nanoseconds.count())});
    }

    /**
     * @brief Arms (starts) or disarms (stops) the timer.
     *
     * @param value Initial expiration and interval for the timer.
     * @return true on success and false otherwise.
     */
    bool set_time(const itimerspec &value)
    {
        if (0 != timerfd_settime(fd(), 0, &value, nullptr)) {
            LOG(ERROR) << "Unable to set timer time: " << strerror(errno);
            return false;
        }

        return true;
    }
};

} // namespace net
} // namespace beerocks

#endif /* BCL_NETWORK_TIMER_IMPL_H_ */
