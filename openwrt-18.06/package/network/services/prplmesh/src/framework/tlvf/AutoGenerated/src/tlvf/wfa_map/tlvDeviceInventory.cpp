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

#include <tlvf/wfa_map/tlvDeviceInventory.h>
#include <tlvf/tlvflogging.h>

using namespace wfa_map;

tlvDeviceInventory::tlvDeviceInventory(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
tlvDeviceInventory::tlvDeviceInventory(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
tlvDeviceInventory::~tlvDeviceInventory() {
}
const eTlvTypeMap& tlvDeviceInventory::type() {
    return (const eTlvTypeMap&)(*m_type);
}

const uint16_t& tlvDeviceInventory::length() {
    return (const uint16_t&)(*m_length);
}

uint8_t& tlvDeviceInventory::serial_number_length() {
    return (uint8_t&)(*m_serial_number_length);
}

std::string tlvDeviceInventory::serial_number_str() {
    char *serial_number_ = serial_number();
    if (!serial_number_) { return std::string(); }
    auto str = std::string(serial_number_, m_serial_number_idx__);
    auto pos = str.find_first_of('\0');
    if (pos != std::string::npos) {
        str.erase(pos);
    }
    return str;
}

char* tlvDeviceInventory::serial_number(size_t length) {
    if( (m_serial_number_idx__ == 0) || (m_serial_number_idx__ < length) ) {
        TLVF_LOG(ERROR) << "serial_number length is smaller than requested length";
        return nullptr;
    }
    return ((char*)m_serial_number);
}

bool tlvDeviceInventory::set_serial_number(const std::string& str) { return set_serial_number(str.c_str(), str.size()); }
bool tlvDeviceInventory::set_serial_number(const char str[], size_t size) {
    if (str == nullptr) {
        TLVF_LOG(WARNING) << "set_serial_number received a null pointer.";
        return false;
    }
    if (m_serial_number_idx__ != 0) {
        TLVF_LOG(ERROR) << "set_serial_number was already allocated!";
        return false;
    }
    if (!alloc_serial_number(size)) { return false; }
    std::copy(str, str + size, m_serial_number);
    return true;
}
bool tlvDeviceInventory::alloc_serial_number(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list serial_number, abort!";
        return false;
    }
    size_t len = sizeof(char) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_serial_number[*m_serial_number_length];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_software_version_length = (uint8_t *)((uint8_t *)(m_software_version_length) + len);
    m_software_version = (char *)((uint8_t *)(m_software_version) + len);
    m_execution_environment_length = (uint8_t *)((uint8_t *)(m_execution_environment_length) + len);
    m_execution_environment = (char *)((uint8_t *)(m_execution_environment) + len);
    m_number_of_radios = (uint8_t *)((uint8_t *)(m_number_of_radios) + len);
    m_radios_vendor_info = (cRadioVendorInfo *)((uint8_t *)(m_radios_vendor_info) + len);
    m_serial_number_idx__ += count;
    *m_serial_number_length += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    return true;
}

uint8_t& tlvDeviceInventory::software_version_length() {
    return (uint8_t&)(*m_software_version_length);
}

std::string tlvDeviceInventory::software_version_str() {
    char *software_version_ = software_version();
    if (!software_version_) { return std::string(); }
    auto str = std::string(software_version_, m_software_version_idx__);
    auto pos = str.find_first_of('\0');
    if (pos != std::string::npos) {
        str.erase(pos);
    }
    return str;
}

char* tlvDeviceInventory::software_version(size_t length) {
    if( (m_software_version_idx__ == 0) || (m_software_version_idx__ < length) ) {
        TLVF_LOG(ERROR) << "software_version length is smaller than requested length";
        return nullptr;
    }
    return ((char*)m_software_version);
}

bool tlvDeviceInventory::set_software_version(const std::string& str) { return set_software_version(str.c_str(), str.size()); }
bool tlvDeviceInventory::set_software_version(const char str[], size_t size) {
    if (str == nullptr) {
        TLVF_LOG(WARNING) << "set_software_version received a null pointer.";
        return false;
    }
    if (m_software_version_idx__ != 0) {
        TLVF_LOG(ERROR) << "set_software_version was already allocated!";
        return false;
    }
    if (!alloc_software_version(size)) { return false; }
    std::copy(str, str + size, m_software_version);
    return true;
}
bool tlvDeviceInventory::alloc_software_version(size_t count) {
    if (m_lock_order_counter__ > 1) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list software_version, abort!";
        return false;
    }
    size_t len = sizeof(char) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 1;
    uint8_t *src = (uint8_t *)&m_software_version[*m_software_version_length];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_execution_environment_length = (uint8_t *)((uint8_t *)(m_execution_environment_length) + len);
    m_execution_environment = (char *)((uint8_t *)(m_execution_environment) + len);
    m_number_of_radios = (uint8_t *)((uint8_t *)(m_number_of_radios) + len);
    m_radios_vendor_info = (cRadioVendorInfo *)((uint8_t *)(m_radios_vendor_info) + len);
    m_software_version_idx__ += count;
    *m_software_version_length += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    return true;
}

