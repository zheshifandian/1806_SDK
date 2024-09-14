/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/beerocks_ucc_parser_stream_impl.h>

#include <bcl/network/buffer_mock.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;

namespace {

TEST(UccParserStreamImplTest, parse_command_should_fail_without_trailer)
{
    StrictMock<beerocks::net::BufferMock> buffer;
    std::string command;

    beerocks::UccParserStreamImpl parser;

    // Command is not ended with a trailer character
    uint8_t data[]{'h', 'e', 'l', 'l', 'o'};
    size_t length = sizeof(data);

    {
        InSequence sequence;

        EXPECT_CALL(buffer, length()).WillOnce(ReturnRef(length));
        EXPECT_CALL(buffer, data()).WillOnce(Return(data));

        // Buffer is not full
        EXPECT_CALL(buffer, size()).WillOnce(Return(length + 1));
    }

    ASSERT_FALSE(parser.parse_command(buffer, command));
}

TEST(UccParserStreamImplTest, parse_command_should_fail_without_trailer_and_buffer_full)
{
    StrictMock<beerocks::net::BufferMock> buffer;
    std::string command;

    beerocks::UccParserStreamImpl parser;

    // Command is not ended with a trailer character
    uint8_t data[]{'h', 'e', 'l', 'l', 'o'};
    size_t length = sizeof(data);

    {
        InSequence sequence;

        EXPECT_CALL(buffer, length()).WillOnce(ReturnRef(length));
        EXPECT_CALL(buffer, data()).WillOnce(Return(data));

        // Buffer is full
        EXPECT_CALL(buffer, size()).WillOnce(Return(length));
        EXPECT_CALL(buffer, clear()).Times(1);
    }

    ASSERT_FALSE(parser.parse_command(buffer, command));
}

/**
 * This is a "Value-Parameterized Test"
 * https://github.com/google/googletest/blob/master/googletest/docs/advanced.md#value-parameterized-tests
 * Value-parameterized tests allow you to test your code with different parameters without writing
 * multiple copies of the same test.
 */
class UccParserStreamImpl_ParseCommandTest : public testing::Test,
                                             public testing::WithParamInterface<std::string> {
};

TEST_P(UccParserStreamImpl_ParseCommandTest, parse_command_should_succeed)
{
    std::string parameter = GetParam();

    size_t frame_length = parameter.find('\n') + 1;
    size_t length       = parameter.length();
    uint8_t data[length];
    std::copy_n(parameter.c_str(), length, data);

    StrictMock<beerocks::net::BufferMock> buffer;
    std::string command;
    const std::string expected_command{"hello"};

    beerocks::UccParserStreamImpl parser;

    {
        InSequence sequence;

        EXPECT_CALL(buffer, length()).WillOnce(ReturnRef(length));
        EXPECT_CALL(buffer, data()).WillOnce(Return(data));

        EXPECT_CALL(buffer, shift(frame_length)).Times(1);
    }

    ASSERT_TRUE(parser.parse_command(buffer, command));
    ASSERT_EQ(expected_command, command);
}

// clang-format off
const std::string parse_command_test_values[] {
    "hello\n",
    "hello\r\n",
    "hello \r\n",
    "hello\r\nhello2\r\n"
    " hello\n",
};
// clang-format on

std::string parse_command_param_to_string(const testing::TestParamInfo<std::string> &info)
{
    const auto &parameter = info.param;

    std::stringstream ss;

    for (auto ch : parameter) {
        if (isprint(ch) && (ch != ' ')) {
            ss << ch;
        } else {
            ss << std::setw(2) << std::hex << std::setfill('0') << (int)ch;
        }
    }

    return ss.str();
}

INSTANTIATE_TEST_SUITE_P(HelloParamsInstance, UccParserStreamImpl_ParseCommandTest,
                         testing::ValuesIn(parse_command_test_values),
                         parse_command_param_to_string);

/**
 * If you think using a "Value-Parameterized Test" for this so simple test is overkill, probably you
 * are right. It is the purist approach to writing unit tests with different parameters and might be
 * used as example to write more complex tests.
 * Since code like this might scare people away from trying to implement their own unit tests, a
 * similar but much simpler test is given below. It consists on repeating the same test in a loop
 * for each parameter. A purist will argue that a good unit test should test only one thing. That
 * is true, but it is better to have a not so perfect test than none at all.
 */
TEST(UccParserStreamImplTest, parse_command_should_succeed)
{
    for (std::string parameter : parse_command_test_values) {

        size_t frame_length = parameter.find('\n') + 1;
        size_t length       = parameter.length();
        uint8_t data[length];
        std::copy_n(parameter.c_str(), length, data);

        StrictMock<beerocks::net::BufferMock> buffer;
        std::string command;
        const std::string expected_command{"hello"};

        beerocks::UccParserStreamImpl parser;

        {
            InSequence sequence;

            EXPECT_CALL(buffer, length()).WillOnce(ReturnRef(length));
            EXPECT_CALL(buffer, data()).WillOnce(Return(data));

            EXPECT_CALL(buffer, shift(frame_length)).Times(1);
        }

        ASSERT_TRUE(parser.parse_command(buffer, command));
        EXPECT_EQ(expected_command, command);
    }
}

} // namespace
