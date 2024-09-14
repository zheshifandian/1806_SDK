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

#include <tlvf/association_frame/cCapInfoDmgSta.h>
#include <tlvf/tlvflogging.h>

using namespace assoc_frame;

cCapInfoDmgSta::cCapInfoDmgSta(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cCapInfoDmgSta::cCapInfoDmgSta(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cCapInfoDmgSta::~cCapInfoDmgSta() {
}
cCapInfoDmgSta::sDmgParam& cCapInfoDmgSta::dmg_param() {
    return (sDmgParam&)(*m_dmg_param);
}

cCapInfoDmgSta::sInfoSubField& cCapInfoDmgSta::cap_info() {
    return (sInfoSubField&)(*m_cap_info);
}

uint16_t& cCapInfoDmgSta::listen_interval() {
    return (uint16_t&)(*m_listen_interval);
}

void cCapInfoDmgSta::class_swap()
{
    m_dmg_param->struct_swap();
    m_cap_info->struct_swap();
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_listen_interval));
}

bool cCapInfoDmgSta::finalize()
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
    }
    class_swap();
    m_finalized__ = true;
    return true;
}

size_t cCapInfoDmgSta::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sDmgParam); // dmg_param
    class_size += sizeof(sInfoSubField); // cap_info
    class_size += sizeof(uint16_t); // listen_interval
    return class_size;
}

bool cCapInfoDmgSta::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_dmg_param = reinterpret_cast<sDmgParam*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sDmgParam))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sDmgParam) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_dmg_param->struct_init(); }
    m_cap_info = reinterpret_cast<sInfoSubField*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sInfoSubField))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sInfoSubField) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_cap_info->struct_init(); }
    m_listen_interval = reinterpret_cast<uint16_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint16_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint16_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}


