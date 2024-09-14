/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2016-2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <tlvf/AttrList.h>
#include <tlvf/CmduMessageRx.h>
#include <tlvf/ieee_1905_1/eTlvType.h>
#include <tlvf/ieee_1905_1/tlv1905NeighborDevice.h>
#include <tlvf/ieee_1905_1/tlvAlMacAddress.h>
#include <tlvf/ieee_1905_1/tlvAutoconfigFreqBand.h>
#include <tlvf/ieee_1905_1/tlvDeviceBridgingCapability.h>
#include <tlvf/ieee_1905_1/tlvDeviceInformation.h>
#include <tlvf/ieee_1905_1/tlvEndOfMessage.h>
#include <tlvf/ieee_1905_1/tlvLinkMetricQuery.h>
#include <tlvf/ieee_1905_1/tlvLinkMetricResultCode.h>
#include <tlvf/ieee_1905_1/tlvMacAddress.h>
#include <tlvf/ieee_1905_1/tlvNon1905neighborDeviceList.h>
#include <tlvf/ieee_1905_1/tlvPushButtonEventNotification.h>
#include <tlvf/ieee_1905_1/tlvPushButtonJoinNotification.h>
#include <tlvf/ieee_1905_1/tlvReceiverLinkMetric.h>
#include <tlvf/ieee_1905_1/tlvSearchedRole.h>
#include <tlvf/ieee_1905_1/tlvSupportedFreqBand.h>
#include <tlvf/ieee_1905_1/tlvSupportedRole.h>
#include <tlvf/ieee_1905_1/tlvTransmitterLinkMetric.h>
#include <tlvf/ieee_1905_1/tlvUnknown.h>
#include <tlvf/ieee_1905_1/tlvVendorSpecific.h>
#include <tlvf/ieee_1905_1/tlvWsc.h>
#include <tlvf/wfa_map/eTlvTypeMap.h>
#include <tlvf/wfa_map/tlvApCapability.h>
#include <tlvf/wfa_map/tlvApExtendedMetrics.h>
#include <tlvf/wfa_map/tlvApHeCapabilities.h>
#include <tlvf/wfa_map/tlvApHtCapabilities.h>
#include <tlvf/wfa_map/tlvApMetricQuery.h>
#include <tlvf/wfa_map/tlvApMetrics.h>
#include <tlvf/wfa_map/tlvApOperationalBSS.h>
#include <tlvf/wfa_map/tlvApRadioBasicCapabilities.h>
#include <tlvf/wfa_map/tlvApRadioIdentifier.h>
#include <tlvf/wfa_map/tlvApVhtCapabilities.h>
#include <tlvf/wfa_map/tlvAssociatedClients.h>
#include <tlvf/wfa_map/tlvAssociatedStaExtendedLinkMetrics.h>
#include <tlvf/wfa_map/tlvAssociatedStaLinkMetrics.h>
#include <tlvf/wfa_map/tlvAssociatedStaTrafficStats.h>
#include <tlvf/wfa_map/tlvAssociatedWiFi6StaStatusReport.h>
#include <tlvf/wfa_map/tlvBackhaulBssConfiguration.h>
#include <tlvf/wfa_map/tlvBackhaulStaRadioCapabilities.h>
#include <tlvf/wfa_map/tlvBackhaulSteeringRequest.h>
#include <tlvf/wfa_map/tlvBackhaulSteeringResponse.h>
#include <tlvf/wfa_map/tlvBeaconMetricsQuery.h>
#include <tlvf/wfa_map/tlvBeaconMetricsResponse.h>
#include <tlvf/wfa_map/tlvChannelPreference.h>
#include <tlvf/wfa_map/tlvChannelScanCapabilities.h>
#include <tlvf/wfa_map/tlvChannelScanReportingPolicy.h>
#include <tlvf/wfa_map/tlvChannelSelectionResponse.h>
#include <tlvf/wfa_map/tlvClientAssociationControlRequest.h>
#include <tlvf/wfa_map/tlvClientAssociationEvent.h>
#include <tlvf/wfa_map/tlvClientCapabilityReport.h>
#include <tlvf/wfa_map/tlvClientInfo.h>
#include <tlvf/wfa_map/tlvDeviceInventory.h>
#include <tlvf/wfa_map/tlvDscpMappingTable.h>
#include <tlvf/wfa_map/tlvErrorCode.h>
#include <tlvf/wfa_map/tlvHigherLayerData.h>
#include <tlvf/wfa_map/tlvMetricReportingPolicy.h>
#include <tlvf/wfa_map/tlvOperatingChannelReport.h>
#include <tlvf/wfa_map/tlvProfile2ApCapability.h>
#include <tlvf/wfa_map/tlvProfile2ApRadioAdvancedCapabilities.h>
#include <tlvf/wfa_map/tlvProfile2AssociationStatusNotification.h>
#include <tlvf/wfa_map/tlvProfile2CacCapabilities.h>
#include <tlvf/wfa_map/tlvProfile2CacCompletionReport.h>
#include <tlvf/wfa_map/tlvProfile2CacRequest.h>
#include <tlvf/wfa_map/tlvProfile2CacStatusReport.h>
#include <tlvf/wfa_map/tlvProfile2CacTermination.h>
#include <tlvf/wfa_map/tlvProfile2ChannelScanRequest.h>
#include <tlvf/wfa_map/tlvProfile2ChannelScanResult.h>
#include <tlvf/wfa_map/tlvProfile2Default802dotQSettings.h>
#include <tlvf/wfa_map/tlvProfile2ErrorCode.h>
#include <tlvf/wfa_map/tlvProfile2MetricCollectionInterval.h>
#include <tlvf/wfa_map/tlvProfile2MultiApProfile.h>
#include <tlvf/wfa_map/tlvProfile2RadioMetrics.h>
#include <tlvf/wfa_map/tlvProfile2ReasonCode.h>
#include <tlvf/wfa_map/tlvProfile2StatusCode.h>
#include <tlvf/wfa_map/tlvProfile2SteeringRequest.h>
#include <tlvf/wfa_map/tlvProfile2TrafficSeparationPolicy.h>
#include <tlvf/wfa_map/tlvProfile2UnsuccessfulAssociationPolicy.h>
#include <tlvf/wfa_map/tlvRadioOperationRestriction.h>
#include <tlvf/wfa_map/tlvSearchedService.h>
#include <tlvf/wfa_map/tlvServicePrioritizationRule.h>
#include <tlvf/wfa_map/tlvStaMacAddressType.h>
#include <tlvf/wfa_map/tlvSteeringBTMReport.h>
#include <tlvf/wfa_map/tlvSteeringPolicy.h>
#include <tlvf/wfa_map/tlvSteeringRequest.h>
#include <tlvf/wfa_map/tlvSupportedService.h>
#include <tlvf/wfa_map/tlvTimestamp.h>
#include <tlvf/wfa_map/tlvTransmitPowerLimit.h>
#include <tlvf/wfa_map/tlvTunnelledData.h>
#include <tlvf/wfa_map/tlvTunnelledProtocolType.h>
#include <tlvf/wfa_map/tlvTunnelledSourceInfo.h>

