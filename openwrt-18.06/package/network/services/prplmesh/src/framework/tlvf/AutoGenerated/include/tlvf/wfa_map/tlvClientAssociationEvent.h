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

#ifndef _TLVF_WFA_MAP_TLVCLIENTASSOCIATIONEVENT_H_
#define _TLVF_WFA_MAP_TLVCLIENTASSOCIATIONEVENT_H_

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


class tlvClientAssociationEvent : public BaseClass
{
    public:
        tlvClientAssociationEvent(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvClientAssociationEvent(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvClientAssociationEvent();

        enum eAssociationEvent: uint8_t {
            CLIENT_HAS_JOINED_THE_BSS = 0x80,
            CLIENT_HAS_LEFT_THE_BSS = 0x0,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eAssociationEvent_str(eAssociationEvent enum_value) {
            switch (enum_value) {
            case CLIENT_HAS_JOINED_THE_BSS: return "CLIENT_HAS_JOINED_THE_BSS";
            case CLIENT_HAS_LEFT_THE_BSS:   return "CLIENT_HAS_LEFT_THE_BSS";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eAssociationEvent value) { return out << eAssociationEvent_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        sMacAddr& client_mac();
        sMacAddr& bssid();
        eAssociationEvent& association_event();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        sMacAddr* m_client_mac = nullptr;
        sMacAddr* m_bssid = nullptr;
        eAssociationEvent* m_association_event = nullptr;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVCLIENTASSOCIATIONEVENT_H_
