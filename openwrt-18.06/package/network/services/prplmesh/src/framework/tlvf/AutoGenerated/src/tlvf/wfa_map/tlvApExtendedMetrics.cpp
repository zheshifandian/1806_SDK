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

#include <tlvf/wfa_map/tlvApExtendedMetrics.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvApExtendedMetrics::tlvApExtendedMetrics(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvApExtendedMetrics::tlvApExtendedMetrics(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvApExtendedMetrics::~tlvApExtendedMetrics() {
}
const eTlvTypeMap& tlvApExtendedMetrics::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvApExtendedMetrics::length() {
    return (const uint16_t&)(*m_length);
}

sMacAddr& tlvApExtendedMetrics::bssid() {
    return (sMacAddr&)(*m_bssid);
}

uint32_t& tlvApExtendedMetrics::unicast_bytes_sent() {
    return (uint32_t&)(*m_unicast_bytes_sent);
}

uint32_t& tlvApExtendedMetrics::unicast_bytes_received() {
    return (uint32_t&)(*m_unicast_bytes_received);
}

uint32_t& tlvApExtendedMetrics::multicast_bytes_sent() {
    return (uint32_t&)(*m_multicast_bytes_sent);
}

uint32_t& tlvApExtendedMetrics::multicast_bytes_received() {
    return (uint32_t&)(*m_multicast_bytes_received);
}

uint32_t& tlvApExtendedMetrics::broadcast_bytes_sent() {
    return (uint32_t&)(*m_broadcast_bytes_sent);
}

uint32_t& tlvApExtendedMetrics::broadcast_bytes_received() {
    return (uint32_t&)(*m_broadcast_bytes_received);
}

void tlvApExtendedMetrics::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    m_bssid->struct_swap();
    tlvf_swap(32, reinterpret_cast<uint8_t*>(m_unicast_bytes_sent));
    tlvf_swap(32, reinterpret_cast<uint8_t*>(m_unicast_bytes_received));
    tlvf_swap(32, reinterpret_cast<uint8_t*>(m_multicast_bytes_sent));
    tlvf_swap(32, reinterpret_cast<uint8_t*>(m_multicast_bytes_received));
    tlvf_swap(32, reinterpret_cast<uint8_t*>(m_broadcast_bytes_sent));
    tlvf_swap(32, reinterpret_cast<uint8_t*>(m_broadcast_bytes_received));
}

bool tlvApExtendedMetrics::finalize()
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

size_t tlvApExtendedMetrics::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(sMacAddr); // bssid
    class_size += sizeof(uint32_t); // unicast_bytes_sent
    class_size += sizeof(uint32_t); // unicast_bytes_received
    class_size += sizeof(uint32_t); // multicast_bytes_sent
    class_size += sizeof(uint32_t); // multicast_bytes_received
    class_size += sizeof(uint32_t); // broadcast_bytes_sent
    class_size += sizeof(uint32_t); // broadcast_bytes_received
    return class_size;
}

bool tlvApExtendedMetrics::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_AP_EXTENDED_METRICS;
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
    m_bssid = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(sMacAddr); }
    if (!m_parse__) { m_bssid->struct_init(); }
    m_unicast_bytes_sent = reinterpret_cast<uint32_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint32_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint32_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint32_t); }
    m_unicast_bytes_received = reinterpret_cast<uint32_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint32_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint32_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint32_t); }
    m_multicast_bytes_sent = reinterpret_cast<uint32_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint32_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint32_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint32_t); }
    m_multicast_bytes_received = reinterpret_cast<uint32_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint32_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint32_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint32_t); }
    m_broadcast_bytes_sent = reinterpret_cast<uint32_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint32_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint32_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint32_t); }
    m_broadcast_bytes_received = reinterpret_cast<uint32_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint32_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint32_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint32_t); }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_AP_EXTENDED_METRICS) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_AP_EXTENDED_METRICS) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}