using namespace ieee1905_1;

int CmduMessageRx::getNextTlvType() const
{
    if (!getCmduHeader())
        return -1;
    sTlvHeader *tlv = reinterpret_cast<sTlvHeader *>(msg.prevClass()->getBuffPtr());
    return tlv->type;
}

uint16_t CmduMessageRx::getNextTlvLength() const
{
    if (!getCmduHeader()) {
        return UINT16_MAX;
    }
    sTlvHeader *tlv = reinterpret_cast<sTlvHeader *>(msg.prevClass()->getBuffPtr());

    uint16_t tlv_length = tlv->length;
    swap_16(tlv_length);

    return tlv_length;
}

std::shared_ptr<BaseClass> CmduMessageRx::parseNextTlv(ieee1905_1::eTlvType tlv_type)
{
    switch (tlv_type) {
    case (ieee1905_1::eTlvType::TLV_END_OF_MESSAGE): {
        return msg.addClass<tlvEndOfMessage>();
    }
    case (ieee1905_1::eTlvType::TLV_AL_MAC_ADDRESS): {
        return msg.addClass<tlvAlMacAddress>();
    }
    case (ieee1905_1::eTlvType::TLV_MAC_ADDRESS): {
        return msg.addClass<tlvMacAddress>();
    }
    case (ieee1905_1::eTlvType::TLV_DEVICE_INFORMATION): {
        return msg.addClass<tlvDeviceInformation>();
    }
    case (ieee1905_1::eTlvType::TLV_DEVICE_BRIDGING_CAPABILITY): {
        return msg.addClass<tlvDeviceBridgingCapability>();
    }
    case (ieee1905_1::eTlvType::TLV_NON_1905_NEIGHBOR_DEVICE_LIST): {
        return msg.addClass<tlvNon1905neighborDeviceList>();
    }
    case (ieee1905_1::eTlvType::TLV_1905_NEIGHBOR_DEVICE): {
        return msg.addClass<tlv1905NeighborDevice>();
    }
    case (ieee1905_1::eTlvType::TLV_LINK_METRIC_QUERY): {
        /**
         * The IEEE 1905.1 standard says about the Link Metric Query TLV and the neighbor type
         * octet that "If the value is 0, then the EUI48 field is not present; if the value is 1,
         * then the EUI-48 field shall be present."
         *
         * However, optional fields are not currently supported by TLVF.
         *
         * As a workaround, instead of defining a tlvLinkMetricQuery TLV with an optional field,
         * we have defined two different TLVs, one with the optional field and the other one
         * without it. Application must then check the length of received TLV to know if optional
         * field (MAC address of neighbor device) is present or not and then create an instance of
         * either tlvLinkMetricQuery or tlvLinkMetricQueryAllNeighbors respectively.
         */
        const uint16_t all_neighbors_tlv_length = 2;
        uint16_t tlv_length                     = getNextTlvLength();

        if (all_neighbors_tlv_length == tlv_length) {
            return msg.addClass<tlvLinkMetricQueryAllNeighbors>();
        } else {
            return msg.addClass<tlvLinkMetricQuery>();
        }
    }
    case (ieee1905_1::eTlvType::TLV_TRANSMITTER_LINK_METRIC): {
        return msg.addClass<tlvTransmitterLinkMetric>();
    }
    case (ieee1905_1::eTlvType::TLV_RECEIVER_LINK_METRIC): {
        return msg.addClass<tlvReceiverLinkMetric>();
    }
    case (ieee1905_1::eTlvType::TLV_VENDOR_SPECIFIC): {
        return msg.addClass<tlvVendorSpecific>();
    }
    case (ieee1905_1::eTlvType::TLV_LINK_METRIC_RESULT_CODE): {
        return msg.addClass<tlvLinkMetricResultCode>();
    }
    case (ieee1905_1::eTlvType::TLV_SEARCHED_ROLE): {
        return msg.addClass<tlvSearchedRole>();
    }
    case (ieee1905_1::eTlvType::TLV_AUTOCONFIG_FREQ_BAND): {
        return msg.addClass<tlvAutoconfigFreqBand>();
    }
    case (ieee1905_1::eTlvType::TLV_SUPPORTED_ROLE): {
        return msg.addClass<tlvSupportedRole>();
    }
    case (ieee1905_1::eTlvType::TLV_SUPPORTED_FREQ_BAND): {
        return msg.addClass<tlvSupportedFreqBand>();
    }
    case (ieee1905_1::eTlvType::TLV_WSC): {
        return msg.addClass<ieee1905_1::tlvWsc>();
    }
    case (ieee1905_1::eTlvType::TLV_PUSH_BUTTON_EVENT_NOTIFICATION): {
        return msg.addClass<tlvPushButtonEventNotification>();
    }
    case (ieee1905_1::eTlvType::TLV_PUSH_BUTTON_JOIN_NOTIFICATION): {
        return msg.addClass<tlvPushButtonJoinNotification>();
    }
    }
    LOG(FATAL) << "Unknown TLV type: " << unsigned(tlv_type);
    return msg.addClass<tlvUnknown>();
}

