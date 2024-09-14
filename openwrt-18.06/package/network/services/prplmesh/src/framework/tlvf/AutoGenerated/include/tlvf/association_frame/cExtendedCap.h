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

#ifndef _TLVF_ASSOCIATION_FRAME_CEXTENDEDCAP_H_
#define _TLVF_ASSOCIATION_FRAME_CEXTENDEDCAP_H_

#include <cstddef>
#include <stdint.h>
#include <tlvf/swap.h>
#include <string.h>
#include <memory>
#include <tlvf/BaseClass.h>
#include <tlvf/ClassList.h>
#include <tuple>
#include <asm/byteorder.h>
#include "tlvf/association_frame/eElementID.h"
#include "tlvf/AssociationRequestFrame/assoc_frame_bitfields.h"

namespace assoc_frame {


class cExtendedCap : public BaseClass
{
    public:
        cExtendedCap(uint8_t* buff, size_t buff_len, bool parse = false);
        explicit cExtendedCap(std::shared_ptr<BaseClass> base, bool parse = false);
        ~cExtendedCap();

        typedef struct sExtendedCapFieldB3 {
            #if defined(__LITTLE_ENDIAN_BITFIELD)
            uint8_t tfs : 1;
            uint8_t wnm_sleep_mode : 1;
            uint8_t tim_broadcast : 1;
            uint8_t bss_transition : 1;
            uint8_t qos_traffic_cap : 1;
            uint8_t ac_sta_count : 1;
            uint8_t multi_bssid : 1;
            uint8_t timing_meas : 1;
            #elif defined(__BIG_ENDIAN_BITFIELD)
            uint8_t timing_meas : 1;
            uint8_t multi_bssid : 1;
            uint8_t ac_sta_count : 1;
            uint8_t qos_traffic_cap : 1;
            uint8_t bss_transition : 1;
            uint8_t tim_broadcast : 1;
            uint8_t wnm_sleep_mode : 1;
            uint8_t tfs : 1;
            #else
            #error "Bitfield macros are not defined"
            #endif
            void struct_swap(){
            }
            void struct_init(){
            }
        } __attribute__((packed)) sExtendedCapFieldB3;
        
        eElementID& type();
        const uint8_t& length();
        size_t extended_cap_field_length() { return m_extended_cap_field_idx__ * sizeof(uint8_t); }
        uint8_t* extended_cap_field(size_t idx = 0);
        bool set_extended_cap_field(const void* buffer, size_t size);
        bool alloc_extended_cap_field(size_t count = 1);
        void class_swap() override;
        bool finalize() override;
        static size_t get_initial_size();

    private:
        bool init();
        eElementID* m_type = nullptr;
        uint8_t* m_length = nullptr;
        uint8_t* m_extended_cap_field = nullptr;
        size_t m_extended_cap_field_idx__ = 0;
        int m_lock_order_counter__ = 0;
};

}; // close namespace: assoc_frame

#endif //_TLVF/ASSOCIATION_FRAME_CEXTENDEDCAP_H_
