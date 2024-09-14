/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2021-2022 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _ASSOC_REQ_FRAME_H_
#define _ASSOC_REQ_FRAME_H_

#include "assoc_frame_bitfields.h"
#include <tlvf/AttrList.h>
#include <tlvf/association_frame/AssocReqFields.h>
#include <tlvf/association_frame/ReassocReqFields.h>
#include <tlvf/association_frame/cCapInfoDmgSta.h>
#include <tlvf/association_frame/cExtendedCap.h>
#include <tlvf/association_frame/cMobilityDomain.h>
#include <tlvf/association_frame/cQosCapability.h>
#include <tlvf/association_frame/cRmEnabledCaps.h>
#include <tlvf/association_frame/cStaHeCapability.h>
#include <tlvf/association_frame/cStaHtCapability.h>
#include <tlvf/association_frame/cStaVhtCapability.h>
#include <tlvf/association_frame/cSupportedChannels.h>
#include <tlvf/association_frame/eElementID.h>
#include <tlvf/association_frame/eExtElementID.h>
#include <tlvf/common/sMacAddr.h>
#include <tlvf/wfa_map/tlvTunnelledData.h>

namespace assoc_frame {

/**
 * @brief Convert source value to mapped equally sized type
 * (usefull to map bitmapped struct to number value).
 *
 * @param source input in initial type.
 * @return output in resulting type (overloaded value).
 */
template <typename TS, typename TR>
static inline typename std::enable_if<sizeof(TS) == sizeof(TR), TR>::type const
convert(const TS source)
{
    union {
        TS src;
        TR res;
    } mapper = {
        source,
    };
    return mapper.res;
}

class AssocReqFrame : public WSC::AttrList<uint8_t, uint8_t> {
public:
    typedef struct sFieldsPresent {
        uint32_t supported_rates : 1;
        uint32_t extended_sup_rates : 1;
        uint32_t extended_cap : 1;
        uint32_t power_capability : 1;
        uint32_t supported_channels : 1;
        uint32_t rsn : 1;
        uint32_t qos_capability : 1;
        uint32_t qos_traffic_cap : 1;
        uint32_t rm_enabled_caps : 1;
        uint32_t mobility_domain : 1;
        uint32_t sup_op_classes : 1;
        uint32_t ht_capability : 1;
        uint32_t bss_coexistence20_49 : 1;
        uint32_t tim_broadcast_request : 1;
        uint32_t interworking : 1;
        uint32_t multi_band : 1;
        uint32_t dmg_capability : 1;
        uint32_t mms : 1;
        uint32_t vht_capability : 1;
        uint32_t op_mode_notification : 1;
        uint32_t fast_bss_trans : 1;
        uint32_t fms_request : 1;
        uint32_t dms_request : 1;
        uint32_t vendor_specific : 1;
        uint32_t he_capability : 1;
    } __attribute__((packed)) sFieldsPresent;

    enum eFrameType : uint8_t {
        ASSOCIATION_REQUEST   = 0x0,
        REASSOCIATION_REQUEST = 0x1,
        UNKNOWN               = 0xff,
    };
    // Enum AutoPrint generated code snippet begining- DON'T EDIT!
    // clang-format off
    static const char *eFrameType_str(eFrameType enum_value) {
        switch (enum_value) {
        case ASSOCIATION_REQUEST:   return "ASSOCIATION_REQUEST";
        case REASSOCIATION_REQUEST: return "REASSOCIATION_REQUEST";
        case UNKNOWN:               return "UNKNOWN";
        }
        static std::string out_str = std::to_string(int(enum_value));
        return out_str.c_str();
    }
    friend inline std::ostream &operator<<(std::ostream &out, eFrameType value) { return out << eFrameType_str(value); }
    // clang-format on
    // Enum AutoPrint generated code snippet end

    AssocReqFrame(uint8_t *buff, size_t buff_len, eFrameType _type, bool parse)
        : AttrList(buff, buff_len, parse), type(_type)
    {
    }
    virtual ~AssocReqFrame() = default;
    static std::shared_ptr<AssocReqFrame> parse(uint8_t *assoc_frame, size_t assoc_frame_len,
                                                const eFrameType frame_type = eFrameType::UNKNOWN);
    bool valid() const override;
    bool init();
    bool finalize();
    uint16_t &listen_interval();
    std::string sta_ssid();
    std::shared_ptr<cStaHtCapability> sta_ht_capability();
    std::shared_ptr<cStaVhtCapability> sta_vht_capability();
    std::shared_ptr<cStaHeCapability> sta_he_capability();
    std::shared_ptr<cPowerCapability> power_capability();
    std::shared_ptr<cRmEnabledCaps> rm_enabled_caps();
    std::shared_ptr<cMultiBand> multi_band();
    uint8_t *supported_rates();

    eFrameType type;
    sFieldsPresent fields_present;

private:
    bool init_assoc_frame();
    bool init_reassoc_frame();
    bool add_ssid_field();
};
}; // namespace assoc_frame

#endif
