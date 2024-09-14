/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

/**
 * This file uses code copied from `iw` (http://git.sipsolutions.net/iw.git/)
 *
 * Copyright (c) 2007, 2008 Johannes Berg
 * Copyright (c) 2007    Andy Lutomirski
 * Copyright (c) 2007    Mike Kershaw
 * Copyright (c) 2008-2009   Luis R. Rodriguez
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "capability_reporting_task.h"
#include "../backhaul_manager/backhaul_manager.h"
#include "../tlvf_utils.h"
#include <tlvf/wfa_map/tlvApCapability.h>
#include <tlvf/wfa_map/tlvApHeCapabilities.h>
#include <tlvf/wfa_map/tlvApHtCapabilities.h>
#include <tlvf/wfa_map/tlvApVhtCapabilities.h>
#include <tlvf/wfa_map/tlvChannelScanCapabilities.h>
#include <tlvf/wfa_map/tlvClientCapabilityReport.h>
#include <tlvf/wfa_map/tlvClientInfo.h>
#include <tlvf/wfa_map/tlvDeviceInventory.h>
#include <tlvf/wfa_map/tlvProfile2ApCapability.h>
#include <tlvf/wfa_map/tlvProfile2CacCapabilities.h>
#include <tlvf/wfa_map/tlvProfile2MetricCollectionInterval.h>

namespace beerocks {

CapabilityReportingTask::CapabilityReportingTask(BackhaulManager &btl_ctx,
                                                 ieee1905_1::CmduMessageTx &cmdu_tx)
    : Task(eTaskType::CAPABILITY_REPORTING), m_btl_ctx(btl_ctx), m_cmdu_tx(cmdu_tx)
{
}

bool CapabilityReportingTask::handle_cmdu(ieee1905_1::CmduMessageRx &cmdu_rx, uint32_t iface_index,
                                          const sMacAddr &dst_mac, const sMacAddr &src_mac, int fd,
                                          std::shared_ptr<beerocks_header> beerocks_header)
{
    switch (cmdu_rx.getMessageType()) {
    case ieee1905_1::eMessageType::CLIENT_CAPABILITY_QUERY_MESSAGE: {
        handle_client_capability_query(cmdu_rx, src_mac);
        break;
    }
    case ieee1905_1::eMessageType::AP_CAPABILITY_QUERY_MESSAGE: {
        handle_ap_capability_query(cmdu_rx, src_mac);
        break;
    }
    default: {
        // Message was not handled, therefore return false.
        return false;
    }
    }
    return true;
}

void CapabilityReportingTask::handle_client_capability_query(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                             const sMacAddr &src_mac)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received CLIENT_CAPABILITY_QUERY_MESSAGE , mid=" << std::hex << mid;

    auto client_info_tlv_r = cmdu_rx.getClass<wfa_map::tlvClientInfo>();
    if (!client_info_tlv_r) {
        LOG(ERROR) << "getClass wfa_map::tlvClientInfo failed";
        return;
    }

    // send CLIENT_CAPABILITY_REPORT_MESSAGE back to the controller
    if (!m_cmdu_tx.create(mid, ieee1905_1::eMessageType::CLIENT_CAPABILITY_REPORT_MESSAGE)) {
        LOG(ERROR) << "cmdu creation of type CLIENT_CAPABILITY_REPORT_MESSAGE, has failed";
        return;
    }

    auto client_info_tlv_t = m_cmdu_tx.addClass<wfa_map::tlvClientInfo>();
    if (!client_info_tlv_t) {
        LOG(ERROR) << "addClass wfa_map::tlvClientInfo has failed";
        return;
    }
    client_info_tlv_t->bssid()      = client_info_tlv_r->bssid();
    client_info_tlv_t->client_mac() = client_info_tlv_r->client_mac();

    auto client_capability_report_tlv = m_cmdu_tx.addClass<wfa_map::tlvClientCapabilityReport>();
    if (!client_capability_report_tlv) {
        LOG(ERROR) << "addClass wfa_map::tlvClientCapabilityReport has failed";
        return;
    }

    auto db = AgentDB::get();

    // Check if it is an error scenario - if the STA specified in the Client Capability Query
    // message is not associated with any of the BSS operated by the Multi-AP Agent [ though the
    // TLV does contain a BSSID, the specification says that we should answer if the client is
    // associated with any BSS on this agent.]
    auto radio = db->get_radio_by_mac(client_info_tlv_r->client_mac(), AgentDB::eMacType::CLIENT);
    if (!radio) {
        LOG(ERROR) << "radio for client mac " << client_info_tlv_r->client_mac() << " not found";

        // If it is an error scenario, set Success status to 0x01 = Failure and do nothing after it.
        client_capability_report_tlv->result_code() = wfa_map::tlvClientCapabilityReport::FAILURE;

        LOG(DEBUG) << "Result Code: FAILURE";
        LOG(DEBUG) << "STA specified in the Client Capability Query message is not associated with "
                      "any of the BSS operated by the Multi-AP Agent ";
        // Add an Error Code TLV
        auto error_code_tlv = m_cmdu_tx.addClass<wfa_map::tlvErrorCode>();
        if (!error_code_tlv) {
            LOG(ERROR) << "addClass wfa_map::tlvErrorCode has failed";
            return;
        }
        error_code_tlv->reason_code() =
            wfa_map::tlvErrorCode::STA_NOT_ASSOCIATED_WITH_ANY_BSS_OPERATED_BY_THE_AGENT;
        error_code_tlv->sta_mac() = client_info_tlv_r->client_mac();
        m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, src_mac, db->bridge.mac);
        return;
    }

    client_capability_report_tlv->result_code() = wfa_map::tlvClientCapabilityReport::SUCCESS;
    LOG(DEBUG) << "Result Code: SUCCESS";

    // Add frame body of the most recently received (Re)Association Request frame from this client.
    auto &client_info = radio->associated_clients.at(client_info_tlv_r->client_mac());
    client_capability_report_tlv->set_association_frame(client_info.association_frame.data(),
                                                        client_info.association_frame_length);

    LOG(DEBUG) << "Send a CLIENT_CAPABILITY_REPORT_MESSAGE back to controller";
    m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, src_mac, db->bridge.mac);
}

void CapabilityReportingTask::handle_ap_capability_query(ieee1905_1::CmduMessageRx &cmdu_rx,
                                                         const sMacAddr &src_mac)
{
    const auto mid = cmdu_rx.getMessageId();
    LOG(DEBUG) << "Received AP_CAPABILITY_QUERY_MESSAGE, mid=" << std::hex << mid;

    if (!m_cmdu_tx.create(mid, ieee1905_1::eMessageType::AP_CAPABILITY_REPORT_MESSAGE)) {
        LOG(ERROR) << "cmdu creation of type AP_CAPABILITY_REPORT_MESSAGE, has failed";
        return;
    }

    auto ap_capability_tlv = m_cmdu_tx.addClass<wfa_map::tlvApCapability>();
    if (!ap_capability_tlv) {
        LOG(ERROR) << "addClass wfa_map::tlvApCapability has failed";
        return;
    }

    auto db = AgentDB::get();

    // 1. The tlvs created in the loop are created per radio and are
    // defined in the specification as "Zero Or More" (multi-ap specification v2, 17.1.7)
    for (const auto &radio_info : m_btl_ctx.m_radios_info) {
        // TODO skip slaves that are not operational
        auto &radio_mac = radio_info->radio_mac;

        auto radio = db->get_radio_by_mac(radio_mac);
        if (!radio) {
            LOG(ERROR) << "radio with mac " << radio_mac << " does not exist in the db";
            continue;
        }

        if (!tlvf_utils::add_ap_radio_basic_capabilities(m_cmdu_tx, radio_mac)) {
            return;
        }

        if (!add_ap_ht_capabilities(radio->front.iface_name)) {
            return;
        }

        if (!add_ap_vht_capabilities(radio->front.iface_name)) {
            return;
        }

        if (!add_ap_he_capabilities(radio->front.iface_name)) {
            return;
        }
    }

    // 2. The tlvs created here are defined in the
    // specification as "One" (multi-ap specification v2, 17.1.7).
    // the one tlv may contain information about few radios

    // 2.1 radio dependent tlvs
    // Add channel scan capabilities
    auto channel_scan_capabilities_tlv = m_cmdu_tx.addClass<wfa_map::tlvChannelScanCapabilities>();
    if (!channel_scan_capabilities_tlv) {
        LOG(ERROR) << "Error creating TLV_CHANNEL_SCAN_CAPABILITIES";
        return;
    }

    // Add Channel Scan Capabilities
    for (const auto &radio_info : m_btl_ctx.m_radios_info) {
        add_channel_scan_capabilities(radio_info->hostap_iface, *channel_scan_capabilities_tlv);
    }

    // 2.2 radio independent tlvs

    if (db->controller_info.profile_support >=
        wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_2) {

        // profile 2 ap capability
        if (!add_profile2_ap_capability_tlv(m_cmdu_tx)) {
            return;
        }

        // profile 2 metric collection interval
        // Note: at the moment we are not setting a value for collection_interval
        auto profile2_meteric_collection_interval_tlv =
            m_cmdu_tx.addClass<wfa_map::tlvProfile2MetricCollectionInterval>();
        if (!profile2_meteric_collection_interval_tlv) {
            LOG(ERROR) << "error creating TLV_PROFILE2_METERIC_COLLECTION_INTERVAL";
            return;
        }

        // 3. tlvs added by external sources
        if (!add_cac_capabilities_tlv()) {
            LOG(ERROR) << "error filling cac capabilities tlv";
            return;
        }
    }

    if (db->controller_info.profile_support >=
        wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_3) {

        if (!add_device_inventory_tlv()) {
            return;
        }
    }

    // send the constructed report
    LOG(DEBUG) << "Sending AP_CAPABILITY_REPORT_MESSAGE , mid: " << std::hex << mid;
    m_btl_ctx.send_cmdu_to_broker(m_cmdu_tx, src_mac, db->bridge.mac);
}

bool CapabilityReportingTask::add_ap_ht_capabilities(const std::string &iface_name)
{
    auto db    = AgentDB::get();
    auto radio = db->radio(iface_name);
    if (!radio) {
        return false;
    }

    if (!radio->ht_supported) {
        return true;
    }

    auto tlv = m_cmdu_tx.addClass<wfa_map::tlvApHtCapabilities>();
    if (!tlv) {
        LOG(ERROR) << "Error creating TLV_AP_HT_CAPABILITIES";
        return false;
    }

    tlv->radio_uid() = radio->front.iface_mac;
    struct beerocks::net::sHTCapabilities *HTCaps =
        (struct beerocks::net::sHTCapabilities *)(&radio->ht_capability);
    /**
     * See iw/util.c for details on how to compute fields.
     * Code has been preserved as close as possible to that in the iw command line tool.
     * AP HT Capabilities TLV Format : (Multi-AP Specification 1.0)
     * tlvValue :
     * Bits 7-6 : Maximum number of supported Tx spatial streams
     * Bits 5-4 : Maximum number of supported Rx spatial streams
     * Bit  3   : Short GI Support for 20 MHz.
     * Bit  2   : Short GI Support for 40 MHz.
     * Bit  1   : HT support for 40MHz
     * Bit  0   : Reserved
    */
    tlv->flags().max_num_of_supported_tx_spatial_streams =
        HTCaps->max_num_of_supported_tx_spatial_streams;
    tlv->flags().max_num_of_supported_rx_spatial_streams =
        HTCaps->max_num_of_supported_rx_spatial_streams;
    tlv->flags().short_gi_support_20mhz = HTCaps->short_gi_support_20mhz;
    tlv->flags().short_gi_support_40mhz = HTCaps->short_gi_support_40mhz;
    tlv->flags().ht_support_40mhz       = HTCaps->ht_support_40mhz;
    return true;
}

