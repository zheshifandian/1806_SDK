/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_ucc_serializer_stream_impl.h>

#include <bcl/network/buffer_mock.h>

#include <cstring>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;

namespace {

class UccSerializerStreamImplTest : public ::testing::Test {
protected:
    StrictMock<beerocks::net::BufferMock> m_buffer;
    const char *m_reply{"hello"};

    beerocks::UccSerializerStreamImpl m_serializer;
};

TEST_F(UccSerializerStreamImplTest, serialize_reply_should_fail_with_buffer_not_empty)
{
    // Buffer is not empty
    size_t length = 1;

    {
        InSequence sequence;

        EXPECT_CALL(m_buffer, length()).WillOnce(ReturnRef(length));
    }

    ASSERT_FALSE(m_serializer.serialize_reply(m_reply, m_buffer));
}

TEST_F(UccSerializerStreamImplTest, serialize_reply_should_fail_with_buffer_too_small)
{
    // Buffer is empty
    size_t length = 0;

    // Buffer size is too small (there is no room for the trailer, but only for the payload)
    size_t size = std::strlen(m_reply);

    {
        InSequence sequence;

        EXPECT_CALL(m_buffer, length()).WillOnce(ReturnRef(length));
        EXPECT_CALL(m_buffer, size()).WillOnce(Return(size));
    }

    ASSERT_FALSE(m_serializer.serialize_reply(m_reply, m_buffer));
}

TEST_F(UccSerializerStreamImplTest, serialize_reply_should_succeed)
{
    // Buffer is empty
    size_t length = 0;

    // Buffer size is large enough to accommodate both payload and trailer
    const size_t frame_length = std::strlen(m_reply) + 1;
    size_t size               = frame_length;

    uint8_t actual_data[size]{};
    size_t actual_length = 0;
    auto actual_append   = [&](const uint8_t *data, size_t length) {
        std::copy_n(data, length, actual_data);
        actual_length += length;
        return true;
    };

    {
        InSequence sequence;

        EXPECT_CALL(m_buffer, length()).WillOnce(ReturnRef(length));
        EXPECT_CALL(m_buffer, size()).WillOnce(Return(size));
        EXPECT_CALL(m_buffer, append(_, frame_length)).WillOnce(Invoke(actual_append));
    }

    ASSERT_TRUE(m_serializer.serialize_reply(m_reply, m_buffer));
    ASSERT_TRUE(std::equal(m_reply, m_reply + std::strlen(m_reply), actual_data));
    ASSERT_TRUE(actual_data[frame_length - 1] == '\n');
}
} // namespace
