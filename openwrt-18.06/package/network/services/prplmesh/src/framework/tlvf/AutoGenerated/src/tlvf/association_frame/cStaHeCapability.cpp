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

#include <tlvf/association_frame/cStaHeCapability.h>
#include <tlvf/tlvflogging.h>

using namespace assoc_frame;

cStaHeCapability::cStaHeCapability(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cStaHeCapability::cStaHeCapability(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cStaHeCapability::~cStaHeCapability() {
}
eElementID& cStaHeCapability::type() {
    return (eElementID&)(*m_type);
}

const uint8_t& cStaHeCapability::length() {
    return (const uint8_t&)(*m_length);
}

eExtElementID& cStaHeCapability::subtype() {
    return (eExtElementID&)(*m_subtype);
}

uint8_t* cStaHeCapability::mac_cap_info(size_t idx) {
    if ( (m_mac_cap_info_idx__ == 0) || (m_mac_cap_info_idx__ <= idx) ) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
        return nullptr;
    }
    return &(m_mac_cap_info[idx]);
}

bool cStaHeCapability::set_mac_cap_info(const void* buffer, size_t size) {
    if (buffer == nullptr) {
        TLVF_LOG(WARNING) << "set_mac_cap_info received a null pointer.";
        return false;
    }
    if (size > 6) {
        TLVF_LOG(ERROR) << "Received buffer size is smaller than buffer length";
        return false;
    }
    std::copy_n(reinterpret_cast<const uint8_t *>(buffer), size, m_mac_cap_info);
    return true;
}
uint8_t* cStaHeCapability::phy_cap_info(size_t idx) {
    if ( (m_phy_cap_info_idx__ == 0) || (m_phy_cap_info_idx__ <= idx) ) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
        return nullptr;
    }
    return &(m_phy_cap_info[idx]);
}

bool cStaHeCapability::set_phy_cap_info(const void* buffer, size_t size) {
    if (buffer == nullptr) {
        TLVF_LOG(WARNING) << "set_phy_cap_info received a null pointer.";
        return false;
    }
    if (size > 11) {
        TLVF_LOG(ERROR) << "Received buffer size is smaller than buffer length";
        return false;
    }
    std::copy_n(reinterpret_cast<const uint8_t *>(buffer), size, m_phy_cap_info);
    return true;
}
uint16_t& cStaHeCapability::rx_mcs_le_80() {
    return (uint16_t&)(*m_rx_mcs_le_80);
}

uint16_t& cStaHeCapability::tx_mcs_le_80() {
    return (uint16_t&)(*m_tx_mcs_le_80);
}

uint8_t* cStaHeCapability::data(size_t idx) {
    if ( (m_data_idx__ == 0) || (m_data_idx__ <= idx) ) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
        return nullptr;
    }
    return &(m_data[idx]);
}

bool cStaHeCapability::set_data(const void* buffer, size_t size) {
    if (buffer == nullptr) {
        TLVF_LOG(WARNING) << "set_data received a null pointer.";
        return false;
    }
    if (m_data_idx__ != 0) {
        TLVF_LOG(ERROR) << "set_data was already allocated!";
        return false;
    }
    if (!alloc_data(size)) { return false; }
    std::copy_n(reinterpret_cast<const uint8_t *>(buffer), size, m_data);
    return true;
}
bool cStaHeCapability::alloc_data(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list data, abort!";
        return false;
    }
    size_t len = sizeof(uint8_t) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_data[m_data_idx__];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_data_idx__ += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    return true;
}

void cStaHeCapability::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_rx_mcs_le_80));
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_tx_mcs_le_80));
}

bool cStaHeCapability::finalize()
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

size_t cStaHeCapability::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eElementID); // type
    class_size += sizeof(uint8_t); // length
    class_size += sizeof(eExtElementID); // subtype
    class_size += 6 * sizeof(uint8_t); // mac_cap_info
    class_size += 11 * sizeof(uint8_t); // phy_cap_info
    class_size += sizeof(uint16_t); // rx_mcs_le_80
    class_size += sizeof(uint16_t); // tx_mcs_le_80
    return class_size;
}

bool cStaHeCapability::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eElementID*>(m_buff_ptr__);
    if (!m_parse__) *m_type = ID_EID_EXTENSION;
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
    m_subtype = reinterpret_cast<eExtElementID*>(m_buff_ptr__);
    if (!m_parse__) *m_subtype = EXTID_HE_CAPABILITIES;
    if (!buffPtrIncrementSafe(sizeof(eExtElementID))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eExtElementID) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(eExtElementID); }
    m_mac_cap_info = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t) * (6))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) * (6) << ") Failed!";
        return false;
    }
    m_mac_cap_info_idx__  = 6;
    if (!m_parse__) {
        if (m_length) { (*m_length) += (sizeof(uint8_t) * 6); }
    }
    m_phy_cap_info = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t) * (11))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) * (11) << ") Failed!";
        return false;
    }
    m_phy_cap_info_idx__  = 11;
    if (!m_parse__) {
        if (m_length) { (*m_length) += (sizeof(uint8_t) * 11); }
    }
    m_rx_mcs_le_80 = reinterpret_cast<uint16_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint16_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint16_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint16_t); }
    m_tx_mcs_le_80 = reinterpret_cast<uint16_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint16_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint16_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint16_t); }
    m_data = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (m_length && m_parse__) {
        auto swap_len = *m_length;
        tlvf_swap((sizeof(swap_len) * 8), reinterpret_cast<uint8_t*>(&swap_len));
        size_t len = swap_len;
        len -= (m_buff_ptr__ - sizeof(*m_type) - sizeof(*m_length) - m_buff__);
        m_data_idx__ = len/sizeof(uint8_t);
        if (!buffPtrIncrementSafe(len)) {
            LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
            return false;
        }
    }
    if (m_parse__) { class_swap(); }
    return true;
}