bool CapabilityReportingTask::add_ap_vht_capabilities(const std::string &iface_name)
{
    auto db    = AgentDB::get();
    auto radio = db->radio(iface_name);
    if (!radio) {
        return false;
    }

    if (!radio->vht_supported) {
        return true;
    }

    auto tlv = m_cmdu_tx.addClass<wfa_map::tlvApVhtCapabilities>();
    if (!tlv) {
        LOG(ERROR) << "Error creating TLV_AP_VHT_CAPABILITIES";
        return false;
    }

    tlv->radio_uid() = radio->front.iface_mac;
    struct beerocks::net::sVHTCapabilities *VHTCaps =
        (struct beerocks::net::sVHTCapabilities *)(&radio->vht_capability);
    /**
     * See iw/util.c for details on how to compute fields
     * Code has been preserved as close as possible to that in the iw command line tool.
     * AP VHT Capabilities TLV Format 16 bits: (Multi-AP Specification 3.0)
     * tlvValue (Flag1) :
     * Bits 15-13 : Maximum number of supported Tx spatial streams
     * Bits 12-10 : Maximum number of supported Rx spatial streams
     * Bit  9     : Short GI Support for 80 MHz.
     * Bit  8     : Short GI Support for 160Mhz and 80+80 MHz.
     * tlvValue (Flag2) :
     * Bit  7     : VHT support for 80+80 MHz.
     * Bit  6     : VHT support for 160 MHz.
     * Bit  5     : SU beamformer capable.
     * Bit  4     : MU beamformer capable.
     * Bit  3-0   : Reserved
     */
    tlv->supported_vht_tx_mcs() = radio->vht_mcs_set[0] | (radio->vht_mcs_set[1] << 8);
    tlv->supported_vht_rx_mcs() = radio->vht_mcs_set[2] | (radio->vht_mcs_set[3] << 8);
    tlv->flags1().max_num_of_supported_tx_spatial_streams =
        VHTCaps->max_num_of_supported_tx_spatial_streams;
    tlv->flags1().max_num_of_supported_rx_spatial_streams =
        VHTCaps->max_num_of_supported_rx_spatial_streams;
    tlv->flags1().short_gi_support_80mhz = VHTCaps->short_gi_support_80mhz;
    tlv->flags1().short_gi_support_160mhz_and_80_80mhz =
        VHTCaps->short_gi_support_160mhz_and_80_80mhz;
    tlv->flags2().vht_support_80_80mhz  = VHTCaps->vht_support_80_80mhz;
    tlv->flags2().vht_support_160mhz    = VHTCaps->vht_support_160mhz;
    tlv->flags2().su_beamformer_capable = VHTCaps->su_beamformer_capable;
    tlv->flags2().mu_beamformer_capable = VHTCaps->mu_beamformer_capable;

    return true;
}

