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

#include <tlvf/association_frame/cStaVhtCapability.h>
#include <tlvf/tlvflogging.h>

using namespace assoc_frame;

cStaVhtCapability::cStaVhtCapability(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cStaVhtCapability::cStaVhtCapability(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cStaVhtCapability::~cStaVhtCapability() {
}
eElementID& cStaVhtCapability::type() {
    return (eElementID&)(*m_type);
}

const uint8_t& cStaVhtCapability::length() {
    return (const uint8_t&)(*m_length);
}

assoc_frame::sStaVhtCapInfo& cStaVhtCapability::vht_cap_info() {
    return (assoc_frame::sStaVhtCapInfo&)(*m_vht_cap_info);
}

assoc_frame::sSupportedVhtMcsSet& cStaVhtCapability::supported_vht_mcs() {
    return (assoc_frame::sSupportedVhtMcsSet&)(*m_supported_vht_mcs);
}

void cStaVhtCapability::class_swap()
{
    m_vht_cap_info->struct_swap();
    m_supported_vht_mcs->struct_swap();
}

bool cStaVhtCapability::finalize()
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

size_t cStaVhtCapability::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eElementID); // type
    class_size += sizeof(uint8_t); // length
    class_size += sizeof(assoc_frame::sStaVhtCapInfo); // vht_cap_info
    class_size += sizeof(assoc_frame::sSupportedVhtMcsSet); // supported_vht_mcs
    return class_size;
}

bool cStaVhtCapability::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eElementID*>(m_buff_ptr__);
    if (!m_parse__) *m_type = ID_VHT_CAPS;
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
    m_vht_cap_info = reinterpret_cast<assoc_frame::sStaVhtCapInfo*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(assoc_frame::sStaVhtCapInfo))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(assoc_frame::sStaVhtCapInfo) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(assoc_frame::sStaVhtCapInfo); }
    if (!m_parse__) { m_vht_cap_info->struct_init(); }
    m_supported_vht_mcs = reinterpret_cast<assoc_frame::sSupportedVhtMcsSet*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(assoc_frame::sSupportedVhtMcsSet))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(assoc_frame::sSupportedVhtMcsSet) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(assoc_frame::sSupportedVhtMcsSet); }
    if (!m_parse__) { m_supported_vht_mcs->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}


