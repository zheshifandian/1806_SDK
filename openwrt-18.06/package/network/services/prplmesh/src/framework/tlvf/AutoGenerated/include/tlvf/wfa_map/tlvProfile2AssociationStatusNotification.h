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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2ASSOCIATIONSTATUSNOTIFICATION_H_
#define _TLVF_WFA_MAP_TLVPROFILE2ASSOCIATIONSTATUSNOTIFICATION_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"
#include <tuple>
#include <ostream>
#include "tlvf/common/sMacAddr.h"

namespace wfa_map {


class tlvProfile2AssociationStatusNotification : public BaseClass
{
    public:
        tlvProfile2AssociationStatusNotification(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2AssociationStatusNotification(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2AssociationStatusNotification();

        enum eAssociationAllowanceStatus: uint8_t {
            NO_MORE_ASSOCIATIONS_ALLOWED = 0x0,
            ASSOCIATIONS_ALLOWED = 0x1,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eAssociationAllowanceStatus_str(eAssociationAllowanceStatus enum_value) {
            switch (enum_value) {
            case NO_MORE_ASSOCIATIONS_ALLOWED: return "NO_MORE_ASSOCIATIONS_ALLOWED";
            case ASSOCIATIONS_ALLOWED:         return "ASSOCIATIONS_ALLOWED";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eAssociationAllowanceStatus value) { return out << eAssociationAllowanceStatus_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        typedef struct sBssidStatus {
            sMacAddr bssid;
            //The status of allowance of new client device associations on the BSSs specified by the BSSIDs
            //in this TLV.
            eAssociationAllowanceStatus association_allowance_status;
            void struct_swap(){
                bssid.struct_swap();
            }
            void struct_init(){
                bssid.struct_init();
            }
        } __attribute__((packed)) sBssidStatus;
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        uint8_t& bssid_status_list_length();
        std::tuple<bool, sBssidStatus&> bssid_status_list(size_t idx);
        bool alloc_bssid_status_list(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        uint8_t* m_bssid_status_list_length = nullptr;
        sBssidStatus* m_bssid_status_list = nullptr;
        size_t m_bssid_status_list_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2ASSOCIATIONSTATUSNOTIFICATION_H_
