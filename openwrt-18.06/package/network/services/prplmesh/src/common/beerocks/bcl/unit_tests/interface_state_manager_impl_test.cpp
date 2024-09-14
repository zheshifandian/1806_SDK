/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/interface_state_manager_impl.h>
#include <bcl/network/interface_state_monitor_mock.h>
#include <bcl/network/interface_state_reader_mock.h>

#include <bcl/beerocks_backport.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Invoke;
using ::testing::StrictMock;

/**
 * In this test, no state-changed event has occurred yet so interface state is obtained with an
 * explicit read through the interface state reader.
 */
TEST(InterfaceStateManagerImpl, read_state_should_succeed_before_event)
{
    auto monitor = std::make_unique<StrictMock<beerocks::net::InterfaceStateMonitorMock>>();
    auto reader  = std::make_unique<StrictMock<beerocks::net::InterfaceStateReaderMock>>();

    const char *iface_name          = "test_iface";
    const bool expected_iface_state = true;
    bool actual_iface_state         = false;

    EXPECT_CALL(*reader, read_state(iface_name, _))
        .WillOnce(Invoke([&](const std::string &iface_name, bool &iface_state) -> bool {
            iface_state = expected_iface_state;
            return true;
        }));

    beerocks::net::InterfaceStateManagerImpl interface_state_manager(std::move(monitor),
                                                                     std::move(reader));

    ASSERT_TRUE(interface_state_manager.read_state(iface_name, actual_iface_state));
    ASSERT_EQ(actual_iface_state, expected_iface_state);
}

/**
 * This test demonstrates that state is cached and only one explicit read is performed through the
 * interface state reader no matter how many times it is queried.
 */
TEST(InterfaceStateManagerImpl, read_state_should_succeed_after_read)
{
    auto monitor = std::make_unique<StrictMock<beerocks::net::InterfaceStateMonitorMock>>();
    auto reader  = std::make_unique<StrictMock<beerocks::net::InterfaceStateReaderMock>>();

    const char *iface_name          = "test_iface";
    const bool expected_iface_state = true;
    bool actual_iface_state         = false;

    EXPECT_CALL(*reader, read_state(iface_name, _))
        .WillOnce(Invoke([&](const std::string &iface_name, bool &iface_state) -> bool {
            iface_state = expected_iface_state;
            return true;
        }));

    beerocks::net::InterfaceStateManagerImpl interface_state_manager(std::move(monitor),
                                                                     std::move(reader));

    ASSERT_TRUE(interface_state_manager.read_state(iface_name, actual_iface_state));
    ASSERT_EQ(actual_iface_state, expected_iface_state);

    ASSERT_TRUE(interface_state_manager.read_state(iface_name, actual_iface_state));
    ASSERT_EQ(actual_iface_state, expected_iface_state);
}

/**
 * In this test, the interface state is obtained after a state-changed event (no explicit read
 * operation is required nor performed)
 */
TEST(InterfaceStateManagerImpl, read_state_should_succeed_after_event)
{
    auto monitor = std::make_unique<StrictMock<beerocks::net::InterfaceStateMonitorMock>>();
    auto reader  = std::make_unique<StrictMock<beerocks::net::InterfaceStateReaderMock>>();

    const char *iface_name          = "test_iface";
    const bool expected_iface_state = true;
    bool actual_iface_state         = false;

    // The monitor mock is needed to emulate that a state-changed event has occurred.
    // Since the unique_ptr to the monitor mock is moved into the interface state manager, it
    // is not available later. To overcome this problem, we use the raw pointer instead.
    auto monitor_raw_ptr = monitor.get();

    beerocks::net::InterfaceStateManagerImpl interface_state_manager(std::move(monitor),
                                                                     std::move(reader));

    monitor_raw_ptr->notify_state_changed(iface_name, expected_iface_state);
    ASSERT_TRUE(interface_state_manager.read_state(iface_name, actual_iface_state));
    ASSERT_EQ(actual_iface_state, expected_iface_state);

    monitor_raw_ptr->notify_state_changed(iface_name, !expected_iface_state);
    ASSERT_TRUE(interface_state_manager.read_state(iface_name, actual_iface_state));
    ASSERT_NE(actual_iface_state, expected_iface_state);
}

TEST(InterfaceStateManagerImpl, notify_state_changed_should_succeed)
{
    auto monitor = std::make_unique<StrictMock<beerocks::net::InterfaceStateMonitorMock>>();
    auto reader  = std::make_unique<StrictMock<beerocks::net::InterfaceStateReaderMock>>();

    const char *iface_name          = "test_iface";
    const bool expected_iface_state = true;
    bool actual_iface_state         = false;

    // The monitor mock is needed to emulate that a state-changed event has occurred.
    // Since the unique_ptr to the monitor mock is moved into the interface state manager, it
    // is not available later. To overcome this problem, we use the raw pointer instead.
    auto monitor_raw_ptr = monitor.get();

    beerocks::net::InterfaceStateManagerImpl interface_state_manager(std::move(monitor),
                                                                     std::move(reader));

    interface_state_manager.set_handler(
        [&](const std::string &iface_name, bool iface_state) { actual_iface_state = iface_state; });

    monitor_raw_ptr->notify_state_changed(iface_name, expected_iface_state);
    ASSERT_EQ(actual_iface_state, expected_iface_state);
}