bool CapabilityReportingTask::add_ap_he_capabilities(const std::string &iface_name)
{
    auto db    = AgentDB::get();
    auto radio = db->radio(iface_name);
    if (!radio) {
        return false;
    }

    if (!radio->he_supported) {
        return true;
    }

    auto tlv = m_cmdu_tx.addClass<wfa_map::tlvApHeCapabilities>();
    if (!tlv) {
        LOG(ERROR) << "Error creating TLV_AP_HE_CAPABILITIES";
        return false;
    }

    tlv->radio_uid() = radio->front.iface_mac;
    struct beerocks::net::sHECapabilities *HECaps =
        (struct beerocks::net::sHECapabilities *)(&radio->he_capability);
    /**
     * HE_MCS_SET[]: 13 Bytes with the first byte contains K-Bytes number
     * AP HE Capabilities TLV Format 16 bits: (Multi-AP Specification 1.0)
     * tlvValue (Flag1) :
     * Bits 15-13 : Maximum number of supported Tx spatial streams.
     * Bits 12-10 : Maximum number of supported Rx spatial streams.
     * Bit  9     : HE support for 80+80 MHz.
     * Bit  8     : HE support for 160 MHz..
     * tlvValue (Flag2) :
     * Bit  7     : SU beamformer capable.
     * Bit  6     : MU beamformer capable.
     * Bit  5     : UL MU-MIMO capable.
     * Bit  4     : UL MU-MIMO + OFDMA capable.
     * Bit  3     : DL MU-MIMO + OFDMA capable.
     * Bit  2     : UL OFDMA capable.
     * Bit  1     : DL OFDMA capable.
    */

    tlv->set_supported_he_mcs(&radio->he_mcs_set[1], radio->he_mcs_set[0]);
    tlv->flags1().max_num_of_supported_tx_spatial_streams =
        HECaps->max_num_of_supported_tx_spatial_streams;
    tlv->flags1().max_num_of_supported_rx_spatial_streams =
        HECaps->max_num_of_supported_rx_spatial_streams;
    tlv->flags1().he_support_80_80mhz         = HECaps->he_support_80_80mhz;
    tlv->flags1().he_support_160mhz           = HECaps->he_support_160mhz;
    tlv->flags2().dl_ofdm_capable             = HECaps->dl_ofdm_capable;
    tlv->flags2().ul_ofdm_capable             = HECaps->ul_ofdm_capable;
    tlv->flags2().dl_mu_mimo_and_ofdm_capable = HECaps->dl_mu_mimo_and_ofdm_capable;
    tlv->flags2().ul_mu_mimo_and_ofdm_capable = HECaps->ul_mu_mimo_and_ofdm_capable;
    tlv->flags2().ul_mu_mimo_capable          = HECaps->ul_mu_mimo_capable;
    tlv->flags2().mu_beamformer_capable       = HECaps->mu_beamformer_capable;
    tlv->flags2().su_beamformer_capable       = HECaps->su_beamformer_capable;

    return true;
}

