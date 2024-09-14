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

#include <beerocks/tlvf/beerocks_message_backhaul.h>
#include <tlvf/tlvflogging.h>

using namespace beerocks_message;

cACTION_BACKHAUL_REGISTER_REQUEST::cACTION_BACKHAUL_REGISTER_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_REGISTER_REQUEST::cACTION_BACKHAUL_REGISTER_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_REGISTER_REQUEST::~cACTION_BACKHAUL_REGISTER_REQUEST() {
}
void cACTION_BACKHAUL_REGISTER_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_REGISTER_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_REGISTER_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    return class_size;
}

bool cACTION_BACKHAUL_REGISTER_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_REGISTER_RESPONSE::cACTION_BACKHAUL_REGISTER_RESPONSE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_REGISTER_RESPONSE::cACTION_BACKHAUL_REGISTER_RESPONSE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_REGISTER_RESPONSE::~cACTION_BACKHAUL_REGISTER_RESPONSE() {
}
void cACTION_BACKHAUL_REGISTER_RESPONSE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_REGISTER_RESPONSE::finalize()
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

size_t cACTION_BACKHAUL_REGISTER_RESPONSE::get_initial_size()
{
    size_t class_size = 0;
    return class_size;
}

bool cACTION_BACKHAUL_REGISTER_RESPONSE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_ENABLE::cACTION_BACKHAUL_ENABLE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_ENABLE::cACTION_BACKHAUL_ENABLE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_ENABLE::~cACTION_BACKHAUL_ENABLE() {
}
void cACTION_BACKHAUL_ENABLE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_ENABLE::finalize()
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

size_t cACTION_BACKHAUL_ENABLE::get_initial_size()
{
    size_t class_size = 0;
    return class_size;
}

bool cACTION_BACKHAUL_ENABLE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CONNECTED_NOTIFICATION::cACTION_BACKHAUL_CONNECTED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CONNECTED_NOTIFICATION::cACTION_BACKHAUL_CONNECTED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CONNECTED_NOTIFICATION::~cACTION_BACKHAUL_CONNECTED_NOTIFICATION() {
}
void cACTION_BACKHAUL_CONNECTED_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_CONNECTED_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_CONNECTED_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    return class_size;
}

bool cACTION_BACKHAUL_CONNECTED_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION::cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION::cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION::~cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION() {
}
uint8_t& cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION::stopped() {
    return (uint8_t&)(*m_stopped);
}

void cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // stopped
    return class_size;
}

bool cACTION_BACKHAUL_DISCONNECTED_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_stopped = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_ENABLE_APS_REQUEST::cACTION_BACKHAUL_ENABLE_APS_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_ENABLE_APS_REQUEST::cACTION_BACKHAUL_ENABLE_APS_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_ENABLE_APS_REQUEST::~cACTION_BACKHAUL_ENABLE_APS_REQUEST() {
}
std::string cACTION_BACKHAUL_ENABLE_APS_REQUEST::iface_str() {
    char *iface_ = iface();
    if (!iface_) { return std::string(); }
    auto str = std::string(iface_, m_iface_idx__);
    auto pos = str.find_first_of('\0');
    if (pos != std::string::npos) {
        str.erase(pos);
    }
    return str;
}

char* cACTION_BACKHAUL_ENABLE_APS_REQUEST::iface(size_t length) {
    if( (m_iface_idx__ == 0) || (m_iface_idx__ < length) ) {
        TLVF_LOG(ERROR) << "iface length is smaller than requested length";
        return nullptr;
    }
    return ((char*)m_iface);
}

