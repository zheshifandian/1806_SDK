/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/transaction.h>

#include <gtest/gtest.h>

namespace {

class TransactionTest : public ::testing::Test {
protected:
    bool m_action1_rolled_back               = false;
    bool m_action2_rolled_back               = false;
    bool m_action1_rolled_back_after_action2 = false;

    std::function<void()> m_rollback_action1 = [this]() {
        EXPECT_FALSE(m_action1_rolled_back);
        m_action1_rolled_back = true;
        if (m_action2_rolled_back) {
            m_action1_rolled_back_after_action2 = true;
        }
    };

    std::function<void()> m_rollback_action2 = [this]() {
        EXPECT_FALSE(m_action2_rolled_back);
        m_action2_rolled_back = true;
    };
};

TEST_F(TransactionTest, commit_should_succeed)
{
    {
        beerocks::Transaction transaction;
        transaction.add_rollback_action(m_rollback_action1);

        transaction.commit();
    }

    EXPECT_FALSE(m_action1_rolled_back);
}

TEST_F(TransactionTest, rollback_should_succeed)
{
    {
        beerocks::Transaction transaction;
        transaction.add_rollback_action(m_rollback_action1);
        transaction.add_rollback_action(m_rollback_action2);

        transaction.rollback();
    }

    EXPECT_TRUE(m_rollback_action1);
    EXPECT_TRUE(m_rollback_action2);
    EXPECT_TRUE(m_action1_rolled_back_after_action2);
}

TEST_F(TransactionTest, destructor_should_call_rollback_if_transaction_was_not_committed)
{
    {
        beerocks::Transaction transaction;
        transaction.add_rollback_action(m_rollback_action1);
        transaction.add_rollback_action(m_rollback_action2);
    }

    EXPECT_TRUE(m_rollback_action1);
    EXPECT_TRUE(m_rollback_action2);
    EXPECT_TRUE(m_action1_rolled_back_after_action2);
}

} // namespace
