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

#include <tlvf/wfa_map/tlvProfile2CacStatusReport.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvProfile2CacStatusReport::tlvProfile2CacStatusReport(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvProfile2CacStatusReport::tlvProfile2CacStatusReport(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvProfile2CacStatusReport::~tlvProfile2CacStatusReport() {
}
const eTlvTypeMap& tlvProfile2CacStatusReport::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvProfile2CacStatusReport::length() {
    return (const uint16_t&)(*m_length);
}

uint8_t& tlvProfile2CacStatusReport::number_of_available_channels() {
    return (uint8_t&)(*m_number_of_available_channels);
}

std::tuple<bool, tlvProfile2CacStatusReport::sAvailableChannels&> tlvProfile2CacStatusReport::available_channels(size_t idx) {
    bool ret_success = ( (m_available_channels_idx__ > 0) && (m_available_channels_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, m_available_channels[ret_idx]);
}

bool tlvProfile2CacStatusReport::alloc_available_channels(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list available_channels, abort!";
        return false;
    }
    size_t len = sizeof(sAvailableChannels) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_available_channels[*m_number_of_available_channels];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_number_of_detected_pairs = (uint8_t *)((uint8_t *)(m_number_of_detected_pairs) + len);
    m_detected_pairs = (sDetectedPairs *)((uint8_t *)(m_detected_pairs) + len);
    m_number_of_active_cac_pairs = (uint8_t *)((uint8_t *)(m_number_of_active_cac_pairs) + len);
    m_active_cac_pairs = (sActiveCacPairs *)((uint8_t *)(m_active_cac_pairs) + len);
    m_available_channels_idx__ += count;
    *m_number_of_available_channels += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    if (!m_parse__) { 
        for (size_t i = m_available_channels_idx__ - count; i < m_available_channels_idx__; i++) { m_available_channels[i].struct_init(); }
    }
    return true;
}

uint8_t& tlvProfile2CacStatusReport::number_of_detected_pairs() {
    return (uint8_t&)(*m_number_of_detected_pairs);
}

std::tuple<bool, tlvProfile2CacStatusReport::sDetectedPairs&> tlvProfile2CacStatusReport::detected_pairs(size_t idx) {
    bool ret_success = ( (m_detected_pairs_idx__ > 0) && (m_detected_pairs_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, m_detected_pairs[ret_idx]);
}

bool tlvProfile2CacStatusReport::alloc_detected_pairs(size_t count) {
    if (m_lock_order_counter__ > 1) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list detected_pairs, abort!";
        return false;
    }
    size_t len = sizeof(sDetectedPairs) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 1;
    uint8_t *src = (uint8_t *)&m_detected_pairs[*m_number_of_detected_pairs];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_number_of_active_cac_pairs = (uint8_t *)((uint8_t *)(m_number_of_active_cac_pairs) + len);
    m_active_cac_pairs = (sActiveCacPairs *)((uint8_t *)(m_active_cac_pairs) + len);
    m_detected_pairs_idx__ += count;
    *m_number_of_detected_pairs += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    if (!m_parse__) { 
        for (size_t i = m_detected_pairs_idx__ - count; i < m_detected_pairs_idx__; i++) { m_detected_pairs[i].struct_init(); }
    }
    return true;
}

uint8_t& tlvProfile2CacStatusReport::number_of_active_cac_pairs() {
    return (uint8_t&)(*m_number_of_active_cac_pairs);
}

std::tuple<bool, tlvProfile2CacStatusReport::sActiveCacPairs&> tlvProfile2CacStatusReport::active_cac_pairs(size_t idx) {
    bool ret_success = ( (m_active_cac_pairs_idx__ > 0) && (m_active_cac_pairs_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, m_active_cac_pairs[ret_idx]);
}

bool tlvProfile2CacStatusReport::alloc_active_cac_pairs(size_t count) {
    if (m_lock_order_counter__ > 2) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list active_cac_pairs, abort!";
        return false;
    }
    size_t len = sizeof(sActiveCacPairs) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 2;
    uint8_t *src = (uint8_t *)&m_active_cac_pairs[*m_number_of_active_cac_pairs];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_active_cac_pairs_idx__ += count;
    *m_number_of_active_cac_pairs += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    if (!m_parse__) { 
        for (size_t i = m_active_cac_pairs_idx__ - count; i < m_active_cac_pairs_idx__; i++) { m_active_cac_pairs[i].struct_init(); }
    }
    return true;
}

void tlvProfile2CacStatusReport::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    for (size_t i = 0; i < m_available_channels_idx__; i++){
        m_available_channels[i].struct_swap();
    }
    for (size_t i = 0; i < m_detected_pairs_idx__; i++){
        m_detected_pairs[i].struct_swap();
    }
    for (size_t i = 0; i < m_active_cac_pairs_idx__; i++){
        m_active_cac_pairs[i].struct_swap();
    }
}

bool tlvProfile2CacStatusReport::finalize()
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

size_t tlvProfile2CacStatusReport::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(uint8_t); // number_of_available_channels
    class_size += sizeof(uint8_t); // number_of_detected_pairs
    class_size += sizeof(uint8_t); // number_of_active_cac_pairs
    return class_size;
}

bool tlvProfile2CacStatusReport::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_PROFILE2_CAC_STATUS_REPORT;
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
    m_number_of_available_channels = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_number_of_available_channels = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_available_channels = reinterpret_cast<sAvailableChannels*>(m_buff_ptr__);
    uint8_t number_of_available_channels = *m_number_of_available_channels;
    m_available_channels_idx__ = number_of_available_channels;
    if (!buffPtrIncrementSafe(sizeof(sAvailableChannels) * (number_of_available_channels))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sAvailableChannels) * (number_of_available_channels) << ") Failed!";
        return false;
    }
    m_number_of_detected_pairs = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_number_of_detected_pairs = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_detected_pairs = reinterpret_cast<sDetectedPairs*>(m_buff_ptr__);
    uint8_t number_of_detected_pairs = *m_number_of_detected_pairs;
    m_detected_pairs_idx__ = number_of_detected_pairs;
    if (!buffPtrIncrementSafe(sizeof(sDetectedPairs) * (number_of_detected_pairs))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sDetectedPairs) * (number_of_detected_pairs) << ") Failed!";
        return false;
    }
    m_number_of_active_cac_pairs = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_number_of_active_cac_pairs = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_active_cac_pairs = reinterpret_cast<sActiveCacPairs*>(m_buff_ptr__);
    uint8_t number_of_active_cac_pairs = *m_number_of_active_cac_pairs;
    m_active_cac_pairs_idx__ = number_of_active_cac_pairs;
    if (!buffPtrIncrementSafe(sizeof(sActiveCacPairs) * (number_of_active_cac_pairs))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sActiveCacPairs) * (number_of_active_cac_pairs) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_PROFILE2_CAC_STATUS_REPORT) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_PROFILE2_CAC_STATUS_REPORT) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}


