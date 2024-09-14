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

#ifndef _TLVF_WFA_MAP_TLVBACKHAULSTARADIOCAPABILITIES_H_
#define _TLVF_WFA_MAP_TLVBACKHAULSTARADIOCAPABILITIES_H_

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


class tlvBackhaulStaRadioCapabilities : public BaseClass
{
    public:
        tlvBackhaulStaRadioCapabilities(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvBackhaulStaRadioCapabilities(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvBackhaulStaRadioCapabilities();

        enum eStaMacIncluded: uint8_t {
            FIELD_PRESENT = 0x80,
            FIELD_NOT_PRESENT = 0x0,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eStaMacIncluded_str(eStaMacIncluded enum_value) {
            switch (enum_value) {
            case FIELD_PRESENT:     return "FIELD_PRESENT";
            case FIELD_NOT_PRESENT: return "FIELD_NOT_PRESENT";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eStaMacIncluded value) { return out << eStaMacIncluded_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        //Radio Unique Identifier of the radio for which capabilities are reported.
        sMacAddr& ruid();
        eStaMacIncluded& sta_mac_included();
        bool alloc_sta_mac();
        sMacAddr* sta_mac();
        bool set_sta_mac(const sMacAddr sta_mac);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        sMacAddr* m_ruid = nullptr;
        eStaMacIncluded* m_sta_mac_included = nullptr;
        sMacAddr* m_sta_mac = nullptr;
        bool m_sta_mac_allocated = false;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVBACKHAULSTARADIOCAPABILITIES_H_