bool cACTION_BACKHAUL_ENABLE_APS_REQUEST::set_iface(const std::string& str) { return set_iface(str.c_str(), str.size()); }
bool cACTION_BACKHAUL_ENABLE_APS_REQUEST::set_iface(const char str[], size_t size) {
    if (str == nullptr) {
        TLVF_LOG(WARNING) << "set_iface received a null pointer.";
        return false;
    }
    if (size > beerocks::message::IFACE_NAME_LENGTH) {
        TLVF_LOG(ERROR) << "Received buffer size is smaller than string length";
        return false;
    }
    std::copy(str, str + size, m_iface);
    return true;
}
uint8_t& cACTION_BACKHAUL_ENABLE_APS_REQUEST::channel() {
    return (uint8_t&)(*m_channel);
}

beerocks::eWiFiBandwidth& cACTION_BACKHAUL_ENABLE_APS_REQUEST::bandwidth() {
    return (beerocks::eWiFiBandwidth&)(*m_bandwidth);
}

uint8_t& cACTION_BACKHAUL_ENABLE_APS_REQUEST::center_channel() {
    return (uint8_t&)(*m_center_channel);
}

void cACTION_BACKHAUL_ENABLE_APS_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    tlvf_swap(8*sizeof(beerocks::eWiFiBandwidth), reinterpret_cast<uint8_t*>(m_bandwidth));
}

bool cACTION_BACKHAUL_ENABLE_APS_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_ENABLE_APS_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    class_size += beerocks::message::IFACE_NAME_LENGTH * sizeof(char); // iface
    class_size += sizeof(uint8_t); // channel
    class_size += sizeof(beerocks::eWiFiBandwidth); // bandwidth
    class_size += sizeof(uint8_t); // center_channel
    return class_size;
}

bool cACTION_BACKHAUL_ENABLE_APS_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_iface = reinterpret_cast<char*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(char) * (beerocks::message::IFACE_NAME_LENGTH))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(char) * (beerocks::message::IFACE_NAME_LENGTH) << ") Failed!";
        return false;
    }
    m_iface_idx__  = beerocks::message::IFACE_NAME_LENGTH;
    m_channel = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_bandwidth = reinterpret_cast<beerocks::eWiFiBandwidth*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(beerocks::eWiFiBandwidth))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(beerocks::eWiFiBandwidth) << ") Failed!";
        return false;
    }
    m_center_channel = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST::cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST::cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST::~cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST() {
}
uint32_t& cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST::attempts() {
    return (uint32_t&)(*m_attempts);
}

void cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    tlvf_swap(32, reinterpret_cast<uint8_t*>(m_attempts));
}

bool cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint32_t); // attempts
    return class_size;
}

bool cACTION_BACKHAUL_UPDATE_STOP_ON_FAILURE_ATTEMPTS_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_attempts = reinterpret_cast<uint32_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint32_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint32_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::~cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION() {
}
std::string cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::iface_str() {
    char *iface_ = iface();
    if (!iface_) { return std::string(); }
    auto str = std::string(iface_, m_iface_idx__);
    auto pos = str.find_first_of('\0');
    if (pos != std::string::npos) {
        str.erase(pos);
    }
    return str;
}

char* cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::iface(size_t length) {
    if( (m_iface_idx__ == 0) || (m_iface_idx__ < length) ) {
        TLVF_LOG(ERROR) << "iface length is smaller than requested length";
        return nullptr;
    }
    return ((char*)m_iface);
}

bool cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::set_iface(const std::string& str) { return set_iface(str.c_str(), str.size()); }
bool cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::set_iface(const char str[], size_t size) {
    if (str == nullptr) {
        TLVF_LOG(WARNING) << "set_iface received a null pointer.";
        return false;
    }
    if (size > beerocks::message::IFACE_NAME_LENGTH) {
        TLVF_LOG(ERROR) << "Received buffer size is smaller than string length";
        return false;
    }
    std::copy(str, str + size, m_iface);
    return true;
}
void cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    class_size += beerocks::message::IFACE_NAME_LENGTH * sizeof(char); // iface
    return class_size;
}

