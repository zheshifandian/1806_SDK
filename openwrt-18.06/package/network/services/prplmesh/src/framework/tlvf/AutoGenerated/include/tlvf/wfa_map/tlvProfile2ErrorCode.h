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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2ERRORCODE_H_
#define _TLVF_WFA_MAP_TLVPROFILE2ERRORCODE_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"
#include "tlvf/common/sMacAddr.h"
#include <ostream>

namespace wfa_map {


class tlvProfile2ErrorCode : public BaseClass
{
    public:
        tlvProfile2ErrorCode(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2ErrorCode(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2ErrorCode();

        enum eReasonCode: uint8_t {
            SERVICE_PRIORITIZATION_RULE_NOT_FOUND = 0x1,
            NUMBER_OF_SERVICE_PRIORITIZATION_RULES_EXCEEDED_THE_MAXIMUM_SUPPORTED = 0x2,
            DEFAULT_PCP_OR_PRIMARY_VLAN_ID_NOT_PROVIDED = 0x3,
            NUMBER_OF_UNIQUE_VLAN_ID_EXCEEDS_MAXIMUM_SUPPORTED = 0x5,
            TRAFFIC_SEPARATION_ON_COMBINED_FRONTHAUL_AND_PROFILE1_BACKHAUL_UNSUPPORTED = 0x7,
            TRAFFIC_SEPARATION_ON_COMBINED_PROFILE1_BACKHAUL_AND_PROFILE2_BACKHAUL_UNSUPPORTED = 0x8,
            SERVICE_PRIORITIZATION_RULE_NOT_SUPPORTED = 0x9,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eReasonCode_str(eReasonCode enum_value) {
            switch (enum_value) {
            case SERVICE_PRIORITIZATION_RULE_NOT_FOUND:                                              return "SERVICE_PRIORITIZATION_RULE_NOT_FOUND";
            case NUMBER_OF_SERVICE_PRIORITIZATION_RULES_EXCEEDED_THE_MAXIMUM_SUPPORTED:              return "NUMBER_OF_SERVICE_PRIORITIZATION_RULES_EXCEEDED_THE_MAXIMUM_SUPPORTED";
            case DEFAULT_PCP_OR_PRIMARY_VLAN_ID_NOT_PROVIDED:                                        return "DEFAULT_PCP_OR_PRIMARY_VLAN_ID_NOT_PROVIDED";
            case NUMBER_OF_UNIQUE_VLAN_ID_EXCEEDS_MAXIMUM_SUPPORTED:                                 return "NUMBER_OF_UNIQUE_VLAN_ID_EXCEEDS_MAXIMUM_SUPPORTED";
            case TRAFFIC_SEPARATION_ON_COMBINED_FRONTHAUL_AND_PROFILE1_BACKHAUL_UNSUPPORTED:         return "TRAFFIC_SEPARATION_ON_COMBINED_FRONTHAUL_AND_PROFILE1_BACKHAUL_UNSUPPORTED";
            case TRAFFIC_SEPARATION_ON_COMBINED_PROFILE1_BACKHAUL_AND_PROFILE2_BACKHAUL_UNSUPPORTED: return "TRAFFIC_SEPARATION_ON_COMBINED_PROFILE1_BACKHAUL_AND_PROFILE2_BACKHAUL_UNSUPPORTED";
            case SERVICE_PRIORITIZATION_RULE_NOT_SUPPORTED:                                          return "SERVICE_PRIORITIZATION_RULE_NOT_SUPPORTED";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eReasonCode value) { return out << eReasonCode_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        eReasonCode& reason_code();
        bool alloc_bssid();
        sMacAddr* bssid();
        bool set_bssid(const sMacAddr bssid);
        bool alloc_service_prioritization_rule_id();
        uint32_t* service_prioritization_rule_id();
        bool set_service_prioritization_rule_id(const uint32_t service_prioritization_rule_id);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        eReasonCode* m_reason_code = nullptr;
        sMacAddr* m_bssid = nullptr;
        bool m_bssid_allocated = false;
        uint32_t* m_service_prioritization_rule_id = nullptr;
        bool m_service_prioritization_rule_id_allocated = false;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2ERRORCODE_H_