bool CapabilityReportingTask::add_channel_scan_capabilities(
    const std::string &iface_name,
    wfa_map::tlvChannelScanCapabilities &channel_scan_capabilities_tlv)
{
    auto db    = AgentDB::get();
    auto radio = db->radio(iface_name);
    if (!radio) {
        return false;
    }

    auto radio_channel_scan_capabilities = channel_scan_capabilities_tlv.create_radio_list();
    if (!radio_channel_scan_capabilities) {
        LOG(ERROR) << "create_radio_list() has failed!";
        return false;
    }
    radio_channel_scan_capabilities->radio_uid() = radio->front.iface_mac;
    // We support "on demand" scans so set the on_boot_only flag to 0
    radio_channel_scan_capabilities->capabilities().on_boot_only = 0;
    // Time slicing impairment (Radio may go off channel for a series of short intervals)
    radio_channel_scan_capabilities->capabilities().scan_impact = wfa_map::
        cRadiosWithScanCapabilities::eScanImpact::SCAN_IMPACT_REDUCED_NUMBER_OF_SPATIAL_STREAM;

    // Create operating class object
    auto op_class_channels = radio_channel_scan_capabilities->create_operating_classes_list();
    if (!op_class_channels) {
        LOG(ERROR) << "create_operating_classes_list() has failed!";
        return false;
    }

    // Push operating class object to the list of operating class objects
    if (!channel_scan_capabilities_tlv.add_radio_list(radio_channel_scan_capabilities)) {
        LOG(ERROR) << "add_radio_list() has failed!";
        return false;
    }

    return true;
}