bool cACTION_BACKHAUL_AP_DISABLED_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_iface = reinterpret_cast<char*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(char) * (beerocks::message::IFACE_NAME_LENGTH))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(char) * (beerocks::message::IFACE_NAME_LENGTH) << ") Failed!";
        return false;
    }
    m_iface_idx__  = beerocks::message::IFACE_NAME_LENGTH;
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST::~cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST() {
}
sNodeRssiMeasurementRequest& cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST::params() {
    return (sNodeRssiMeasurementRequest&)(*m_params);
}

void cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_params->struct_swap();
}

bool cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sNodeRssiMeasurementRequest); // params
    return class_size;
}

bool cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_params = reinterpret_cast<sNodeRssiMeasurementRequest*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sNodeRssiMeasurementRequest))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sNodeRssiMeasurementRequest) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_params->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE::~cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE() {
}
sNodeRssiMeasurement& cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE::params() {
    return (sNodeRssiMeasurement&)(*m_params);
}

void cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_params->struct_swap();
}

bool cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE::finalize()
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

size_t cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sNodeRssiMeasurement); // params
    return class_size;
}

bool cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_RESPONSE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_params = reinterpret_cast<sNodeRssiMeasurement*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sNodeRssiMeasurement))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sNodeRssiMeasurement) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_params->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE::cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE::~cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE() {
}
sMacAddr& cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE::mac() {
    return (sMacAddr&)(*m_mac);
}

void cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_mac->struct_swap();
}

bool cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE::finalize()
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

size_t cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sMacAddr); // mac
    return class_size;
}

bool cACTION_BACKHAUL_CLIENT_RX_RSSI_MEASUREMENT_CMD_RESPONSE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_mac = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_mac->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST::cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST::cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST::~cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST() {
}
uint8_t& cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST::sync() {
    return (uint8_t&)(*m_sync);
}

sMacAddr& cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST::sta_mac() {
    return (sMacAddr&)(*m_sta_mac);
}

void cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_sta_mac->struct_swap();
}

bool cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // sync
    class_size += sizeof(sMacAddr); // sta_mac
    return class_size;
}

bool cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_sync = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_sta_mac = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_sta_mac->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::~cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE() {
}
const uint16_t& cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::length() {
    return (const uint16_t&)(*m_length);
}

sMacAddr& cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::sta_mac() {
    return (sMacAddr&)(*m_sta_mac);
}

uint8_t& cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::bssid_info_list_length() {
    return (uint8_t&)(*m_bssid_info_list_length);
}

std::tuple<bool, sBssidInfo&> cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::bssid_info_list(size_t idx) {
    bool ret_success = ( (m_bssid_info_list_idx__ > 0) && (m_bssid_info_list_idx__ > idx) );
    size_t ret_idx = ret_success ? idx : 0;
    if (!ret_success) {
        TLVF_LOG(ERROR) << "Requested index is greater than the number of available entries";
    }
    return std::forward_as_tuple(ret_success, m_bssid_info_list[ret_idx]);
}

bool cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::alloc_bssid_info_list(size_t count) {
    if (m_lock_order_counter__ > 0) {;
        TLVF_LOG(ERROR) << "Out of order allocation for variable length list bssid_info_list, abort!";
        return false;
    }
    size_t len = sizeof(sBssidInfo) * count;
    if(getBuffRemainingBytes() < len )  {
        TLVF_LOG(ERROR) << "Not enough available space on buffer - can't allocate";
        return false;
    }
    m_lock_order_counter__ = 0;
    uint8_t *src = (uint8_t *)&m_bssid_info_list[*m_bssid_info_list_length];
    uint8_t *dst = src + len;
    if (!m_parse__) {
        size_t move_length = getBuffRemainingBytes(src) - len;
        std::copy_n(src, move_length, dst);
    }
    m_bssid_info_list_idx__ += count;
    *m_bssid_info_list_length += count;
    if (!buffPtrIncrementSafe(len)) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << len << ") Failed!";
        return false;
    }
    if(m_length){ (*m_length) += len; }
    if (!m_parse__) { 
        for (size_t i = m_bssid_info_list_idx__ - count; i < m_bssid_info_list_idx__; i++) { m_bssid_info_list[i].struct_init(); }
    }
    return true;
}

void cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    tlvf_swap(16, reinterpret_cast<uint8_t*>(m_length));
    m_sta_mac->struct_swap();
    for (size_t i = 0; i < m_bssid_info_list_idx__; i++){
        m_bssid_info_list[i].struct_swap();
    }
}

bool cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::finalize()
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

size_t cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint16_t); // length
    class_size += sizeof(sMacAddr); // sta_mac
    class_size += sizeof(uint8_t); // bssid_info_list_length
    return class_size;
}

bool cACTION_BACKHAUL_ASSOCIATED_STA_LINK_METRICS_RESPONSE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_length = reinterpret_cast<uint16_t*>(m_buff_ptr__);
    if (!m_parse__) *m_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint16_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint16_t) << ") Failed!";
        return false;
    }
    m_sta_mac = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(sMacAddr); }
    if (!m_parse__) { m_sta_mac->struct_init(); }
    m_bssid_info_list_length = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_bssid_info_list_length = 0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if(m_length && !m_parse__){ (*m_length) += sizeof(uint8_t); }
    m_bssid_info_list = reinterpret_cast<sBssidInfo*>(m_buff_ptr__);
    uint8_t bssid_info_list_length = *m_bssid_info_list_length;
    m_bssid_info_list_idx__ = bssid_info_list_length;
    if (!buffPtrIncrementSafe(sizeof(sBssidInfo) * (bssid_info_list_length))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sBssidInfo) * (bssid_info_list_length) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_START_WPS_PBC_REQUEST::cACTION_BACKHAUL_START_WPS_PBC_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_START_WPS_PBC_REQUEST::cACTION_BACKHAUL_START_WPS_PBC_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_START_WPS_PBC_REQUEST::~cACTION_BACKHAUL_START_WPS_PBC_REQUEST() {
}
std::string cACTION_BACKHAUL_START_WPS_PBC_REQUEST::iface_str() {
    char *iface_ = iface();
    if (!iface_) { return std::string(); }
    auto str = std::string(iface_, m_iface_idx__);
    auto pos = str.find_first_of('\0');
    if (pos != std::string::npos) {
        str.erase(pos);
    }
    return str;
}

char* cACTION_BACKHAUL_START_WPS_PBC_REQUEST::iface(size_t length) {
    if( (m_iface_idx__ == 0) || (m_iface_idx__ < length) ) {
        TLVF_LOG(ERROR) << "iface length is smaller than requested length";
        return nullptr;
    }
    return ((char*)m_iface);
}

bool cACTION_BACKHAUL_START_WPS_PBC_REQUEST::set_iface(const std::string& str) { return set_iface(str.c_str(), str.size()); }
bool cACTION_BACKHAUL_START_WPS_PBC_REQUEST::set_iface(const char str[], size_t size) {
    if (str == nullptr) {
        TLVF_LOG(WARNING) << "set_iface received a null pointer.";
        return false;
    }
    if (size > beerocks::message::IFACE_NAME_LENGTH) {
        TLVF_LOG(ERROR) << "Received buffer size is smaller than string length";
        return false;
    }
    std::copy(str, str + size, m_iface);
    return true;
}
void cACTION_BACKHAUL_START_WPS_PBC_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_START_WPS_PBC_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_START_WPS_PBC_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    class_size += beerocks::message::IFACE_NAME_LENGTH * sizeof(char); // iface
    return class_size;
}

bool cACTION_BACKHAUL_START_WPS_PBC_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_iface = reinterpret_cast<char*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(char) * (beerocks::message::IFACE_NAME_LENGTH))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(char) * (beerocks::message::IFACE_NAME_LENGTH) << ") Failed!";
        return false;
    }
    m_iface_idx__  = beerocks::message::IFACE_NAME_LENGTH;
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST::cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST::cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST::~cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST() {
}
uint8_t& cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST::enable() {
    return (uint8_t&)(*m_enable);
}