uint8_t& tlvDeviceInventory::execution_environment_length() {
    return (uint8_t&)(*m_execution_environment_length);
}

std::string tlvDeviceInventory::execution_environment_str() {
    char *execution_environment_ = execution_environment();
    if (!execution_environment_) { return std::string(); }
    auto str = std::string(execution_environment_, m_execution_environment_idx__);
    auto pos = str.find_first_of('\0');
    if (pos != std::string::npos) {
        str.erase(pos);
    }
    return str;
}

char* tlvDeviceInventory::execution_environment(size_t length) {
    if( (m_execution_environment_idx__ == 0) || (m_execution_environment_idx__ < length) ) {
        TLVF_LOG(ERROR) << "execution_environment length is smaller than requested length";
        return nullptr;
    }
    return ((char*)m_execution_environment);
}

bool tlvDeviceInventory::set_execution_environment(const std::string& str) { return set_execution_environment(str.c_str(), str.size()); }
bool tlvDeviceInventory::set_execution_environment(const char str[], size_t size) {
    if (str == nullptr) {
        TLVF_LOG(WARNING) << "set_execution_environment received a null pointer.";
        return false;
    }
    if (m_execution_environment_idx__ != 0) {
        TLVF_LOG(ERROR) << "set_execution_environment was already allocated!";
        return false;
    }
    if (!alloc_execution_environment(size)) { return false; }
    std::copy(str, str + size, m_execution_environment);
    return true;
}
bool tlvDeviceInventory::alloc_execution_environment(size_t count) {
    if (m_lock_order_counter__ > 2) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list execution_environment, abort!";
        return false;
    }
    size_t len = sizeof(char) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 2;
    uint8_t *src = (uint8_t *)&m_execution_environment[*m_execution_environment_length];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_number_of_radios = (uint8_t *)((uint8_t *)(m_number_of_radios) + len);
    m_radios_vendor_info = (cRadioVendorInfo *)((uint8_t *)(m_radios_vendor_info) + len);
    m_execution_environment_idx__ += count;
    *m_execution_environment_length += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    return true;
}

uint8_t& tlvDeviceInventory::number_of_radios() {
    return (uint8_t&)(*m_number_of_radios);
}

