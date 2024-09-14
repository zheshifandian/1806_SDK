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

#include <tlvf/association_frame/cExtendedCap.h>
#include <tlvf/tlvflogging.h>

using namespace assoc_frame;

cExtendedCap::cExtendedCap(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cExtendedCap::cExtendedCap(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cExtendedCap::~cExtendedCap() {
}
eElementID& cExtendedCap::type() {
    return (eElementID&)(*m_type);
}

const uint8_t& cExtendedCap::length() {
    return (const uint8_t&)(*m_length);
}

uint8_t* cExtendedCap::extended_cap_field(size_t idx) {
    if ( (m_extended_cap_field_idx__ == 0) || (m_extended_cap_field_idx__ <= idx) ) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
        return nullptr;
    }
    return &(m_extended_cap_field[idx]);
}

bool cExtendedCap::set_extended_cap_field(const void* buffer, size_t size) {
    if (buffer == nullptr) {
        TLVF_LOG(WARNING) << "set_extended_cap_field received a null pointer.";
        return false;
    }
    if (m_extended_cap_field_idx__ != 0) {
        TLVF_LOG(ERROR) << "set_extended_cap_field was already allocated!";
        return false;
    }
    if (!alloc_extended_cap_field(size)) { return false; }
    std::copy_n(reinterpret_cast<const uint8_t *>(buffer), size, m_extended_cap_field);
    return true;
}
bool cExtendedCap::alloc_extended_cap_field(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list extended_cap_field, abort!";
        return false;
    }
    size_t len = sizeof(uint8_t) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_extended_cap_field[m_extended_cap_field_idx__];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_extended_cap_field_idx__ += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    return true;
}

void cExtendedCap::class_swap()
{
}

bool cExtendedCap::finalize()
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

size_t cExtendedCap::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eElementID); // type
    class_size += sizeof(uint8_t); // length
    return class_size;
}

bool cExtendedCap::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eElementID*>(m_buff_ptr__);
    if (!m_parse__) *m_type = ID_EXTENDED_CAPABILITY;
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
    m_extended_cap_field = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (m_length && m_parse__) {
        auto swap_len = *m_length;
        tlvf_swap((sizeof(swap_len) * 8), reinterpret_cast<uint8_t*>(&swap_len));
        size_t len = swap_len;
        len -= (m_buff_ptr__ - sizeof(*m_type) - sizeof(*m_length) - m_buff__);
        m_extended_cap_field_idx__ = len/sizeof(uint8_t);
        if (!buffPtrIncrementSafe(len)) {
            LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
            return false;
        }
    }
    if (m_parse__) { class_swap(); }
    return true;
}


