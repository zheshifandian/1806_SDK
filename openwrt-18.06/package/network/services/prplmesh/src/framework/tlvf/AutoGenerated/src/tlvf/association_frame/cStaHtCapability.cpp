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

#include <tlvf/association_frame/cStaHtCapability.h>
#include <tlvf/tlvflogging.h>

using namespace assoc_frame;

cStaHtCapability::cStaHtCapability(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cStaHtCapability::cStaHtCapability(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cStaHtCapability::~cStaHtCapability() {
}
eElementID& cStaHtCapability::type() {
    return (eElementID&)(*m_type);
}

const uint8_t& cStaHtCapability::length() {
    return (const uint8_t&)(*m_length);
}

assoc_frame::sStaHtCapabilityInfo& cStaHtCapability::ht_cap_info() {
    return (assoc_frame::sStaHtCapabilityInfo&)(*m_ht_cap_info);
}

cStaHtCapability::sA_MpduParam& cStaHtCapability::a_mpdu_param() {
    return (sA_MpduParam&)(*m_a_mpdu_param);
}

uint8_t* cStaHtCapability::ht_mcs_set(size_t idx) {
    if ( (m_ht_mcs_set_idx__ == 0) || (m_ht_mcs_set_idx__ <= idx) ) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
        return nullptr;
    }
    return &(m_ht_mcs_set[idx]);
}

bool cStaHtCapability::set_ht_mcs_set(const void* buffer, size_t size) {
    if (buffer == nullptr) {
        TLVF_LOG(WARNING) << "set_ht_mcs_set received a null pointer.";
        return false;
    }
    if (size > 16) {
        TLVF_LOG(ERROR) << "Received buffer size is smaller than buffer length";
        return false;
    }
    std::copy_n(reinterpret_cast<const uint8_t *>(buffer), size, m_ht_mcs_set);
    return true;
}
uint16_t& cStaHtCapability::ht_extended_caps() {
    return (uint16_t&)(*m_ht_extended_caps);
}

uint32_t& cStaHtCapability::tx_beamforming_caps() {
    return (uint32_t&)(*m_tx_beamforming_caps);
}

uint8_t& cStaHtCapability::asel_caps() {
    return (uint8_t&)(*m_asel_caps);
}

void cStaHtCapability::class_swap()
{
    m_ht_cap_info->struct_swap();
    m_a_mpdu_param->struct_swap();
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_ht_extended_caps));
    tlvf_swap(32, reinterpret_cast<uint8_t*>(m_tx_beamforming_caps));
}

bool cStaHtCapability::finalize()
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

size_t cStaHtCapability::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eElementID); // type
    class_size += sizeof(uint8_t); // length
    class_size += sizeof(assoc_frame::sStaHtCapabilityInfo); // ht_cap_info
    class_size += sizeof(sA_MpduParam); // a_mpdu_param
    class_size += 16 * sizeof(uint8_t); // ht_mcs_set
    class_size += sizeof(uint16_t); // ht_extended_caps
    class_size += sizeof(uint32_t); // tx_beamforming_caps
    class_size += sizeof(uint8_t); // asel_caps
    return class_size;
}

bool cStaHtCapability::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eElementID*>(m_buff_ptr__);
    if (!m_parse__) *m_type = ID_HT_CAPABILITY;
    if (!buffPtrIncrementSafe(sizeof(eElementID))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eElementID) << ") Failed!";
        return false;
    }
    m_length = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_ht_cap_info = reinterpret_cast<assoc_frame::sStaHtCapabilityInfo*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(assoc_frame::sStaHtCapabilityInfo))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(assoc_frame::sStaHtCapabilityInfo) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(assoc_frame::sStaHtCapabilityInfo); }
    if (!m_parse__) { m_ht_cap_info->struct_init(); }
    m_a_mpdu_param = reinterpret_cast<sA_MpduParam*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sA_MpduParam))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sA_MpduParam) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(sA_MpduParam); }
    if (!m_parse__) { m_a_mpdu_param->struct_init(); }
    m_ht_mcs_set = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t) * (16))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) * (16) << ") Failed!";
        return false;
    }
    m_ht_mcs_set_idx__  = 16;
    if (!m_parse__) {
        if (m_length) { (*m_length) += (sizeof(uint8_t) * 16); }
    }
    m_ht_extended_caps = reinterpret_cast<uint16_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint16_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint16_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint16_t); }
    m_tx_beamforming_caps = reinterpret_cast<uint32_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint32_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint32_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint32_t); }
    m_asel_caps = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    if (m_parse__) { class_swap(); }
    return true;
}