std::tuple<bool, cRadioVendorInfo&> tlvDeviceInventory::radios_vendor_info(size_t idx) {
    bool ret_success = ( (m_radios_vendor_info_idx__ > 0) && (m_radios_vendor_info_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, *(m_radios_vendor_info_vector[ret_idx]));
}

std::shared_ptr<cRadioVendorInfo> tlvDeviceInventory::create_radios_vendor_info() {
    if (m_lock_order_counter__ > 3) {
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list radios_vendor_info, abort!";
        return nullptr;
    }
    size_t len = cRadioVendorInfo::get_initial_size();
    if (m_lock_allocation__) {
        TLVF_LOG(ERROR) << "Can't create new element before adding the previous one";
        return nullptr;
    }
    if (getBuffRemainingBytes() < len) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer";
        return nullptr;
    }
    m_lock_order_counter__ = 3;
    m_lock_allocation__ = true;
    uint8_t *src = (uint8_t *)m_radios_vendor_info;
    if (m_radios_vendor_info_idx__ > 0) {
        src = (uint8_t *)m_radios_vendor_info_vector[m_radios_vendor_info_idx__ - 1]->getBuffPtr();
    }
    if (!m_parse__) {
        uint8_t *dst = src + len;
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    return std::make_shared<cRadioVendorInfo>(src, getBuffRemainingBytes(src), m_parse__);
}

bool tlvDeviceInventory::add_radios_vendor_info(std::shared_ptr<cRadioVendorInfo> ptr) {
    if (ptr == nullptr) {
        TLVF_LOG(ERROR) << "Received entry is nullptr";
        return false;
    }
    if (m_lock_allocation__ == false) {
        TLVF_LOG(ERROR) << "No call to create_radios_vendor_info was called before add_radios_vendor_info";
        return false;
    }
    uint8_t *src = (uint8_t *)m_radios_vendor_info;
    if (m_radios_vendor_info_idx__ > 0) {
        src = (uint8_t *)m_radios_vendor_info_vector[m_radios_vendor_info_idx__ - 1]->getBuffPtr();
    }
    if (ptr->getStartBuffPtr() != src) {
        TLVF_LOG(ERROR) << "Received entry pointer is different than expected (expecting the same pointer returned from add method)";
        return false;
    }
    if (ptr->getLen() > getBuffRemainingBytes(ptr->getStartBuffPtr())) {;
        TLVF_LOG(ERROR) << "Not enough available space on buffer";
        return false;
    }
    m_radios_vendor_info_idx__++;
    if (!m_parse__) { (*m_number_of_radios)++; }
    size_t len = ptr->getLen();
    m_radios_vendor_info_vector.push_back(ptr);
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(!m_parse__ && m_length){ (*m_length) += len; }
    m_lock_allocation__ = false;
    return true;
}

void tlvDeviceInventory::class_swap()
{
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    for (size_t i = 0; i < m_radios_vendor_info_idx__; i++){
        std::get<1>(radios_vendor_info(i)).class_swap();
    }
}

bool tlvDeviceInventory::finalize()
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

size_t tlvDeviceInventory::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(eTlvTypeMap); // type
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(uint8_t); // serial_number_length
    class_size += sizeof(uint8_t); // software_version_length
    class_size += sizeof(uint8_t); // execution_environment_length
    class_size += sizeof(uint8_t); // number_of_radios
    return class_size;
}

bool tlvDeviceInventory::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_type = reinterpret_cast<eTlvTypeMap*>(m_buff_ptr__);
    if (!m_parse__) *m_type = eTlvTypeMap::TLV_DEVICE_INVENTORY;
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
    m_serial_number_length = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_serial_number_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_serial_number = reinterpret_cast<char*>(m_buff_ptr__);
    uint8_t serial_number_length = *m_serial_number_length;
    m_serial_number_idx__ = serial_number_length;
    if (!buffPtrIncrementSafe(sizeof(char) * (serial_number_length))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(char) * (serial_number_length) << ") Failed!";
        return false;
    }
    m_software_version_length = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_software_version_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_software_version = reinterpret_cast<char*>(m_buff_ptr__);
    uint8_t software_version_length = *m_software_version_length;
    m_software_version_idx__ = software_version_length;
    if (!buffPtrIncrementSafe(sizeof(char) * (software_version_length))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(char) * (software_version_length) << ") Failed!";
        return false;
    }
    m_execution_environment_length = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_execution_environment_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_execution_environment = reinterpret_cast<char*>(m_buff_ptr__);
    uint8_t execution_environment_length = *m_execution_environment_length;
    m_execution_environment_idx__ = execution_environment_length;
    if (!buffPtrIncrementSafe(sizeof(char) * (execution_environment_length))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(char) * (execution_environment_length) << ") Failed!";
        return false;
    }
    m_number_of_radios = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_number_of_radios = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_radios_vendor_info = reinterpret_cast<cRadioVendorInfo*>(m_buff_ptr__);
    uint8_t number_of_radios = *m_number_of_radios;
    m_radios_vendor_info_idx__ = 0;
    for (size_t i = 0; i < number_of_radios; i++) {
        auto radios_vendor_info = create_radios_vendor_info();
        if (!radios_vendor_info || !radios_vendor_info->isInitialized()) {
            TLVF_LOG(ERROR) << "create_radios_vendor_info() failed";
            return false;
        }
        if (!add_radios_vendor_info(radios_vendor_info)) {
            TLVF_LOG(ERROR) << "add_radios_vendor_info() failed";
            return false;
        }
        // swap back since radios_vendor_info will be swapped as part of the whole class swap
        radios_vendor_info->class_swap();
    }
    if (m_parse__) { class_swap(); }
    if (m_parse__) {
        if (*m_type != eTlvTypeMap::TLV_DEVICE_INVENTORY) {
            TLVF_LOG(ERROR) << "TLV type mismatch. Expected value: " << int(eTlvTypeMap::TLV_DEVICE_INVENTORY) << ", received value: " << int(*m_type);
            return false;
        }
    }
    return true;
}

