/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_mac_map.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

namespace {

constexpr sMacAddr mac_1 = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
constexpr sMacAddr mac_2 = {0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
constexpr int value_1    = 1;
constexpr int value_2    = 2;

class MacMapTest : public ::testing::Test {
public:
    struct sTestType {
        sMacAddr mac;
        int value;

        sTestType(const sMacAddr &mac_, int value_) : mac(mac_), value(value_) {}
    };

    beerocks::mac_map<sTestType> m_test_mac_map;
};

TEST_F(MacMapTest, get_non_existing_returns_null) { EXPECT_FALSE(m_test_mac_map.get(mac_1)); }

TEST_F(MacMapTest, get_returns_existing)
{
    auto tt1 = m_test_mac_map.add(mac_1, value_1);
    EXPECT_TRUE(tt1);
    auto tt_get = m_test_mac_map.get(mac_1);
    ASSERT_TRUE(tt_get);
    EXPECT_EQ(tt_get->mac, mac_1);
    EXPECT_EQ(tt_get->value, value_1);
}

TEST_F(MacMapTest, add_keeps_old)
{
    auto tt1 = m_test_mac_map.add(mac_1, value_1);
    ASSERT_TRUE(tt1);
    EXPECT_EQ(tt1->mac, mac_1);
    EXPECT_EQ(tt1->value, value_1);
    auto tt2 = m_test_mac_map.add(mac_1, value_2);
    ASSERT_TRUE(tt2);
    EXPECT_EQ(tt2, tt1);
}

TEST_F(MacMapTest, add_keeps_old_sharedptr)
{
    auto tt1 = m_test_mac_map.add(mac_1, value_1);
    ASSERT_TRUE(tt1);
    EXPECT_EQ(tt1->mac, mac_1);
    EXPECT_EQ(tt1->value, value_1);
    auto tt2    = std::make_shared<sTestType>(mac_1, value_2);
    auto tt_get = m_test_mac_map.add(tt2);
    ASSERT_TRUE(tt_get);
    EXPECT_EQ(tt_get, tt1);
}

TEST_F(MacMapTest, keep_new_removes_old)
{
    auto tt1 = m_test_mac_map.add(mac_1, value_1);
    auto tt2 = m_test_mac_map.add(mac_2, value_2);

    m_test_mac_map.keep_new_prepare();
    m_test_mac_map.keep_new(mac_1);
    auto removed = m_test_mac_map.keep_new_remove_old();
    EXPECT_THAT(removed, ::testing::UnorderedElementsAreArray({tt2}));
    EXPECT_TRUE(m_test_mac_map.get(mac_1));
    EXPECT_FALSE(m_test_mac_map.get(mac_2));
}

TEST_F(MacMapTest, keep_new_twice_has_no_effect)
{
    auto tt1 = m_test_mac_map.add(mac_1, value_1);
    auto tt2 = m_test_mac_map.add(mac_2, value_2);

    m_test_mac_map.keep_new_prepare();
    m_test_mac_map.keep_new(mac_1);
    m_test_mac_map.keep_new(mac_1);
    auto removed = m_test_mac_map.keep_new_remove_old();
    EXPECT_THAT(removed, ::testing::UnorderedElementsAreArray({tt2}));
    EXPECT_TRUE(m_test_mac_map.get(mac_1));
    EXPECT_FALSE(m_test_mac_map.get(mac_2));
}

TEST_F(MacMapTest, keep_no_new_removes_all)
{
    auto tt1 = m_test_mac_map.add(mac_1, value_1);
    auto tt2 = m_test_mac_map.add(mac_2, value_2);

    m_test_mac_map.keep_new_prepare();
    auto removed = m_test_mac_map.keep_new_remove_old();
    EXPECT_THAT(removed, ::testing::UnorderedElementsAreArray({tt1, tt2}));
    EXPECT_TRUE(m_test_mac_map.empty());
    EXPECT_FALSE(m_test_mac_map.get(mac_1));
    EXPECT_FALSE(m_test_mac_map.get(mac_2));
}

TEST_F(MacMapTest, keep_all_new_removes_none)
{
    auto tt1 = m_test_mac_map.add(mac_1, value_1);
    auto tt2 = m_test_mac_map.add(mac_2, value_2);

    m_test_mac_map.keep_new_prepare();
    m_test_mac_map.keep_new(mac_1);
    m_test_mac_map.keep_new(mac_2);
    auto removed = m_test_mac_map.keep_new_remove_old();
    EXPECT_TRUE(removed.empty());
    EXPECT_TRUE(m_test_mac_map.get(mac_1));
    EXPECT_TRUE(m_test_mac_map.get(mac_2));
}

TEST_F(MacMapTest, keep_new_implied_by_add)
{
    auto tt1 = m_test_mac_map.add(mac_1, value_1);
    auto tt2 = m_test_mac_map.add(mac_2, value_2);

    m_test_mac_map.keep_new_prepare();
    EXPECT_EQ(m_test_mac_map.add(mac_1, value_2), tt1);
    auto removed = m_test_mac_map.keep_new_remove_old();
    EXPECT_THAT(removed, ::testing::UnorderedElementsAreArray({tt2}));
    EXPECT_TRUE(m_test_mac_map.get(mac_1));
    EXPECT_FALSE(m_test_mac_map.get(mac_2));
}

TEST_F(MacMapTest, keep_new_implied_by_add_sharedptr)
{
    auto tt1 = m_test_mac_map.add(mac_1, value_1);
    auto tt2 = m_test_mac_map.add(mac_2, value_2);

    m_test_mac_map.keep_new_prepare();
    auto tt1_new = std::make_shared<sTestType>(mac_1, value_2);
    EXPECT_EQ(m_test_mac_map.add(tt1_new), tt1);
    auto removed = m_test_mac_map.keep_new_remove_old();
    EXPECT_THAT(removed, ::testing::UnorderedElementsAreArray({tt2}));
    EXPECT_TRUE(m_test_mac_map.get(mac_1));
    EXPECT_FALSE(m_test_mac_map.get(mac_2));
}

} // namespace
