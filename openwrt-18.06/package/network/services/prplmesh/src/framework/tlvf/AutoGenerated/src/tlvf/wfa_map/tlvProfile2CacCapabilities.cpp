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

#include <tlvf/wfa_map/tlvProfile2CacCapabilities.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvProfile2CacCapabilities::tlvProfile2CacCapabilities(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvProfile2CacCapabilities::tlvProfile2CacCapabilities(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvProfile2CacCapabilities::~tlvProfile2CacCapabilities() {
}
const eTlvTypeMap& tlvProfile2CacCapabilities::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvProfile2CacCapabilities::length() {
    return (const uint16_t&)(*m_length);
}

uint8_t* tlvProfile2CacCapabilities::country_code(size_t idx) {
    if ( (m_country_code_idx__ == 0) || (m_country_code_idx__ <= idx) ) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
        return nullptr;
    }
    return &(m_country_code[idx]);
}

bool tlvProfile2CacCapabilities::set_country_code(const void* buffer, size_t size) {
    if (buffer == nullptr) {
        TLVF_LOG(WARNING) << "set_country_code received a null pointer.";
        return false;
    }
    if (size > 2) {
        TLVF_LOG(ERROR) << "Received buffer size is smaller than buffer length";
        return false;
    }
    std::copy_n(reinterpret_cast<const uint8_t *>(buffer), size, m_country_code);
    return true;
}
uint8_t& tlvProfile2CacCapabilities::number_of_cac_radios() {
    return (uint8_t&)(*m_number_of_cac_radios);
}

std::tuple<bool, cCacCapabilitiesRadio&> tlvProfile2CacCapabilities::cac_radios(size_t idx) {
    bool ret_success = ( (m_cac_radios_idx__ > 0) && (m_cac_radios_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, *(m_cac_radios_vector[ret_idx]));
}

std::shared_ptr<cCacCapabilitiesRadio> tlvProfile2CacCapabilities::create_cac_radios() {
    if (m_lock_order_counter__ > 0) {
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list cac_radios, abort!";
        return nullptr;
    }
    size_t len = cCacCapabilitiesRadio::get_initial_size();
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
    return std::make_shared<cCacCapabilitiesRadio>(src, getBuffRemainingBytes(src), m_parse__);
}

bool tlvProfile2CacCapabilities::add_cac_radios(std::shared_ptr<cCacCapabilitiesRadio> ptr) {
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

void tlvProfile2CacCapabilities::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    for (size_t i = 0; i < m_cac_radios_idx__; i++){
        std::get<1>(cac_radios(i)).class_swap();
    }
}

bool tlvProfile2CacCapabilities::finalize()
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

size_t tlvProfile2CacCapabilities::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += 2 * sizeof(uint8_t); // country_code
    class_size += sizeof(uint8_t); // number_of_cac_radios
    return class_size;
}

bool tlvProfile2CacCapabilities::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_PROFILE2_CAC_CAPABILITIES;
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
    m_country_code = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t) * (2))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) * (2) << ") Failed!";
        return false;
    }
    m_country_code_idx__  = 2;
    if (!m_parse__) {
        if (m_length) { (*m_length) += (sizeof(uint8_t) * 2); }
    }
    m_number_of_cac_radios = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_number_of_cac_radios = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_cac_radios = reinterpret_cast<cCacCapabilitiesRadio*>(m_buff_ptr__);
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
        if (*m_type != eTlvTypeMap::TLV_PROFILE2_CAC_CAPABILITIES) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_PROFILE2_CAC_CAPABILITIES) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}

