/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/bridge_state_manager_impl.h>
#include <bcl/network/bridge_state_monitor_mock.h>
#include <bcl/network/bridge_state_reader_mock.h>

#include <bcl/beerocks_backport.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::StrictMock;

/**
 * In this test, there is no bridge state information cached yet so it is obtained with an explicit
 * read using the bridge state reader.
 */
TEST(BridgeStateManagerImpl, read_state_should_succeed_before_read)
{
    auto monitor = std::make_unique<StrictMock<beerocks::net::BridgeStateMonitorMock>>();
    auto reader  = std::make_unique<StrictMock<beerocks::net::BridgeStateReaderMock>>();

    const char *bridge_name = "test_bridge";
    std::set<std::string> expected_iface_names{"test_iface_0", "test_iface_1", "test_iface_2"};
    std::set<std::string> actual_iface_names;

    EXPECT_CALL(*reader, read_state(bridge_name, _))
        .WillOnce(
            Invoke([&](const std::string &bridge_name, std::set<std::string> &iface_names) -> bool {
                iface_names = expected_iface_names;
                return true;
            }));

    beerocks::net::BridgeStateManagerImpl bridge_state_manager(std::move(monitor),
                                                               std::move(reader));

    ASSERT_TRUE(bridge_state_manager.read_state(bridge_name, actual_iface_names));
    ASSERT_EQ(actual_iface_names, expected_iface_names);
}

/**
 * This test demonstrates that bridge state is cached and only one explicit read is performed
 * using the bridge state reader no matter how many times it is queried.
 */
TEST(BridgeStateManagerImpl, read_state_should_succeed_after_read)
{
    auto monitor = std::make_unique<StrictMock<beerocks::net::BridgeStateMonitorMock>>();
    auto reader  = std::make_unique<StrictMock<beerocks::net::BridgeStateReaderMock>>();

    const char *bridge_name = "test_bridge";
    std::set<std::string> expected_iface_names{"test_iface_0", "test_iface_1", "test_iface_2"};
    std::set<std::string> actual_iface_names;

    EXPECT_CALL(*reader, read_state(bridge_name, _))
        .WillOnce(
            Invoke([&](const std::string &bridge_name, std::set<std::string> &iface_names) -> bool {
                iface_names = expected_iface_names;
                return true;
            }));

    beerocks::net::BridgeStateManagerImpl bridge_state_manager(std::move(monitor),
                                                               std::move(reader));

    ASSERT_TRUE(bridge_state_manager.read_state(bridge_name, actual_iface_names));
    ASSERT_EQ(actual_iface_names, expected_iface_names);

    ASSERT_TRUE(bridge_state_manager.read_state(bridge_name, actual_iface_names));
    ASSERT_EQ(actual_iface_names, expected_iface_names);
}

TEST(BridgeStateManagerImpl, notify_state_changed_should_succeed)
{
    auto monitor = std::make_unique<StrictMock<beerocks::net::BridgeStateMonitorMock>>();
    auto reader  = std::make_unique<StrictMock<beerocks::net::BridgeStateReaderMock>>();

    const char *bridge_name = "test_bridge";
    const char *iface_name  = "test_iface";
    std::set<std::string> initial_iface_names{"test_iface_0", "test_iface_1", "test_iface_2"};
    std::set<std::string> updated_iface_names = initial_iface_names;
    updated_iface_names.emplace(iface_name);
    std::set<std::string> iface_names;

    {
        InSequence sequence;

        // Expectation for the initial read
        EXPECT_CALL(*reader, read_state(bridge_name, _))
            .WillOnce(Invoke(
                [&](const std::string &bridge_name, std::set<std::string> &iface_names) -> bool {
                    iface_names = initial_iface_names;
                    return true;
                }));
        // Expectation for the verification performed after the interface is added to the bridge
        EXPECT_CALL(*reader, read_state(bridge_name, _))
            .WillOnce(Invoke(
                [&](const std::string &bridge_name, std::set<std::string> &iface_names) -> bool {
                    iface_names = updated_iface_names;
                    return true;
                }));
        // Expectation for the verification performed after the interface is removed from the bridge
        EXPECT_CALL(*reader, read_state(bridge_name, _))
            .WillOnce(Invoke(
                [&](const std::string &bridge_name, std::set<std::string> &iface_names) -> bool {
                    iface_names = initial_iface_names;
                    return true;
                }));
    }

    // The monitor mock is needed to emulate that a state-changed event has occurred.
    // Since the unique_ptr to the monitor mock is moved into the bridge state manager, it
    // is not available later. To overcome this problem, we use the raw pointer instead.
    auto monitor_raw_ptr = monitor.get();

    beerocks::net::BridgeStateManagerImpl bridge_state_manager(std::move(monitor),
                                                               std::move(reader));

    bridge_state_manager.set_handler(
        [&](const std::string &bridge_name, const std::string &iface_name, bool iface_in_bridge) {
            if (iface_in_bridge) {
                iface_names.emplace(iface_name);
            } else {
                iface_names.erase(iface_name);
            }
        });

    ASSERT_TRUE(bridge_state_manager.read_state(bridge_name, iface_names));
    ASSERT_EQ(iface_names, initial_iface_names);

    // Emulate that a new interface is added to the bridge
    monitor_raw_ptr->notify_state_changed(bridge_name, iface_name, true);
    ASSERT_NE(std::find(iface_names.begin(), iface_names.end(), iface_name), iface_names.end());

    // Verify that cache has been updated and now it contains the new interface
    ASSERT_TRUE(bridge_state_manager.read_state(bridge_name, iface_names));
    ASSERT_EQ(iface_names, updated_iface_names);

    // Emulate that an existing interface is removed from the bridge
    monitor_raw_ptr->notify_state_changed(bridge_name, iface_name, false);
    ASSERT_EQ(std::find(iface_names.begin(), iface_names.end(), iface_name), iface_names.end());

    // Verify that cache has been updated and now it does not contain the removed interface
    ASSERT_TRUE(bridge_state_manager.read_state(bridge_name, iface_names));
    ASSERT_EQ(iface_names, initial_iface_names);
}

