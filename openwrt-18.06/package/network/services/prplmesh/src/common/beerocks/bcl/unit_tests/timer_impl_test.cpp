/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/timer_impl.h>

#include <bcl/beerocks_event_loop_impl.h>

#include <gtest/gtest.h>

namespace {

TEST(TimerImplTest, timer_should_succeed)
{
    const auto delay              = std::chrono::milliseconds(500);
    const auto period             = std::chrono::milliseconds(100);
    const auto total_time         = std::chrono::seconds(1);
    const uint32_t expected_count = 6;

    uint32_t count = 0;
    auto start     = std::chrono::steady_clock::now();

    beerocks::EventLoopImpl event_loop;

    beerocks::net::TimerImpl<> timer;
    timer.schedule(delay, period);

    beerocks::EventLoop::EventHandlers handlers;
    handlers.on_read = [&](int fd, beerocks::EventLoop &loop) {
        uint64_t number_of_expirations;
        if (!timer.read(number_of_expirations)) {
            return false;
        }

        count++;

        auto now          = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

        if (elapsed_time >= total_time) {
            timer.cancel();
            return false;
        }

        return true;
    };

    event_loop.register_handlers(timer.fd(), handlers);
    while (event_loop.run() > 0) {
    };
    event_loop.remove_handlers(timer.fd());

    ASSERT_EQ(expected_count, count);
}

TEST(TimerImplTest, timer_should_succeed_with_microseconds)
{
    const auto delay              = std::chrono::microseconds(500000);
    const auto period             = std::chrono::microseconds(100000);
    const auto total_time         = std::chrono::seconds(1);
    const uint32_t expected_count = 6;

    uint32_t count = 0;
    auto start     = std::chrono::steady_clock::now();

    beerocks::EventLoopImpl event_loop;

    beerocks::net::TimerImpl<std::chrono::microseconds> timer;
    timer.schedule(delay, period);

    beerocks::EventLoop::EventHandlers handlers;
    handlers.on_read = [&](int fd, beerocks::EventLoop &loop) {
        uint64_t number_of_expirations;
        if (!timer.read(number_of_expirations)) {
            return false;
        }

        count++;

        auto now          = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

        if (elapsed_time >= total_time) {
            timer.cancel();
            return false;
        }

        return true;
    };

    event_loop.register_handlers(timer.fd(), handlers);
    while (event_loop.run() > 0) {
    };
    event_loop.remove_handlers(timer.fd());

    ASSERT_EQ(expected_count, count);
}
} // namespace
