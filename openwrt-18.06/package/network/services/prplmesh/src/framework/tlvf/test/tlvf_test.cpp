/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "tlvf/CmduMessageRx.h"
#include "tlvf/CmduMessageTx.h"
#include "tlvf/tlvftypes.h"
#include <cstring>
#include <iostream>
#include <sstream>

#include "tlvf/WSC/configData.h"
#include "tlvf/WSC/m1.h"
#include "tlvf/WSC/m2.h"
#include "tlvf/ieee_1905_1/tlv1905NeighborDevice.h"
#include "tlvf/ieee_1905_1/tlvLinkMetricQuery.h"
#include "tlvf/ieee_1905_1/tlvMacAddress.h"
#include "tlvf/ieee_1905_1/tlvNon1905neighborDeviceList.h"
#include "tlvf/ieee_1905_1/tlvUnknown.h"
#include "tlvf/ieee_1905_1/tlvVendorSpecific.h"
#include "tlvf/ieee_1905_1/tlvWsc.h"
#include "tlvf/wfa_map/tlvApCapability.h"
#include "tlvf/wfa_map/tlvProfile2ChannelScanResult.h"
#include "tlvf/wfa_map/tlvProfile2ErrorCode.h"
#include <tlvf/AssociationRequestFrame/AssocReqFrame.h>
#include <tlvf/test/tlvVarList.h>
#include <tlvf/tlvftypes.h>

#include <mapf/common/encryption.h>
#include <mapf/common/err.h>
#include <mapf/common/logger.h>
#include <mapf/common/utils.h>
#include <tlvf/wfa_map/tlvApCapability.h>

#include <algorithm>
#include <iterator>
#include <stdio.h>

using namespace ieee1905_1;
using namespace wfa_map;

using namespace mapf;

