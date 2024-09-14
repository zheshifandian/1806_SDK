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

#include <tlvf/association_frame/cMobilityDomain.h>
#include <tlvf/tlvflogging.h>

using namespace assoc_frame;

cMobilityDomain::cMobilityDomain(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cMobilityDomain::cMobilityDomain(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cMobilityDomain::~cMobilityDomain() {
}
eElementID& cMobilityDomain::type() {
    return (eElementID&)(*m_type);
}

const uint8_t& cMobilityDomain::length() {
    return (const uint8_t&)(*m_length);
}

uint16_t& cMobilityDomain::mdid() {
    return (uint16_t&)(*m_mdid);
}

cMobilityDomain::sFtCapabilityPolicy& cMobilityDomain::ft_cap_policy() {
    return (sFtCapabilityPolicy&)(*m_ft_cap_policy);
}

void cMobilityDomain::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_mdid));
    m_ft_cap_policy->struct_swap();
}

bool cMobilityDomain::finalize()
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

size_t cMobilityDomain::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eElementID); // type
    class_size += sizeof(uint8_t); // length
    class_size += sizeof(uint16_t); // mdid
    class_size += sizeof(sFtCapabilityPolicy); // ft_cap_policy
    return class_size;
}

bool cMobilityDomain::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eElementID*>(m_buff_ptr__);
    if (!m_parse__) *m_type = ID_MOBILITY_DOMAIN;
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
    m_mdid = reinterpret_cast<uint16_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint16_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint16_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint16_t); }
    m_ft_cap_policy = reinterpret_cast<sFtCapabilityPolicy*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sFtCapabilityPolicy))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sFtCapabilityPolicy) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(sFtCapabilityPolicy); }
    if (!m_parse__) { m_ft_cap_policy->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}


