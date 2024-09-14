///////////////////////////////////////
// AUTO GENERATED FILE - DO NOT EDIT //
///////////////////////////////////////

/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <tlvf/wfa_map/tlvTunnelledProtocolType.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvTunnelledProtocolType::tlvTunnelledProtocolType(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvTunnelledProtocolType::tlvTunnelledProtocolType(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvTunnelledProtocolType::~tlvTunnelledProtocolType() {
}
const eTlvTypeMap& tlvTunnelledProtocolType::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvTunnelledProtocolType::length() {
    return (const uint16_t&)(*m_length);
}

tlvTunnelledProtocolType::eTunnelledProtocolType& tlvTunnelledProtocolType::protocol_type() {
    return (eTunnelledProtocolType&)(*m_protocol_type);
}

void tlvTunnelledProtocolType::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
}

bool tlvTunnelledProtocolType::finalize()
{
    if (m_parse__) {
        TLVF_LOG(DEBUG) << "finalize() called but m_parse__ is set";
        return true;
    }
    if (m_finalized__) {
        TLVF_LOG(DEBUG) << "finalize() called for already finalized class";
        return true;
    }
    if (!isPostInitSucceeded()) {
        TLVF_LOG(ERROR) << "post init check failed";
        return false;
    }
    if (m_inner__) {
        if (!m_inner__->finalize()) {
            TLVF_LOG(ERROR) << "m_inner__->finalize() failed";
            return false;
        }
        auto tailroom = m_inner__->getMessageBuffLength() - m_inner__->getMessageLength();
        m_buff_ptr__ -= tailroom;
        *m_length -= tailroom;
    }
    class_swap();
    m_finalized__ = true;
    return true;
}

size_t tlvTunnelledProtocolType::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(eTunnelledProtocolType); // protocol_type
    return class_size;
}

bool tlvTunnelledProtocolType::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_TUNNELLED_PROTOCOL_TYPE;
    if (!buffPtrIncrementSafe(sizeof(eTlvTypeMap))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eTlvTypeMap) << ") Failed!";
        return false;
    }
    m_length = reinterpret_cast<uint16_t*>(m_buff_ptr__);
    if (!m_parse__) *m_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint16_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint16_t) << ") Failed!";
        return false;
    }
    m_protocol_type = reinterpret_cast<eTunnelledProtocolType*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(eTunnelledProtocolType))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eTunnelledProtocolType) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(eTunnelledProtocolType); }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_TUNNELLED_PROTOCOL_TYPE) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_TUNNELLED_PROTOCOL_TYPE) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}