int test_int_len_list()
{
    int errors = 0;
    uint8_t tx_buffer[1024];
    const uint8_t gTlvMacAddress[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

    MAPF_INFO(__FUNCTION__ << " start");
    memset(tx_buffer, 0, sizeof(tx_buffer));
    {
        auto tlv = tlvMacAddress(tx_buffer, sizeof(tx_buffer), false);
        tlvf::mac_from_array(gTlvMacAddress, tlv.mac());
        tlv.class_swap(); //finalize
        LOG(DEBUG) << "TX: " << std::endl << utils::dump_buffer(tx_buffer, tlv.getLen());
    }

    uint8_t rx_buffer[sizeof(tx_buffer)];
    memcpy(rx_buffer, tx_buffer, sizeof(rx_buffer));
    {
        auto tlv = tlvMacAddress(tx_buffer, sizeof(tx_buffer), true);
        auto mac = tlv.mac();
        if (!std::equal(mac.oct, mac.oct + 6, gTlvMacAddress)) {
            MAPF_ERR("MAC address in received TLV does not match expected result");
            errors++;
        }
        LOG(DEBUG) << "RX: " << std::endl << utils::dump_buffer(rx_buffer, tlv.getLen());
    }

    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

int test_complex_list()
{
    int errors                     = 0;
    const int complex_list_entries = 3;
    uint8_t tx_buffer[4096];

    MAPF_INFO(__FUNCTION__ << " start");

    //START BUILDING THE MESSAGE HERE
    memset(tx_buffer, 0, sizeof(tx_buffer));

    //creating cmdu message class and setting the header
    CmduMessageTx msg = CmduMessageTx(tx_buffer, sizeof(tx_buffer));
    //create method initializes the buffer and returns shared pointer to the message header
    auto header = msg.create(0, eMessageType::BACKHAUL_STEERING_REQUEST_MESSAGE);
    header->flags().last_fragment_indicator = 1;
    header->flags().relay_indicator         = 1;

    auto fourthTlv    = msg.addClass<tlvTestVarList>();
    fourthTlv->var0() = 0xa0;
    if (!fourthTlv->alloc_simple_list(2)) {
        MAPF_ERR("Failed to allocate simple list");
        errors++;
    }
    *(fourthTlv->simple_list(0)) = 0x0bb0;
    *(fourthTlv->simple_list(1)) = 0x0bb1;

    if (true == fourthTlv->set_test_string("1234567890")) {
        LOG(ERROR) << "FAIL test maximum size string";
        errors++;
    }
    // test allocation of correct length (less then max)
    if (false == fourthTlv->set_test_string("1234567")) {
        LOG(ERROR) << "FAIL test normal size string";
        errors++;
    }

    // test additional set of correct length (less then max)
    if (true == fourthTlv->set_test_string("1234567")) {
        LOG(ERROR) << "FAIL test normal size string set twice";
        errors++;
    }

    for (int i = 0; i < complex_list_entries; i++) {
        auto cmplx    = fourthTlv->create_complex_list();
        cmplx->var1() = 0xbbbbaaaa;
        cmplx->alloc_list(3);
        *(cmplx->list(0)) = 0xc0;
        *(cmplx->list(1)) = 0xc1;
        *(cmplx->list(2)) = 0xc2;
        if (!fourthTlv->add_complex_list(cmplx)) {
            LOG(ERROR) << "Failed to add complex list";
            errors++;
        }
        //test multiple add (should Fail!)
        if (fourthTlv->add_complex_list(cmplx)) {
            LOG(ERROR) << "Could add complex list a second time";
            errors++;
        }
    }

    fourthTlv->var2() = 0xabababab;
    auto cmplx        = fourthTlv->create_var1();
    cmplx->var1()     = 0xb11b;
    cmplx->set_unknown_length_list_inner("prplMesh");
    if (!fourthTlv->add_var1(cmplx)) {
        LOG(ERROR) << "Failed to add var1";
        errors++;
    }
    // Test multiple add - should fail
    if (fourthTlv->add_var1(cmplx)) {
        LOG(ERROR) << "Could add var1 a second time";
        errors++;
    }

    cmplx         = fourthTlv->create_var3();
    cmplx->var1() = 0xb11b;
    if (!fourthTlv->add_var3(cmplx)) {
        LOG(ERROR) << "Failed to add var3";
        errors++;
    }

    LOG(DEBUG) << "TLV 4 length " << fourthTlv->length();

    LOG(DEBUG) << "Finalize";
    //MANDATORY - swaps to little indian.
    if (!msg.finalize()) {
        LOG(ERROR) << "Finalize step failed";
        errors++;
    }

    LOG(DEBUG) << "TX: " << std::endl << utils::dump_buffer(tx_buffer, msg.getMessageLength());

    uint8_t recv_buffer[sizeof(tx_buffer)];
    memcpy(recv_buffer, tx_buffer, sizeof(recv_buffer));

    CmduMessageRx received_message(recv_buffer, sizeof(recv_buffer));
    received_message.parse();

    auto tlv4 = received_message.getClass<tlvUnknown>()->class_cast<tlvTestVarList>();
    if (tlv4 == nullptr) {
        MAPF_ERR("TLV4 is NULL");
        return ++errors;
    }
    if (tlv4->test_string_str().compare("1234567")) {
        MAPF_ERR("FAIL, expected  \"1234567\", received " << tlv4->test_string_str());
        errors++;
    }
    if (tlv4->complex_list_length() != complex_list_entries) {
        MAPF_ERR("Invalid complex list num of entries" << tlv4->complex_list_length());
        return ++errors;
    }
    for (int i = 0; i < complex_list_entries; i++) {
        auto cmplx_tlv4_i = std::get<1>(tlv4->complex_list(i));
        if (cmplx_tlv4_i.var1() != 0xbbbbaaaa) {
            MAPF_ERR("wrong value for cmplx_tlv4_i.var1 " << std::hex << cmplx_tlv4_i.var1());
            errors++;
        }
    }

    if (tlv4->var1()->var1() != 0xb11b) {
        MAPF_ERR("Unexpected var1 value " << tlv4->var1()->var1());
        errors++;
    }

    if (tlv4->var2() != 0xabababab) {
        MAPF_ERR("Unexpected var2 value " << tlv4->var2());
        errors++;
    }

    auto str = std::string(tlv4->var1()->unknown_length_list_inner(),
                           tlv4->var1()->unknown_length_list_inner_length());
    if (str.compare("prplMesh")) {
        MAPF_ERR("unknown length list failure - expected \"prplMesh\", received " << str);
        errors++;
    }

    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

bool add_encrypted_settings(tlvWsc &tlv, uint8_t *keywrapkey, WSC::m2::config &m2_cfg)
{
    // Encrypted settings
    // Encrypted settings are the ConfigData + IV. First create the ConfigData,
    // Then copy it to the encrypted data, add an IV and encrypt.
    // Finally, add HMAC
    uint8_t buf[1024];
    WSC::configData::config cfg;
    cfg.ssid         = "test_ssid";
    cfg.auth_type    = WSC::eWscAuth::WSC_AUTH_WPA2;
    cfg.encr_type    = WSC::eWscEncr::WSC_ENCR_AES;
    cfg.network_key  = "test1234";
    auto config_data = WSC::configData::create(cfg, buf, sizeof(buf));
    if (!config_data) {
        LOG(ERROR) << "Failed to create configData";
        return false;
    }

    LOG(DEBUG) << "WSC config_data:" << std::endl
               << "     ssid: " << config_data->ssid() << std::endl
               << "     authentication_type: " << int(config_data->auth_type()) << std::endl
               << "     network_key: " << config_data->network_key() << std::endl
               << "     encryption_type: " << int(config_data->encr_type()) << std::endl;
    config_data->finalize();

    int datalen                  = config_data->getMessageLength();
    int cipherlen                = datalen + 16;
    uint8_t encrypted[cipherlen] = {0};
    std::copy_n(config_data->getMessageBuff(), datalen, encrypted);
    if (!mapf::encryption::aes_encrypt(keywrapkey, m2_cfg.iv, encrypted, datalen, encrypted,
                                       cipherlen)) {
        LOG(ERROR) << "aes encrypt";
        return false;
    }
    m2_cfg.encrypted_settings = std::vector<uint8_t>(encrypted, encrypted + cipherlen);
    // Allocate maximum allowed length for the payload, so it can accommodate variable length
    // data inside the internal TLV list.
    // On finalize(), the buffer is shrunk back to its real size.
    size_t payload_length = tlv.getBuffRemainingBytes();
    tlv.alloc_payload(payload_length);
    auto m2 = WSC::m2::create(tlv, m2_cfg);
    if (!m2) {
        LOG(ERROR) << "create m2";
        return false;
    }
    // Finalize m2 since it needs to be in network byte order for global authentication
    m2->finalize();
    std::fill(m2->authenticator(), m2->authenticator() + WSC::eWscLengths::WSC_AUTHENTICATOR_LENGTH,
              0xbb);

    LOG(DEBUG) << "encrypted settings length: "
               << m2->encrypted_settings().encrypted_settings_length();
    LOG(DEBUG) << "encrypted settings buffer: " << std::endl
               << utils::dump_buffer((uint8_t *)m2->encrypted_settings().encrypted_settings(),
                                     m2->encrypted_settings().encrypted_settings_length());

    LOG(DEBUG) << "authenticator buffer: " << std::endl
               << utils::dump_buffer((uint8_t *)m2->authenticator(),
                                     WSC::eWscLengths::WSC_AUTHENTICATOR_LENGTH);

    return true;
}

bool parse_encrypted_settings(std::shared_ptr<tlvWsc> tlv, uint8_t *keywrapkey, uint8_t *iv)
{
    if (!tlv) {
        LOG(ERROR) << "tlv is nullptr!";
        return false;
    }
    auto m2 = WSC::m2::parse(*tlv);
    if (!m2) {
        LOG(ERROR) << "Not an M2!";
        return false;
    }
    auto encrypted_settings = m2->encrypted_settings();
    LOG(DEBUG) << "type: " << encrypted_settings.type();
    LOG(DEBUG) << "encrypted settings length: " << encrypted_settings.getLen();
    LOG(DEBUG) << "encrypted settings buffer: " << std::endl
               << utils::dump_buffer((uint8_t *)encrypted_settings.encrypted_settings(),
                                     encrypted_settings.encrypted_settings_length());
    int dlen = encrypted_settings.encrypted_settings_length() + 16;
    uint8_t buf[dlen];
    mapf::encryption::aes_decrypt(keywrapkey, iv,
                                  (uint8_t *)encrypted_settings.encrypted_settings(),
                                  encrypted_settings.encrypted_settings_length(), buf, dlen);
    LOG(DEBUG) << "configData buffer: " << std::hex << ptrdiff_t(buf) << std::endl
               << utils::dump_buffer(buf, dlen);
    auto config_data = WSC::configData::parse(buf, dlen);
    if (!config_data) {
        LOG(ERROR) << "Failed to parse config data";
        return false;
    }

    LOG(DEBUG) << "WSC config_data:" << std::endl
               << "     ssid: " << config_data->ssid() << std::endl
               << "     authentication_type: " << int(config_data->auth_type()) << std::endl
               << "     encryption_type: " << int(config_data->encr_type()) << std::endl
               << "     network_key: " << config_data->network_key() << std::endl;
    LOG(DEBUG) << "authenticator buffer: " << std::endl
               << utils::dump_buffer((uint8_t *)m2->authenticator(),
                                     WSC::eWscLengths::WSC_AUTHENTICATOR_LENGTH);
    return true;
}

int test_parser()
{
    int errors = 0;
    uint8_t tx_buffer[4096];
    //creating cmdu message class and setting the header
    CmduMessageTx msg = CmduMessageTx(tx_buffer, sizeof(tx_buffer));

    //create method initializes the buffer and returns shared pointer to the message header
    auto header = msg.create(0, eMessageType::BACKHAUL_STEERING_REQUEST_MESSAGE);
    header->flags().last_fragment_indicator = 1;
    header->flags().relay_indicator         = 1;

    auto tlv1 = msg.addClass<tlvNon1905neighborDeviceList>();
    auto tlv2 = msg.addClass<tlvLinkMetricQuery>();
    auto tlv3 = msg.addClass<tlvWsc>();
    auto tlv4 = msg.addClass<tlvTestVarList>();
    tlv4->add_var1(tlv4->create_var1());

    LOG(DEBUG) << "Finalize";
    if (msg.finalize()) {
        LOG(ERROR)
            << "Finalize should fail since the last tlv of the CMDU is not fully initialized";
        errors++;
    }

    tlv4->add_var3(tlv4->create_var3());
    LOG(DEBUG) << "Finalize";
    if (!msg.finalize()) {
        LOG(ERROR) << "Finalize step failed";
        errors++;
    }

    LOG(DEBUG) << "TX: " << std::endl << utils::dump_buffer(tx_buffer, msg.getMessageLength());

    uint8_t recv_buffer[sizeof(tx_buffer)];
    memcpy(recv_buffer, tx_buffer, sizeof(recv_buffer));

    CmduMessageRx received_message(recv_buffer, sizeof(recv_buffer));
    received_message.parse();
    auto tlv4_ = received_message.getClass<tlvUnknown>();
    if (!tlv4_) {
        LOG(ERROR) << "getClass<tlvUnknown> failed";
        errors++;
    }
    auto tlv3_ = received_message.getClass<tlvWsc>();
    if (!tlv3_) {
        LOG(ERROR) << "getClass<tlvWsc> failed";
        errors++;
    }
    auto tlv2_ = received_message.getClass<tlvLinkMetricQuery>();
    if (!tlv2_) {
        LOG(ERROR) << "getClass<tlvLinkMetricQuery> failed";
        errors++;
    }
    auto tlv1_ = received_message.getClass<tlvNon1905neighborDeviceList>();
    if (!tlv1_) {
        LOG(ERROR) << "getClass<tlvNon1905neighborDeviceList> failed";
        errors++;
    }

    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

int test_all()
{
    int errors = 0;
    uint8_t tx_buffer[4096];

    MAPF_INFO(__FUNCTION__ << " start");
    //START BUILDING THE MESSAGE HERE
    memset(tx_buffer, 0, sizeof(tx_buffer));

    //creating cmdu message class and setting the header
    CmduMessageTx msg = CmduMessageTx(tx_buffer, sizeof(tx_buffer));

    //create method initializes the buffer and returns shared pointer to the message header
    auto header = msg.create(0, eMessageType::BACKHAUL_STEERING_REQUEST_MESSAGE);
    header->flags().last_fragment_indicator = 1;
    header->flags().relay_indicator         = 1;

    //NOTE: I used random TLVs for the example, don't expect a standard IEEE1905 message
    MAPF_DBG("CLASS SIZE: " << sizeof(tlvNon1905neighborDeviceList));
    auto firstTlv           = msg.addClass<tlvNon1905neighborDeviceList>();
    bool allocation_succeed = firstTlv->alloc_mac_non_1905_device(
        3); //3 mac addresses for the example, can be any number, only limited by the buffer size
    mapf_assert(
        allocation_succeed ==
        true); //false is returned in case that there isn't enough space on the buffer for the allocation

    auto first_mac = firstTlv->mac_non_1905_device(
        0); //get the first mac address struct in this tlv. returns a <bool,sMacAddr&> collection.
    bool isExist = std::get<0>(
        first_mac); //checking the first parameter, boolean, if the address in this index exists
    if (isExist) {
        auto &address =
            std::get<1>(first_mac); //knowing that the address exists, get the address struct
        address.oct[0] = 0x00;
        address.oct[1] = 0x01;
        address.oct[2] = 0x02;
        address.oct[3] = 0x03;
        address.oct[4] = 0x04;
        address.oct[5] = 0x05;
        MAPF_DBG("WRITE 1 : " << (int)address.oct[3]);
    }

    auto second_mac = firstTlv->mac_non_1905_device(
        1); //get the second mac address struct in this tlv. returns a bool-sMacAddr& collection.
    isExist = std::get<0>(
        second_mac); //checking the first parameter, boolean, if the address in this index exists
    if (isExist) {
        auto &address =
            std::get<1>(second_mac); //knowing that the address exists, get the address struct
        address.oct[0] = 0x05;
        address.oct[1] = 0x05;
        address.oct[2] = 0x05;
        address.oct[3] = 0x05;
        address.oct[4] = 0x05;
        address.oct[5] = 0x05;
        MAPF_DBG("WRITE 2 : " << (int)address.oct[3]);
    }

    auto third_mac = firstTlv->mac_non_1905_device(
        2); //get the third mac address struct in this tlv. returns a bool-sMacAddr& collection.
    isExist = std::get<0>(
        third_mac); //checking the first parameter, boolean, if the address in this index exists
    if (isExist) {
        auto &address =
            std::get<1>(third_mac); //knowing that the address exists, get the address struct
        address.oct[0] = 0xFF;
        address.oct[1] = 0xFF;
        address.oct[2] = 0xFF;
        address.oct[3] = 0xFF;
        address.oct[4] = 0xFF;
        address.oct[5] = 0xFF;

        // Remove "unused variable" warning
        (void)address;
    }

    MAPF_DBG("TLV LENGTH START: " << firstTlv->length());
    auto secondTlv = msg.addClass<tlvLinkMetricQuery>(); // another tlv for the example
    secondTlv->link_metrics_type() = eLinkMetricsType::RX_LINK_METRICS_ONLY;

    LOG(DEBUG) << "Start WSC M2";
    auto thirdTlv = msg.addClass<tlvWsc>();

    /**
     * @brief Diffie helman key exchange
     *
     */
    uint8_t key1[192];
    uint8_t key2[192];
    unsigned key1_length = sizeof(key1);
    unsigned key2_length = sizeof(key2);
    std::fill(key1, key1 + key1_length, 1);
    std::fill(key2, key2 + key2_length, 2);
    mapf::encryption::diffie_hellman m1;
    mapf::encryption::diffie_hellman m2;
    // diffie-helman key-exchange
    m1.compute_key(key1, key1_length, m2.pubkey(), m2.pubkey_length());
    m2.compute_key(key2, key2_length, m1.pubkey(), m1.pubkey_length());
    key1_length    = sizeof(key1);
    uint8_t mac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8_t authkey[32];
    uint8_t keywrapkey[16];
    WSC::m2::config cfg;
    cfg.msg_type            = WSC::eWscMessageType::WSC_MSG_TYPE_M2;
    cfg.manufacturer        = "prplMesh";
    cfg.model_name          = "Ubuntu";
    cfg.model_number        = "18.04";
    cfg.serial_number       = "prpl12345";
    cfg.primary_dev_type_id = WSC::WSC_DEV_NETWORK_INFRA_GATEWAY;
    cfg.device_name         = "prplmesh-controller";
    cfg.encr_type_flags     = uint16_t(WSC::eWscEncr::WSC_ENCR_NONE);
    cfg.auth_type_flags     = WSC::eWscAuth::WSC_AUTH_OPEN;
    cfg.bands               = WSC::WSC_RF_BAND_2GHZ;
    mapf::encryption::create_iv(cfg.iv, sizeof(cfg.iv));
    wps_calculate_keys(m1, key1, key1_length, m1.nonce(), mac, m2.nonce(), authkey, keywrapkey);
    if (!add_encrypted_settings(*thirdTlv, keywrapkey, cfg)) {
        MAPF_ERR("add encrypted settings failed");
        return false;
    }
    LOG(DEBUG) << "Done (WSC M2)";
    MAPF_DBG("TLV LENGTH WSC M2: " << thirdTlv->length());

    auto mactlv                     = msg.addClass<tlvMacAddress>();
    const uint8_t gTlvMacAddress[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    tlvf::mac_from_array(gTlvMacAddress, mactlv->mac());

    MAPF_DBG("TLV LENGHT MAC: " << mactlv->length());

    auto fourthTlv     = msg.addClass<tlvTestVarList>();
    fourthTlv->var0()  = 0xa0;
    allocation_succeed = fourthTlv->alloc_simple_list(2);
    mapf_assert(allocation_succeed);
    *(fourthTlv->simple_list(0)) = 0x0bb0;
    *(fourthTlv->simple_list(1)) = 0x0bb1;

    if (fourthTlv->alloc_simple_list(7)) {
        LOG(ERROR) << "Allocation succeeded despite list max length overflow!";
        errors++;
    }

    auto cmplx = fourthTlv->create_complex_list();
    cmplx->alloc_list(3);
    *(cmplx->list(0)) = 0xc0;
    *(cmplx->list(1)) = 0xc1;
    *(cmplx->list(2)) = 0xc2;
    cmplx->var1()     = 0xd00d;
    cmplx->alloc_list();
    *(cmplx->list(3)) = 0xc3;
    cmplx->alloc_list();
    *(cmplx->list(4)) = 0xc4;
    cmplx->alloc_list();
    *(cmplx->list(5)) = 0xc5;
    if (!fourthTlv->add_complex_list(cmplx)) { //first entry
        LOG(ERROR) << "Failed to add complex list";
        errors++;
    }

    // TODO the complex list doesn't work at the moment if it has more than one element
    // Cfr. #137
    cmplx         = fourthTlv->create_complex_list();
    cmplx->var1() = 0xa11ad11d;
    if (!fourthTlv->add_complex_list(cmplx)) { //second entry
        LOG(ERROR) << "Failed to add complex list";
        errors++;
    }
    if (fourthTlv->add_complex_list(cmplx)) {
        LOG(ERROR) << "Could add complex list a second time";
        errors++;
    }
    cmplx         = fourthTlv->create_var1();
    cmplx->var1() = 0xeeee;
    if (!fourthTlv->add_var1(cmplx)) {
        LOG(ERROR) << "Failed to add var1";
        errors++;
    }
    if (fourthTlv->add_var1(cmplx)) {
        LOG(ERROR) << "Could add var1 a second time";
        errors++;
    }

    cmplx         = fourthTlv->create_var3();
    cmplx->var1() = 0xeeee;

    if (!fourthTlv->add_var3(cmplx)) {
        LOG(ERROR) << "Failed to add var3";
        errors++;
    }
    if (fourthTlv->add_var3(cmplx)) {
        LOG(ERROR) << "Could add var3 a second time";
        errors++;
    }

    LOG(DEBUG) << "TLV 4 length " << fourthTlv->length();
    auto unknown    = fourthTlv->create_unknown_length_list();
    unknown->var1() = 0xbbbbaaaa;
    fourthTlv->add_unknown_length_list(unknown);
    LOG(DEBUG) << "Unknown list size: " << unknown->getLen();
    LOG(DEBUG) << "Total unknown Length: " << fourthTlv->unknown_length_list_length();
    LOG(DEBUG) << "TLV 4 length " << fourthTlv->length();

    LOG(DEBUG) << "Total Message length=" << int(msg.getMessageLength());

    LOG(DEBUG) << "Finalize";
    //MANDATORY - swaps to little indian.
    if (!msg.finalize()) {
        LOG(ERROR) << "Finalize step failed";
        errors++;
    }

    LOG(DEBUG) << "TX: " << std::endl << utils::dump_buffer(tx_buffer, msg.getMessageLength());

    uint8_t recv_buffer[sizeof(tx_buffer)];
    memcpy(recv_buffer, tx_buffer, sizeof(recv_buffer));

    CmduMessageRx received_message(recv_buffer, sizeof(recv_buffer));
    received_message.parse();

    auto tlv1 = received_message.getClass<tlvNon1905neighborDeviceList>();
    if (tlv1 != nullptr) {
        MAPF_DBG("LENGTH AFTER INIT: " << tlv1->length());
        //tlv1->alloc_mac_non_1905_device(3);

        auto mac2 = tlv1->mac_non_1905_device(
            2); //get the second mac address struct in this tlv. returns a bool-sMacAddr& collection.
        isExist = std::get<0>(
            mac2); //checking the first parameter, boolean, if the address in this index exists
        if (isExist) {
            auto address =
                std::get<1>(mac2); //knowing that the address exists, get the address struct
                                   /*       address.oct[0] = 0x05;
            address.oct[1] = 0x05;
            address.oct[2] = 0x05;
            address.oct[3] = 0x05;
            address.oct[4] = 0x05;
            address.oct[5] = 0x05;*/

            LOG(DEBUG) << "ADDRESS IS " << (int)address.oct[0];
        } else {
            MAPF_ERR("TLV DOES NOT EXIST");
            errors++;
        }
    } else {
        MAPF_ERR("TLV1 IS NULL");
        errors++;
    }

    auto tlv2 = received_message.getClass<tlvLinkMetricQuery>();
    if (tlv2 != nullptr) {
        MAPF_DBG("TLV2 LENGTH AFTER INIT: " << tlv2->length());
    } else {
        MAPF_ERR("TLV2 IS NULL");
        errors++;
    }

    auto tlv3 = received_message.getClass<tlvWsc>();
    if (tlv3 != nullptr) {
        MAPF_DBG("TLV3 LENGTH AFTER INIT: " << tlv3->length());
    } else {
        MAPF_ERR("TLV3 IS NULL");
        errors++;
    }
    if (!parse_encrypted_settings(tlv3, keywrapkey, cfg.iv)) {
        MAPF_ERR("TLV3 parse encrypted settings failed");
        errors++;
    }

    auto tlv4 = received_message.getClass<tlvUnknown>()->class_cast<tlvTestVarList>();
    if (tlv4 == nullptr) {
        MAPF_ERR("TLV4 is NULL");
        errors++;
    } else {
        if (tlv4->var0() != 0xa0) {
            MAPF_ERR("TLV4 var0 is 0x" << std::hex << tlv4->var0() << " instead of 0xa0");
            errors++;
        }

        if (tlv4->simple_list_length() != 2) {
            MAPF_ERR("TLV4 simple list length is " << unsigned(tlv4->simple_list_length())
                                                   << " instead of 2");
            errors++;
        }
        for (uint8_t list_idx = 0; list_idx < tlv4->simple_list_length(); list_idx++) {
            if (!tlv4->simple_list(list_idx)) {
                MAPF_ERR("TLV4 has no simple " << list_idx);
                errors++;
            } else {
                uint16_t expected = 0x0bb0;
                auto value        = tlv4->simple_list(list_idx);
                if (*value != expected + list_idx) {
                    MAPF_ERR("TLV4 simple ") << list_idx << " has value " << std::hex << *value
                                             << " instead of " << std::hex << expected + list_idx;
                    errors++;
                }
            }
        }

        LOG(DEBUG) << "Total unknown Length: " << tlv4->unknown_length_list_length();
        if (tlv4->unknown_length_list_length() != fourthTlv->unknown_length_list_length()) {
            MAPF_ERR("TLV 4 unknown length list length mismatch");
            errors++;
        }

        LOG(DEBUG) << "TLV 4 length " << tlv4->length();
        // TODO the complex list doesn't work at the moment if it has more than one element
        // Cfr. #137
        if (tlv4->complex_list_length() != 2) {
            MAPF_ERR("TLV4 complex list length is " << unsigned(tlv4->complex_list_length())
                                                    << " instead of 2");
            errors++;
        }

        if (!std::get<0>(tlv4->complex_list(0))) {
            MAPF_ERR("TLV4 has no complex 0");
            errors++;
        } else {
            auto cmplx_tlv4_0 = std::get<1>(tlv4->complex_list(0));
            if (cmplx_tlv4_0.list_length() != 6) {
                MAPF_ERR("TLV4 complex 0 list length is " << unsigned(cmplx_tlv4_0.list_length())
                                                          << " instead of 6");
                errors++;
            }
            uint8_t expected = 0xc0;
            for (uint8_t list_idx = 0; list_idx < cmplx_tlv4_0.list_length(); list_idx++) {
                if (!cmplx_tlv4_0.list(list_idx)) {
                    MAPF_ERR("TLV4 complex 0 has no list[" << list_idx << "]");
                    errors++;
                } else {
                    auto value = cmplx_tlv4_0.list(list_idx);
                    if (*value != expected + list_idx) {
                        MAPF_ERR("TLV4 complex 0 list ")
                            << list_idx << " has value " << std::hex << *value << " instead of "
                            << std::hex << expected + list_idx;
                        errors++;
                    }
                }
            }

            if (cmplx_tlv4_0.var1() != 0xd00d) {
                MAPF_ERR("TLV4 complex 0 var1 is " << std::hex << cmplx_tlv4_0.var1()
                                                   << " instead of 0xd00d");
                errors++;
            }
        }
        if (!std::get<0>(tlv4->complex_list(1))) {
            MAPF_ERR("TLV4 has no complex 1");
            errors++;
        } else {
            auto cmplx_tlv4_1 = std::get<1>(tlv4->complex_list(1));
            if (cmplx_tlv4_1.list_length() != 0) {
                MAPF_ERR("TLV4 complex 1 list length is " << unsigned(cmplx_tlv4_1.list_length())
                                                          << " instead of 0");
                errors++;
            }
            if (cmplx_tlv4_1.var1() != 0xa11ad11d) {
                MAPF_ERR("TLV4 complex 1 var1 is " << std::hex << cmplx_tlv4_1.var1()
                                                   << " instead of 0xa11ad11d");
                errors++;
            }
        }

        auto var1 = tlv4->var1();
        if (!var1) {
            MAPF_ERR("TLV4 var1 is not set");
        } else {
            if (var1->list_length() != 0) {
                MAPF_ERR("TLV4 var1 list length is " << unsigned(var1->list_length())
                                                     << " instead of 0");
                errors++;
            }
            if (var1->var1() != 0xeeee) {
                MAPF_ERR("TLV4 var1 var1 is " << std::hex << var1->var1() << " instead of 0xeeee");
                errors++;
            }
        }
    }

    int invalidBufferSize = 26;
    uint8_t invalidBuffer[invalidBufferSize];
    memcpy(invalidBuffer, recv_buffer, 26);

    CmduMessageRx invmsg(invalidBuffer, invalidBufferSize);
    if (!invmsg.parse()) {
        MAPF_DBG("Parse PROTECTION SUCCESS");
    }

    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

int test_mac_from_string()
{
    MAPF_INFO(__FUNCTION__ << " Starting");
    int errors = 0;

    // To make tests easy, we use MAC digits in the range 0x30-0x39, which is just 0-9 as string.
    // To have some letters as well, we can use 0x3d which is = and 0x3f which is ?
    auto check_success = [&errors](const std::string &mac, const char *as_int) {
        uint8_t mac_oct[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
        if (tlvf::mac_from_string(mac_oct, mac)) {
            if (std::memcmp(mac_oct, as_int, sizeof(mac_oct)) != 0) {
                MAPF_INFO("mac_from_string doesn't match expected: "
                          << mac << " -> " << as_int << " got " << tlvf::mac_to_string(mac_oct));
                errors++;
            }
        } else {
            MAPF_INFO("mac_from_string failed for " << mac);
            errors++;
        }
    };

    check_success("30:31:32:33:34:35", "012345");
    check_success("0x363738393d3f", "6789=?");
    check_success("0x363738393D3F", "6789=?");
    check_success("36:37:38:39:3D:3F", "6789=?");
    check_success("333333333333", "333333");
    check_success("", "\0\0\0\0\0\0");

    auto check_fail = [&errors](const std::string &mac) {
        uint8_t mac_oct[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
        if (tlvf::mac_from_string(mac_oct, mac)) {
            MAPF_INFO("mac_from_string should fail for " << mac << " got "
                                                         << tlvf::mac_to_string(mac_oct));
            errors++;
        } else {
            const uint8_t zero[6] = {0, 0, 0, 0, 0, 0};
            if (std::memcmp(mac_oct, zero, sizeof(mac_oct)) != 0) {
                MAPF_INFO("mac_oct not wiped after failed conversion of " << mac);
                errors++;
            }
        }
    };
    check_fail("1");
    check_fail("30:31:32:33:34:3g");    // Non-hex digit
    check_fail("30:31:32:33:34");       // Too few
    check_fail("30:31:32:33:34:35:36"); // Too many
    check_fail("30 31 32 33 34 35");    // Invalid separator
    check_fail("30031:32:33:34:35");    // Wrong format
    check_fail("30:31:32:33:340:35:3"); // Wrong format

    check_fail("0x363738393d3g");  // Non-hex digit
    check_fail("0x363738393d3");   // Too few
    check_fail("0x363738393d3f3"); // Too many
    check_fail("0y363738393d3f3"); // Doesn't start with 0x

    check_fail("363738393d3g");  // Non-hex digit
    check_fail("363738393d3");   // Too few
    check_fail("363738393d3f3"); // Too many

    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

/*
 * Test the conditional parameters by using tlvProfile2ChannelScanResult.
 * Parse rx_buffer, and try to access the conditional elements if they
 * must be present (i.e. if the condition is met).
 * Then, try to create a tx TLV with the same values, and check that rx and tx match.
 */
int _test_conditional_parameters_rx_tx(uint8_t *rx_buffer, size_t rx_size)
{
    int errors  = 0;
    auto tlv_rx = tlvProfile2ChannelScanResult(rx_buffer, rx_size, true);
    auto len    = tlv_rx.neighbors_list_length();
    LOG(DEBUG) << "len: " << len << std::endl;
    for (unsigned i = 0; i < len; i++) {
        auto t = tlv_rx.neighbors_list(i);
        if (!std::get<0>(t)) {
            MAPF_ERR("Failed to get neighbor " + i);
            errors++;
        }
        auto &n = std::get<1>(t);
        // bssid follows the station count, when debugging it's useful to print to check that the sizes are correct:
        LOG(DEBUG) << "neighbor " << i << " bssid: " << n.bssid() << std::endl;
        LOG(DEBUG) << "neighbor " << i << " load presence flag: " << n.bss_load_element_present()
                   << std::endl;
        if (n.bss_load_element_present() ==
            wfa_map::cNeighbors::eBssLoadElementPresent::FIELD_PRESENT) {
            LOG(DEBUG) << "neighbor " << i << " channel utilization: " << *(n.channel_utilization())
                       << std::endl;
            LOG(DEBUG) << "neighbor " << i << " station count: " << *(n.station_count())
                       << std::endl;
        } else {
            if (n.channel_utilization()) {
                MAPF_ERR("Load presence flag not set but channel_utilization not nullptr!");
            }
            if (n.station_count()) {
                MAPF_ERR("Load presence flag not set but station_count not nullptr!");
            }
        }
    }

    uint8_t tx_buffer[4096];
    memset(tx_buffer, 0, sizeof(tx_buffer));
    auto tlv_tx              = tlvProfile2ChannelScanResult(tx_buffer, sizeof(tx_buffer), false);
    tlv_tx.radio_uid()       = tlv_rx.radio_uid();
    tlv_tx.operating_class() = tlv_rx.operating_class();
    tlv_tx.channel()         = tlv_rx.channel();
    tlv_tx.success()         = tlv_rx.success();
    tlv_tx.set_timestamp(tlv_rx.timestamp_str());
    tlv_tx.utilization() = tlv_rx.utilization();
    tlv_tx.noise()       = tlv_rx.noise();
    for (unsigned i = 0; i < len; i++) {
        auto t = tlv_rx.neighbors_list(i);
        if (!std::get<0>(t)) {
            MAPF_ERR("Failed to get neighbor " + i);
            errors++;
        }
        auto &rx_neigh                       = std::get<1>(t);
        std::shared_ptr<cNeighbors> tx_neigh = tlv_tx.create_neighbors_list();
        tx_neigh->bssid()                    = rx_neigh.bssid();
        tx_neigh->set_ssid(rx_neigh.ssid_str());
        tx_neigh->signal_strength() = rx_neigh.signal_strength();
        tx_neigh->set_channels_bw_list(rx_neigh.channels_bw_list_str());
        if (rx_neigh.bss_load_element_present() ==
            wfa_map::cNeighbors::eBssLoadElementPresent::FIELD_PRESENT) {
            tx_neigh->bss_load_element_present() =
                wfa_map::cNeighbors::eBssLoadElementPresent::FIELD_PRESENT;
            if (!tx_neigh->set_channel_utilization(*rx_neigh.channel_utilization())) {
                MAPF_ERR("Failed to set_channel_utilization");
                errors++;
            }
            if (!tx_neigh->set_station_count(*rx_neigh.station_count())) {
                MAPF_ERR("Failed to set_station_count");
                errors++;
            }
        }

        tlv_tx.add_neighbors_list(tx_neigh);
    }
    tlv_tx.aggregate_scan_duration() = tlv_rx.aggregate_scan_duration();
    tlv_tx.scan_type()               = tlv_rx.scan_type();

    // tlv_rx is already in host byte order (swapping is done in
    // init()). If we want to compare tx to rx, we have to NOT swap
    // tx, to keep it in host byte order as well.

    if (!std::equal(tlv_rx.getStartBuffPtr(), tlv_rx.getStartBuffPtr() + tlv_rx.getLen(),
                    tlv_tx.getStartBuffPtr())) {
        MAPF_ERR("RX and TX buffers do not match!");
        errors++;
        LOG(DEBUG) << "RX: " << std::endl << utils::dump_buffer(tlv_rx.getStartBuffPtr(), rx_size);
        LOG(DEBUG) << "TX: " << std::endl
                   << utils::dump_buffer(tlv_tx.getStartBuffPtr(), tlv_tx.getLen());
    }

    if (tlv_rx.length() != tlv_tx.length()) {
        MAPF_ERR("RX and TX buffers don't report the same length!");
        LOG(DEBUG) << "RX: " << tlv_rx.length() << std::endl;
        LOG(DEBUG) << "TX: " << tlv_tx.length() << std::endl;
        errors++;
    }

    if (tlv_rx.length() != rx_size - sizeof(ieee1905_1::sTlvHeader)) {
        MAPF_ERR("RX doesn't report a correct length!");
        LOG(DEBUG) << "RX length: " << tlv_rx.length()
                   << "Buffer length (excluding type and length size): "
                   << rx_size - sizeof(ieee1905_1::sTlvHeader) << std::endl;
        errors++;
    }

    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

/**
 * @brief Check if \p field matches \p value.
 *
 * This is meant to be used to increment an `errors` variable on failure.
 * @return 0 if \p field matches \p value, 1 otherwise.
 **/
template <typename T> int check_field(T field, T value, std::string name)
{
    if (field != value) {
        MAPF_ERR(name + " does not match!");
        MAPF_ERR(field);
        MAPF_ERR(value);
        return 1;
    }
    return 0;
}

int test_channel_scan_results()
{
    int errors = 0;
    MAPF_INFO(__FUNCTION__ << " start");

    // 2 neighbors, the second one doesn't have the bss load elements:
    MAPF_INFO(__FUNCTION__ << " start with rx_buffer_3");
    uint8_t rx_buffer[] = {
        0xa7, 0x00, 0x6f, 0x00, 0x50, 0x43, 0x24, 0x19, 0x30, 0x51, 0x06, 0x00, 0x1f, 0x32, 0x30,
        0x38, 0x39, 0x2d, 0x30, 0x32, 0x2d, 0x30, 0x31, 0x54, 0x30, 0x30, 0x3a, 0x33, 0x35, 0x3a,
        0x30, 0x37, 0x2e, 0x30, 0x30, 0x30, 0x30, 0x30, 0x2b, 0x30, 0x30, 0x3a, 0x30, 0x30, 0x0a,
        0x64, 0x00, 0x02, 0x00, 0x0c, 0x43, 0x48, 0xa0, 0x26, 0x0e, 0x4d, 0x75, 0x6c, 0x74, 0x69,
        0x2d, 0x41, 0x50, 0x2d, 0x32, 0x34, 0x2d, 0x54, 0x31, 0xe7, 0x05, 0x32, 0x30, 0x4d, 0x48,
        0x7a, 0x80, 0x06, 0x00, 0x00, 0x00, 0x50, 0x43, 0x24, 0x18, 0xb0, 0x0e, 0x4d, 0x75, 0x6c,
        0x74, 0x69, 0x2d, 0x41, 0x50, 0x2d, 0x32, 0x34, 0x2d, 0x54, 0x32, 0xe6, 0x05, 0x32, 0x30,
        0x4d, 0x48, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00};
    auto tlv_rx = tlvProfile2ChannelScanResult(rx_buffer, sizeof(rx_buffer), true);

    errors += check_field<sMacAddr>(tlv_rx.radio_uid(), tlvf::mac_from_string("00:50:43:24:19:30"),
                                    "radio_uid");
    errors += check_field<uint8_t>(tlv_rx.operating_class(), 0x51, "operating_class");
    errors += check_field<uint8_t>(tlv_rx.channel(), 0x06, "channel");
    errors += check_field<tlvProfile2ChannelScanResult::eScanStatus>(
        tlv_rx.success(), tlvProfile2ChannelScanResult::eScanStatus::SUCCESS, "success");
    errors += check_field<std::string>(tlv_rx.timestamp_str(), "2089-02-01T00:35:07.00000+00:00",
                                       "timestamp");
    errors += check_field<uint8_t>(tlv_rx.utilization(), 10, "utilization");
    errors += check_field<uint8_t>(tlv_rx.noise(), 100, "noise");

    errors += check_field<uint16_t>(tlv_rx.neighbors_list_length(), 2, "neighbors list length");
    int neigh_num = 0;

    {
        // Neighbor 0, it has load elements.
        auto t = tlv_rx.neighbors_list(neigh_num);
        if (!std::get<0>(t)) {
            MAPF_ERR("Failed to get neighbor " + neigh_num);
            errors++;
        }
        auto &rx_neigh = std::get<1>(t);
        errors +=
            check_field<sMacAddr>(rx_neigh.bssid(), tlvf::mac_from_string("00:0c:43:48:a0:26"),
                                  "neigh" + std::to_string(neigh_num) + " bssid");
        errors += check_field<uint8_t>(rx_neigh.ssid_length(), 14,
                                       "neigh " + std::to_string(neigh_num) + " ssid_length");
        errors += check_field<std::string>(rx_neigh.ssid_str(), "Multi-AP-24-T1",
                                           "neigh " + std::to_string(neigh_num) + " ssid");
        errors += check_field<uint8_t>(rx_neigh.signal_strength(), 231,
                                       "neigh " + std::to_string(neigh_num) + " signal strength");
        errors += check_field<std::string>(rx_neigh.channels_bw_list_str(), "20MHz",
                                           "neigh " + std::to_string(neigh_num) + " bandwidth");
        errors +=
            check_field<uint8_t>(rx_neigh.bss_load_element_present(),
                                 wfa_map::cNeighbors::eBssLoadElementPresent::FIELD_PRESENT,
                                 "neigh " + std::to_string(neigh_num) + " load element presence");
        errors +=
            check_field<uint8_t>(*rx_neigh.channel_utilization(), 6,
                                 "neigh " + std::to_string(neigh_num) + " channel_utilization");
        errors += check_field<uint16_t>(*rx_neigh.station_count(), 0,
                                        "neigh " + std::to_string(neigh_num) + " station_count");
    }

    ++neigh_num;
    {
        // Neighbor 1, it does NOT have load elements.
        auto t = tlv_rx.neighbors_list(neigh_num);
        if (!std::get<0>(t)) {
            MAPF_ERR("Failed to get neighbor " + neigh_num);
            errors++;
        }
        auto &rx_neigh = std::get<1>(t);
        errors +=
            check_field<sMacAddr>(rx_neigh.bssid(), tlvf::mac_from_string("00:50:43:24:18:b0"),
                                  "neigh" + std::to_string(neigh_num) + " bssid");
        errors += check_field<uint8_t>(rx_neigh.ssid_length(), 14,
                                       "neigh " + std::to_string(neigh_num) + " ssid_length");
        errors += check_field<std::string>(rx_neigh.ssid_str(), "Multi-AP-24-T2",
                                           "neigh " + std::to_string(neigh_num) + " ssid");
        errors += check_field<uint8_t>(rx_neigh.signal_strength(), 230,
                                       "neigh " + std::to_string(neigh_num) + " signal strength");
        errors += check_field<std::string>(rx_neigh.channels_bw_list_str(), "20MHz",
                                           "neigh " + std::to_string(neigh_num) + " bandwidth");
        errors +=
            check_field<uint8_t>(rx_neigh.bss_load_element_present(),
                                 wfa_map::cNeighbors::eBssLoadElementPresent::FIELD_NOT_PRESENT,
                                 "neigh " + std::to_string(neigh_num) + " load element presence");
    }

    errors +=
        check_field<uint32_t>(tlv_rx.aggregate_scan_duration(), 100, "aggregate scan duration");
    errors += check_field<uint8_t>(
        tlv_rx.scan_type(), wfa_map::tlvProfile2ChannelScanResult::eScanType::SCAN_WAS_PASSIVE_SCAN,
        "scan type");

    if (tlv_rx.length() != sizeof(rx_buffer) - sizeof(ieee1905_1::sTlvHeader)) {
        MAPF_ERR("RX doesn't report a correct length!");
        LOG(DEBUG) << "RX length: " << tlv_rx.length()
                   << "Buffer length (excluding type and length size): "
                   << sizeof(rx_buffer) - sizeof(ieee1905_1::sTlvHeader) << std::endl;
        errors++;
    }

    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

int test_conditional_parameters_rx_tx()
{
    int errors = 0;
    MAPF_INFO(__FUNCTION__ << " start with rx_buffer_1");

    // 2 neighbors, none have the bss load elements:
    uint8_t rx_buffer_1[] = {
        0xa7, 0x00, 0x6c, 0x00, 0x50, 0x43, 0x24, 0x19, 0x30, 0x51, 0x06, 0x00, 0x1f, 0x32,
        0x30, 0x38, 0x39, 0x2d, 0x30, 0x32, 0x2d, 0x30, 0x31, 0x54, 0x30, 0x30, 0x3a, 0x33,
        0x35, 0x3a, 0x30, 0x37, 0x2e, 0x30, 0x30, 0x30, 0x30, 0x30, 0x2b, 0x30, 0x30, 0x3a,
        0x30, 0x30, 0x0a, 0x64, 0x00, 0x02, 0x00, 0x0c, 0x43, 0x48, 0xa0, 0x26, 0x0e, 0x4d,
        0x75, 0x6c, 0x74, 0x69, 0x2d, 0x41, 0x50, 0x2d, 0x32, 0x34, 0x2d, 0x54, 0x31, 0xe7,
        0x05, 0x32, 0x30, 0x4d, 0x48, 0x7a, 0x00, 0x00, 0x50, 0x43, 0x24, 0x18, 0xb0, 0x0e,
        0x4d, 0x75, 0x6c, 0x74, 0x69, 0x2d, 0x41, 0x50, 0x2d, 0x32, 0x34, 0x2d, 0x54, 0x32,
        0xe6, 0x05, 0x32, 0x30, 0x4d, 0x48, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00};
    errors += _test_conditional_parameters_rx_tx(rx_buffer_1, sizeof(rx_buffer_1));

    // 2 neighbors, the first one doesn't have the bss load elements:
    MAPF_INFO(__FUNCTION__ << " start with rx_buffer_2");
    uint8_t rx_buffer_2[] = {
        0xa7, 0x00, 0x6f, 0x00, 0x50, 0x43, 0x24, 0x19, 0x30, 0x51, 0x06, 0x00, 0x1f, 0x32, 0x30,
        0x38, 0x39, 0x2d, 0x30, 0x32, 0x2d, 0x30, 0x31, 0x54, 0x30, 0x30, 0x3a, 0x33, 0x35, 0x3a,
        0x30, 0x37, 0x2e, 0x30, 0x30, 0x30, 0x30, 0x30, 0x2b, 0x30, 0x30, 0x3a, 0x30, 0x30, 0x0a,
        0x64, 0x00, 0x02, 0x00, 0x0c, 0x43, 0x48, 0xa0, 0x26, 0x0e, 0x4d, 0x75, 0x6c, 0x74, 0x69,
        0x2d, 0x41, 0x50, 0x2d, 0x32, 0x34, 0x2d, 0x54, 0x31, 0xe7, 0x05, 0x32, 0x30, 0x4d, 0x48,
        0x7a, 0x00, 0x00, 0x50, 0x43, 0x24, 0x18, 0xb0, 0x0e, 0x4d, 0x75, 0x6c, 0x74, 0x69, 0x2d,
        0x41, 0x50, 0x2d, 0x32, 0x34, 0x2d, 0x54, 0x32, 0xe6, 0x05, 0x32, 0x30, 0x4d, 0x48, 0x7a,
        0x80, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00};
    errors += _test_conditional_parameters_rx_tx(rx_buffer_2, sizeof(rx_buffer_2));

    // 2 neighbors, the second one doesn't have the bss load elements:
    MAPF_INFO(__FUNCTION__ << " start with rx_buffer_3");
    uint8_t rx_buffer_3[] = {
        0xa7, 0x00, 0x6f, 0x00, 0x50, 0x43, 0x24, 0x19, 0x30, 0x51, 0x06, 0x00, 0x1f, 0x32, 0x30,
        0x38, 0x39, 0x2d, 0x30, 0x32, 0x2d, 0x30, 0x31, 0x54, 0x30, 0x30, 0x3a, 0x33, 0x35, 0x3a,
        0x30, 0x37, 0x2e, 0x30, 0x30, 0x30, 0x30, 0x30, 0x2b, 0x30, 0x30, 0x3a, 0x30, 0x30, 0x0a,
        0x64, 0x00, 0x02, 0x00, 0x0c, 0x43, 0x48, 0xa0, 0x26, 0x0e, 0x4d, 0x75, 0x6c, 0x74, 0x69,
        0x2d, 0x41, 0x50, 0x2d, 0x32, 0x34, 0x2d, 0x54, 0x31, 0xe7, 0x05, 0x32, 0x30, 0x4d, 0x48,
        0x7a, 0x80, 0x06, 0x00, 0x00, 0x00, 0x50, 0x43, 0x24, 0x18, 0xb0, 0x0e, 0x4d, 0x75, 0x6c,
        0x74, 0x69, 0x2d, 0x41, 0x50, 0x2d, 0x32, 0x34, 0x2d, 0x54, 0x32, 0xe6, 0x05, 0x32, 0x30,
        0x4d, 0x48, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00};
    errors += _test_conditional_parameters_rx_tx(rx_buffer_3, sizeof(rx_buffer_3));

    // 2 neighbors, both of them have the bss load elements:
    MAPF_INFO(__FUNCTION__ << " start with rx_buffer_4");
    uint8_t rx_buffer_4[] = {
        0xa7, 0x00, 0x72, 0x00, 0x50, 0x43, 0x24, 0x19, 0x30, 0x51, 0x06, 0x00, 0x1f, 0x32, 0x30,
        0x38, 0x39, 0x2d, 0x30, 0x32, 0x2d, 0x30, 0x31, 0x54, 0x30, 0x30, 0x3a, 0x33, 0x35, 0x3a,
        0x30, 0x37, 0x2e, 0x30, 0x30, 0x30, 0x30, 0x30, 0x2b, 0x30, 0x30, 0x3a, 0x30, 0x30, 0x0a,
        0x64, 0x00, 0x02, 0x00, 0x0c, 0x43, 0x48, 0xa0, 0x26, 0x0e, 0x4d, 0x75, 0x6c, 0x74, 0x69,
        0x2d, 0x41, 0x50, 0x2d, 0x32, 0x34, 0x2d, 0x54, 0x31, 0xe7, 0x05, 0x32, 0x30, 0x4d, 0x48,
        0x7a, 0x80, 0x06, 0x00, 0x00, 0x00, 0x50, 0x43, 0x24, 0x18, 0xb0, 0x0e, 0x4d, 0x75, 0x6c,
        0x74, 0x69, 0x2d, 0x41, 0x50, 0x2d, 0x32, 0x34, 0x2d, 0x54, 0x32, 0xe6, 0x05, 0x32, 0x30,
        0x4d, 0x48, 0x7a, 0x80, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00};
    errors += _test_conditional_parameters_rx_tx(rx_buffer_4, sizeof(rx_buffer_4));

    return errors;
}

int test_create_error_response_message()
{
    int errors = 0;

    constexpr uint16_t buf_size = 4096;
    uint8_t buff[buf_size];
    ieee1905_1::CmduMessageTx cmdu_tx(buff, buf_size);

    if (!cmdu_tx.create(1 /*mid*/, ieee1905_1::eMessageType::ERROR_RESPONSE_MESSAGE)) {
        LOG(ERROR) << "cmdu creation of type ERROR_RESPONSE_MESSAGE, has failed";
        errors++;
    }
    auto profile2_error_code_tlv = cmdu_tx.addClass<wfa_map::tlvProfile2ErrorCode>();
    if (!profile2_error_code_tlv) {
        LOG(ERROR) << "addClass has failed";
        errors++;
    }

    // Check the size before adding the conditional element:
    if (profile2_error_code_tlv->length() != 1) {
        LOG(ERROR) << "tlvProfile2ErrorCode has a wrong length! length: "
                   << profile2_error_code_tlv->length();
        errors++;
    }

    profile2_error_code_tlv->reason_code() = wfa_map::tlvProfile2ErrorCode::eReasonCode::
        NUMBER_OF_UNIQUE_VLAN_ID_EXCEEDS_MAXIMUM_SUPPORTED;
    profile2_error_code_tlv->set_bssid(tlvf::mac_from_string("01:02:03:04:05:06"));

    // Check the size after adding the conditional element (it should have increased by the size of the MAC address):
    int expected_len = 7;
    if (profile2_error_code_tlv->length() != expected_len) {
        LOG(ERROR) << "tlvProfile2ErrorCode has a wrong length! length: "
                   << profile2_error_code_tlv->length() << ". Expected: " << expected_len;
        errors++;
    }

    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

int test_parse_assoc_frame()
{
    int errors = 0;
    MAPF_INFO(__FUNCTION__ << " start");

    /*
     * frame raw buffer in network byte order
     */
    std::vector<uint8_t> assoc_req_frame_body_buffer = {
        0x31, 0x14, 0x14, 0x00, 0x00, 0x0e, 0x4d, 0x75, 0x6c, 0x74, 0x69, 0x2d, 0x41, 0x50, 0x2d,
        0x32, 0x34, 0x47, 0x2d, 0x31, 0x01, 0x08, 0x02, 0x04, 0x0b, 0x0c, 0x12, 0x16, 0x18, 0x24,
        0x21, 0x02, 0x00, 0x14, 0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00,
        0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x00, 0x32, 0x04, 0x30, 0x48,
        0x60, 0x6c, 0x3b, 0x10, 0x51, 0x51, 0x53, 0x54, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x7c,
        0x7d, 0x7e, 0x7f, 0x80, 0x82, 0x3b, 0x16, 0x0c, 0x01, 0x02, 0x03, 0x04, 0x05, 0x0c, 0x16,
        0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x80, 0x81, 0x82, 0x46,
        0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x46, 0x05, 0x71, 0x50, 0x50, 0x00, 0x04, 0x7f, 0x0a,
        0x04, 0x00, 0x0a, 0x82, 0x21, 0x40, 0x00, 0x40, 0x80, 0x00, 0xdd, 0x07, 0x00, 0x50, 0xf2,
        0x02, 0x00, 0x01, 0x00, 0x2d, 0x1a, 0x2d, 0x11, 0x03, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xe6, 0xe1,
        0x09, 0x00, 0xbf, 0x0c, 0xb0, 0x79, 0xd1, 0x33, 0xfa, 0xff, 0x0c, 0x03, 0xfa, 0xff, 0x0c,
        0x03, 0xc7, 0x01, 0x10, 0xdd, 0x07, 0x50, 0x6f, 0x9a, 0x16, 0x03, 0x01, 0x03};

    auto assoc_frame = assoc_frame::AssocReqFrame::parse(
        assoc_req_frame_body_buffer.data(), assoc_req_frame_body_buffer.size(),
        assoc_frame::AssocReqFrame::ASSOCIATION_REQUEST);
    if (!assoc_frame) {
        MAPF_ERR("Failed to parse association frame.");
        errors++;
    } else {

        /*
         * Frame Body:
         * 3114 => capability info
         * 1400 => listen interval
         */
        auto capInfoDmgStaField = assoc_frame->getAttr<assoc_frame::cCapInfoDmgSta>();
        if (!capInfoDmgStaField) {
            MAPF_ERR("no capInfo field.");
            errors++;
        } else {
            errors += check_field<uint16_t>(capInfoDmgStaField->listen_interval(), 0x14,
                                            "listen_interval");
        }

        /*
         * 00 0E 4D756C74692D41502D3234472D31
         * => ID_SSID "Multi-AP-24G-1"
         */
        errors += check_field<std::string>(assoc_frame->sta_ssid(), "Multi-AP-24G-1", "ssid");

        /*
         * 01 08 02040B0C12161824
         * => ID_SUPPORT_RATES: 1 2 5.5 6 9 11 12 18 mbps
         */
        auto suppRates = assoc_frame->getAttr<assoc_frame::cSupportRates>();
        if (!suppRates) {
            MAPF_ERR("no supported rates field.");
            errors++;
        } else {
            std::vector<uint8_t> values = {
                0x02, 0x04, 0x0b, 0x0c, 0x12, 0x16, 0x18, 0x24,
            };
            std::vector<uint8_t> rates(suppRates->supported_rated(),
                                       suppRates->supported_rated() + suppRates->length());
            errors += check_field<std::vector<uint8_t>>(rates, values, "supported_rates");
        }

        /*
         * 21 02 0014
         * => ID_POWER_CAPABILITY tx_pwr max: 20
         */
        auto pwrCap = assoc_frame->power_capability();
        if (!pwrCap) {
            MAPF_ERR("no power capability field.");
            errors++;
        } else {
            errors += check_field<uint8_t>(pwrCap->min_tx_power(), 0, "min_tx_power");
            errors += check_field<uint8_t>(pwrCap->max_tx_power(), 20, "max_tx_power");
        }

        /*
         * 30 14 0100 000FAC040100000FAC040100000FAC020000
         * => ID_RSN version 1
         */
        auto rsn = assoc_frame->getAttr<assoc_frame::cRSN>();
        if (!rsn) {
            MAPF_ERR("no Rsn field.");
            errors++;
        } else {
            errors += check_field<uint16_t>(rsn->version(), 1, "version");
        }

        /*
         * 32 04 3048606C
         * => ID_EXTENDED_SUP_RATES 24 36 48 54 mbps
         */
        auto extSuppRates = assoc_frame->getAttr<assoc_frame::cExtendedSupportRates>();
        if (!extSuppRates) {
            MAPF_ERR("no extended supported rates field.");
            errors++;
        } else {
            std::vector<uint8_t> values = {
                0x30,
                0x48,
                0x60,
                0x6c,
            };
            std::vector<uint8_t> rates(extSuppRates->extended_suport_rated(),
                                       extSuppRates->extended_suport_rated() +
                                           extSuppRates->length());
            errors += check_field<std::vector<uint8_t>>(rates, values, "extended_supported_rates");
        }

        /*
         * check 1st:
         * 3B 10 515153547374757677787C7D7E7F8082
         * => ID_SUP_OP_CLASSES current 81 supp 81 83 84 115 116 117 118 119 120 124 125 126 127 128 130
         * skip 2nd:
         * 3B 16 0C01020304050C161718191A1B1C1D1E1F2021808182
         * => ID_SUP_OP_CLASSES ?
         *
         */
        auto suppOperClasses = assoc_frame->getAttr<assoc_frame::cSupportedOpClasses>();
        if (!suppOperClasses) {
            MAPF_ERR("no supported operating classes field.");
            errors++;
        } else {
            errors +=
                check_field<uint8_t>(suppOperClasses->current_op_class(), 0x51, "current_op_class");
            std::vector<uint8_t> values = {
                0x51, 0x53, 0x54, 0x73, 0x74, 0x75, 0x76, 0x77,
                0x78, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x82,
            };
            std::vector<uint8_t> operClasses(suppOperClasses->op_classes(),
                                             suppOperClasses->op_classes() +
                                                 suppOperClasses->op_classes_length());
            errors += check_field<std::vector<uint8_t>>(operClasses, values, "op_classes");
        }

        /*
         * check 1st:
         * 46 05 7000000000
         * => ID_RM_ENABLED_CAPS: beacon (active/passive/table) meas,
         * skip 2nd:
         * 46 05 7150500004
         * => ID_RM_ENABLED_CAPS: beacon (active/passive/table) meas, link meas,
         */
        auto rmCaps = assoc_frame->rm_enabled_caps();
        if (!rmCaps) {
            MAPF_ERR("no radio measurement capability field.");
            errors++;
        } else {
            errors += check_field<uint8_t>(rmCaps->data1().beacon_passive_measure, 1,
                                           "beacon_passive_measure");
            errors += check_field<uint8_t>(rmCaps->data1().beacon_active_measure, 1,
                                           "beacon_active_measure");
            errors += check_field<uint8_t>(rmCaps->data1().beacon_table_measure, 1,
                                           "beacon_table_measure");
        }

        /*
         * 7F 0A 04000A82214000408000
         * => ID_EXTENDED_CAPABILITY: wnm-sleep-mode bss_transition
         */
        auto extCap = assoc_frame->getAttr<assoc_frame::cExtendedCap>();
        if (!extCap) {
            MAPF_ERR("no extended capability field.");
            errors++;
        } else {
            std::vector<uint8_t> values = {
                0x04, 0x00, 0x0a, 0x82, 0x21, 0x40, 0x00, 0x40, 0x80, 0x00,
            };
            std::vector<uint8_t> caps(extCap->extended_cap_field(),
                                      extCap->extended_cap_field() +
                                          extCap->extended_cap_field_length());
            errors += check_field<std::vector<uint8_t>>(caps, values, "extended_cap_field");
        }

        /*
         * check 2 VendorSpecific IEs
         */
        auto vs_list = assoc_frame->getClassList<assoc_frame::cVendorSpecific>();
        errors += check_field<uint8_t>(vs_list.size(), 2, "number of vendor specific fields");

        /*
         * check 1st VS
         * DD 07 0050F202000100
         * => ID_VENDOR_SPECIFIC MS.Corp WMM/WME
         */
        auto vs1 = assoc_frame->getAttr<assoc_frame::cVendorSpecific>();
        if (!vs1) {
            MAPF_ERR("no Vendor Specific 1 field.");
            errors++;
        } else {
            std::vector<uint8_t> values = {
                0x00,
                0x50,
                0xf2,
            };
            std::vector<uint8_t> oui(vs1->oui(), vs1->oui() + 3);
            errors += check_field<std::vector<uint8_t>>(oui, values, "vendor specific MS.Corp");
            errors += check_field<uint8_t>(vs1->oui_type(), 0x2, "oui type WMM/WME");
        }

        /*
         * 2D 1A 2D11 03 FFFF0000000000000000000000000000 0000 18E6E109 00
         * => ID_HT_CAPABILITY: capinfo 112d sm_power disable, only 20mhz(+shortGI), nss_2, mcs 0-15
         */
        auto htcap = assoc_frame->sta_ht_capability();
        if (!htcap) {
            MAPF_ERR("no HT capability field.");
            errors++;
        } else {
            errors += check_field<uint8_t>(htcap->ht_cap_info().support_ch_width_set, 0,
                                           "support_ch_width_set");
            errors += check_field<uint8_t>(htcap->ht_cap_info().short_gi20mhz, 1, "short_gi20mhz");
            std::vector<uint8_t> values = {
                0xff,
                0xff,
                0x00,
                0x00,
            };
            std::vector<uint8_t> mcs_rx_1_4(htcap->ht_mcs_set(), htcap->ht_mcs_set() + 4);
            errors += check_field<std::vector<uint8_t>>(mcs_rx_1_4, values, "mcs_rx_1_4");
            errors += check_field<uint32_t>(htcap->tx_beamforming_caps(), 0x09E1E618,
                                            "tx_beamforming_caps");
        }

        /*
         * BF 0C B079D133 FAFF 0C03 FAFF 0C03
         * => ID_VHT_CAPS vht_cap_info:33d179b0 tx_stbc, 80+shortGI, rx_ldpc, su bfr/bfe, nss 2, mcs 0-9
         */
        auto vhtcap = assoc_frame->sta_vht_capability();
        if (!vhtcap) {
            MAPF_ERR("no VHT capability field.");
            errors++;
        } else {
            errors += check_field<uint8_t>(vhtcap->vht_cap_info().support_ch_width_set, 0,
                                           "support_ch_width_set");
            errors += check_field<uint8_t>(vhtcap->vht_cap_info().short_gi80mhz_tvht_mode4c, 1,
                                           "short_gi80mhz_tvht_mode4c");
            errors +=
                check_field<uint8_t>(vhtcap->vht_cap_info().su_beamformer, 1, "su_beamformer");
            errors +=
                check_field<uint8_t>(vhtcap->vht_cap_info().su_beamformee, 1, "su_beamformee");
            errors +=
                check_field<uint16_t>(vhtcap->supported_vht_mcs().rx_mcs_map, 0xfffa, "rx_mcs_map");
            errors +=
                check_field<uint16_t>(vhtcap->supported_vht_mcs().tx_mcs_map, 0xfffa, "tx_mcs_map");
        }

        /*
         * C7 01 10
         * => ID_OP_MODE_NOTIFICATION op_mode 16
         */
        auto opModeNotif = assoc_frame->getAttr<assoc_frame::cOperatingModeNotify>();
        if (!opModeNotif) {
            MAPF_ERR("no operating mode notification field.");
            errors++;
        } else {
            errors += check_field<uint8_t>(opModeNotif->op_mode(), 0x10, "op_mode");
        }

        /*
         * check 2nd VS
         * DD 07 506F9A16030103
         * => ID_VENDOR_SPECIFIC Wifi-alliance
         */
        auto vs2 = assoc_frame->getClass<assoc_frame::cVendorSpecific>(1);
        if (!vs2) {
            MAPF_ERR("no Vendor Specific 2 field.");
            errors++;
        } else {
            std::vector<uint8_t> values = {
                0x50,
                0x6f,
                0x9a,
            };
            std::vector<uint8_t> oui(vs2->oui(), vs2->oui() + 3);
            errors +=
                check_field<std::vector<uint8_t>>(oui, values, "vendor specific Wifi-alliance");
        }
    }

    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

int test_parse_reassoc_frame()
{
    int errors = 0;
    MAPF_INFO(__FUNCTION__ << " start");

    /*
     * frame raw buffer in network byte order
     */
    std::vector<uint8_t> reassoc_req_frame_body_buffer = {
        0x11, 0x11, 0x0a, 0x00, 0x08, 0xbe, 0xac, 0x1b, 0xc6, 0xa2, 0x00, 0x08, 0x70, 0x72,
        0x70, 0x6c, 0x6d, 0x65, 0x73, 0x68, 0x01, 0x08, 0x8c, 0x92, 0x98, 0xa4, 0xb0, 0xc8,
        0xe0, 0xec, 0x21, 0x02, 0x0a, 0x14, 0x24, 0x0a, 0x24, 0x04, 0x34, 0x04, 0x64, 0x0b,
        0x95, 0x04, 0xa5, 0x01, 0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00,
        0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x8c, 0x00, 0x46, 0x05,
        0x73, 0x08, 0x01, 0x00, 0x00, 0x3b, 0x15, 0x73, 0x70, 0x73, 0x74, 0x75, 0x7c, 0x7d,
        0x7e, 0x7f, 0x80, 0x81, 0x82, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x51, 0x53, 0x54,
        0x2d, 0x1a, 0x6f, 0x00, 0x1b, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x7f, 0x0a, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x20, 0xbf, 0x0c,
        0x32, 0x78, 0x91, 0x0f, 0xfa, 0xff, 0x00, 0x00, 0xfa, 0xff, 0x00, 0x00, 0xc7, 0x01,
        0x10, 0xdd, 0x0b, 0x00, 0x00, 0xf0, 0x22, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x07,
        0xdd, 0x05, 0x00, 0x90, 0x4c, 0x04, 0x17, 0xdd, 0x0a, 0x00, 0x10, 0x18, 0x02, 0x00,
        0x00, 0x10, 0x00, 0x00, 0x00, 0xdd, 0x07, 0x00, 0x50, 0xf2, 0x02, 0x00, 0x01, 0x00};

    auto reassoc_frame = assoc_frame::AssocReqFrame::parse(reassoc_req_frame_body_buffer.data(),
                                                           reassoc_req_frame_body_buffer.size());
    if (!reassoc_frame) {
        MAPF_ERR("Failed to parse reassociation frame.");
        errors++;
    } else {

        /*
         * Frame Body:
         * 1111 => capability info
         * 0a00 => listen interval
         */
        auto capInfoDmgStaField = reassoc_frame->getAttr<assoc_frame::cCapInfoDmgSta>();
        if (!capInfoDmgStaField) {
            MAPF_ERR("no capInfo field.");
            errors++;
        } else {
            errors += check_field<uint8_t>(capInfoDmgStaField->cap_info().radio_measurement, 1,
                                           "radio_measurement");
        }

        /*
         * 08 be ac 1b c6 a2
         * => currenBSSID:  08:be:ac:1b:c6:a2
         */
        auto currentApAddress = reassoc_frame->getAttr<assoc_frame::cCurrentApAddress>();
        if (!currentApAddress) {
            MAPF_ERR("no currentAP field.");
            errors++;
        } else {
            std::vector<uint8_t> bssid = {0x08, 0xbe, 0xac, 0x1b, 0xc6, 0xa2};
            std::vector<uint8_t> currBssid(currentApAddress->ap_addr().oct,
                                           currentApAddress->ap_addr().oct + 6);
            errors += check_field<std::vector<uint8_t>>(bssid, currBssid, "currentBSSID");
        }

        /*
         * 00 08 7072706c6d657368
         * => ID_SSID "prplmesh"
         */
        errors += check_field<std::string>(reassoc_frame->sta_ssid(), "prplmesh", "ssid");

        /*
         * 2d 1a 6f00 1b ffff0000000000000000000000000000 0000 00000000 00
         * => ID_HT_CAPABILITY: capinfo 006f sm_power disable, 20mhz(+shortGI),
         *                      40mhz (+shortGI), nss_2, mcs 0-15
         */
        auto htcap = reassoc_frame->sta_ht_capability();
        if (!htcap) {
            MAPF_ERR("no HT capability field.");
            errors++;
        } else {
            errors += check_field<uint8_t>(htcap->ht_cap_info().support_ch_width_set, 1,
                                           "support_ch_width_set");
            errors += check_field<uint8_t>(htcap->ht_cap_info().short_gi20mhz, 1, "short_gi20mhz");
            errors += check_field<uint8_t>(htcap->ht_cap_info().short_gi40mhz, 1, "short_gi40mhz");
            std::vector<uint8_t> values = {
                0xff,
                0xff,
                0x00,
                0x00,
            };
            std::vector<uint8_t> mcs_rx_1_4(htcap->ht_mcs_set(), htcap->ht_mcs_set() + 4);
            errors += check_field<std::vector<uint8_t>>(mcs_rx_1_4, values, "mcs_rx_1_4");
        }
    }
    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

int test_build_assoc_frame()
{
    int errors = 0;
    MAPF_INFO(__FUNCTION__ << " start");

    std::array<uint8_t, 2048> assoc_frame_body = {};

    auto fields = std::make_shared<assoc_frame::AssocReqFrame>(
        assoc_frame_body.data(), assoc_frame_body.size(),
        assoc_frame::AssocReqFrame::ASSOCIATION_REQUEST, false);
    if (!fields) {
        MAPF_ERR("Failed to contruct assoc frame obj.");
        errors++;
    } else {

        /*
         * Frame Body:
         * 0000 => capability info
         * 1400 => listen interval
         */
        auto capInfoDmgStaField = fields->addAttr<assoc_frame::cCapInfoDmgSta>();
        if (!capInfoDmgStaField) {
            MAPF_ERR("Failed to add capInfo field.");
            errors++;
        } else {
            capInfoDmgStaField->listen_interval() = 0x14;
        }

        /*
         * 00 0E 4D756C74692D41502D3234472D31
         * => ID_SSID "Multi-AP-24G-1"
         */
        auto ssidField = fields->addAttr<assoc_frame::cSSID>();
        if (!ssidField) {
            MAPF_ERR("Failed to add ssid field.");
            errors++;
        } else {
            std::string ssid("Multi-AP-24G-1");
            if (!ssidField->set_ssid(ssid)) {
                MAPF_ERR("Failed to fill ssid field.");
                errors++;
            }
            errors += check_field<uint8_t>(ssidField->length(), ssid.length(), "ssid_length");
        }

        /*
         * 01 08 02040B0C12161824
         * => ID_SUPPORT_RATES: 1 2 5.5 6 9 11 12 18 mbps
         */
        auto suppRatesField = fields->addAttr<assoc_frame::cSupportRates>();
        if (!suppRatesField) {
            MAPF_ERR("Failed to add supported rates field.");
            errors++;
        } else {
            const std::vector<uint8_t> values = {
                0x02, 0x04, 0x0b, 0x0c, 0x12, 0x16, 0x18, 0x24,
            };
            if (!suppRatesField->set_supported_rated(static_cast<const void *>(values.data()),
                                                     values.size())) {
                MAPF_ERR("Failed to fill supported rate list.");
                errors++;
            }
        }

        /*
         * 21 02 0014
         * => ID_POWER_CAPABILITY tx_pwr max: 20
         */
        auto pwrCapField = fields->addAttr<assoc_frame::cPowerCapability>();
        if (!pwrCapField) {
            MAPF_ERR("Failed to add power capability field.");
            errors++;
        } else {
            pwrCapField->max_tx_power() = 20;
        }

        /*
         * 32 04 3048606C
         * => ID_EXTENDED_SUP_RATES 24 36 48 54 mbps
         */
        auto extSuppRatesField = fields->addAttr<assoc_frame::cExtendedSupportRates>();
        if (!extSuppRatesField) {
            MAPF_ERR("Failed to add extended supported rates field.");
            errors++;
        } else {
            const std::vector<uint8_t> values = {
                0x30,
                0x48,
                0x60,
                0x6c,
            };
            if (!extSuppRatesField->set_extended_suport_rated(
                    static_cast<const void *>(values.data()), values.size())) {
                MAPF_ERR("Failed to fill extended supported rate list.");
                errors++;
            }
        }

        /*
         * 3B 10 515153547374757677787C7D7E7F8082
         * => ID_SUP_OP_CLASSES current 81 supp 81 83 84 115 116 117 118 119 120 124 125 126 127 128 130
         */
        auto suppOpClassField = fields->addAttr<assoc_frame::cSupportedOpClasses>();
        if (!suppOpClassField) {
            MAPF_ERR("Failed to add supported operating classes field.");
            errors++;
        } else {
            suppOpClassField->current_op_class() = 81;
            const std::vector<uint8_t> values    = {
                0x51, 0x53, 0x54, 0x73, 0x74, 0x75, 0x76, 0x77,
                0x78, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x82,
            };
            if (!suppOpClassField->set_op_classes(static_cast<const void *>(values.data()),
                                                  values.size())) {
                MAPF_ERR("Failed to fill supported operating class list.");
                errors++;
            }
        }

        /*
         * 46 05 7000000000
         * => ID_RM_ENABLED_CAPS: beacon (active/passive/table) meas,
         */
        auto RmCapsField = fields->addAttr<assoc_frame::cRmEnabledCaps>();
        if (!RmCapsField) {
            MAPF_ERR("Failed to add RM enabled capability field.");
            errors++;
        } else {
            RmCapsField->data1().beacon_passive_measure = 1;
            RmCapsField->data1().beacon_active_measure  = 1;
            RmCapsField->data1().beacon_table_measure   = 1;
        }

        /*
         * 7F 0A 04000A82214000408000
         * => ID_EXTENDED_CAPABILITY: wnm-sleep-mode bss_transition
         */
        auto extCapField = fields->addAttr<assoc_frame::cExtendedCap>();
        if (!extCapField) {
            MAPF_ERR("Failed to add extended capability field.");
            errors++;
        } else {
            const std::vector<uint8_t> values = {
                0x04, 0x00, 0x0a, 0x82, 0x21, 0x40, 0x00, 0x40, 0x80, 0x00,
            };
            if (!extCapField->set_extended_cap_field(static_cast<const void *>(values.data()),
                                                     values.size())) {
                MAPF_ERR("Failed to fill extended caps.");
                errors++;
            }
        }

        /*
         * 2D 1A 6F00 1B FFFF0000000000000000000000000000 0000 18E6E109 00
         * => ID_HT_CAPABILITY: capinfo 006f sm_power disable, htLdpc, 20/40mhz(+shortGI), nss_2, mcs 0-15
         */
        auto htCapField = fields->addAttr<assoc_frame::cStaHtCapability>();
        if (!htCapField) {
            MAPF_ERR("Failed to add HT capability field.");
            errors++;
        } else {
            htCapField->ht_cap_info().ldcp_coding_capability  = 1;
            htCapField->ht_cap_info().support_ch_width_set    = 1;
            htCapField->ht_cap_info().sm_power_save           = 0x3; //disabled
            htCapField->ht_cap_info().short_gi20mhz           = 1;
            htCapField->ht_cap_info().short_gi40mhz           = 1;
            htCapField->a_mpdu_param().max_ampdu_len_expo     = 0x3; //65535 bytes
            htCapField->a_mpdu_param().min_mpdu_start_spacing = 0x6; //8 usec
            const std::vector<uint8_t> mcs_values             = {
                0xff,
                0xff,
                0x00,
                0x00,
            };
            if (!htCapField->set_ht_mcs_set(static_cast<const void *>(mcs_values.data()),
                                            mcs_values.size())) {
                MAPF_ERR("Failed to add HT MCS 1-4 ss field.");
                errors++;
            }
            htCapField->tx_beamforming_caps() = 0x09E1E618;
        }

        /*
         * BF 0C 3278910F FAFF 0000 FAFF 0000
         * => ID_VHT_CAPS vht_cap_info:0F917832 rx_ldpc, 80+shortGI, rx_ldpc, su bfr/bfe, mu bfe, nss 2, mcs 0-9
         */
        auto vhtCapField = fields->addAttr<assoc_frame::cStaVhtCapability>();
        if (!vhtCapField) {
            MAPF_ERR("Failed to add VHT capability field.");
            errors++;
        } else {
            vhtCapField->vht_cap_info().max_mpdu_len              = 0x2; //11454
            vhtCapField->vht_cap_info().support_ch_width_set      = 0x0; //neither 160 nor 80p80
            vhtCapField->vht_cap_info().rx_ldpc                   = 1;
            vhtCapField->vht_cap_info().short_gi80mhz_tvht_mode4c = 1;
            vhtCapField->vht_cap_info().su_beamformer             = 1;
            vhtCapField->vht_cap_info().su_beamformee             = 1;
            vhtCapField->vht_cap_info().beamformee_sts            = 0x3; //4
            vhtCapField->vht_cap_info().sound_dimensions          = 0x1; //2
            vhtCapField->vht_cap_info().mu_beamformee             = 1;
            vhtCapField->vht_cap_info().max_a_mpdu_len            = 0x7; //1048575
            vhtCapField->vht_cap_info().vht_link_adaptation =
                0x3; //Both: unsolic.feedback & VHT MRQ

            // check conversion between bitmap struct and raw value
            auto raw_value = assoc_frame::convert<assoc_frame::sStaVhtCapInfo, uint32_t>(
                vhtCapField->vht_cap_info());
            errors += check_field<uint32_t>(raw_value, 0x0F917832, "vht_cap_info.raw");

            vhtCapField->supported_vht_mcs().rx_mcs_map = 0xfffa; //nss 2, mcs 0-9
            vhtCapField->supported_vht_mcs().tx_mcs_map = 0xfffa; //nss 2, mcs 0-9
        }

        fields->finalize();

        /*
         * frame raw buffer in network byte order
         */
        std::vector<uint8_t> frame_body_ref = {
            0x00, 0x00, 0x14, 0x00, 0x00, 0x0E, 0x4D, 0x75, 0x6C, 0x74, 0x69, 0x2D, 0x41, 0x50,
            0x2D, 0x32, 0x34, 0x47, 0x2D, 0x31, 0x01, 0x08, 0x02, 0x04, 0x0B, 0x0C, 0x12, 0x16,
            0x18, 0x24, 0x21, 0x02, 0x00, 0x14, 0x32, 0x04, 0x30, 0x48, 0x60, 0x6C, 0x3B, 0x10,
            0x51, 0x51, 0x53, 0x54, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x7C, 0x7D, 0x7E, 0x7F,
            0x80, 0x82, 0x46, 0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x0A, 0x04, 0x00, 0x0A,
            0x82, 0x21, 0x40, 0x00, 0x40, 0x80, 0x00, 0x2D, 0x1A, 0x6F, 0x00, 0x1B, 0xFF, 0xFF,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x18, 0xE6, 0xE1, 0x09, 0x00, 0xBF, 0x0C, 0x32, 0x78, 0x91, 0x0F, 0xFA,
            0xFF, 0x00, 0x00, 0xFA, 0xFF, 0x00, 0x00};

        std::vector<uint8_t> frame_body_result(
            fields->getMessageBuff(), fields->getMessageBuff() + fields->getMessageLength());

        errors += check_field<std::vector<uint8_t>>(frame_body_result, frame_body_ref,
                                                    "assoc_req_frame_body");
    }

    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}

int main(int argc, char *argv[])
{
    int errors = 0;
    mapf::Logger::Instance();
    MAPF_INFO(__FUNCTION__ << " Starting tests");
    errors += test_int_len_list();
    errors += test_complex_list();
    errors += test_all();
    errors += test_parser();
    errors += test_mac_from_string();
    errors += test_conditional_parameters_rx_tx();
    errors += test_channel_scan_results();
    errors += test_parse_assoc_frame();
    errors += test_build_assoc_frame();
    errors += test_parse_reassoc_frame();
    errors += test_create_error_response_message();
    MAPF_INFO(__FUNCTION__ << " Finished, errors = " << errors << std::endl);
    return errors;
}