sMacAddr& cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST::bssid() {
    return (sMacAddr&)(*m_bssid);
}

void cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_bssid->struct_swap();
}

bool cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // enable
    class_size += sizeof(sMacAddr); // bssid
    return class_size;
}

bool cACTION_BACKHAUL_SET_ASSOC_DISALLOW_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_enable = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_bssid = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_bssid->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST::cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST::cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST::~cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST() {
}
void cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    return class_size;
}

bool cACTION_BACKHAUL_APPLY_VLAN_POLICY_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::~cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED() {
}
std::string cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::front_iface_name_str() {
    char *front_iface_name_ = front_iface_name();
    if (!front_iface_name_) { return std::string(); }
    auto str = std::string(front_iface_name_, m_front_iface_name_idx__);
    auto pos = str.find_first_of('\0');
    if (pos != std::string::npos) {
        str.erase(pos);
    }
    return str;
}

char* cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::front_iface_name(size_t length) {
    if( (m_front_iface_name_idx__ == 0) || (m_front_iface_name_idx__ < length) ) {
        TLVF_LOG(ERROR) << "front_iface_name length is smaller than requested length";
        return nullptr;
    }
    return ((char*)m_front_iface_name);
}

bool cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::set_front_iface_name(const std::string& str) { return set_front_iface_name(str.c_str(), str.size()); }
bool cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::set_front_iface_name(const char str[], size_t size) {
    if (str == nullptr) {
        TLVF_LOG(WARNING) << "set_front_iface_name received a null pointer.";
        return false;
    }
    if (size > beerocks::message::IFACE_NAME_LENGTH) {
        TLVF_LOG(ERROR) << "Received buffer size is smaller than string length";
        return false;
    }
    std::copy(str, str + size, m_front_iface_name);
    return true;
}
void cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::finalize()
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

size_t cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::get_initial_size()
{
    size_t class_size = 0;
    class_size += beerocks::message::IFACE_NAME_LENGTH * sizeof(char); // front_iface_name
    return class_size;
}

bool cACTION_BACKHAUL_ZWDFS_RADIO_DETECTED::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_front_iface_name = reinterpret_cast<char*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(char) * (beerocks::message::IFACE_NAME_LENGTH))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(char) * (beerocks::message::IFACE_NAME_LENGTH) << ") Failed!";
        return false;
    }
    m_front_iface_name_idx__  = beerocks::message::IFACE_NAME_LENGTH;
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNELS_LIST_REQUEST::cACTION_BACKHAUL_CHANNELS_LIST_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNELS_LIST_REQUEST::cACTION_BACKHAUL_CHANNELS_LIST_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNELS_LIST_REQUEST::~cACTION_BACKHAUL_CHANNELS_LIST_REQUEST() {
}
void cACTION_BACKHAUL_CHANNELS_LIST_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_CHANNELS_LIST_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_CHANNELS_LIST_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    return class_size;
}

bool cACTION_BACKHAUL_CHANNELS_LIST_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE::cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE::cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE::~cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE() {
}
void cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE::finalize()
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

size_t cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE::get_initial_size()
{
    size_t class_size = 0;
    return class_size;
}

bool cACTION_BACKHAUL_CHANNELS_LIST_RESPONSE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START::cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START::cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START::~cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START() {
}
sApChannelSwitch& cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START::cs_params() {
    return (sApChannelSwitch&)(*m_cs_params);
}

int8_t& cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START::tx_limit() {
    return (int8_t&)(*m_tx_limit);
}

uint8_t& cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START::tx_limit_valid() {
    return (uint8_t&)(*m_tx_limit_valid);
}

void cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_cs_params->struct_swap();
}

bool cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START::finalize()
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

size_t cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sApChannelSwitch); // cs_params
    class_size += sizeof(int8_t); // tx_limit
    class_size += sizeof(uint8_t); // tx_limit_valid
    return class_size;
}