cRadioVendorInfo::cRadioVendorInfo(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cRadioVendorInfo::cRadioVendorInfo(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cRadioVendorInfo::~cRadioVendorInfo() {
}
sMacAddr& cRadioVendorInfo::ruid() {
    return (sMacAddr&)(*m_ruid);
}

uint8_t& cRadioVendorInfo::chipset_vendor_length() {
    return (uint8_t&)(*m_chipset_vendor_length);
}

std::string cRadioVendorInfo::chipset_vendor_str() {
    char *chipset_vendor_ = chipset_vendor();
    if (!chipset_vendor_) { return std::string(); }
    auto str = std::string(chipset_vendor_, m_chipset_vendor_idx__);
    auto pos = str.find_first_of('\0');
    if (pos != std::string::npos) {
        str.erase(pos);
    }
    return str;
}

char* cRadioVendorInfo::chipset_vendor(size_t length) {
    if( (m_chipset_vendor_idx__ == 0) || (m_chipset_vendor_idx__ < length) ) {
        TLVF_LOG(ERROR) << "chipset_vendor length is smaller than requested length";
        return nullptr;
    }
    return ((char*)m_chipset_vendor);
}

bool cRadioVendorInfo::set_chipset_vendor(const std::string& str) { return set_chipset_vendor(str.c_str(), str.size()); }
bool cRadioVendorInfo::set_chipset_vendor(const char str[], size_t size) {
    if (str == nullptr) {
        TLVF_LOG(WARNING) << "set_chipset_vendor received a null pointer.";
        return false;
    }
    if (m_chipset_vendor_idx__ != 0) {
        TLVF_LOG(ERROR) << "set_chipset_vendor was already allocated!";
        return false;
    }
    if (!alloc_chipset_vendor(size)) { return false; }
    std::copy(str, str + size, m_chipset_vendor);
    return true;
}
bool cRadioVendorInfo::alloc_chipset_vendor(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list chipset_vendor, abort!";
        return false;
    }
    size_t len = sizeof(char) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_chipset_vendor[*m_chipset_vendor_length];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_chipset_vendor_idx__ += count;
    *m_chipset_vendor_length += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    return true;
}

void cRadioVendorInfo::class_swap()
{
    m_ruid->struct_swap();
}

bool cRadioVendorInfo::finalize()
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

size_t cRadioVendorInfo::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sMacAddr); // ruid
    class_size += sizeof(uint8_t); // chipset_vendor_length
    return class_size;
}

bool cRadioVendorInfo::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_ruid = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_ruid->struct_init(); }
    m_chipset_vendor_length = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_chipset_vendor_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_chipset_vendor = reinterpret_cast<char*>(m_buff_ptr__);
    uint8_t chipset_vendor_length = *m_chipset_vendor_length;
    m_chipset_vendor_idx__ = chipset_vendor_length;
    if (!buffPtrIncrementSafe(sizeof(char) * (chipset_vendor_length))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(char) * (chipset_vendor_length) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}


