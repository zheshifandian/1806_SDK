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

#ifndef _TLVF_WFA_MAP_TLVCHANNELSELECTIONRESPONSE_H_
#define _TLVF_WFA_MAP_TLVCHANNELSELECTIONRESPONSE_H_

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


class tlvChannelSelectionResponse : public BaseClass
{
    public:
        tlvChannelSelectionResponse(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvChannelSelectionResponse(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvChannelSelectionResponse();

        enum eResponseCode: uint8_t {
            ACCEPT = 0x0,
            DECLINE_VIOLATES_CURRENT_PREFERENCES = 0x1,
            DECLINE_VIOLATES_MOST_RECENTLY_REPORTED_PREFERENCES = 0x2,
            DECLINE_PREVENT_OPERATION_OF_BACKHAUL_LINK = 0x3,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eResponseCode_str(eResponseCode enum_value) {
            switch (enum_value) {
            case ACCEPT:                                              return "ACCEPT";
            case DECLINE_VIOLATES_CURRENT_PREFERENCES:                return "DECLINE_VIOLATES_CURRENT_PREFERENCES";
            case DECLINE_VIOLATES_MOST_RECENTLY_REPORTED_PREFERENCES: return "DECLINE_VIOLATES_MOST_RECENTLY_REPORTED_PREFERENCES";
            case DECLINE_PREVENT_OPERATION_OF_BACKHAUL_LINK:          return "DECLINE_PREVENT_OPERATION_OF_BACKHAUL_LINK";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eResponseCode value) { return out << eResponseCode_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        sMacAddr& radio_uid();
        eResponseCode& response_code();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        sMacAddr* m_radio_uid = nullptr;
        eResponseCode* m_response_code = nullptr;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVCHANNELSELECTIONRESPONSE_H_
