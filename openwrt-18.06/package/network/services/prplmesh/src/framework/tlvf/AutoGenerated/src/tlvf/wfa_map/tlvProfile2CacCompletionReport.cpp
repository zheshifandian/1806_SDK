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

#include <tlvf/wfa_map/tlvProfile2CacCompletionReport.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvProfile2CacCompletionReport::tlvProfile2CacCompletionReport(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvProfile2CacCompletionReport::tlvProfile2CacCompletionReport(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvProfile2CacCompletionReport::~tlvProfile2CacCompletionReport() {
}
const eTlvTypeMap& tlvProfile2CacCompletionReport::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvProfile2CacCompletionReport::length() {
    return (const uint16_t&)(*m_length);
}

uint8_t& tlvProfile2CacCompletionReport::number_of_cac_radios() {
    return (uint8_t&)(*m_number_of_cac_radios);
}

std::tuple<bool, cCacCompletionReportRadio&> tlvProfile2CacCompletionReport::cac_radios(size_t idx) {
    bool ret_success = ( (m_cac_radios_idx__ > 0) && (m_cac_radios_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, *(m_cac_radios_vector[ret_idx]));
}

std::shared_ptr<cCacCompletionReportRadio> tlvProfile2CacCompletionReport::create_cac_radios() {
    if (m_lock_order_counter__ > 0) {
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list cac_radios, abort!";
        return nullptr;
    }
    size_t len = cCacCompletionReportRadio::get_initial_size();
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
    uint8_t *src = (uint8_t *)m_cac_radios;
    if (m_cac_radios_idx__ > 0) {
        src = (uint8_t *)m_cac_radios_vector[m_cac_radios_idx__ - 1]->getBuffPtr();
    }
    if (!m_parse__) {
        uint8_t *dst = src + len;
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    return std::make_shared<cCacCompletionReportRadio>(src, getBuffRemainingBytes(src), m_parse__);
}

bool tlvProfile2CacCompletionReport::add_cac_radios(std::shared_ptr<cCacCompletionReportRadio> ptr) {
    if (ptr == nullptr) {
        TLVF_LOG(ERROR) << "Received entry is nullptr";
        return false;
    }
    if (m_lock_allocation__ == false) {
        TLVF_LOG(ERROR) << "No call to create_cac_radios was called before add_cac_radios";
        return false;
    }
    uint8_t *src = (uint8_t *)m_cac_radios;
    if (m_cac_radios_idx__ > 0) {
        src = (uint8_t *)m_cac_radios_vector[m_cac_radios_idx__ - 1]->getBuffPtr();
    }
    if (ptr->getStartBuffPtr() != src) {
        TLVF_LOG(ERROR) << "Received entry pointer is different than expected (expecting the same pointer returned from add method)";
        return false;
    }
    if (ptr->getLen() > getBuffRemainingBytes(ptr->getStartBuffPtr())) {;
        TLVF_LOG(ERROR) << "Not enough available space on buffer";
        return false;
    }
    m_cac_radios_idx__++;
    if (!m_parse__) { (*m_number_of_cac_radios)++; }
    size_t len = ptr->getLen();
    m_cac_radios_vector.push_back(ptr);
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(!m_parse__ && m_length){ (*m_length) += len; }
    m_lock_allocation__ = false;
    return true;
}

void tlvProfile2CacCompletionReport::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    for (size_t i = 0; i < m_cac_radios_idx__; i++){
        std::get<1>(cac_radios(i)).class_swap();
    }
}

bool tlvProfile2CacCompletionReport::finalize()
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

size_t tlvProfile2CacCompletionReport::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(uint8_t); // number_of_cac_radios
    return class_size;
}

