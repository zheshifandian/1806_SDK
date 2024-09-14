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

#include <tlvf/association_frame/cRmEnabledCaps.h>
#include <tlvf/tlvflogging.h>

using namespace assoc_frame;

cRmEnabledCaps::cRmEnabledCaps(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cRmEnabledCaps::cRmEnabledCaps(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cRmEnabledCaps::~cRmEnabledCaps() {
}
eElementID& cRmEnabledCaps::type() {
    return (eElementID&)(*m_type);
}

const uint8_t& cRmEnabledCaps::length() {
    return (const uint8_t&)(*m_length);
}

assoc_frame::sRmEnabledCaps1& cRmEnabledCaps::data1() {
    return (assoc_frame::sRmEnabledCaps1&)(*m_data1);
}

assoc_frame::sRmEnabledCaps2& cRmEnabledCaps::data2() {
    return (assoc_frame::sRmEnabledCaps2&)(*m_data2);
}

void cRmEnabledCaps::class_swap()
{
    m_data1->struct_swap();
    m_data2->struct_swap();
}

bool cRmEnabledCaps::finalize()
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

size_t cRmEnabledCaps::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eElementID); // type
    class_size += sizeof(uint8_t); // length
    class_size += sizeof(assoc_frame::sRmEnabledCaps1); // data1
    class_size += sizeof(assoc_frame::sRmEnabledCaps2); // data2
    return class_size;
}

bool cRmEnabledCaps::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eElementID*>(m_buff_ptr__);
    if (!m_parse__) *m_type = ID_RM_ENABLED_CAPS;
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
    m_data1 = reinterpret_cast<assoc_frame::sRmEnabledCaps1*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(assoc_frame::sRmEnabledCaps1))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(assoc_frame::sRmEnabledCaps1) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(assoc_frame::sRmEnabledCaps1); }
    if (!m_parse__) { m_data1->struct_init(); }
    m_data2 = reinterpret_cast<assoc_frame::sRmEnabledCaps2*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(assoc_frame::sRmEnabledCaps2))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(assoc_frame::sRmEnabledCaps2) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(assoc_frame::sRmEnabledCaps2); }
    if (!m_parse__) { m_data2->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}


