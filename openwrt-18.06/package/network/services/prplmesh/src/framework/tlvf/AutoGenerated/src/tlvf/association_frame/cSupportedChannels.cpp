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

#include <tlvf/association_frame/cSupportedChannels.h>
#include <tlvf/tlvflogging.h>

using namespace assoc_frame;

cSupportedChannels::cSupportedChannels(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cSupportedChannels::cSupportedChannels(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cSupportedChannels::~cSupportedChannels() {
}
eElementID& cSupportedChannels::type() {
    return (eElementID&)(*m_type);
}

const uint8_t& cSupportedChannels::length() {
    return (const uint8_t&)(*m_length);
}

std::tuple<bool, cSupportedChannels::sSupportedChannelsSet&> cSupportedChannels::supported_channel_sets(size_t idx) {
    bool ret_success = ( (m_supported_channel_sets_idx__ > 0) && (m_supported_channel_sets_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, m_supported_channel_sets[ret_idx]);
}

bool cSupportedChannels::alloc_supported_channel_sets(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list supported_channel_sets, abort!";
        return false;
    }
    size_t len = sizeof(sSupportedChannelsSet) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_supported_channel_sets[m_supported_channel_sets_idx__];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_supported_channel_sets_idx__ += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    if (!m_parse__) { 
        for (size_t i = m_supported_channel_sets_idx__ - count; i < m_supported_channel_sets_idx__; i++) { m_supported_channel_sets[i].struct_init(); }
    }
    return true;
}

void cSupportedChannels::class_swap()
{
    for (size_t i = 0; i < m_supported_channel_sets_idx__; i++){
        m_supported_channel_sets[i].struct_swap();
    }
}

bool cSupportedChannels::finalize()
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

size_t cSupportedChannels::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eElementID); // type
    class_size += sizeof(uint8_t); // length
    return class_size;
}

bool cSupportedChannels::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eElementID*>(m_buff_ptr__);
    if (!m_parse__) *m_type = ID_SUP_CHANNELS;
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
    m_supported_channel_sets = reinterpret_cast<sSupportedChannelsSet*>(m_buff_ptr__);
    if (m_length && m_parse__) {
        auto swap_len = *m_length;
        tlvf_swap((sizeof(swap_len) * 8), reinterpret_cast<uint8_t*>(&swap_len));
        size_t len = swap_len;
        len -= (m_buff_ptr__ - sizeof(*m_type) - sizeof(*m_length) - m_buff__);
        m_supported_channel_sets_idx__ = len/sizeof(sSupportedChannelsSet);
        if (!buffPtrIncrementSafe(len)) {
            LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
            return false;
        }
    }
    if (m_parse__) { class_swap(); }
    return true;
}


