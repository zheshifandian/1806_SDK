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

#include <tlvf/wfa_map/tlvBackhaulStaRadioCapabilities.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvBackhaulStaRadioCapabilities::tlvBackhaulStaRadioCapabilities(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvBackhaulStaRadioCapabilities::tlvBackhaulStaRadioCapabilities(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvBackhaulStaRadioCapabilities::~tlvBackhaulStaRadioCapabilities() {
}
const eTlvTypeMap& tlvBackhaulStaRadioCapabilities::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvBackhaulStaRadioCapabilities::length() {
    return (const uint16_t&)(*m_length);
}

sMacAddr& tlvBackhaulStaRadioCapabilities::ruid() {
    return (sMacAddr&)(*m_ruid);
}

tlvBackhaulStaRadioCapabilities::eStaMacIncluded& tlvBackhaulStaRadioCapabilities::sta_mac_included() {
    return (eStaMacIncluded&)(*m_sta_mac_included);
}

bool tlvBackhaulStaRadioCapabilities::alloc_sta_mac() {
    if (m_sta_mac_allocated) {
        LOG(ERROR) << "sta_mac already allocated!";
        return false;
    }
    size_t len = sizeof(sMacAddr);
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    uint8_t *src = (uint8_t *)m_sta_mac;
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
    m_sta_mac_allocated = true;
    return true;
}

sMacAddr* tlvBackhaulStaRadioCapabilities::sta_mac() {
    if (!m_sta_mac_included || !(*m_sta_mac_included == eStaMacIncluded::FIELD_PRESENT)) {
        TLVF_LOG(ERROR) << "sta_mac requested but condition not met: *m_sta_mac_included == eStaMacIncluded::FIELD_PRESENT";
        return nullptr;
    }
    return (sMacAddr*)(m_sta_mac);
}

bool tlvBackhaulStaRadioCapabilities::set_sta_mac(const sMacAddr sta_mac) {
    if (!m_sta_mac_allocated && !alloc_sta_mac()) {
        LOG(ERROR) << "Could not allocate sta_mac!";
        return false;
    }
    *m_sta_mac = sta_mac;
    return true;
}

void tlvBackhaulStaRadioCapabilities::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    m_ruid->struct_swap();
    if (*m_sta_mac_included == eStaMacIncluded::FIELD_PRESENT) {
        m_sta_mac->struct_swap();
    }
}

bool tlvBackhaulStaRadioCapabilities::finalize()
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

size_t tlvBackhaulStaRadioCapabilities::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(sMacAddr); // ruid
    class_size += sizeof(eStaMacIncluded); // sta_mac_included
    return class_size;
}

bool tlvBackhaulStaRadioCapabilities::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_BACKHAUL_STA_RADIO_CAPABILITIES;
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
    m_ruid = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(sMacAddr); }
    if (!m_parse__) { m_ruid->struct_init(); }
    m_sta_mac_included = reinterpret_cast<eStaMacIncluded*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(eStaMacIncluded))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eStaMacIncluded) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(eStaMacIncluded); }
    m_sta_mac = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if ((*m_sta_mac_included == eStaMacIncluded::FIELD_PRESENT) && !buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_sta_mac->struct_init(); }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_BACKHAUL_STA_RADIO_CAPABILITIES) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_BACKHAUL_STA_RADIO_CAPABILITIES) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}


