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

#include <tlvf/wfa_map/tlvProfile2TrafficSeparationPolicy.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvProfile2TrafficSeparationPolicy::tlvProfile2TrafficSeparationPolicy(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvProfile2TrafficSeparationPolicy::tlvProfile2TrafficSeparationPolicy(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvProfile2TrafficSeparationPolicy::~tlvProfile2TrafficSeparationPolicy() {
}
const eTlvTypeMap& tlvProfile2TrafficSeparationPolicy::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvProfile2TrafficSeparationPolicy::length() {
    return (const uint16_t&)(*m_length);
}

uint8_t& tlvProfile2TrafficSeparationPolicy::ssids_vlan_id_list_length() {
    return (uint8_t&)(*m_ssids_vlan_id_list_length);
}

std::tuple<bool, cSsidVlanId&> tlvProfile2TrafficSeparationPolicy::ssids_vlan_id_list(size_t idx) {
    bool ret_success = ( (m_ssids_vlan_id_list_idx__ > 0) && (m_ssids_vlan_id_list_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, *(m_ssids_vlan_id_list_vector[ret_idx]));
}

std::shared_ptr<cSsidVlanId> tlvProfile2TrafficSeparationPolicy::create_ssids_vlan_id_list() {
    if (m_lock_order_counter__ > 0) {
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list ssids_vlan_id_list, abort!";
        return nullptr;
    }
    size_t len = cSsidVlanId::get_initial_size();
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
    uint8_t *src = (uint8_t *)m_ssids_vlan_id_list;
    if (m_ssids_vlan_id_list_idx__ > 0) {
        src = (uint8_t *)m_ssids_vlan_id_list_vector[m_ssids_vlan_id_list_idx__ - 1]->getBuffPtr();
    }
    if (!m_parse__) {
        uint8_t *dst = src + len;
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    return std::make_shared<cSsidVlanId>(src, getBuffRemainingBytes(src), m_parse__);
}

bool tlvProfile2TrafficSeparationPolicy::add_ssids_vlan_id_list(std::shared_ptr<cSsidVlanId> ptr) {
    if (ptr == nullptr) {
        TLVF_LOG(ERROR) << "Received entry is nullptr";
        return false;
    }
    if (m_lock_allocation__ == false) {
        TLVF_LOG(ERROR) << "No call to create_ssids_vlan_id_list was called before add_ssids_vlan_id_list";
        return false;
    }
    uint8_t *src = (uint8_t *)m_ssids_vlan_id_list;
    if (m_ssids_vlan_id_list_idx__ > 0) {
        src = (uint8_t *)m_ssids_vlan_id_list_vector[m_ssids_vlan_id_list_idx__ - 1]->getBuffPtr();
    }
    if (ptr->getStartBuffPtr() != src) {
        TLVF_LOG(ERROR) << "Received entry pointer is different than expected (expecting the same pointer returned from add method)";
        return false;
    }
    if (ptr->getLen() > getBuffRemainingBytes(ptr->getStartBuffPtr())) {;
        TLVF_LOG(ERROR) << "Not enough available space on buffer";
        return false;
    }
    m_ssids_vlan_id_list_idx__++;
    if (!m_parse__) { (*m_ssids_vlan_id_list_length)++; }
    size_t len = ptr->getLen();
    m_ssids_vlan_id_list_vector.push_back(ptr);
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(!m_parse__ && m_length){ (*m_length) += len; }
    m_lock_allocation__ = false;
    return true;
}

void tlvProfile2TrafficSeparationPolicy::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    for (size_t i = 0; i < m_ssids_vlan_id_list_idx__; i++){
        std::get<1>(ssids_vlan_id_list(i)).class_swap();
    }
}

bool tlvProfile2TrafficSeparationPolicy::finalize()
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

size_t tlvProfile2TrafficSeparationPolicy::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(uint8_t); // ssids_vlan_id_list_length
    return class_size;
}