bool CapabilityReportingTask::add_cac_capabilities_tlv()
{
    auto cac_capabilities_tlv = m_cmdu_tx.addClass<wfa_map::tlvProfile2CacCapabilities>();
    if (!cac_capabilities_tlv) {
        LOG(ERROR) << "addClass wfa_map::tlvProfile2CacCapabilities has failed";
        return false;
    }

    // country code
    const auto &country_code               = m_cac_capabilities.get_country_code();
    *cac_capabilities_tlv->country_code(0) = country_code[0];
    *cac_capabilities_tlv->country_code(1) = country_code[1];

    // get all cac radios
    auto cac_radios = m_cac_capabilities.get_cac_radios();

    // fill in the tlv

    // for each radio
    for (const auto &radio : cac_radios) {
        // read cac methods for the radio
        auto cac_radio_methods = beerocks::get_radio_cac_methods(m_cac_capabilities, radio);

        // create tlv radios
        auto radios_tlv = cac_capabilities_tlv->create_cac_radios();
        if (!radios_tlv) {
            LOG(ERROR) << "unable to create cac radios";
            return false;
        }

        radios_tlv->radio_uid() = radio;

        // create cac type tlv for each CAC method
        for (const auto &cac_method : cac_radio_methods.second) {
            auto cac_type_tlv = radios_tlv->create_cac_types();
            if (!cac_type_tlv) {
                LOG(ERROR) << "unable to create cac types";
                return false;
            }
            cac_type_tlv->cac_method() = static_cast<wfa_map::eCacMethod>(cac_method);

            uint32_t duration = m_cac_capabilities.get_cac_completion_duration(radio, cac_method);
            memcpy(cac_type_tlv->duration(), &duration, 3);

            // operating classes
            const CacCapabilities::CacOperatingClasses &cac_operating_classes =
                m_cac_capabilities.get_cac_operating_classes(radio, cac_method);

            // for each {operating-class,[channels]}
            for (auto &operating_class_channels : cac_operating_classes) {
                auto operating_classes_tlv = cac_type_tlv->create_operating_classes();
                if (!operating_classes_tlv) {
                    LOG(ERROR) << "unable to create cac operating classes";
                    return false;
                }
                operating_classes_tlv->operating_class() = operating_class_channels.first;
                auto channels_tlv =
                    operating_classes_tlv->alloc_channels(operating_class_channels.second.size());
                if (!channels_tlv) {
                    LOG(ERROR) << "unable to create cac channles";
                    return false;
                }
                for (size_t i = 0; i < operating_class_channels.second.size(); ++i) {
                    *operating_classes_tlv->channels(i) = operating_class_channels.second[i];
                }

                // add to cac type
                if (!cac_type_tlv->add_operating_classes(operating_classes_tlv)) {
                    LOG(ERROR) << "Failed adding operating classes to CAC type TLV";
                    return false;
                }
            }
            if (!radios_tlv->add_cac_types(cac_type_tlv)) {
                LOG(ERROR) << "Failed adding CAC types to CAC radios tlv";
                return false;
            }
        }
        // add the cac type back to the radios tlv
        if (!cac_capabilities_tlv->add_cac_radios(radios_tlv)) {
            LOG(ERROR) << "Failed adding CAC radios to CAC capabilities TLV";
            return false;
        }
    }
    return true;
}

