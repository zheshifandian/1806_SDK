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

#include <beerocks/tlvf/classes/ChannelList.h>
#include <tlvf/tlvflogging.h>

using namespace beerocks_message;

cChannelList::cChannelList(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cChannelList::cChannelList(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cChannelList::~cChannelList() {
}
uint8_t& cChannelList::channels_list_length() {
    return (uint8_t&)(*m_channels_list_length);
}

std::tuple<bool, cChannel&> cChannelList::channels_list(size_t idx) {
    bool ret_success = ( (m_channels_list_idx__ > 0) && (m_channels_list_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, *(m_channels_list_vector[ret_idx]));
}

std::shared_ptr<cChannel> cChannelList::create_channels_list() {
    if (m_lock_order_counter__ > 0) {
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list channels_list, abort!";
        return nullptr;
    }
    size_t len = cChannel::get_initial_size();
    if (m_lock_allocation__) {
        TLVF_LOG(ERROR) << "Can't create new element before adding the previous one";
        return nullptr;
    }
    if (getBuffRemainingBytes() < len) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer";
        return nullptr;
    }
    m_lock_order_counter__ = 0;
    m_lock_allocation__ = true;
    uint8_t *src = (uint8_t *)m_channels_list;
    if (m_channels_list_idx__ > 0) {
        src = (uint8_t *)m_channels_list_vector[m_channels_list_idx__ - 1]->getBuffPtr();
    }
    if (!m_parse__) {
        uint8_t *dst = src + len;
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    return std::make_shared<cChannel>(src, getBuffRemainingBytes(src), m_parse__);
}

bool cChannelList::add_channels_list(std::shared_ptr<cChannel> ptr) {
    if (ptr == nullptr) {
        TLVF_LOG(ERROR) << "Received entry is nullptr";
        return false;
    }
    if (m_lock_allocation__ == false) {
        TLVF_LOG(ERROR) << "No call to create_channels_list was called before add_channels_list";
        return false;
    }
    uint8_t *src = (uint8_t *)m_channels_list;
    if (m_channels_list_idx__ > 0) {
        src = (uint8_t *)m_channels_list_vector[m_channels_list_idx__ - 1]->getBuffPtr();
    }
    if (ptr->getStartBuffPtr() != src) {
        TLVF_LOG(ERROR) << "Received entry pointer is different than expected (expecting the same pointer returned from add method)";
        return false;
    }
    if (ptr->getLen() > getBuffRemainingBytes(ptr->getStartBuffPtr())) {;
        TLVF_LOG(ERROR) << "Not enough available space on buffer";
        return false;
    }
    m_channels_list_idx__++;
    if (!m_parse__) { (*m_channels_list_length)++; }
    size_t len = ptr->getLen();
    m_channels_list_vector.push_back(ptr);
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    m_lock_allocation__ = false;
    return true;
}

void cChannelList::class_swap()
{
    for (size_t i = 0; i < m_channels_list_idx__; i++){
        std::get<1>(channels_list(i)).class_swap();
    }
}

bool cChannelList::finalize()
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

size_t cChannelList::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // channels_list_length
    return class_size;
}

bool cChannelList::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_channels_list_length = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_channels_list_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_channels_list = reinterpret_cast<cChannel*>(m_buff_ptr__);
    uint8_t channels_list_length = *m_channels_list_length;
    m_channels_list_idx__ = 0;
    for (size_t i = 0; i < channels_list_length; i++) {
        auto channels_list = create_channels_list();
        if (!channels_list || !channels_list->isInitialized()) {
            TLVF_LOG(ERROR) << "create_channels_list() failed";
            return false;
        }
        if (!add_channels_list(channels_list)) {
            TLVF_LOG(ERROR) << "add_channels_list() failed";
            return false;
        }
        // swap back since channels_list will be swapped as part of the whole class swap
        channels_list->class_swap();
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cChannel::cChannel(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cChannel::cChannel(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cChannel::~cChannel() {
}
uint8_t& cChannel::beacon_channel() {
    return (uint8_t&)(*m_beacon_channel);
}

int8_t& cChannel::tx_power_dbm() {
    return (int8_t&)(*m_tx_power_dbm);
}

eDfsState& cChannel::dfs_state() {
    return (eDfsState&)(*m_dfs_state);
}

uint8_t& cChannel::supported_bandwidths_length() {
    return (uint8_t&)(*m_supported_bandwidths_length);
}

std::tuple<bool, sSupportedBandwidth&> cChannel::supported_bandwidths(size_t idx) {
    bool ret_success = ( (m_supported_bandwidths_idx__ > 0) && (m_supported_bandwidths_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, m_supported_bandwidths[ret_idx]);
}

bool cChannel::alloc_supported_bandwidths(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list supported_bandwidths, abort!";
        return false;
    }
    size_t len = sizeof(sSupportedBandwidth) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_supported_bandwidths[*m_supported_bandwidths_length];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_supported_bandwidths_idx__ += count;
    *m_supported_bandwidths_length += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if (!m_parse__) { 
        for (size_t i = m_supported_bandwidths_idx__ - count; i < m_supported_bandwidths_idx__; i++) { m_supported_bandwidths[i].struct_init(); }
    }
    return true;
}

void cChannel::class_swap()
{
    for (size_t i = 0; i < m_supported_bandwidths_idx__; i++){
        m_supported_bandwidths[i].struct_swap();
    }
}

bool cChannel::finalize()
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

size_t cChannel::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // beacon_channel
    class_size += sizeof(int8_t); // tx_power_dbm
    class_size += sizeof(eDfsState); // dfs_state
    class_size += sizeof(uint8_t); // supported_bandwidths_length
    return class_size;
}

bool cChannel::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_beacon_channel = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_tx_power_dbm = reinterpret_cast<int8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_tx_power_dbm = INT8_MIN;
    if (!buffPtrIncrementSafe(sizeof(int8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(int8_t) << ") Failed!";
        return false;
    }
    m_dfs_state = reinterpret_cast<eDfsState*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(eDfsState))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eDfsState) << ") Failed!";
        return false;
    }
    m_supported_bandwidths_length = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_supported_bandwidths_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_supported_bandwidths = reinterpret_cast<sSupportedBandwidth*>(m_buff_ptr__);
    uint8_t supported_bandwidths_length = *m_supported_bandwidths_length;
    m_supported_bandwidths_idx__ = supported_bandwidths_length;
    if (!buffPtrIncrementSafe(sizeof(sSupportedBandwidth) * (supported_bandwidths_length))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sSupportedBandwidth) * (supported_bandwidths_length) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}