cCacCapabilitiesRadio::cCacCapabilitiesRadio(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cCacCapabilitiesRadio::cCacCapabilitiesRadio(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cCacCapabilitiesRadio::~cCacCapabilitiesRadio() {
}
sMacAddr& cCacCapabilitiesRadio::radio_uid() {
    return (sMacAddr&)(*m_radio_uid);
}

uint8_t& cCacCapabilitiesRadio::number_of_cac_type_supported() {
    return (uint8_t&)(*m_number_of_cac_type_supported);
}

std::tuple<bool, cCacTypes&> cCacCapabilitiesRadio::cac_types(size_t idx) {
    bool ret_success = ( (m_cac_types_idx__ > 0) && (m_cac_types_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, *(m_cac_types_vector[ret_idx]));
}

std::shared_ptr<cCacTypes> cCacCapabilitiesRadio::create_cac_types() {
    if (m_lock_order_counter__ > 0) {
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list cac_types, abort!";
        return nullptr;
    }
    size_t len = cCacTypes::get_initial_size();
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
    uint8_t *src = (uint8_t *)m_cac_types;
    if (m_cac_types_idx__ > 0) {
        src = (uint8_t *)m_cac_types_vector[m_cac_types_idx__ - 1]->getBuffPtr();
    }
    if (!m_parse__) {
        uint8_t *dst = src + len;
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    return std::make_shared<cCacTypes>(src, getBuffRemainingBytes(src), m_parse__);
}

bool cCacCapabilitiesRadio::add_cac_types(std::shared_ptr<cCacTypes> ptr) {
    if (ptr == nullptr) {
        TLVF_LOG(ERROR) << "Received entry is nullptr";
        return false;
    }
    if (m_lock_allocation__ == false) {
        TLVF_LOG(ERROR) << "No call to create_cac_types was called before add_cac_types";
        return false;
    }
    uint8_t *src = (uint8_t *)m_cac_types;
    if (m_cac_types_idx__ > 0) {
        src = (uint8_t *)m_cac_types_vector[m_cac_types_idx__ - 1]->getBuffPtr();
    }
    if (ptr->getStartBuffPtr() != src) {
        TLVF_LOG(ERROR) << "Received entry pointer is different than expected (expecting the same pointer returned from add method)";
        return false;
    }
    if (ptr->getLen() > getBuffRemainingBytes(ptr->getStartBuffPtr())) {;
        TLVF_LOG(ERROR) << "Not enough available space on buffer";
        return false;
    }
    m_cac_types_idx__++;
    if (!m_parse__) { (*m_number_of_cac_type_supported)++; }
    size_t len = ptr->getLen();
    m_cac_types_vector.push_back(ptr);
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    m_lock_allocation__ = false;
    return true;
}

void cCacCapabilitiesRadio::class_swap()
{
    m_radio_uid->struct_swap();
    for (size_t i = 0; i < m_cac_types_idx__; i++){
        std::get<1>(cac_types(i)).class_swap();
    }
}

bool cCacCapabilitiesRadio::finalize()
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

size_t cCacCapabilitiesRadio::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sMacAddr); // radio_uid
    class_size += sizeof(uint8_t); // number_of_cac_type_supported
    return class_size;
}

bool cCacCapabilitiesRadio::init()
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
    m_number_of_cac_type_supported = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_cac_types = reinterpret_cast<cCacTypes*>(m_buff_ptr__);
    uint8_t number_of_cac_type_supported = *m_number_of_cac_type_supported;
    m_cac_types_idx__ = 0;
    for (size_t i = 0; i < number_of_cac_type_supported; i++) {
        auto cac_types = create_cac_types();
        if (!cac_types || !cac_types->isInitialized()) {
            TLVF_LOG(ERROR) << "create_cac_types() failed";
            return false;
        }
        if (!add_cac_types(cac_types)) {
            TLVF_LOG(ERROR) << "add_cac_types() failed";
            return false;
        }
        // swap back since cac_types will be swapped as part of the whole class swap
        cac_types->class_swap();
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cCacTypes::cCacTypes(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cCacTypes::cCacTypes(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cCacTypes::~cCacTypes() {
}
eCacMethod& cCacTypes::cac_method() {
    return (eCacMethod&)(*m_cac_method);
}

uint8_t* cCacTypes::duration(size_t idx) {
    if ( (m_duration_idx__ == 0) || (m_duration_idx__ <= idx) ) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
        return nullptr;
    }
    return &(m_duration[idx]);
}

bool cCacTypes::set_duration(const void* buffer, size_t size) {
    if (buffer == nullptr) {
        TLVF_LOG(WARNING) << "set_duration received a null pointer.";
        return false;
    }
    if (size > 3) {
        TLVF_LOG(ERROR) << "Received buffer size is smaller than buffer length";
        return false;
    }
    std::copy_n(reinterpret_cast<const uint8_t *>(buffer), size, m_duration);
    return true;
}
uint8_t& cCacTypes::number_of_operating_classes() {
    return (uint8_t&)(*m_number_of_operating_classes);
}

std::tuple<bool, cCacCapabilitiesOperatingClasses&> cCacTypes::operating_classes(size_t idx) {
    bool ret_success = ( (m_operating_classes_idx__ > 0) && (m_operating_classes_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, *(m_operating_classes_vector[ret_idx]));
}

std::shared_ptr<cCacCapabilitiesOperatingClasses> cCacTypes::create_operating_classes() {
    if (m_lock_order_counter__ > 0) {
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list operating_classes, abort!";
        return nullptr;
    }
    size_t len = cCacCapabilitiesOperatingClasses::get_initial_size();
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
    uint8_t *src = (uint8_t *)m_operating_classes;
    if (m_operating_classes_idx__ > 0) {
        src = (uint8_t *)m_operating_classes_vector[m_operating_classes_idx__ - 1]->getBuffPtr();
    }
    if (!m_parse__) {
        uint8_t *dst = src + len;
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    return std::make_shared<cCacCapabilitiesOperatingClasses>(src, getBuffRemainingBytes(src), m_parse__);
}

bool cCacTypes::add_operating_classes(std::shared_ptr<cCacCapabilitiesOperatingClasses> ptr) {
    if (ptr == nullptr) {
        TLVF_LOG(ERROR) << "Received entry is nullptr";
        return false;
    }
    if (m_lock_allocation__ == false) {
        TLVF_LOG(ERROR) << "No call to create_operating_classes was called before add_operating_classes";
        return false;
    }
    uint8_t *src = (uint8_t *)m_operating_classes;
    if (m_operating_classes_idx__ > 0) {
        src = (uint8_t *)m_operating_classes_vector[m_operating_classes_idx__ - 1]->getBuffPtr();
    }
    if (ptr->getStartBuffPtr() != src) {
        TLVF_LOG(ERROR) << "Received entry pointer is different than expected (expecting the same pointer returned from add method)";
        return false;
    }
    if (ptr->getLen() > getBuffRemainingBytes(ptr->getStartBuffPtr())) {;
        TLVF_LOG(ERROR) << "Not enough available space on buffer";
        return false;
    }
    m_operating_classes_idx__++;
    if (!m_parse__) { (*m_number_of_operating_classes)++; }
    size_t len = ptr->getLen();
    m_operating_classes_vector.push_back(ptr);
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    m_lock_allocation__ = false;
    return true;
}

void cCacTypes::class_swap()
{
    for (size_t i = 0; i < m_operating_classes_idx__; i++){
        std::get<1>(operating_classes(i)).class_swap();
    }
}

bool cCacTypes::finalize()
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

size_t cCacTypes::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eCacMethod); // cac_method
    class_size += 3 * sizeof(uint8_t); // duration
    class_size += sizeof(uint8_t); // number_of_operating_classes
    return class_size;
}

bool cCacTypes::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_cac_method = reinterpret_cast<eCacMethod*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(eCacMethod))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(eCacMethod) << ") Failed!";
        return false;
    }
    m_duration = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t) * (3))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) * (3) << ") Failed!";
        return false;
    }
    m_duration_idx__  = 3;
    m_number_of_operating_classes = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_operating_classes = reinterpret_cast<cCacCapabilitiesOperatingClasses*>(m_buff_ptr__);
    uint8_t number_of_operating_classes = *m_number_of_operating_classes;
    m_operating_classes_idx__ = 0;
    for (size_t i = 0; i < number_of_operating_classes; i++) {
        auto operating_classes = create_operating_classes();
        if (!operating_classes || !operating_classes->isInitialized()) {
            TLVF_LOG(ERROR) << "create_operating_classes() failed";
            return false;
        }
        if (!add_operating_classes(operating_classes)) {
            TLVF_LOG(ERROR) << "add_operating_classes() failed";
            return false;
        }
        // swap back since operating_classes will be swapped as part of the whole class swap
        operating_classes->class_swap();
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cCacCapabilitiesOperatingClasses::cCacCapabilitiesOperatingClasses(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cCacCapabilitiesOperatingClasses::cCacCapabilitiesOperatingClasses(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cCacCapabilitiesOperatingClasses::~cCacCapabilitiesOperatingClasses() {
}
uint8_t& cCacCapabilitiesOperatingClasses::operating_class() {
    return (uint8_t&)(*m_operating_class);
}

uint8_t& cCacCapabilitiesOperatingClasses::number_of_channels() {
    return (uint8_t&)(*m_number_of_channels);
}

uint8_t* cCacCapabilitiesOperatingClasses::channels(size_t idx) {
    if ( (m_channels_idx__ == 0) || (m_channels_idx__ <= idx) ) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
        return nullptr;
    }
    return &(m_channels[idx]);
}

bool cCacCapabilitiesOperatingClasses::set_channels(const void* buffer, size_t size) {
    if (buffer == nullptr) {
        TLVF_LOG(WARNING) << "set_channels received a null pointer.";
        return false;
    }
    if (m_channels_idx__ != 0) {
        TLVF_LOG(ERROR) << "set_channels was already allocated!";
        return false;
    }
    if (!alloc_channels(size)) { return false; }
    std::copy_n(reinterpret_cast<const uint8_t *>(buffer), size, m_channels);
    return true;
}
bool cCacCapabilitiesOperatingClasses::alloc_channels(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list channels, abort!";
        return false;
    }
    size_t len = sizeof(uint8_t) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_channels[*m_number_of_channels];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_channels_idx__ += count;
    *m_number_of_channels += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    return true;
}

void cCacCapabilitiesOperatingClasses::class_swap()
{
}

bool cCacCapabilitiesOperatingClasses::finalize()
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

size_t cCacCapabilitiesOperatingClasses::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // operating_class
    class_size += sizeof(uint8_t); // number_of_channels
    return class_size;
}

bool cCacCapabilitiesOperatingClasses::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_operating_class = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_number_of_channels = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_channels = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    uint8_t number_of_channels = *m_number_of_channels;
    m_channels_idx__ = number_of_channels;
    if (!buffPtrIncrementSafe(sizeof(uint8_t) * (number_of_channels))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) * (number_of_channels) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}


