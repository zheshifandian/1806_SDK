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

#ifndef _TLVF_WFA_MAP_TLVPROFILE2APCAPABILITY_H_
#define _TLVF_WFA_MAP_TLVPROFILE2APCAPABILITY_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include "tlvf/wfa_map/eTlvTypeMap.h"
#include <ostream>
#include <asm/byteorder.h>

namespace wfa_map {


class tlvProfile2ApCapability : public BaseClass
{
    public:
        tlvProfile2ApCapability(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit tlvProfile2ApCapability(std::shared_ptr<BaseClass> base, bool parse = false);
        ~tlvProfile2ApCapability();

        enum eByteCounterUnits: uint8_t {
            BYTES = 0x0,
            KIBIBYTES = 0x1,
            MEBIBYTES = 0x2,
        };
        // Enum AutoPrint generated code snippet begining- DON'T EDIT!
        // clang-format off
        static const char *eByteCounterUnits_str(eByteCounterUnits enum_value) {
            switch (enum_value) {
            case BYTES:     return "BYTES";
            case KIBIBYTES: return "KIBIBYTES";
            case MEBIBYTES: return "MEBIBYTES";
            }
            static std::string out_str = std::to_string(int(enum_value));
            return out_str.c_str();
        }
        friend inline std::ostream &operator<<(std::ostream &out, eByteCounterUnits value) { return out << eByteCounterUnits_str(value); }
        // clang-format on
        // Enum AutoPrint generated code snippet end
        
        typedef struct sCapabilitiesBitsField {
            #if defined(__LITTLE_ENDIAN_BITFIELD)
            uint8_t reserved : 5;
            uint8_t prioritization : 1;
            uint8_t byte_counter_units : 2;
            #elif defined(__BIG_ENDIAN_BITFIELD)
            uint8_t byte_counter_units : 2;
            uint8_t prioritization : 1;
            uint8_t reserved : 5;
            #else
            #error "Bitfield macros are not defined"
            #endif
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sCapabilitiesBitsField;
        
        const eTlvTypeMap& type();
        const uint16_t& length();
        //The maximum total number of service prioritization rules supported by the Multi-AP Agent
        uint8_t& max_prioritization_rules();
        uint8_t& reserved();
        sCapabilitiesBitsField& capabilities_bit_field();
        //Max Total Number of unique VLAN identifiers the Multi-AP Agent supports
        uint8_t& max_total_number_of_vids();
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eTlvTypeMap* m_type = nullptr;
        uint16_t* m_length = nullptr;
        uint8_t* m_max_prioritization_rules = nullptr;
        uint8_t* m_reserved = nullptr;
        sCapabilitiesBitsField* m_capabilities_bit_field = nullptr;
        uint8_t* m_max_total_number_of_vids = nullptr;
};

}; // close namespace: wfa_map

#endif //_TLVF/WFA_MAP_TLVPROFILE2APCAPABILITY_H_