bool cACTION_BACKHAUL_HOSTAP_CHANNEL_SWITCH_ACS_START::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_cs_params = reinterpret_cast<sApChannelSwitch*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sApChannelSwitch))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sApChannelSwitch) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_cs_params->struct_init(); }
    m_tx_limit = reinterpret_cast<int8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(int8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(int8_t) << ") Failed!";
        return false;
    }
    m_tx_limit_valid = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST::cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST::cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST::~cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST() {
}
sApChannelSwitch& cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST::cs_params() {
    return (sApChannelSwitch&)(*m_cs_params);
}

void cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_cs_params->struct_swap();
}

bool cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sApChannelSwitch); // cs_params
    return class_size;
}

bool cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_cs_params = reinterpret_cast<sApChannelSwitch*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sApChannelSwitch))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sApChannelSwitch) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_cs_params->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE::cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE::cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE::~cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE() {
}
uint8_t& cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE::success() {
    return (uint8_t&)(*m_success);
}

void cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE::finalize()
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

size_t cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // success
    return class_size;
}

bool cACTION_BACKHAUL_HOSTAP_CANCEL_ACTIVE_CAC_RESPONSE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_success = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION::cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION::cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION::~cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION() {
}
sApChannelSwitch& cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION::cs_params() {
    return (sApChannelSwitch&)(*m_cs_params);
}

void cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_cs_params->struct_swap();
}

bool cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sApChannelSwitch); // cs_params
    return class_size;
}

bool cACTION_BACKHAUL_HOSTAP_CSA_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_cs_params = reinterpret_cast<sApChannelSwitch*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sApChannelSwitch))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sApChannelSwitch) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_cs_params->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION::cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION::cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION::~cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION() {
}
sApChannelSwitch& cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION::cs_params() {
    return (sApChannelSwitch&)(*m_cs_params);
}

void cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_cs_params->struct_swap();
}

bool cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sApChannelSwitch); // cs_params
    return class_size;
}

bool cACTION_BACKHAUL_HOSTAP_CSA_ERROR_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_cs_params = reinterpret_cast<sApChannelSwitch*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sApChannelSwitch))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sApChannelSwitch) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_cs_params->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION::cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION::cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION::~cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION() {
}
sCacStartedNotificationParams& cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION::params() {
    return (sCacStartedNotificationParams&)(*m_params);
}

void cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_params->struct_swap();
}

bool cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sCacStartedNotificationParams); // params
    return class_size;
}

bool cACTION_BACKHAUL_HOSTAP_DFS_CAC_STARTED_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_params = reinterpret_cast<sCacStartedNotificationParams*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sCacStartedNotificationParams))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sCacStartedNotificationParams) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_params->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION::cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION::cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION::~cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION() {
}
sDfsCacCompleted& cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION::params() {
    return (sDfsCacCompleted&)(*m_params);
}

void cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_params->struct_swap();
}

bool cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sDfsCacCompleted); // params
    return class_size;
}

bool cACTION_BACKHAUL_HOSTAP_DFS_CAC_COMPLETED_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_params = reinterpret_cast<sDfsCacCompleted*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sDfsCacCompleted))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sDfsCacCompleted) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_params->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::~cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST() {
}
uint8_t& cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::ant_switch_on() {
    return (uint8_t&)(*m_ant_switch_on);
}

uint8_t& cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::channel() {
    return (uint8_t&)(*m_channel);
}

beerocks::eWiFiBandwidth& cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::bandwidth() {
    return (beerocks::eWiFiBandwidth&)(*m_bandwidth);
}

uint32_t& cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::center_frequency() {
    return (uint32_t&)(*m_center_frequency);
}

void cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    tlvf_swap(8*sizeof(beerocks::eWiFiBandwidth), reinterpret_cast<uint8_t*>(m_bandwidth));
    tlvf_swap(32, reinterpret_cast<uint8_t*>(m_center_frequency));
}