std::shared_ptr<BaseClass> CmduMessageRx::parseNextTlv(wfa_map::eTlvTypeMap tlv_type)
{
    switch (tlv_type) {
    case (wfa_map::eTlvTypeMap::TLV_SUPPORTED_SERVICE): {
        return msg.addClass<wfa_map::tlvSupportedService>();
    }
    case (wfa_map::eTlvTypeMap::TLV_SEARCHED_SERVICE): {
        return msg.addClass<wfa_map::tlvSearchedService>();
    }
    case (wfa_map::eTlvTypeMap::TLV_AP_RADIO_IDENTIFIER): {
        return msg.addClass<wfa_map::tlvApRadioIdentifier>();
    }
    case (wfa_map::eTlvTypeMap::TLV_AP_OPERATIONAL_BSS): {
        return msg.addClass<wfa_map::tlvApOperationalBSS>();
    }
    case (wfa_map::eTlvTypeMap::TLV_ASSOCIATED_CLIENTS): {
        return msg.addClass<wfa_map::tlvAssociatedClients>();
    }
    case (wfa_map::eTlvTypeMap::TLV_AP_RADIO_BASIC_CAPABILITIES): {
        return msg.addClass<wfa_map::tlvApRadioBasicCapabilities>();
    }
    case (wfa_map::eTlvTypeMap::TLV_AP_HT_CAPABILITIES): {
        return msg.addClass<wfa_map::tlvApHtCapabilities>();
    }
    case (wfa_map::eTlvTypeMap::TLV_AP_VHT_CAPABILITIES): {
        return msg.addClass<wfa_map::tlvApVhtCapabilities>();
    }
    case (wfa_map::eTlvTypeMap::TLV_AP_HE_CAPABILITIES): {
        return msg.addClass<wfa_map::tlvApHeCapabilities>();
    }
    case (wfa_map::eTlvTypeMap::TLV_STEERING_POLICY): {
        return msg.addClass<wfa_map::tlvSteeringPolicy>();
    }
    case (wfa_map::eTlvTypeMap::TLV_METRIC_REPORTING_POLICY): {
        return msg.addClass<wfa_map::tlvMetricReportingPolicy>();
    }
    case (wfa_map::eTlvTypeMap::TLV_CHANNEL_PREFERENCE): {
        return msg.addClass<wfa_map::tlvChannelPreference>();
    }
    case (wfa_map::eTlvTypeMap::TLV_RADIO_OPERATION_RESTRICTION): {
        return msg.addClass<wfa_map::tlvRadioOperationRestriction>();
    }
    case (wfa_map::eTlvTypeMap::TLV_TRANSMIT_POWER_LIMIT): {
        return msg.addClass<wfa_map::tlvTransmitPowerLimit>();
    }
    case (wfa_map::eTlvTypeMap::TLV_CHANNEL_SELECTION_RESPONSE): {
        return msg.addClass<wfa_map::tlvChannelSelectionResponse>();
    }
    case (wfa_map::eTlvTypeMap::TLV_OPERATING_CHANNEL_REPORT): {
        return msg.addClass<wfa_map::tlvOperatingChannelReport>();
    }
    case (wfa_map::eTlvTypeMap::TLV_CLIENT_INFO): {
        return msg.addClass<wfa_map::tlvClientInfo>();
    }
    case (wfa_map::eTlvTypeMap::TLV_CLIENT_CAPABILITY_REPORT): {
        return msg.addClass<wfa_map::tlvClientCapabilityReport>();
    }
    case (wfa_map::eTlvTypeMap::TLV_CLIENT_ASSOCIATION_EVENT): {
        return msg.addClass<wfa_map::tlvClientAssociationEvent>();
    }
    case (wfa_map::eTlvTypeMap::TLV_AP_METRIC_QUERY): {
        return msg.addClass<wfa_map::tlvApMetricQuery>();
    }
    case (wfa_map::eTlvTypeMap::TLV_AP_METRIC): {
        return msg.addClass<wfa_map::tlvApMetrics>();
    }
    case (wfa_map::eTlvTypeMap::TLV_STAMAC_ADDRESS_TYPE): {
        return msg.addClass<wfa_map::tlvStaMacAddressType>();
    }
    case (wfa_map::eTlvTypeMap::TLV_ASSOCIATED_STA_LINK_METRICS): {
        return msg.addClass<wfa_map::tlvAssociatedStaLinkMetrics>();
    }
    case (wfa_map::eTlvTypeMap::TLV_UNASSOCIATED_STA_LINK_METRICS_QUERY): {
        LOG(DEBUG) << "TLV_UNASSOCIATED_STA_LINK_METRICS_QUERY not supported";
        return msg.addClass<ieee1905_1::tlvUnknown>();
    }
    case (wfa_map::eTlvTypeMap::TLV_UNASSOCIATED_STA_LINK_METRICS_RESPONSE): {
        LOG(DEBUG) << "TLV_UNASSOCIATED_STA_LINK_METRICS_RESPONSE not supported";
        return msg.addClass<ieee1905_1::tlvUnknown>();
    }
    case (wfa_map::eTlvTypeMap::TLV_BEACON_METRICS_QUERY): {
        return msg.addClass<wfa_map::tlvBeaconMetricsQuery>();
    }
    case (wfa_map::eTlvTypeMap::TLV_BEACON_METRICS_RESPONSE): {
        return msg.addClass<wfa_map::tlvBeaconMetricsResponse>();
    }
    case (wfa_map::eTlvTypeMap::TLV_STEERING_REQUEST): {
        return msg.addClass<wfa_map::tlvSteeringRequest>();
    }
    case (wfa_map::eTlvTypeMap::TLV_STEERING_BTM_REPORT): {
        return msg.addClass<wfa_map::tlvSteeringBTMReport>();
    }
    case (wfa_map::eTlvTypeMap::TLV_CLIENT_ASSOCIATION_CONTROL_REQUEST): {
        return msg.addClass<wfa_map::tlvClientAssociationControlRequest>();
    }
    case (wfa_map::eTlvTypeMap::TLV_BACKHAUL_STEERING_REQUEST): {
        return msg.addClass<wfa_map::tlvBackhaulSteeringRequest>();
    }
    case (wfa_map::eTlvTypeMap::TLV_BACKHAUL_STEERING_RESPONSE): {
        return msg.addClass<wfa_map::tlvBackhaulSteeringResponse>();
    }
    case (wfa_map::eTlvTypeMap::TLV_HIGHER_LAYER_DATA): {
        return msg.addClass<wfa_map::tlvHigherLayerData>();
    }
    case (wfa_map::eTlvTypeMap::TLV_AP_CAPABILITY): {
        return msg.addClass<wfa_map::tlvApCapability>();
    }
    case (wfa_map::eTlvTypeMap::TLV_ASSOCIATED_STA_TRAFFIC_STATS): {
        return msg.addClass<wfa_map::tlvAssociatedStaTrafficStats>();
    }
    case (wfa_map::eTlvTypeMap::TLV_ERROR_CODE): {
        return msg.addClass<wfa_map::tlvErrorCode>();
    }
    case (wfa_map::eTlvTypeMap::TLV_CHANNEL_SCAN_REPORTING_POLICY): {
        return msg.addClass<wfa_map::tlvChannelScanReportingPolicy>();
    }
    case (wfa_map::eTlvTypeMap::TLV_CHANNEL_SCAN_CAPABILITIES): {
        return msg.addClass<wfa_map::tlvChannelScanCapabilities>();
    }
    case (wfa_map::eTlvTypeMap::TLV_CHANNEL_SCAN_REQUEST): {
        return msg.addClass<wfa_map::tlvProfile2ChannelScanRequest>();
    }
    case (wfa_map::eTlvTypeMap::TLV_CHANNEL_SCAN_RESULT): {
        return msg.addClass<wfa_map::tlvProfile2ChannelScanResult>();
    }
    case (wfa_map::eTlvTypeMap::TLV_BACKHAUL_BSS_CONFIGURATION): {
        return msg.addClass<wfa_map::tlvBackhaulBssConfiguration>();
    }
    case (wfa_map::eTlvTypeMap::TLV_TIMESTAMP): {
        return msg.addClass<wfa_map::tlvTimestamp>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_CAC_REQUEST): {
        return msg.addClass<wfa_map::tlvProfile2CacRequest>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_CAC_TERMINATION): {
        return msg.addClass<wfa_map::tlvProfile2CacTermination>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_CAC_COMPLETION_REPORT): {
        return msg.addClass<wfa_map::tlvProfile2CacCompletionReport>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_CAC_STATUS_REPORT): {
        return msg.addClass<wfa_map::tlvProfile2CacStatusReport>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_CAC_CAPABILITIES): {
        return msg.addClass<wfa_map::tlvProfile2CacCapabilities>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_MULTIAP_PROFILE): {
        return msg.addClass<wfa_map::tlvProfile2MultiApProfile>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_AP_CAPABILITY): {
        return msg.addClass<wfa_map::tlvProfile2ApCapability>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_DEFAULT_802_1Q_SETTINGS): {
        return msg.addClass<wfa_map::tlvProfile2Default802dotQSettings>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_TRAFFIC_SEPARATION_POLICY): {
        return msg.addClass<wfa_map::tlvProfile2TrafficSeparationPolicy>();
    }
    case (wfa_map::eTlvTypeMap::TLV_SERVICE_PRIORITIZATION_RULE): {
        return msg.addClass<wfa_map::tlvServicePrioritizationRule>();
    }
    case (wfa_map::eTlvTypeMap::TLV_DSCP_MAPPING_TABLE): {
        return msg.addClass<wfa_map::tlvDscpMappingTable>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_ERROR_CODE): {
        return msg.addClass<wfa_map::tlvProfile2ErrorCode>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_AP_RADIO_ADVANCED_CAPABILITIES): {
        return msg.addClass<wfa_map::tlvProfile2ApRadioAdvancedCapabilities>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_ASSOCIATION_STATUS_NOTIFICATION): {
        return msg.addClass<wfa_map::tlvProfile2AssociationStatusNotification>();
    }
    case (wfa_map::eTlvTypeMap::TLV_TUNNELLED_SOURCE_INFO): {
        return msg.addClass<wfa_map::tlvTunnelledSourceInfo>();
    }
    case (wfa_map::eTlvTypeMap::TLV_TUNNELLED_PROTOCOL_TYPE): {
        return msg.addClass<wfa_map::tlvTunnelledProtocolType>();
    }
    case (wfa_map::eTlvTypeMap::TLV_TUNNELLED_DATA): {
        return msg.addClass<wfa_map::tlvTunnelledData>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_STEERING_REQUEST): {
        return msg.addClass<wfa_map::tlvProfile2SteeringRequest>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_UNSUCCESSFUL_ASSOCIATION_POLICY): {
        return msg.addClass<wfa_map::tlvProfile2UnsuccessfulAssociationPolicy>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_METRIC_COLLECTION_INTERVAL): {
        return msg.addClass<wfa_map::tlvProfile2MetricCollectionInterval>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_RADIO_METRICS): {
        return msg.addClass<wfa_map::tlvProfile2RadioMetrics>();
    }
    case (wfa_map::eTlvTypeMap::TLV_AP_EXTENDED_METRICS): {
        return msg.addClass<wfa_map::tlvApExtendedMetrics>();
    }
    case (wfa_map::eTlvTypeMap::TLV_ASSOCIATED_STA_EXTENDED_LINK_METRICS): {
        return msg.addClass<wfa_map::tlvAssociatedStaExtendedLinkMetrics>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_STATUS_CODE): {
        return msg.addClass<wfa_map::tlvProfile2StatusCode>();
    }
    case (wfa_map::eTlvTypeMap::TLV_PROFILE2_REASON_CODE): {
        return msg.addClass<wfa_map::tlvProfile2ReasonCode>();
    }
    case (wfa_map::eTlvTypeMap::TLV_BACKHAUL_STA_RADIO_CAPABILITIES): {
        return msg.addClass<wfa_map::tlvBackhaulStaRadioCapabilities>();
    }
    case (wfa_map::eTlvTypeMap::TLV_DEVICE_INVENTORY): {
        return msg.addClass<wfa_map::tlvDeviceInventory>();
    }
    case (wfa_map::eTlvTypeMap::TLV_ASSOCIATED_WIFI_6_STA_STATUS_REPORT): {
        return msg.addClass<wfa_map::tlvAssociatedWiFi6StaStatusReport>();
    }
    }
    LOG(FATAL) << "Unknown TLV type: " << unsigned(tlv_type);
    return msg.addClass<tlvUnknown>();
}

std::shared_ptr<BaseClass> CmduMessageRx::parseNextTlv()
{
    auto tlv_type = getNextTlvType();

    if (ieee1905_1::eTlvTypeValidate::check(tlv_type)) {
        return parseNextTlv(ieee1905_1::eTlvType(tlv_type));
    } else if (wfa_map::eTlvTypeMapValidate::check(tlv_type)) {
        return parseNextTlv(wfa_map::eTlvTypeMap(tlv_type));
    } else {
        LOG(INFO) << "Unknown TLV type: " << tlv_type;
        return msg.addClass<tlvUnknown>();
    }
}

bool CmduMessageRx::parse()
{
    msg.reset(true);
    auto cmduhdr = msg.addClass<cCmduHeader>();
    if (!cmduhdr)
        return false;

    while (auto tlv = parseNextTlv()) {
        if (std::dynamic_pointer_cast<tlvEndOfMessage>(tlv)) {
            return true;
        }
    }

    return false;
}
