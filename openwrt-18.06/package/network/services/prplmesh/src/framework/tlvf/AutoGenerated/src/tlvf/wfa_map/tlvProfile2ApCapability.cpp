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

#include <tlvf/wfa_map/tlvProfile2ApCapability.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvProfile2ApCapability::tlvProfile2ApCapability(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvProfile2ApCapability::tlvProfile2ApCapability(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvProfile2ApCapability::~tlvProfile2ApCapability() {
}
const eTlvTypeMap& tlvProfile2ApCapability::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvProfile2ApCapability::length() {
    return (const uint16_t&)(*m_length);
}

uint8_t& tlvProfile2ApCapability::max_prioritization_rules() {
    return (uint8_t&)(*m_max_prioritization_rules);
}

uint8_t& tlvProfile2ApCapability::reserved() {
    return (uint8_t&)(*m_reserved);
}

tlvProfile2ApCapability::sCapabilitiesBitsField& tlvProfile2ApCapability::capabilities_bit_field() {
    return (sCapabilitiesBitsField&)(*m_capabilities_bit_field);
}

uint8_t& tlvProfile2ApCapability::max_total_number_of_vids() {
    return (uint8_t&)(*m_max_total_number_of_vids);
}

void tlvProfile2ApCapability::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    m_capabilities_bit_field->struct_swap();
}

bool tlvProfile2ApCapability::finalize()
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

size_t tlvProfile2ApCapability::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(uint8_t); // max_prioritization_rules
    class_size += sizeof(uint8_t); // reserved
    class_size += sizeof(sCapabilitiesBitsField); // capabilities_bit_field
    class_size += sizeof(uint8_t); // max_total_number_of_vids
    return class_size;
}

bool tlvProfile2ApCapability::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_PROFILE2_AP_CAPABILITY;
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
    m_max_prioritization_rules = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_reserved = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_capabilities_bit_field = reinterpret_cast<sCapabilitiesBitsField*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sCapabilitiesBitsField))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sCapabilitiesBitsField) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(sCapabilitiesBitsField); }
    if (!m_parse__) { m_capabilities_bit_field->struct_init(); }
    m_max_total_number_of_vids = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_PROFILE2_AP_CAPABILITY) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_PROFILE2_AP_CAPABILITY) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}


