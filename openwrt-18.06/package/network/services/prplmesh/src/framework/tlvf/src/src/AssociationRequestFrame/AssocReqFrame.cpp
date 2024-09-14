/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021-2022 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include <tlvf/AssociationRequestFrame/AssocReqFrame.h>
#include <tlvf/tlvfdefines.h>
#include <tlvf/tlvflogging.h>

namespace assoc_frame {

std::shared_ptr<AssocReqFrame> AssocReqFrame::parse(uint8_t *assoc_frame_buff,
                                                    size_t assoc_frame_len, const eFrameType type)
{
    if (!assoc_frame_buff) {
        TLVF_LOG(WARNING) << "Frame data buffer is null";
        return {};
    }

    if (!assoc_frame_len) {
        TLVF_LOG(WARNING) << "Frame data length is 0";
        return {};
    }

    auto fields = std::make_shared<AssocReqFrame>(assoc_frame_buff, assoc_frame_len, type, true);
    if (!fields) {
        TLVF_LOG(ERROR) << "Failed to initialize Association Request frame fields";
        return {};
    }

    switch (type) {
    case ASSOCIATION_REQUEST:
        if (!fields->init_assoc_frame()) {
            LOG(ERROR) << "Failed to parse Association Request frame.";
            return {};
        }
        break;
    case REASSOCIATION_REQUEST:
        if (!fields->init_reassoc_frame()) {
            LOG(ERROR) << "Failed to parse Reassociation Request frame.";
            return {};
        }
        break;
    default:
        // The (Re)Association Request frame which comes with Capability Report
        // contains only frame body, so we don't know it's type.
        if (fields->init_assoc_frame()) {
            fields->type = ASSOCIATION_REQUEST;
        } else if (fields->init_reassoc_frame()) {
            fields->type = REASSOCIATION_REQUEST;
        } else {
            LOG(ERROR) << "Failed to parse (Re)Association Request frame.";
            return {};
        }
    }
    if (!fields->init()) {
        LOG(ERROR) << fields->type << " frame fields weren't parsed successfully";
        return {};
    }
    if (!fields->valid()) {
        LOG(ERROR) << fields->type << " frame validation failed";
        return {};
    }
    LOG(DEBUG) << fields->type << " parsed successfully";

    /*
     * swap once to host byte order
     */
    if (!fields->is_swapped()) {
        fields->swap();
    }
    return fields;
}

bool AssocReqFrame::init_assoc_frame()
{
    if (!m_parse) {
        TLVF_LOG(ERROR) << "init() called but m_parse is not set!";
        return false;
    }
    reset(m_parse);

    // Parse first mandatory fields
    // Here need to be checked if sta is dmg, if it's not
    // addAttr<sCapabilityInfoNonDmgSta>. But for now just assume it's dmg.
    if (getRemainingBytes() > 0 && !addAttr<cCapInfoDmgSta>()) {
        TLVF_LOG(ERROR) << "Failed to add mandatory field cCapInfoDmgSta";
        return false;
    }
    if (getRemainingBytes() == 0) {
        TLVF_LOG(ERROR) << "No bytes left to parse mandatory field cCurrentApAddress";
        return false;
    }
    return add_ssid_field();
}

bool AssocReqFrame::init_reassoc_frame()
{
    if (!m_parse) {
        TLVF_LOG(ERROR) << "init() called but m_parse is not set!";
        return false;
    }
    reset(m_parse);
    if (getRemainingBytes() > 0 && !addAttr<cCapInfoDmgSta>()) {
        TLVF_LOG(ERROR) << "Failed to add mandatory field cCapInfoDmgSta";
        return false;
    }
    if (getRemainingBytes() == 0) {
        TLVF_LOG(ERROR) << "No bytes left to parse mandatory field cCurrentApAddress";
        return false;
    }
    if (!addAttr<cCurrentApAddress>()) {
        TLVF_LOG(ERROR) << "Failed to add mandatory field cCurrentApAddress";
        return false;
    }
    return add_ssid_field();
}

bool AssocReqFrame::add_ssid_field()
{
    if (getRemainingBytes() == 0) {
        TLVF_LOG(ERROR) << "No bytes left to parse mandatory field cSSID";
        return false;
    }
    if (getNextAttrType() != ID_SSID) {
        TLVF_LOG(ERROR) << "Forth mandatory field of Reassociation Request frame should be SSID";
        return false;
    }
    if (!addAttr<cSSID>()) {
        TLVF_LOG(ERROR) << "Failed to add cSSID";
        return false;
    }
    return true;
}

bool AssocReqFrame::init()
{
    fields_present = {};
    // Parse optional fields.
    while (getRemainingBytes() > 0) {
        switch (getNextAttrType()) {
        case ID_SUPPORT_RATES:
            if (!addAttr<cSupportRates>()) {
                TLVF_LOG(ERROR) << "Failed to add cSupportRates";
                return false;
            }
            fields_present.supported_rates = 1;
            break;
        case ID_EXTENDED_SUP_RATES:
            if (!addAttr<cExtendedSupportRates>()) {
                TLVF_LOG(ERROR) << "Failed to add cExtendedSupportRates";
                return false;
            }
            fields_present.extended_sup_rates = 1;
            break;
        case ID_POWER_CAPABILITY:
            if (!addAttr<cPowerCapability>()) {
                TLVF_LOG(ERROR) << "Failed to add cPowerCapability";
                return false;
            }
            fields_present.power_capability = 1;
            break;
        case ID_SUP_CHANNELS:
            if (!addAttr<cSupportedChannels>()) {
                TLVF_LOG(ERROR) << "Failed to add cSupportedChannels";
                return false;
            }
            fields_present.supported_channels = 1;
            break;
        case ID_RSN:
            if (!addAttr<cRSN>()) {
                TLVF_LOG(ERROR) << "Failed to add cRSN";
                return false;
            }
            fields_present.rsn = 1;
            break;
        case ID_QOS_CAPABILITY:
            if (!addAttr<cQosCapability>()) {
                TLVF_LOG(ERROR) << "Failed to add cQosCapability";
                return false;
            }
            fields_present.qos_capability = 1;
            break;
        case ID_RM_ENABLED_CAPS:
            if (!addAttr<cRmEnabledCaps>()) {
                TLVF_LOG(ERROR) << "Failed to add cRmEnabledCaps";
                return false;
            }
            fields_present.rm_enabled_caps = 1;
            break;
        case ID_MOBILITY_DOMAIN:
            if (!addAttr<cMobilityDomain>()) {
                TLVF_LOG(ERROR) << "Failed to add cMobilityDomain";
                return false;
            }
            fields_present.mobility_domain = 1;
            break;
        case ID_SUP_OP_CLASSES:
            if (!addAttr<cSupportedOpClasses>()) {
                TLVF_LOG(ERROR) << "Failed to add cSupportedOpClasses";
                return false;
            }
            fields_present.sup_op_classes = 1;
            break;
        case ID_HT_CAPABILITY:
            if (!addAttr<cStaHtCapability>()) {
                TLVF_LOG(ERROR) << "Failed to add cStaHtCapability";
                return false;
            }
            fields_present.ht_capability = 1;
            break;
        case ID_BSS_COEXISTENCE20_40:
            if (!addAttr<cBssCoexistence20_40>()) {
                TLVF_LOG(ERROR) << "Failed to add cBssCoexistence20_40";
                return false;
            }
            fields_present.bss_coexistence20_49 = 1;
            break;
        case ID_EXTENDED_CAPABILITY:
            if (!addAttr<cExtendedCap>()) {
                TLVF_LOG(ERROR) << "Failed to add cExtendedCap";
                return false;
            }
            fields_present.extended_cap = 1;
            break;
        case ID_QOS_TRAFFIC_CAP:
            if (!addAttr<cQosTrafficCap>()) {
                TLVF_LOG(ERROR) << "Failed to add cQosTrafficCap";
                return false;
            }
            fields_present.qos_traffic_cap = 1;
            break;
        case ID_TIM_BROADCAST_REQUEST:
            if (!addAttr<cTimBroadcastRequest>()) {
                TLVF_LOG(ERROR) << "Failed to add cTimBroadcastRequest";
                return false;
            }
            fields_present.tim_broadcast_request = 1;
            break;
        case ID_INTERWORKING:
            if (!addAttr<cInterworking>()) {
                TLVF_LOG(ERROR) << "Failed to add cInterworking";
                return false;
            }
            fields_present.interworking = 1;
            break;
        case ID_MULTI_BAND:
            if (!addAttr<cMultiBand>()) {
                TLVF_LOG(ERROR) << "Failed to add cMultiBand";
                return false;
            }
            fields_present.multi_band = 1;
            break;
        case ID_DMG_CAPS:
            if (!addAttr<cDmgCapabilities>()) {
                TLVF_LOG(ERROR) << "Failed to add cDmgCapabilities";
                return false;
            }
            fields_present.dmg_capability = 1;
            break;
        case ID_MMS:
            if (!addAttr<cMultipleMacSublayers>()) {
                TLVF_LOG(ERROR) << "Failed to add cMultipleMacSublayers";
                return false;
            }
            fields_present.mms = 1;
            break;
        case ID_VHT_CAPS:
            if (!addAttr<cStaVhtCapability>()) {
                TLVF_LOG(ERROR) << "Failed to add cStaVhtCapability";
                return false;
            }
            fields_present.vht_capability = 1;
            break;
        case ID_OP_MODE_NOTIFICATION:
            if (!addAttr<cOperatingModeNotify>()) {
                TLVF_LOG(ERROR) << "Failed to add cOperatingModeNotify";
                return false;
            }
            fields_present.op_mode_notification = 1;
            break;
        case ID_FAST_BSS_TRANS:
            if (!addAttr<cFastBssTrans>()) {
                TLVF_LOG(ERROR) << "Failed to add cFastBssTrans";
                return false;
            }
            fields_present.fast_bss_trans = 1;
            break;
        case ID_FMS_REQUEST:
            if (!addAttr<cFmsRequest>()) {
                TLVF_LOG(ERROR) << "Failed to add cFmsRequest";
                return false;
            }
            fields_present.fms_request = 1;
            break;
        case ID_DMS_REQUEST:
            if (!addAttr<cDmsRequest>()) {
                TLVF_LOG(ERROR) << "Failed to add cDmsRequest";
                return false;
            }
            fields_present.dms_request = 1;
            break;

        case ID_VENDOR_SPECIFIC:
            // store vendor specific IEs to manage those common
            // like MS.Corp WMM/WME
            // They may be multiple, so shall be accessed
            // by iteration loop over indexes
            if (!addAttr<cVendorSpecific>()) {
                TLVF_LOG(ERROR) << "Failed to add cVendorSpecific";
                return false;
            }
            fields_present.vendor_specific = 1;
            break;
        case ID_EID_EXTENSION:
            switch (getNextAttrHdr()->data()[0]) {
            case EXTID_HE_CAPABILITIES:
                if (!addAttr<cStaHeCapability>()) {
                    TLVF_LOG(ERROR) << "Failed to add cStaHeCapability";
                    return false;
                }
                fields_present.he_capability = 1;
                break;
            default:
                // if Element ID extension is un-handled
                // then store it like unknown field
                if (!addAttr<cUnknownField>()) {
                    TLVF_LOG(DEBUG) << "Failed to add unhandled Element ID extension "
                                    << getNextAttrHdr()->data()[0];
                    return false;
                }
                break;
            }
            break;
        // Other fields (un-handled) are stored as unknown IE (raw data)
        // to continue parsing (IEs may be nested)
        default:
            TLVF_LOG(DEBUG) << "Unhandled field " << getNextAttrType()
                            << " : store it and parse next IEs"
                            << " in RemainingBytes: " << getRemainingBytes();
            if (!addAttr<cUnknownField>()) {
                TLVF_LOG(DEBUG) << "Failed to add cUnknownField for ID " << getNextAttrType();
                // ignore failure appending unhandled fields
                // and return the available FieldList
                return true;
            }
            break;
        }
    }
    return true;
}

bool AssocReqFrame::valid() const
{
    if (!getAttr<cCapInfoDmgSta>()) {
        TLVF_LOG(ERROR) << "getAttr<cCapInfoDmgSta> failed";
        return false;
    }
    if (!getAttr<cSSID>()) {
        TLVF_LOG(ERROR) << "getAttr<cSSID> failed";
        return false;
    }
    if (fields_present.supported_rates && !getAttr<cSupportRates>()) {
        TLVF_LOG(ERROR) << "getAttr<cSupportRates> failed";
        return false;
    }
    if (fields_present.power_capability && !getAttr<cPowerCapability>()) {
        TLVF_LOG(ERROR) << "getAttr<cPowerCapability> failed";
        return false;
    }
    if (fields_present.supported_channels && !getAttr<cSupportedChannels>()) {
        TLVF_LOG(ERROR) << "getAttr<cSupportedChannels> failed";
        return false;
    }
    if (fields_present.rsn && !getAttr<cRSN>()) {
        TLVF_LOG(ERROR) << "getAttr<cRSN> failed";
        return false;
    }
    if (fields_present.qos_capability && !getAttr<cQosTrafficCap>()) {
        TLVF_LOG(ERROR) << "getAttr<cQosTrafficCap> failed";
        return false;
    }
    if (fields_present.rm_enabled_caps && !getAttr<cRmEnabledCaps>()) {
        TLVF_LOG(ERROR) << "getAttr<cRmEnabledCaps> failed";
        return false;
    }
    if (fields_present.mobility_domain && !getAttr<cMobilityDomain>()) {
        TLVF_LOG(ERROR) << "getAttr<cMobilityDomain> failed";
        return false;
    }
    if (fields_present.sup_op_classes && !getAttr<cSupportedOpClasses>()) {
        TLVF_LOG(ERROR) << "getAttr<cSupportedOpClasses> failed";
        return false;
    }
    if (fields_present.ht_capability && !getAttr<cStaHtCapability>()) {
        TLVF_LOG(ERROR) << "getAttr<cStaHtCapability> failed";
        return false;
    }
    if (fields_present.bss_coexistence20_49 && !getAttr<cBssCoexistence20_40>()) {
        TLVF_LOG(ERROR) << "getAttr<cBssCoexistence20_40> failed";
        return false;
    }
    if (fields_present.tim_broadcast_request && !getAttr<cTimBroadcastRequest>()) {
        TLVF_LOG(ERROR) << "getAttr<cTimBroadcastRequest> failed";
        return false;
    }
    if (fields_present.interworking && !getAttr<cInterworking>()) {
        TLVF_LOG(ERROR) << "getAttr<cInterworking> failed";
        return false;
    }
    if (fields_present.multi_band && !getAttr<cMultiBand>()) {
        TLVF_LOG(ERROR) << "getAttr<cMultiBand> failed";
        return false;
    }
    if (fields_present.dmg_capability && !getAttr<cDmgCapabilities>()) {
        TLVF_LOG(ERROR) << "getAttr<cDmgCapabilities> failed";
        return false;
    }
    if (fields_present.mms && !getAttr<cMultipleMacSublayers>()) {
        TLVF_LOG(ERROR) << "getAttr<cMultipleMacSublayers> failed";
        return false;
    }
    if (fields_present.vht_capability && !getAttr<cStaVhtCapability>()) {
        TLVF_LOG(ERROR) << "getAttr<cStaVhtCapability> failed";
        return false;
    }
    if (fields_present.fast_bss_trans && !getAttr<cFastBssTrans>()) {
        TLVF_LOG(ERROR) << "getAttr<cFastBssTrans> failed";
        return false;
    }
    if (fields_present.fms_request && !getAttr<cFmsRequest>()) {
        TLVF_LOG(ERROR) << "getAttr<cFmsRequest> failed";
        return false;
    }
    if (fields_present.dms_request && !getAttr<cDmsRequest>()) {
        TLVF_LOG(ERROR) << "getAttr<cDmsRequest> failed";
        return false;
    }
    if (fields_present.op_mode_notification && !getAttr<cOperatingModeNotify>()) {
        TLVF_LOG(ERROR) << "getAttr<cOperatingModeNotify> failed";
        return false;
    }
    if (fields_present.vendor_specific && !getAttr<cVendorSpecific>()) {
        TLVF_LOG(ERROR) << "getAttr<cVendorSpecific> failed";
        return false;
    }
    if (fields_present.he_capability && !getAttr<cStaHeCapability>()) {
        TLVF_LOG(ERROR) << "getAttr<cStaHeCapability> failed";
        return false;
    }
    return true;
}

bool AssocReqFrame::finalize()
{
    //finalize last added attribute
    if (!this->ClassList::finalize()) {
        return false;
    }

    //swap once to network byte order
    if (!this->is_swapped()) {
        this->swap();
    }
    return true;
}

uint16_t &AssocReqFrame::listen_interval() { return getAttr<cCapInfoDmgSta>()->listen_interval(); }

std::string AssocReqFrame::sta_ssid() { return getAttr<cSSID>()->ssid_str(); }

std::shared_ptr<assoc_frame::cStaHtCapability> AssocReqFrame::sta_ht_capability()
{
    if (fields_present.ht_capability) {
        return getAttr<cStaHtCapability>();
    }
    return {};
}

std::shared_ptr<assoc_frame::cStaVhtCapability> AssocReqFrame::sta_vht_capability()
{
    if (fields_present.vht_capability) {
        return getAttr<cStaVhtCapability>();
    }
    return {};
}

uint8_t *AssocReqFrame::supported_rates()
{
    if (fields_present.supported_rates) {
        return getAttr<cSupportRates>()->supported_rated();
    };
    return {};
}

std::shared_ptr<assoc_frame::cPowerCapability> AssocReqFrame::power_capability()
{
    if (fields_present.power_capability) {
        return getAttr<cPowerCapability>();
    };
    return {};
}

std::shared_ptr<assoc_frame::cRmEnabledCaps> AssocReqFrame::rm_enabled_caps()
{
    if (fields_present.rm_enabled_caps) {
        return getAttr<cRmEnabledCaps>();
    };
    return {};
}

std::shared_ptr<assoc_frame::cMultiBand> AssocReqFrame::multi_band()
{
    if (fields_present.multi_band) {
        return getAttr<cMultiBand>();
    };
    return {};
}

std::shared_ptr<assoc_frame::cStaHeCapability> AssocReqFrame::sta_he_capability()
{
    if (fields_present.he_capability) {
        return getAttr<cStaHeCapability>();
    }
    return {};
}

}; // namespace assoc_frame