TEST(BridgeStateManagerImpl, notify_state_changed_should_succeed_with_misplaced_add_event)
{
    auto monitor = std::make_unique<StrictMock<beerocks::net::BridgeStateMonitorMock>>();
    auto reader  = std::make_unique<StrictMock<beerocks::net::BridgeStateReaderMock>>();

    const char *bridge_name = "test_bridge";
    const char *iface_name  = "test_iface";
    std::set<std::string> initial_iface_names{iface_name, "test_iface_0", "test_iface_1",
                                              "test_iface_2"};
    std::set<std::string> updated_iface_names = initial_iface_names;
    updated_iface_names.erase(iface_name);
    std::set<std::string> iface_names;

    {
        InSequence sequence;

        // Expectation for the initial read
        EXPECT_CALL(*reader, read_state(bridge_name, _))
            .WillOnce(Invoke(
                [&](const std::string &bridge_name, std::set<std::string> &iface_names) -> bool {
                    iface_names = initial_iface_names;
                    return true;
                }));
        // Expectation for the verification performed after the interface is removed from the bridge
        EXPECT_CALL(*reader, read_state(bridge_name, _))
            .WillOnce(Invoke(
                [&](const std::string &bridge_name, std::set<std::string> &iface_names) -> bool {
                    iface_names = updated_iface_names;
                    return true;
                }));
        // Expectation for the verification performed after the misplaced add event is received
        // (the list of interfaces returned is the same as in previous expectation)
        EXPECT_CALL(*reader, read_state(bridge_name, _))
            .WillOnce(Invoke(
                [&](const std::string &bridge_name, std::set<std::string> &iface_names) -> bool {
                    iface_names = updated_iface_names;
                    return true;
                }));
    }

    // The monitor mock is needed to emulate that a state-changed event has occurred.
    // Since the unique_ptr to the monitor mock is moved into the bridge state manager, it
    // is not available later. To overcome this problem, we use the raw pointer instead.
    auto monitor_raw_ptr = monitor.get();

    beerocks::net::BridgeStateManagerImpl bridge_state_manager(std::move(monitor),
                                                               std::move(reader));

    bridge_state_manager.set_handler(
        [&](const std::string &bridge_name, const std::string &iface_name, bool iface_in_bridge) {
            if (iface_in_bridge) {
                iface_names.emplace(iface_name);
            } else {
                iface_names.erase(iface_name);
            }
        });

    ASSERT_TRUE(bridge_state_manager.read_state(bridge_name, iface_names));
    ASSERT_EQ(iface_names, initial_iface_names);

    // Emulate that an existing interface is removed from the bridge
    monitor_raw_ptr->notify_state_changed(bridge_name, iface_name, false);
    ASSERT_EQ(std::find(iface_names.begin(), iface_names.end(), iface_name), iface_names.end());

    // Emulate that a misplaced RTM_NEWLINK event is received from kernel and assert that it is
    // ignored
    monitor_raw_ptr->notify_state_changed(bridge_name, iface_name, true);
    ASSERT_EQ(std::find(iface_names.begin(), iface_names.end(), iface_name), iface_names.end());
}