bool tlvProfile2TrafficSeparationPolicy::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_PROFILE2_TRAFFIC_SEPARATION_POLICY;
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
    m_ssids_vlan_id_list_length = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_ssids_vlan_id_list_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_ssids_vlan_id_list = reinterpret_cast<cSsidVlanId*>(m_buff_ptr__);
    uint8_t ssids_vlan_id_list_length = *m_ssids_vlan_id_list_length;
    m_ssids_vlan_id_list_idx__ = 0;
    for (size_t i = 0; i < ssids_vlan_id_list_length; i++) {
        auto ssids_vlan_id_list = create_ssids_vlan_id_list();
        if (!ssids_vlan_id_list || !ssids_vlan_id_list->isInitialized()) {
            TLVF_LOG(ERROR) << "create_ssids_vlan_id_list() failed";
            return false;
        }
        if (!add_ssids_vlan_id_list(ssids_vlan_id_list)) {
            TLVF_LOG(ERROR) << "add_ssids_vlan_id_list() failed";
            return false;
        }
        // swap back since ssids_vlan_id_list will be swapped as part of the whole class swap
        ssids_vlan_id_list->class_swap();
    }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_PROFILE2_TRAFFIC_SEPARATION_POLICY) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_PROFILE2_TRAFFIC_SEPARATION_POLICY) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}

cSsidVlanId::cSsidVlanId(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cSsidVlanId::cSsidVlanId(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cSsidVlanId::~cSsidVlanId() {
}
uint8_t& cSsidVlanId::ssid_name_length() {
    return (uint8_t&)(*m_ssid_name_length);
}

std::string cSsidVlanId::ssid_name_str() {
    char *ssid_name_ = ssid_name();
    if (!ssid_name_) { return std::string(); }
    auto str = std::string(ssid_name_, m_ssid_name_idx__);
    auto pos = str.find_first_of('\0');
    if (pos != std::string::npos) {
        str.erase(pos);
    }
    return str;
}

char* cSsidVlanId::ssid_name(size_t length) {
    if( (m_ssid_name_idx__ == 0) || (m_ssid_name_idx__ < length) ) {
        TLVF_LOG(ERROR) << "ssid_name length is smaller than requested length";
        return nullptr;
    }
    return ((char*)m_ssid_name);
}

bool cSsidVlanId::set_ssid_name(const std::string& str) { return set_ssid_name(str.c_str(), str.size()); }
bool cSsidVlanId::set_ssid_name(const char str[], size_t size) {
    if (str == nullptr) {
        TLVF_LOG(WARNING) << "set_ssid_name received a null pointer.";
        return false;
    }
    if (m_ssid_name_idx__ != 0) {
        TLVF_LOG(ERROR) << "set_ssid_name was already allocated!";
        return false;
    }
    if (!alloc_ssid_name(size)) { return false; }
    std::copy(str, str + size, m_ssid_name);
    return true;
}
bool cSsidVlanId::alloc_ssid_name(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list ssid_name, abort!";
        return false;
    }
    size_t len = sizeof(char) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_ssid_name[*m_ssid_name_length];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_vlan_id = (uint16_t *)((uint8_t *)(m_vlan_id) + len);
    m_ssid_name_idx__ += count;
    *m_ssid_name_length += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    return true;
}

uint16_t& cSsidVlanId::vlan_id() {
    return (uint16_t&)(*m_vlan_id);
}

void cSsidVlanId::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_vlan_id));
}

bool cSsidVlanId::finalize()
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

size_t cSsidVlanId::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // ssid_name_length
    class_size += sizeof(uint16_t); // vlan_id
    return class_size;
}

bool cSsidVlanId::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_ssid_name_length = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_ssid_name_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_ssid_name = reinterpret_cast<char*>(m_buff_ptr__);
    uint8_t ssid_name_length = *m_ssid_name_length;
    m_ssid_name_idx__ = ssid_name_length;
    if (!buffPtrIncrementSafe(sizeof(char) * (ssid_name_length))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(char) * (ssid_name_length) << ") Failed!";
        return false;
    }
    m_vlan_id = reinterpret_cast<uint16_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint16_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint16_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}


