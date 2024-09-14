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

#ifndef _TLVF_ASSOCIATION_FRAME_CCAPINFODMGSTA_H_
#define _TLVF_ASSOCIATION_FRAME_CCAPINFODMGSTA_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include <asm/byteorder.h>

namespace assoc_frame {


class cCapInfoDmgSta : public BaseClass
{
    public:
        cCapInfoDmgSta(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cCapInfoDmgSta(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cCapInfoDmgSta();

        typedef struct sInfoSubField {
            #if defined(__LITTLE_ENDIAN_BITFIELD)
            uint8_t spectrum_management : 1;
            uint8_t triggered_unscheduled_ps : 1;
            uint8_t reserved1 : 2;
            uint8_t radio_measurement : 1;
            uint8_t reserved2 : 3;
            #elif defined(__BIG_ENDIAN_BITFIELD)
            uint8_t reserved2 : 3;
            uint8_t radio_measurement : 1;
            uint8_t reserved1 : 2;
            uint8_t triggered_unscheduled_ps : 1;
            uint8_t spectrum_management : 1;
            #else
            #error "Bitfield macros are not defined"
            #endif
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sInfoSubField;
        
        typedef struct sDmgParam {
            #if defined(__LITTLE_ENDIAN_BITFIELD)
            uint8_t cbap_only : 1;
            uint8_t cbap_source : 1;
            uint8_t dmg_privacy : 1;
            uint8_t ecap_policy_enforced : 1;
            uint8_t bss_type : 2;
            #elif defined(__BIG_ENDIAN_BITFIELD)
            uint8_t bss_type : 2;
            uint8_t ecap_policy_enforced : 1;
            uint8_t dmg_privacy : 1;
            uint8_t cbap_source : 1;
            uint8_t cbap_only : 1;
            #else
            #error "Bitfield macros are not defined"
            #endif
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sDmgParam;
        
        sDmgParam& dmg_param();
        sInfoSubField& cap_info();
        uint16_t& listen_interval();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        sDmgParam* m_dmg_param = nullptr;
        sInfoSubField* m_cap_info = nullptr;
        uint16_t* m_listen_interval = nullptr;
};

}; // close namespace: assoc_frame

#endif //_TLVF/ASSOCIATION_FRAME_CCAPINFODMGSTA_H_
