/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <bcl/network/cmdu_serializer_stream_impl.h>

#include <bcl/network/buffer_mock.h>
#include <bcl/network/network_utils.h>

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
constexpr uint32_t iface_index      = 1;

class CmduSerializerStreamImplTest : public ::testing::Test {
protected:
    StrictMock<beerocks::net::BufferMock> m_buffer;

    beerocks::net::CmduSerializerStreamImpl m_serializer;
};

TEST_F(CmduSerializerStreamImplTest, serialize_cmdu_should_fail_with_invalid_src_mac)
{
    // Invalid parameters: destination MAC address is given but source MAC address is not
    sMacAddr dst_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    sMacAddr src_mac = beerocks::net::network_utils::ZERO_MAC;

    uint8_t tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageTx cmdu_tx(tx_buffer, sizeof(tx_buffer));

    cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_QUERY_MESSAGE);

    ASSERT_FALSE(m_serializer.serialize_cmdu(iface_index, dst_mac, src_mac, cmdu_tx, m_buffer));
}

TEST_F(CmduSerializerStreamImplTest, serialize_cmdu_should_fail_with_buffer_not_empty)
{
    sMacAddr dst_mac = beerocks::net::network_utils::ZERO_MAC;
    sMacAddr src_mac = beerocks::net::network_utils::ZERO_MAC;

    uint8_t tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageTx cmdu_tx(tx_buffer, sizeof(tx_buffer));

    cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_QUERY_MESSAGE);

    // Buffer is not empty
    size_t length = 1;

    {
        InSequence sequence;

        EXPECT_CALL(m_buffer, length()).WillOnce(ReturnRef(length));
    }

    ASSERT_FALSE(m_serializer.serialize_cmdu(iface_index, dst_mac, src_mac, cmdu_tx, m_buffer));
}

TEST_F(CmduSerializerStreamImplTest, serialize_cmdu_should_fail_with_buffer_too_small)
{
    sMacAddr dst_mac = beerocks::net::network_utils::ZERO_MAC;
    sMacAddr src_mac = beerocks::net::network_utils::ZERO_MAC;

    uint8_t tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageTx cmdu_tx(tx_buffer, sizeof(tx_buffer));

    cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_QUERY_MESSAGE);

    // Buffer is empty
    size_t length = 0;

    // Buffer size is too small (there is no room for the payload, but only for the header)
    size_t size = size_of_uds_header;

    {
        InSequence sequence;

        EXPECT_CALL(m_buffer, length()).WillOnce(ReturnRef(length));
        EXPECT_CALL(m_buffer, size()).WillOnce(Return(size));
    }

    ASSERT_FALSE(m_serializer.serialize_cmdu(iface_index, dst_mac, src_mac, cmdu_tx, m_buffer));
}

TEST_F(CmduSerializerStreamImplTest, serialize_cmdu_should_succeed)
{
    sMacAddr dst_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    sMacAddr src_mac = {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5};

    uint8_t tx_buffer[beerocks::message::MESSAGE_BUFFER_LENGTH];
    ieee1905_1::CmduMessageTx cmdu_tx(tx_buffer, sizeof(tx_buffer));

    cmdu_tx.create(0, ieee1905_1::eMessageType::TOPOLOGY_QUERY_MESSAGE);

    size_t size = size_of_uds_header + beerocks::message::MESSAGE_BUFFER_LENGTH;

    uint8_t actual_data[size]{};
    size_t actual_length = 0;
    auto actual_append   = [&](const uint8_t *data, size_t length) {
        std::copy_n(data, length, &actual_data[actual_length]);
        actual_length += length;
        return true;
    };

    {
        InSequence sequence;

        EXPECT_CALL(m_buffer, length()).WillOnce(ReturnRef(actual_length));
        EXPECT_CALL(m_buffer, size()).WillOnce(Return(size));
        EXPECT_CALL(m_buffer, append(_, size_of_uds_header)).WillOnce(Invoke(actual_append));
        EXPECT_CALL(m_buffer, append(_, _)).WillOnce(Invoke(actual_append));
    }

    ASSERT_TRUE(m_serializer.serialize_cmdu(iface_index, dst_mac, src_mac, cmdu_tx, m_buffer));

    const size_t expected_length = size_of_uds_header + cmdu_tx.getMessageLength();
    ASSERT_EQ(actual_length, expected_length);

    auto uds_header = reinterpret_cast<beerocks::message::sUdsHeader *>(actual_data);
    ASSERT_EQ(uds_header->if_index, iface_index);
    ASSERT_TRUE(std::equal(dst_mac.oct, dst_mac.oct + beerocks::net::MAC_ADDR_LEN,
                           uds_header->dst_bridge_mac));
    ASSERT_TRUE(std::equal(src_mac.oct, src_mac.oct + beerocks::net::MAC_ADDR_LEN,
                           uds_header->src_bridge_mac));
    ASSERT_EQ(uds_header->length, cmdu_tx.getMessageLength());
}

} // namespace