bool CapabilityReportingTask::add_device_inventory_tlv()
{
    auto device_inventory_tlv = m_cmdu_tx.addClass<wfa_map::tlvDeviceInventory>();
    if (!device_inventory_tlv) {
        LOG(ERROR) << "Failed building message!";
        return false;
    }
    auto db = AgentDB::get();

    device_inventory_tlv->set_serial_number(db->device_conf.device_serial_number);
    device_inventory_tlv->set_software_version(db->device_conf.software_version);
    device_inventory_tlv->set_execution_environment(db->device_conf.operating_system);

    for (auto radio : db->get_radios_list()) {
        auto radio_vendor    = device_inventory_tlv->create_radios_vendor_info();
        radio_vendor->ruid() = radio->front.iface_mac;
        radio_vendor->set_chipset_vendor(radio->chipset_vendor);
        device_inventory_tlv->add_radios_vendor_info(radio_vendor);
    }

    return true;
}

bool CapabilityReportingTask::add_profile2_ap_capability_tlv(ieee1905_1::CmduMessageTx &cmdu_tx)
{
    auto profile2_ap_capability_tlv = cmdu_tx.addClass<wfa_map::tlvProfile2ApCapability>();
    if (!profile2_ap_capability_tlv) {
        LOG(ERROR) << "Failed building message!";
        return false;
    }

    auto db = AgentDB::get();
    if (db->controller_info.profile_support ==
        wfa_map::tlvProfile2MultiApProfile::eMultiApProfile::MULTIAP_PROFILE_1) {
        // If the Multi-AP Agent onboards to a Multi-AP Controller that implements Profile-1, the
        // Multi-AP Agent shall set the Byte Counter Units field to 0x00 (bytes) and report the
        // values of the BytesSent and BytesReceived fields in the Associated STA Traffic Stats TLV
        // in bytes. Section 9.1 of the spec.
        db->device_conf.byte_counter_units =
            wfa_map::tlvProfile2ApCapability::eByteCounterUnits::BYTES;
    } else {
        // If a Multi-AP Agent that implements Profile-2 sends a Profile-2 AP Capability TLV
        // shall set the Byte Counter Units field to 0x01 (KiB (kibibytes)). Section 9.1 of the spec.
        db->device_conf.byte_counter_units =
            wfa_map::tlvProfile2ApCapability::eByteCounterUnits::KIBIBYTES;
    }

    profile2_ap_capability_tlv->capabilities_bit_field().byte_counter_units =
        db->device_conf.byte_counter_units;

    // Calculate max total number of VLANs which can be configured on the Agent, and save it on
    // on the AgentDB.
    db->traffic_separation.max_number_of_vlans_ids =
        db->get_radios_list().size() * eBeeRocksIfaceIds::IFACE_TOTAL_VAPS;

    profile2_ap_capability_tlv->max_total_number_of_vids() =
        db->traffic_separation.max_number_of_vlans_ids;

    profile2_ap_capability_tlv->max_prioritization_rules() =
        db->device_conf.max_prioritization_rules;
    profile2_ap_capability_tlv->capabilities_bit_field().prioritization =
        db->device_conf.max_prioritization_rules > 0;
    return true;
}

} // namespace beerocks