bool tlvProfile2CacCompletionReport::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_PROFILE2_CAC_COMPLETION_REPORT;
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
    m_number_of_cac_radios = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_number_of_cac_radios = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_cac_radios = reinterpret_cast<cCacCompletionReportRadio*>(m_buff_ptr__);
    uint8_t number_of_cac_radios = *m_number_of_cac_radios;
    m_cac_radios_idx__ = 0;
    for (size_t i = 0; i < number_of_cac_radios; i++) {
        auto cac_radios = create_cac_radios();
        if (!cac_radios || !cac_radios->isInitialized()) {
            TLVF_LOG(ERROR) << "create_cac_radios() failed";
            return false;
        }
        if (!add_cac_radios(cac_radios)) {
            TLVF_LOG(ERROR) << "add_cac_radios() failed";
            return false;
        }
        // swap back since cac_radios will be swapped as part of the whole class swap
        cac_radios->class_swap();
    }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_PROFILE2_CAC_COMPLETION_REPORT) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_PROFILE2_CAC_COMPLETION_REPORT) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}

cCacCompletionReportRadio::cCacCompletionReportRadio(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cCacCompletionReportRadio::cCacCompletionReportRadio(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cCacCompletionReportRadio::~cCacCompletionReportRadio() {
}
sMacAddr& cCacCompletionReportRadio::radio_uid() {
    return (sMacAddr&)(*m_radio_uid);
}

uint8_t& cCacCompletionReportRadio::operating_class() {
    return (uint8_t&)(*m_operating_class);
}

uint8_t& cCacCompletionReportRadio::channel() {
    return (uint8_t&)(*m_channel);
}

cCacCompletionReportRadio::eCompletionStatus& cCacCompletionReportRadio::cac_completion_status() {
    return (eCompletionStatus&)(*m_cac_completion_status);
}

uint8_t& cCacCompletionReportRadio::number_of_detected_pairs() {
    return (uint8_t&)(*m_number_of_detected_pairs);
}

std::tuple<bool, cCacCompletionReportRadio::sCacDetectedPair&> cCacCompletionReportRadio::detected_pairs(size_t idx) {
    bool ret_success = ( (m_detected_pairs_idx__ > 0) && (m_detected_pairs_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, m_detected_pairs[ret_idx]);
}

bool cCacCompletionReportRadio::alloc_detected_pairs(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list detected_pairs, abort!";
        return false;
    }
    size_t len = sizeof(sCacDetectedPair) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_detected_pairs[*m_number_of_detected_pairs];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_detected_pairs_idx__ += count;
    *m_number_of_detected_pairs += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if (!m_parse__) { 
        for (size_t i = m_detected_pairs_idx__ - count; i < m_detected_pairs_idx__; i++) { m_detected_pairs[i].struct_init(); }
    }
    return true;
}

void cCacCompletionReportRadio::class_swap()
{
    m_radio_uid->struct_swap();
    for (size_t i = 0; i < m_detected_pairs_idx__; i++){
        m_detected_pairs[i].struct_swap();
    }
}

bool cCacCompletionReportRadio::finalize()
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

size_t cCacCompletionReportRadio::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sMacAddr); // radio_uid
    class_size += sizeof(uint8_t); // operating_class
    class_size += sizeof(uint8_t); // channel
    class_size += sizeof(eCompletionStatus); // cac_completion_status
    class_size += sizeof(uint8_t); // number_of_detected_pairs
    return class_size;
}

bool cCacCompletionReportRadio::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_radio_uid = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_radio_uid->struct_init(); }
    m_operating_class = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_channel = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_cac_completion_status = reinterpret_cast<eCompletionStatus*>(m_buff_ptr__);
    if (!m_parse__) *m_cac_completion_status = NOT_PERFORMED;
    if (!buffPtrIncrementSafe(sizeof(eCompletionStatus))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eCompletionStatus) << ") Failed!";
        return false;
    }
    m_number_of_detected_pairs = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_number_of_detected_pairs = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_detected_pairs = reinterpret_cast<sCacDetectedPair*>(m_buff_ptr__);
    uint8_t number_of_detected_pairs = *m_number_of_detected_pairs;
    m_detected_pairs_idx__ = number_of_detected_pairs;
    if (!buffPtrIncrementSafe(sizeof(sCacDetectedPair) * (number_of_detected_pairs))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sCacDetectedPair) * (number_of_detected_pairs) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}


