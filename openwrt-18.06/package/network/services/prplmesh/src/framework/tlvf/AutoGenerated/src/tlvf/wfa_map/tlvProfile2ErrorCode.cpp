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

#include <tlvf/wfa_map/tlvProfile2ErrorCode.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvProfile2ErrorCode::tlvProfile2ErrorCode(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvProfile2ErrorCode::tlvProfile2ErrorCode(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvProfile2ErrorCode::~tlvProfile2ErrorCode() {
}
const eTlvTypeMap& tlvProfile2ErrorCode::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvProfile2ErrorCode::length() {
    return (const uint16_t&)(*m_length);
}

tlvProfile2ErrorCode::eReasonCode& tlvProfile2ErrorCode::reason_code() {
    return (eReasonCode&)(*m_reason_code);
}

bool tlvProfile2ErrorCode::alloc_bssid() {
    if (m_bssid_allocated) {
        LOG(ERROR) << "bssid already allocated!";
        return false;
    }
    size_t len = sizeof(sMacAddr);
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    uint8_t *src = (uint8_t *)m_bssid;
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_service_prioritization_rule_id = (uint32_t *)((uint8_t *)(m_service_prioritization_rule_id) + len);
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    m_bssid_allocated = true;
    return true;
}

sMacAddr* tlvProfile2ErrorCode::bssid() {
    if (!m_reason_code || !(*m_reason_code == eReasonCode::DEFAULT_PCP_OR_PRIMARY_VLAN_ID_NOT_PROVIDED || *m_reason_code == eReasonCode::NUMBER_OF_UNIQUE_VLAN_ID_EXCEEDS_MAXIMUM_SUPPORTED)) {
        TLVF_LOG(ERROR) << "bssid requested but condition not met: *m_reason_code == eReasonCode::DEFAULT_PCP_OR_PRIMARY_VLAN_ID_NOT_PROVIDED || *m_reason_code == eReasonCode::NUMBER_OF_UNIQUE_VLAN_ID_EXCEEDS_MAXIMUM_SUPPORTED";
        return nullptr;
    }
    return (sMacAddr*)(m_bssid);
}

bool tlvProfile2ErrorCode::set_bssid(const sMacAddr bssid) {
    if (!m_bssid_allocated && !alloc_bssid()) {
        LOG(ERROR) << "Could not allocate bssid!";
        return false;
    }
    *m_bssid = bssid;
    return true;
}

bool tlvProfile2ErrorCode::alloc_service_prioritization_rule_id() {
    if (m_service_prioritization_rule_id_allocated) {
        LOG(ERROR) << "service_prioritization_rule_id already allocated!";
        return false;
    }
    size_t len = sizeof(uint32_t);
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    uint8_t *src = (uint8_t *)m_service_prioritization_rule_id;
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    m_service_prioritization_rule_id_allocated = true;
    return true;
}

uint32_t* tlvProfile2ErrorCode::service_prioritization_rule_id() {
    if (!m_reason_code || !(*m_reason_code == eReasonCode::SERVICE_PRIORITIZATION_RULE_NOT_FOUND || *m_reason_code == eReasonCode::NUMBER_OF_SERVICE_PRIORITIZATION_RULES_EXCEEDED_THE_MAXIMUM_SUPPORTED)) {
        TLVF_LOG(ERROR) << "service_prioritization_rule_id requested but condition not met: *m_reason_code == eReasonCode::SERVICE_PRIORITIZATION_RULE_NOT_FOUND || *m_reason_code == eReasonCode::NUMBER_OF_SERVICE_PRIORITIZATION_RULES_EXCEEDED_THE_MAXIMUM_SUPPORTED";
        return nullptr;
    }
    return (uint32_t*)(m_service_prioritization_rule_id);
}

bool tlvProfile2ErrorCode::set_service_prioritization_rule_id(const uint32_t service_prioritization_rule_id) {
    if (!m_service_prioritization_rule_id_allocated && !alloc_service_prioritization_rule_id()) {
        LOG(ERROR) << "Could not allocate service_prioritization_rule_id!";
        return false;
    }
    *m_service_prioritization_rule_id = service_prioritization_rule_id;
    return true;
}

void tlvProfile2ErrorCode::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    tlvf_swap(8*sizeof(eReasonCode), reinterpret_cast<uint8_t*>(m_reason_code));
    if (*m_reason_code == eReasonCode::DEFAULT_PCP_OR_PRIMARY_VLAN_ID_NOT_PROVIDED || *m_reason_code == eReasonCode::NUMBER_OF_UNIQUE_VLAN_ID_EXCEEDS_MAXIMUM_SUPPORTED) {
        m_bssid->struct_swap();
    }
    if (*m_reason_code == eReasonCode::SERVICE_PRIORITIZATION_RULE_NOT_FOUND || *m_reason_code == eReasonCode::NUMBER_OF_SERVICE_PRIORITIZATION_RULES_EXCEEDED_THE_MAXIMUM_SUPPORTED) {
        tlvf_swap(32, reinterpret_cast<uint8_t*>(m_service_prioritization_rule_id));
    }
}

bool tlvProfile2ErrorCode::finalize()
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

size_t tlvProfile2ErrorCode::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(eReasonCode); // reason_code
    return class_size;
}

bool tlvProfile2ErrorCode::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_PROFILE2_ERROR_CODE;
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
    m_reason_code = reinterpret_cast<eReasonCode*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(eReasonCode))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eReasonCode) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(eReasonCode); }
    m_bssid = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if ((*m_reason_code == eReasonCode::DEFAULT_PCP_OR_PRIMARY_VLAN_ID_NOT_PROVIDED || *m_reason_code == eReasonCode::NUMBER_OF_UNIQUE_VLAN_ID_EXCEEDS_MAXIMUM_SUPPORTED) && !buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_bssid->struct_init(); }
    m_service_prioritization_rule_id = reinterpret_cast<uint32_t*>(m_buff_ptr__);
    if ((*m_reason_code == eReasonCode::SERVICE_PRIORITIZATION_RULE_NOT_FOUND || *m_reason_code == eReasonCode::NUMBER_OF_SERVICE_PRIORITIZATION_RULES_EXCEEDED_THE_MAXIMUM_SUPPORTED) && !buffPtrIncrementSafe(sizeof(uint32_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint32_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_PROFILE2_ERROR_CODE) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_PROFILE2_ERROR_CODE) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}