bool cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // ant_switch_on
    class_size += sizeof(uint8_t); // channel
    class_size += sizeof(beerocks::eWiFiBandwidth); // bandwidth
    class_size += sizeof(uint32_t); // center_frequency
    return class_size;
}

bool cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_ant_switch_on = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_channel = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_bandwidth = reinterpret_cast<beerocks::eWiFiBandwidth*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(beerocks::eWiFiBandwidth))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(beerocks::eWiFiBandwidth) << ") Failed!";
        return false;
    }
    m_center_frequency = reinterpret_cast<uint32_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint32_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint32_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE::cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE::cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE::~cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE() {
}
uint8_t& cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE::success() {
    return (uint8_t&)(*m_success);
}

void cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE::finalize()
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

size_t cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // success
    return class_size;
}

bool cACTION_BACKHAUL_HOSTAP_ZWDFS_ANT_CHANNEL_SWITCH_RESPONSE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_success = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::cACTION_BACKHAUL_RADIO_DISABLE_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::cACTION_BACKHAUL_RADIO_DISABLE_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::~cACTION_BACKHAUL_RADIO_DISABLE_REQUEST() {
}
std::string cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::iface_str() {
    char *iface_ = iface();
    if (!iface_) { return std::string(); }
    auto str = std::string(iface_, m_iface_idx__);
    auto pos = str.find_first_of('\0');
    if (pos != std::string::npos) {
        str.erase(pos);
    }
    return str;
}

char* cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::iface(size_t length) {
    if( (m_iface_idx__ == 0) || (m_iface_idx__ < length) ) {
        TLVF_LOG(ERROR) << "iface length is smaller than requested length";
        return nullptr;
    }
    return ((char*)m_iface);
}

bool cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::set_iface(const std::string& str) { return set_iface(str.c_str(), str.size()); }
bool cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::set_iface(const char str[], size_t size) {
    if (str == nullptr) {
        TLVF_LOG(WARNING) << "set_iface received a null pointer.";
        return false;
    }
    if (size > beerocks::message::IFACE_NAME_LENGTH) {
        TLVF_LOG(ERROR) << "Received buffer size is smaller than string length";
        return false;
    }
    std::copy(str, str + size, m_iface);
    return true;
}
void cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    class_size += beerocks::message::IFACE_NAME_LENGTH * sizeof(char); // iface
    return class_size;
}

bool cACTION_BACKHAUL_RADIO_DISABLE_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_iface = reinterpret_cast<char*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(char) * (beerocks::message::IFACE_NAME_LENGTH))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(char) * (beerocks::message::IFACE_NAME_LENGTH) << ") Failed!";
        return false;
    }
    m_iface_idx__  = beerocks::message::IFACE_NAME_LENGTH;
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST::cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST::cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST::~cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST() {
}
void cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    return class_size;
}

bool cACTION_BACKHAUL_RADIO_TEAR_DOWN_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST::~cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST() {
}
sTriggerChannelScanParams& cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST::scan_params() {
    return (sTriggerChannelScanParams&)(*m_scan_params);
}

void cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_scan_params->struct_swap();
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sTriggerChannelScanParams); // scan_params
    return class_size;
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_scan_params = reinterpret_cast<sTriggerChannelScanParams*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sTriggerChannelScanParams))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sTriggerChannelScanParams) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_scan_params->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE::~cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE() {
}
uint8_t& cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE::success() {
    return (uint8_t&)(*m_success);
}

void cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE::finalize()
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

size_t cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // success
    return class_size;
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGER_SCAN_RESPONSE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_success = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST::cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST::cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST::~cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST() {
}
void cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    return class_size;
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE::cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE::cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE::~cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE() {
}
uint8_t& cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE::success() {
    return (uint8_t&)(*m_success);
}

void cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE::finalize()
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

size_t cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // success
    return class_size;
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_DUMP_RESULTS_RESPONSE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_success = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION::cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION::~cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION() {
}
sMacAddr& cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION::radio_mac() {
    return (sMacAddr&)(*m_radio_mac);
}

void cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_radio_mac->struct_swap();
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sMacAddr); // radio_mac
    return class_size;
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_TRIGGERED_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_radio_mac = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_radio_mac->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION::cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION::cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION::~cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION() {
}
sChannelScanResults& cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION::scan_results() {
    return (sChannelScanResults&)(*m_scan_results);
}

sMacAddr& cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION::radio_mac() {
    return (sMacAddr&)(*m_radio_mac);
}

uint8_t& cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION::is_dump() {
    return (uint8_t&)(*m_is_dump);
}

void cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_scan_results->struct_swap();
    m_radio_mac->struct_swap();
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sChannelScanResults); // scan_results
    class_size += sizeof(sMacAddr); // radio_mac
    class_size += sizeof(uint8_t); // is_dump
    return class_size;
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_RESULTS_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_scan_results = reinterpret_cast<sChannelScanResults*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sChannelScanResults))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sChannelScanResults) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_scan_results->struct_init(); }
    m_radio_mac = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_radio_mac->struct_init(); }
    m_is_dump = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!m_parse__) *m_is_dump = 0x0;
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST::cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST::cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST::~cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST() {
}
void cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST::finalize()
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

size_t cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST::get_initial_size()
{
    size_t class_size = 0;
    return class_size;
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_REQUEST::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE::cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE::cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE::~cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE() {
}
uint8_t& cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE::success() {
    return (uint8_t&)(*m_success);
}

void cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE::finalize()
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

size_t cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // success
    return class_size;
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_ABORT_RESPONSE::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_success = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION::cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION::cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION::~cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION() {
}
uint8_t& cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION::reason() {
    return (uint8_t&)(*m_reason);
}

sMacAddr& cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION::radio_mac() {
    return (sMacAddr&)(*m_radio_mac);
}

void cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_radio_mac->struct_swap();
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(uint8_t); // reason
    class_size += sizeof(sMacAddr); // radio_mac
    return class_size;
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_ABORTED_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_reason = reinterpret_cast<uint8_t*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(uint8_t))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(uint8_t) << ") Failed!";
        return false;
    }
    m_radio_mac = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_radio_mac->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}

cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION::cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION(uint8_t* buff, size_t buff_len, bool parse) :
    BaseClass(buff, buff_len, parse) {
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION::cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION(std::shared_ptr<BaseClass> base, bool parse) :
BaseClass(base->getBuffPtr(), base->getBuffRemainingBytes(), parse){
    m_init_succeeded = init();
}
cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION::~cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION() {
}
sMacAddr& cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION::radio_mac() {
    return (sMacAddr&)(*m_radio_mac);
}

void cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION::class_swap()
{
    tlvf_swap(8*sizeof(eActionOp_BACKHAUL), reinterpret_cast<uint8_t*>(m_action_op));
    m_radio_mac->struct_swap();
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION::finalize()
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

size_t cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION::get_initial_size()
{
    size_t class_size = 0;
    class_size += sizeof(sMacAddr); // radio_mac
    return class_size;
}

bool cACTION_BACKHAUL_CHANNEL_SCAN_FINISHED_NOTIFICATION::init()
{
    if (getBuffRemainingBytes() < get_initial_size()) {
        TLVF_LOG(ERROR) << "Not enough available space on buffer. Class init failed";
        return false;
    }
    m_radio_mac = reinterpret_cast<sMacAddr*>(m_buff_ptr__);
    if (!buffPtrIncrementSafe(sizeof(sMacAddr))) {
        LOG(ERROR) << "buffPtrIncrementSafe(" << std::dec << sizeof(sMacAddr) << ") Failed!";
        return false;
    }
    if (!m_parse__) { m_radio_mac->struct_init(); }
    if (m_parse__) { class_swap(); }
    return true;
}


