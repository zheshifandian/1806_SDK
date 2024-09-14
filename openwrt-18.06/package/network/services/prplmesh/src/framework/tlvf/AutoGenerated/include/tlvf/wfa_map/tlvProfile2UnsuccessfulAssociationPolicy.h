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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2UNSUCCESSFULASSOCIATIONPOLICY_H_
#define _TLVF_WFA_MAP_TLVPROFILE2UNSUCCESSFULASSOCIATIONPOLICY_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"
#include <asm/byteorder.h>

namespace wfa_map {


class tlvProfile2UnsuccessfulAssociationPolicy : public BaseClass
{
    public:
        tlvProfile2UnsuccessfulAssociationPolicy(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2UnsuccessfulAssociationPolicy(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2UnsuccessfulAssociationPolicy();

        typedef struct sUnsuccessfulAssociations {
            #if defined(__LITTLE_ENDIAN_BITFIELD)
            uint8_t reserved : 7;
            uint8_t report : 1;
            #elif defined(__BIG_ENDIAN_BITFIELD)
            uint8_t report : 1;
            uint8_t reserved : 7;
            #else
            #error "Bitfield macros are not defined"
            #endif
            void struct_swap(){
            }
            void struct_init(){
                reserved = 0x0;
            }
        } __attribute__((packed)) sUnsuccessfulAssociations;
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        sUnsuccessfulAssociations& report_unsuccessful_associations();
        uint32_t& maximum_reporting_rate();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        sUnsuccessfulAssociations* m_report_unsuccessful_associations = nullptr;
        uint32_t* m_maximum_reporting_rate = nullptr;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2UNSUCCESSFULASSOCIATIONPOLICY_H_
