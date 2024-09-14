/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/cmdu_parser_stream_impl.h>

#include <bcl/beerocks_backport.h>
#include <bcl/beerocks_defines.h>
#include <bcl/beerocks_message_structs.h>
#include <bcl/network/buffer_mock.h>
#include <bcl/network/net_struct.h>

#include <tlvf/CmduMessageTx.h>
#include <tlvf/ieee_1905_1/eMessageType.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;

namespace {

constexpr size_t size_of_uds_header = sizeof(beerocks::message::sUdsHeader);

class CmduParserStreamImplTest : public ::testing::Test {
protected:
    CmduParserStreamImplTest() : m_cmdu_rx(m_rx_buffer, sizeof(m_rx_buffer)) {}

    uint32_t m_iface_index = 0;
    sMacAddr m_dst_mac;
    sMacAddr m_src_mac;

    uint8_t m_rx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageRx m_cmdu_rx;

    StrictMock<beerocks::net::BufferMock> m_buffer;

    beerocks::net::CmduParserStreamImpl m_parser;
};

TEST_F(CmduParserStreamImplTest, parse_cmdu_should_fail_with_incomplete_header)
{
    // Header is not complete
    const size_t length = size_of_uds_header - 1;

    {
        InSequence sequence;

        EXPECT_CALL(m_buffer, length()).WillOnce(ReturnRef(length));
    }

    ASSERT_FALSE(m_parser.parse_cmdu(m_buffer, m_iface_index, m_dst_mac, m_src_mac, m_cmdu_rx));
}

TEST_F(CmduParserStreamImplTest, parse_cmdu_should_fail_with_incomplete_message)
{
    beerocks::message::sUdsHeader uds_header;
    uds_header.length = 1;

    // Only the header has been received
    const size_t length = size_of_uds_header;

    uint8_t data[size_of_uds_header + uds_header.length];
    std::copy_n(reinterpret_cast<uint8_t *>(&uds_header), size_of_uds_header, data);
    std::fill_n(data + size_of_uds_header, uds_header.length, 0);

    {
        InSequence sequence;

        EXPECT_CALL(m_buffer, length()).WillOnce(ReturnRef(length));
        EXPECT_CALL(m_buffer, data()).WillOnce(Return(data));
    }

    ASSERT_FALSE(m_parser.parse_cmdu(m_buffer, m_iface_index, m_dst_mac, m_src_mac, m_cmdu_rx));
}

TEST_F(CmduParserStreamImplTest, parse_cmdu_should_fail_with_invalid_message)
{
    beerocks::message::sUdsHeader uds_header;
    uds_header.length = 1;

    // Message is invalid (one byte length only)
    const size_t length = size_of_uds_header + uds_header.length;

    uint8_t data[size_of_uds_header + uds_header.length];
    std::copy_n(reinterpret_cast<uint8_t *>(&uds_header), size_of_uds_header, data);
    std::fill_n(data + size_of_uds_header, uds_header.length, 0);

    {
        InSequence sequence;

        EXPECT_CALL(m_buffer, length()).WillOnce(ReturnRef(length));
        EXPECT_CALL(m_buffer, data()).WillOnce(Return(data));
        EXPECT_CALL(m_buffer, shift(length)).Times(1);
    }

    ASSERT_FALSE(m_parser.parse_cmdu(m_buffer, m_iface_index, m_dst_mac, m_src_mac, m_cmdu_rx));
}

TEST_F(CmduParserStreamImplTest, parse_cmdu_should_succeed)
{
    uint8_t tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageTx cmdu_tx(tx_buffer, sizeof(tx_buffer));

    cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_QUERY_MESSAGE);
    cmdu_tx.finalize();

    const uint32_t expected_iface_index = 1;
    const sMacAddr expected_dst_mac     = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    const sMacAddr expected_src_mac     = {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5};

    beerocks::message::sUdsHeader uds_header;
    uds_header.if_index = expected_iface_index;
    tlvf::mac_to_array(expected_dst_mac, uds_header.dst_bridge_mac);
    tlvf::mac_to_array(expected_src_mac, uds_header.src_bridge_mac);
    uds_header.length = cmdu_tx.getMessageLength();

    const size_t length = size_of_uds_header + uds_header.length;

    uint8_t data[size_of_uds_header + uds_header.length];
    std::copy_n(reinterpret_cast<uint8_t *>(&uds_header), size_of_uds_header, data);
    std::copy_n(tx_buffer, uds_header.length, data + size_of_uds_header);

    {
        InSequence sequence;

        EXPECT_CALL(m_buffer, length()).WillOnce(ReturnRef(length));
        EXPECT_CALL(m_buffer, data()).WillOnce(Return(data));
        EXPECT_CALL(m_buffer, shift(length)).Times(1);
    }

    ASSERT_TRUE(m_parser.parse_cmdu(m_buffer, m_iface_index, m_dst_mac, m_src_mac, m_cmdu_rx));
    ASSERT_EQ(m_iface_index, expected_iface_index);
    ASSERT_EQ(m_dst_mac, expected_dst_mac);
    ASSERT_EQ(m_src_mac, expected_src_mac);
    ASSERT_EQ(cmdu_tx.getMessageType(), m_cmdu_rx.getMessageType());
}
} // namespace
