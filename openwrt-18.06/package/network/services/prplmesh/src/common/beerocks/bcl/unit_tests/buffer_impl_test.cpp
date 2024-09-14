/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/buffer_impl.h>

#include <gtest/gtest.h>

namespace {
constexpr size_t buffer_size = 16;

class BufferImplTest : public ::testing::Test {
protected:
    BufferImplTest() { fill_buffer_with_test_data(m_buffer); }

    void fill_buffer_with_test_data(beerocks::net::Buffer &buffer)
    {
        uint8_t *data = buffer.data();
        for (size_t i = 0; i < buffer.size(); i++) {
            data[i] = i;
        }
        buffer.length() = buffer.size();
    }

    beerocks::net::BufferImpl<buffer_size> m_buffer;
};

TEST_F(BufferImplTest, clear_should_succeed)
{
    m_buffer.clear();

    ASSERT_EQ(0, m_buffer.length());
    for (size_t i = 0; i < m_buffer.size(); i++) {
        ASSERT_EQ(0, m_buffer.data()[i]);
    }
}

TEST_F(BufferImplTest, append_should_fail_with_buffer_full)
{
    uint8_t data[]{0xff};

    ASSERT_FALSE(m_buffer.append(data, sizeof(data)));
}

TEST_F(BufferImplTest, append_should_succeed)
{
    m_buffer.clear();

    uint8_t data[]{0xff};

    ASSERT_TRUE(m_buffer.append(data, sizeof(data)));
    ASSERT_EQ(sizeof(data), m_buffer.length());
    for (size_t i = 0; i < m_buffer.size(); i++) {
        if (i < sizeof(data)) {
            ASSERT_EQ(data[i], m_buffer.data()[i]);
        } else {
            ASSERT_EQ(0, m_buffer.data()[i]);
        }
    }
}

TEST_F(BufferImplTest, shift_should_succeed)
{
    const size_t count = 1;
    ASSERT_TRUE(m_buffer.shift(count));

    ASSERT_EQ(m_buffer.length(), m_buffer.size() - count);
    for (size_t i = 0; i < m_buffer.size(); i++) {
        if (i < (m_buffer.size() - count)) {
            ASSERT_EQ(i + count, m_buffer.data()[i]);
        } else {
            ASSERT_EQ(0, m_buffer.data()[i]);
        }
    }
}

TEST_F(BufferImplTest, shift_should_fail_if_count_greater_than_length)
{
    const size_t count = m_buffer.length() + 1;
    ASSERT_FALSE(m_buffer.shift(count));
}
} // namespace
