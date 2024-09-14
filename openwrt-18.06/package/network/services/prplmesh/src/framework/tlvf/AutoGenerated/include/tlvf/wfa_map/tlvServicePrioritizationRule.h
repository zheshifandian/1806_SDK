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

#ifndef _TLVF_WFA_MAP_TLVSERVICEPRIORITIZATIONRULE_H_
#define _TLVF_WFA_MAP_TLVSERVICEPRIORITIZATIONRULE_H_

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


class tlvServicePrioritizationRule : public BaseClass
{
    public:
        tlvServicePrioritizationRule(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvServicePrioritizationRule(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvServicePrioritizationRule();

        typedef struct sServicePrioritizationRuleBitsField1 {
            #if defined(__LITTLE_ENDIAN_BITFIELD)
            uint8_t reserved : 7;
            uint8_t add_remove : 1;
            #elif defined(__BIG_ENDIAN_BITFIELD)
            uint8_t add_remove : 1;
            uint8_t reserved : 7;
            #else
            #error "Bitfield macros are not defined"
            #endif
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sServicePrioritizationRuleBitsField1;
        
        typedef struct sServicePrioritizationRuleBitsField2 {
            #if defined(__LITTLE_ENDIAN_BITFIELD)
            uint8_t reserved : 7;
            uint8_t always_match : 1;
            #elif defined(__BIG_ENDIAN_BITFIELD)
            uint8_t always_match : 1;
            uint8_t reserved : 7;
            #else
            #error "Bitfield macros are not defined"
            #endif
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sServicePrioritizationRuleBitsField2;
        
        typedef struct sServicePrioritizationRule {
            uint32_t id;
            sServicePrioritizationRuleBitsField1 bits_field1;
            //Rule Precedence - higher number means higher priority. Possible values: 0x00 - 0xFE
            uint8_t precedence;
            //Rule Output. The value of, or method used to select, the 802.1Q C-TAG PCP output value.
            //Possible values: 0x00 - 0x09
            uint8_t output;
            sServicePrioritizationRuleBitsField2 bits_field2;
            void struct_swap(){
                tlvf_swap(32, reinterpret_cast<uint8_t*>(&id));
                bits_field1.struct_swap();
                bits_field2.struct_swap();
            }
            void struct_init(){
                bits_field1.struct_init();
                bits_field2.struct_init();
            }
        } __attribute__((packed)) sServicePrioritizationRule;
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        sServicePrioritizationRule& rule_params();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        sServicePrioritizationRule* m_rule_params = nullptr;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVSERVICEPRIORITIZATIONRULE_H_
