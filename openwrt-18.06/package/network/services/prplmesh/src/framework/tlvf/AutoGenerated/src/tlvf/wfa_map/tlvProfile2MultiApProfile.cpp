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

#include <tlvf/wfa_map/tlvProfile2MultiApProfile.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvProfile2MultiApProfile::tlvProfile2MultiApProfile(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvProfile2MultiApProfile::tlvProfile2MultiApProfile(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvProfile2MultiApProfile::~tlvProfile2MultiApProfile() {
}
const eTlvTypeMap& tlvProfile2MultiApProfile::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvProfile2MultiApProfile::length() {
    return (const uint16_t&)(*m_length);
}

tlvProfile2MultiApProfile::eMultiApProfile& tlvProfile2MultiApProfile::profile() {
    return (eMultiApProfile&)(*m_profile);
}

void tlvProfile2MultiApProfile::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
}

bool tlvProfile2MultiApProfile::finalize()
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

size_t tlvProfile2MultiApProfile::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(eMultiApProfile); // profile
    return class_size;
}

bool tlvProfile2MultiApProfile::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_PROFILE2_MULTIAP_PROFILE;
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
    m_profile = reinterpret_cast<eMultiApProfile*>(m_buff_ptr__);
    if (!m_parse__) *m_profile = eMultiApProfile::MULTIAP_PROFILE_3;
    if (!buffPtrIncrementSafe(sizeof(eMultiApProfile))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eMultiApProfile) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(eMultiApProfile); }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_PROFILE2_MULTIAP_PROFILE) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_PROFILE2_MULTIAP_PROFILE) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}


